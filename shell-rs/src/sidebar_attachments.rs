use crate::deps::*;
use papers_document::{Attachment, DocumentAttachments};
use papers_shell::SidebarPage;
use papers_view::{JobAttachments, JobPriority};

mod imp {
    use super::*;

    #[derive(CompositeTemplate, Debug, Default, Properties)]
    #[properties(wrapper_type = super::PpsSidebarAttachments)]
    #[template(resource = "/org/gnome/papers/ui/sidebar-attachments.ui")]
    pub struct PpsSidebarAttachments {
        #[template_child]
        grid_view: TemplateChild<gtk::GridView>,
        #[template_child]
        model: TemplateChild<gio::ListStore>,
        #[property(name = "document-model", nullable, set = Self::set_model)]
        doc_model: RefCell<Option<DocumentModel>>,
    }

    #[gtk::template_callbacks]
    impl PpsSidebarAttachments {
        #[template_callback]
        fn grid_view_factory_setup(&self, item: &gtk::ListItem, _factory: &gtk::ListItemFactory) {
            let box_ = gtk::Box::builder()
                .orientation(gtk::Orientation::Vertical)
                .spacing(12)
                .build();

            let image = gtk::Image::builder()
                .pixel_size(60)
                .margin_bottom(6)
                .margin_top(6)
                .margin_start(6)
                .margin_end(6)
                .build();

            let label = gtk::Label::builder()
                .wrap_mode(gtk::pango::WrapMode::WordChar)
                .wrap(true)
                .build();

            box_.append(&image);
            box_.append(&label);

            let drag = gtk::DragSource::new();

            drag.connect_prepare(
                glib::clone!(@weak self as obj, @weak item => @default-return None, move |_drag, _x, _y| {
                    obj.attachments_drag_prepare(&item)
                }),
            );
            box_.add_controller(drag);

            let secondary_click = gtk::GestureClick::builder()
                .button(gdk::BUTTON_SECONDARY)
                .build();

            secondary_click.connect_pressed(
                glib::clone!(@weak self as obj, @weak item => move |click, _n_press, x, y| {
                    obj.secondary_button_clicked(x, y, &item, click);
                }),
            );

            box_.add_controller(secondary_click);

            item.set_child(Some(&box_));
        }

        #[template_callback]
        fn grid_view_factory_bind(&self, item: &gtk::ListItem, _factory: &gtk::ListItemFactory) {
            let box_ = item.child().and_downcast::<gtk::Box>().unwrap();
            let image = box_.first_child().and_downcast::<gtk::Image>().unwrap();
            let label = box_.last_child().and_downcast::<gtk::Label>().unwrap();
            let attachment = item.item().and_downcast::<Attachment>().unwrap();

            if let Some(description) = attachment
                .description()
                .map(|s| glib::gformat!("{}", glib::markup_escape_text(s.as_str())))
            {
                if !description.is_empty() {
                    box_.set_tooltip_text(Some(description.as_str()));
                }
            }

            if let Some(icon) = attachment
                .mime_type()
                .map(|mime| gio::content_type_get_symbolic_icon(mime.as_str()))
            {
                image.set_gicon(Some(&icon));
            }

            if let Some(name) = attachment.name() {
                label.set_text(name.as_str());
            }
        }

        fn set_model(&self, model: DocumentModel) {
            model.connect_document_notify(glib::clone!(@weak self as obj => move |model| {
                obj.document_changed_cb(model);
            }));

            self.doc_model.replace(Some(model));
        }

        fn document_changed_cb(&self, model: &DocumentModel) {
            let Some(doc) = model.document() else {
                return;
            };

            self.model.remove_all();

            if !self.support_document(&doc) {
                return;
            }

            let job_attachments = JobAttachments::new(&doc);

            job_attachments.connect_finished(
                glib::clone!(@weak self as obj => move |job_attachments| {
                    for attachment in job_attachments.attachments().iter() {
                        obj.model.append(attachment);
                    }
                }),
            );

            job_attachments.scheduler_push_job(JobPriority::PriorityNone);
        }

        #[template_callback]
        fn grid_view_item_activated(&self, position: u32, _grid_view: &gtk::GridView) {
            let Some(attachment) = self
                .grid_view
                .model()
                .and_then(|model| model.item(position))
                .and_downcast::<Attachment>()
            else {
                return;
            };

            let context = self.obj().display().app_launch_context();

            if let Err(e) = attachment.open(&context) {
                warn!("{e}");
            }
        }

        #[template_callback]
        fn button_clicked(&self, _n_press: i32, x: f64, y: f64, click: &gtk::GestureClick) {
            let Some(selection) = self.grid_view.model() else {
                return;
            };

            let point = gtk::graphene::Point::new(x as f32, y as f32);

            if click
                .widget()
                .compute_point(self.grid_view.upcast_ref::<gtk::Widget>(), &point)
                .and_then(|point| {
                    self.grid_view
                        .pick(point.x() as f64, point.y() as f64, gtk::PickFlags::DEFAULT)
                })
                .is_none()
            {
                selection.unselect_all();
            }
        }

        fn secondary_button_clicked(
            &self,
            x: f64,
            y: f64,
            item: &gtk::ListItem,
            click: &gtk::GestureClick,
        ) {
            let Some(selection) = self.grid_view.model() else {
                return;
            };

            selection.select_item(item.position(), false);

            let bitset = selection.selection();
            let Some((iter, index)) = gtk::BitsetIter::init_first(&bitset) else {
                return;
            };

            let Some(attachment) = selection.item(index).and_downcast::<Attachment>() else {
                return;
            };

            let attachments = gio::ListStore::new::<Attachment>();
            attachments.append(&attachment);

            for attachment in iter
                .filter_map(|index| selection.item(index))
                .filter_map(|obj| obj.downcast::<Attachment>().ok())
            {
                attachments.append(&attachment);
            }

            let point = gtk::graphene::Point::new(x as f32, y as f32);

            let point = click
                .widget()
                .compute_point(self.obj().upcast_ref::<gtk::Widget>(), &point)
                .unwrap_or_default();

            let point = point.as_ptr() as glib::Pointer;

            self.obj().emit_by_name::<()>("popup", &[&point, &attachments]);
        }

        fn selected_attachment(&self) -> glib::List<Attachment> {
            let mut attachments = glib::List::new();

            let Some(selection) = &self.grid_view.model() else {
                return attachments;
            };

            let bitset = selection.selection();

            let Some((iter, index)) = gtk::BitsetIter::init_first(&bitset) else {
                return attachments;
            };

            let attachment = selection.item(index).and_downcast::<Attachment>();
            if let Some(attachment) = attachment {
                attachments.push_front(attachment);
            }

            for attachment in iter
                .filter_map(|index| selection.item(index))
                .filter_map(|obj| obj.downcast::<Attachment>().ok())
            {
                attachments.push_front(attachment);
            }

            attachments
        }

        fn attachments_drag_prepare(&self, item: &gtk::ListItem) -> Option<gdk::ContentProvider> {
            let selection = self.grid_view.model()?;

            if !selection.is_selected(item.position()) {
                return None;
            }

            let mut files = Vec::new();

            for attachment in self.selected_attachment() {
                let tempdir = match papers_document::mkdtemp("attachments.XXXXXX") {
                    Ok(s) => s,
                    Err(e) => {
                        warn!("{e}");
                        continue;
                    }
                };

                let Some(name) = attachment.name() else {
                    continue;
                };

                let template = tempdir.join(name);
                let file = gio::File::for_path(template);

                if let Err(e) = attachment.save(&file) {
                    warn!("{e}");
                    continue;
                }

                files.push(file);
            }

            if files.is_empty() {
                return None;
            }

            let file_list = gdk::FileList::from_array(&files);

            Some(gdk::ContentProvider::for_value(&file_list.to_value()))
        }
    }

    #[glib::object_subclass]
    impl ObjectSubclass for PpsSidebarAttachments {
        const NAME: &'static str = "PpsSidebarAttachments";
        type Type = super::PpsSidebarAttachments;
        type ParentType = gtk::Box;
        type Interfaces = (SidebarPage,);

        fn class_init(klass: &mut Self::Class) {
            klass.bind_template();
            klass.bind_template_callbacks();
        }

        fn instance_init(obj: &InitializingObject<Self>) {
            obj.init_template();
        }
    }

    #[glib::derived_properties]
    impl ObjectImpl for PpsSidebarAttachments {
        fn signals() -> &'static [Signal] {
            static SIGNALS: OnceLock<Vec<Signal>> = OnceLock::new();
            SIGNALS.get_or_init(|| {
                vec![
                    Signal::builder("popup")
                        .run_last()
                        .action()
                        .param_types([glib::Type::POINTER, gio::ListModel::static_type()])
                        .build(),
                    Signal::builder("save-attachment")
                        .run_last()
                        .action()
                        .return_type::<bool>()
                        .param_types([glib::Type::OBJECT, glib::Type::STRING])
                        .build(),
                ]
            })
        }
    }

    impl WidgetImpl for PpsSidebarAttachments {}

    impl BoxImpl for PpsSidebarAttachments {}

    impl SidebarPageImpl for PpsSidebarAttachments {
        fn support_document(&self, document: &Document) -> bool {
            document
                .dynamic_cast_ref::<DocumentAttachments>()
                .map(|d| d.has_attachments())
                .unwrap_or_default()
        }
    }
}

glib::wrapper! {
    pub struct PpsSidebarAttachments(ObjectSubclass<imp::PpsSidebarAttachments>)
    @extends gtk::Widget, gtk::Box;
}

impl Default for PpsSidebarAttachments {
    fn default() -> Self {
        Self::new()
    }
}

impl PpsSidebarAttachments {
    fn new() -> Self {
        glib::Object::builder().build()
    }
}
