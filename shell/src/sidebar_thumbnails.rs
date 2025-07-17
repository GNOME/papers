use crate::deps::*;

use gdk::pango::{AttrList, WrapMode};
use papers_view::{JobPriority, PageLayout};

/// The size used as pixel size of [gtk::Image]
const THUMBNAIL_WIDTH: f64 = 180.0;

/// Maximum LRU cache size without eviction
///
/// This is a heuristic number and should be large enough, at least larger than
/// the maximum list items of [gtk::GridView] in BIND state. Or we may experience
/// cache thrashing. Keep in mind the dual page mode which doubles the items in
/// BIND state.
const MAX_LRU_SIZE: usize = 200;

use lru::LruCache;
use std::cell::Cell;

mod imp {
    use super::*;

    #[derive(Properties, Default, Debug, CompositeTemplate)]
    #[properties(wrapper_type = super::PpsSidebarThumbnails)]
    #[template(resource = "/org/gnome/papers/ui/sidebar-thumbnails.ui")]
    pub struct PpsSidebarThumbnails {
        #[template_child]
        pub(super) clamp: TemplateChild<adw::ClampScrollable>,
        #[template_child]
        pub(super) grid_view: TemplateChild<gtk::GridView>,
        #[template_child]
        pub(super) list_store: TemplateChild<gio::ListStore>,
        #[template_child]
        pub(super) selection_model: TemplateChild<gtk::SingleSelection>,
        #[template_child]
        pub(super) factory: TemplateChild<gtk::SignalListItemFactory>,
        /// Switch of blank_head mode. When this mode is enabled. A blank page is
        /// inserted into the start of thumbnail list. We explicitly distinguish
        /// model index between page index due to support of this mode.
        #[property(set, get)]
        pub(super) blank_head: Cell<bool>,
        pub(super) block_page_changed: Cell<bool>,
        pub(super) block_activate: Cell<bool>,
        pub(super) lru: RefCell<Option<LruCache<u32, PpsThumbnailItem>>>,
    }

    #[glib::object_subclass]
    impl ObjectSubclass for PpsSidebarThumbnails {
        const NAME: &'static str = "PpsSidebarThumbnails";
        type Type = super::PpsSidebarThumbnails;
        type ParentType = PpsSidebarPage;

        fn class_init(klass: &mut Self::Class) {
            PpsThumbnailItem::ensure_type();

            klass.bind_template();
            klass.bind_template_callbacks();

            klass.set_css_name("pps-sidebar-thumbnails");
        }

        fn instance_init(obj: &InitializingObject<Self>) {
            obj.init_template();
        }
    }

    #[glib::derived_properties]
    impl ObjectImpl for PpsSidebarThumbnails {
        fn constructed(&self) {
            self.blank_head.set(false);
            self.lru.replace(Some(LruCache::unbounded()));

            if let Some(model) = self.obj().document_model() {
                model.connect_page_changed(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |model, _, new| {
                        if obj.block_page_changed.get() {
                            return;
                        }

                        if model.document().is_none() {
                            return;
                        }

                        debug!("page changed callback {new}");

                        obj.set_current_page(new);
                    }
                ));

                model.connect_document_notify(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |model| {
                        if let Some(document) = model.document() {
                            if document.n_pages() > 0 && document.check_dimensions() {
                                obj.reload();
                            }
                        }
                    }
                ));

                model.connect_inverted_colors_notify(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_| {
                        obj.reload();
                    }
                ));

                model.connect_rotation_notify(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_| {
                        obj.reload();
                    }
                ));

                model.connect_page_layout_notify(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_| {
                        obj.relayout();
                    }
                ));

                model.connect_dual_odd_left_notify(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_| {
                        obj.relayout();
                    }
                ));
            }
        }
    }

    impl BinImpl for PpsSidebarThumbnails {}

    impl WidgetImpl for PpsSidebarThumbnails {
        fn map(&self) {
            self.parent_map();

            if let Some(model) = self.obj().document_model() {
                let page = model.page();

                if page >= 0 && model.document().is_some() {
                    self.set_current_page(page);
                }
            }
        }
    }

    impl PpsSidebarPageImpl for PpsSidebarThumbnails {
        fn support_document(&self, _document: &Document) -> bool {
            true
        }
    }

    #[gtk::template_callbacks]
    impl PpsSidebarThumbnails {
        #[template_callback]
        fn grid_view_factory_setup(
            &self,
            item: &gtk::ListItem,
            _factory: &gtk::SignalListItemFactory,
        ) {
            let box_ = gtk::Box::builder()
                .orientation(gtk::Orientation::Vertical)
                .spacing(6)
                .build();

            let mut css_classes = vec!["icon-dropshadow"];

            if self.inverted_color() {
                css_classes.push("inverted-color");
            }

            let image = gtk::Picture::builder()
                .width_request(60)
                .content_fit(gtk::ContentFit::Contain)
                .css_classes(css_classes)
                .accessible_role(gtk::AccessibleRole::Presentation)
                .build();

            let label = gtk::Label::builder()
                .vexpand(true)
                .valign(gtk::Align::End)
                .attributes(&AttrList::from_string("0 -1 style italic").unwrap())
                .wrap_mode(WrapMode::WordChar)
                .wrap(true)
                .build();
            box_.prepend(&label);
            box_.prepend(&image);
            item.set_child(Some(&box_));
        }

        fn render_item(&self, model_index: i32) -> JobThumbnailTexture {
            let doc_page = self.page_of_document(model_index);
            let document = self.obj().document_model().unwrap().document().unwrap();
            let (width, height) = self.thumbnail_size_for_page(doc_page).unwrap();

            let job = JobThumbnailTexture::with_target_size(
                &document,
                doc_page,
                self.rotation(),
                width,
                height,
            );

            job.connect_finished(glib::clone!(
                #[weak(rename_to = obj)]
                self,
                move |job| {
                    if let Some(item) = obj.list_store.item(model_index as u32) {
                        if let Ok(item) = item.downcast::<PpsThumbnailItem>() {
                            // TODO: Take care of failed job code-path
                            debug!("load thumbnail of page: {doc_page}");

                            obj.lru
                                .borrow_mut()
                                .as_mut()
                                .unwrap()
                                .put(model_index as u32, item.clone());
                            item.set_paintable(job.texture());
                        }
                    }
                }
            ));

            debug!("push render job for page: {doc_page}");

            job.scheduler_push_job(JobPriority::PriorityHigh);

            job
        }

        #[template_callback]
        fn grid_view_factory_bind(
            &self,
            list_item: &gtk::ListItem,
            _factory: &gtk::SignalListItemFactory,
        ) {
            let item = list_item.item().and_downcast::<PpsThumbnailItem>().unwrap();
            let model_index = list_item.position() as i32;
            let box_ = list_item.child().unwrap();
            let image = box_.first_child().and_downcast::<gtk::Picture>().unwrap();
            let label = box_.last_child().and_downcast::<gtk::Label>().unwrap();
            let blank_head_mode = self.blank_head_mode();

            // Set the text here to make the label of first page cleared when
            // blank head mode is enabled.
            label.set_text(&item.text());

            if blank_head_mode && model_index == 0 {
                debug!("blank_head_mode and skip first page");
                image.set_paintable(None::<gdk::Paintable>.as_ref());
                list_item.set_selectable(false);
                list_item.set_activatable(false);
                return;
            }

            let binding = item.bind_property("paintable", &image, "paintable").build();

            item.set_binding(Some(binding));

            if let Some(paintable) = item.paintable() {
                item.set_paintable(Some(&paintable));
            } else if item.job().is_none() {
                item.set_job(Some(self.render_item(model_index)));

                let (width, height) = self
                    .thumbnail_size_for_page(self.page_of_document(model_index))
                    .unwrap_or((210, 297));
                image.set_paintable(Some(&gdk::Paintable::new_empty(width, height)));
            }
        }

        #[template_callback]
        fn grid_view_factory_unbind(
            &self,
            list_item: &gtk::ListItem,
            _factory: &gtk::SignalListItemFactory,
        ) {
            {
                let item = list_item.item().and_downcast::<PpsThumbnailItem>().unwrap();
                let model_index = list_item.position() as i32;
                let selected = self.selection_model.selected() as i32;

                if self.blank_head_mode() && model_index == 0 {
                    list_item.set_activatable(true);
                    list_item.set_selectable(true);
                }

                if let Some(binding) = item.binding() {
                    binding.unbind();
                    item.set_binding(None::<glib::Binding>);
                }

                // HACK: GtkGridView.scroll_to with SELECT flag set will trigger
                // an extra unbind/bind cycle. We don't need to remove and cancel
                // the job in this situation.
                if model_index != selected {
                    if let Some(job) = item.job() {
                        job.cancel();
                        item.set_job(None::<JobThumbnailTexture>);
                    }
                }

                self.lru_evict();
            }
        }

        #[template_callback]
        fn grid_view_selection_changed(&self) {
            if self.block_activate.get() {
                return;
            }

            if let Some(model) = self.obj().document_model() {
                let selected = self.selection_model.selected();
                let doc_page = self.page_of_document(selected as i32);

                if doc_page >= 0 {
                    self.block_page_changed.set(true);
                    model.set_page(doc_page);
                    self.obj().navigate_to_view();
                    self.block_page_changed.set(false);
                }
            }
        }

        /// Evict the LRU cache.
        ///
        /// The item inside the LRU cache is actually [gdk::Texture] referenced
        /// by a [PpsThumbnailItem]. Note: item already bind to list item should
        /// not be removed.
        fn lru_evict(&self) {
            if let Some(lru) = self.lru.borrow_mut().as_mut() {
                let lru_size = lru.len();

                if lru_size > MAX_LRU_SIZE {
                    let should_remove = lru_size - MAX_LRU_SIZE;
                    let mut drop = 0usize;
                    let mut iterate = 0usize;

                    while drop < should_remove && iterate < lru_size {
                        // safe to unwrap since the lru cache is not empty
                        let (key, item) = lru.peek_lru().unwrap();
                        let (key, item) = (*key, item.clone());

                        if item.binding().is_none() {
                            item.set_paintable(None::<gdk::Paintable>);
                            lru.pop_entry(&key);
                            drop += 1;

                            debug!("drop index {key} in cached thumbnail");
                        } else {
                            lru.promote(&key);
                        }

                        iterate += 1;
                    }
                    debug!("drop {drop} (expect {should_remove}) cached thumbnail");
                }
            }
        }

        #[template_callback]
        fn reload(&self) {
            debug!("reload list model");

            self.relayout();
            self.fill_list_store();
            self.lru.borrow_mut().as_mut().unwrap().clear();

            if let Some(model) = self.obj().document_model() {
                if let Some(document) = model.document() {
                    if document.n_pages() > 0 && document.check_dimensions() {
                        self.set_current_page(model.page());
                    }
                }
            }
        }

        fn inverted_color(&self) -> bool {
            if let Some(model) = self.obj().document_model() {
                model.is_inverted_colors()
            } else {
                false
            }
        }

        fn rotation(&self) -> i32 {
            if let Some(model) = self.obj().document_model() {
                model.rotation()
            } else {
                0
            }
        }

        fn dual_page(&self) -> bool {
            if let Some(model) = self.obj().document_model() {
                model.page_layout() == PageLayout::Dual
            } else {
                false
            }
        }

        fn dual_page_odd_pages_left(&self) -> bool {
            if let Some(model) = self.obj().document_model() {
                model.is_dual_page_odd_pages_left()
            } else {
                false
            }
        }

        fn need_blank_head(&self) -> bool {
            self.dual_page() && !self.dual_page_odd_pages_left()
        }

        fn relayout(&self) {
            let need_blank_head = self.need_blank_head();

            if self.blank_head_mode() != need_blank_head {
                self.blank_head.set(need_blank_head);

                if need_blank_head {
                    self.list_store.insert(0, &PpsThumbnailItem::default());
                } else {
                    self.list_store.remove(0);
                }
            }

            let columns = if self.dual_page() { 2 } else { 1 };
            self.grid_view.set_max_columns(columns);
            self.grid_view.set_min_columns(columns);

            self.clamp.set_maximum_size(210 * columns as i32);
            self.clamp.set_tightening_threshold(150 * columns as i32);

            self.obj().remove_css_class("columns-1");
            self.obj().remove_css_class("columns-2");
            self.obj().add_css_class(&format!("columns-{columns}"));
        }

        fn set_current_page(&self, doc_page: i32) {
            self.block_activate.set(true);

            let store_index = self.index_of_store(doc_page);

            debug!("set current selected page to {store_index}");

            if self.obj().is_mapped() && self.list_store.n_items() > 0 {
                self.grid_view.scroll_to(
                    store_index as u32,
                    gtk::ListScrollFlags::FOCUS | gtk::ListScrollFlags::SELECT,
                    None,
                );
            }

            self.block_activate.set(false);
        }

        /// This is a special mode that a blank page is inserted into the start
        /// of thumbnails.
        fn blank_head_mode(&self) -> bool {
            self.blank_head.get()
        }

        fn fill_list_store(&self) {
            self.list_store.remove_all();
            if let Some(model) = self.obj().document_model() {
                if let Some(document) = model.document() {
                    let mut thumbnails = Vec::new();

                    if self.blank_head_mode() {
                        thumbnails.push(PpsThumbnailItem::default());
                    }

                    for i in 0..document.n_pages() {
                        let label = document.page_label(i);
                        let item = PpsThumbnailItem::default();
                        item.set_text(label);
                        thumbnails.push(item);
                    }

                    self.list_store.splice(0, 0, &thumbnails);
                }
            }
        }

        fn page_of_document(&self, store_index: i32) -> i32 {
            if self.blank_head_mode() {
                store_index - 1
            } else {
                store_index
            }
        }

        fn index_of_store(&self, doc_page: i32) -> i32 {
            if self.blank_head_mode() {
                doc_page + 1
            } else {
                doc_page
            }
        }

        fn thumbnail_size_for_page(&self, doc_page: i32) -> Option<(i32, i32)> {
            let scale = self
                .obj()
                .native()
                .and_then(|native| native.surface())
                .map(|surface| surface.scale())
                .unwrap_or(1.0);
            let model = self.obj().document_model()?;
            let document = model.document()?;
            let (mut width, mut height) = document.page_size(doc_page);
            let rotation = self.rotation();

            height = THUMBNAIL_WIDTH * height / width + 0.5;

            if rotation == 90 || rotation == 270 {
                width = height * scale;
                height = THUMBNAIL_WIDTH * scale;
            } else {
                width = THUMBNAIL_WIDTH * scale;
                height *= scale;
            }

            Some((width as i32, height as i32))
        }
    }
}

glib::wrapper! {
    pub struct PpsSidebarThumbnails(ObjectSubclass<imp::PpsSidebarThumbnails>)
        @extends PpsSidebarPage, adw::Bin, gtk::Widget,
        @implements gtk::Accessible, gtk::Buildable, gtk::ConstraintTarget;
}

impl PpsSidebarThumbnails {
    pub fn new() -> Self {
        glib::Object::builder().build()
    }
}

impl Default for PpsSidebarThumbnails {
    fn default() -> Self {
        Self::new()
    }
}
