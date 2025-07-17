use crate::deps::*;
use papers_document::Layer;
use papers_view::{JobLayers, JobPriority};

use std::collections::HashMap;

mod imp {
    use super::*;

    #[derive(Default, Debug, CompositeTemplate)]
    #[template(resource = "/org/gnome/papers/ui/sidebar-layers.ui")]
    pub struct PpsSidebarLayers {
        #[template_child]
        pub(super) list_view: TemplateChild<gtk::ListView>,
        #[template_child]
        pub(super) selection_model: TemplateChild<gtk::NoSelection>,
        pub(super) sig_handlers: RefCell<HashMap<Layer, SignalHandlerId>>,
        pub(super) groups: RefCell<HashMap<usize, Vec<Layer>>>,
    }

    #[glib::object_subclass]
    impl ObjectSubclass for PpsSidebarLayers {
        const NAME: &'static str = "PpsSidebarLayers";
        type Type = super::PpsSidebarLayers;
        type ParentType = PpsSidebarPage;

        fn class_init(klass: &mut Self::Class) {
            klass.bind_template();
            klass.bind_template_callbacks();
        }

        fn instance_init(obj: &InitializingObject<Self>) {
            obj.init_template();
        }
    }

    impl ObjectImpl for PpsSidebarLayers {
        fn signals() -> &'static [Signal] {
            static SIGNALS: OnceLock<Vec<Signal>> = OnceLock::new();
            SIGNALS.get_or_init(|| vec![Signal::builder("visibility-changed").run_last().build()])
        }

        fn constructed(&self) {
            if let Some(model) = self.obj().document_model() {
                model.connect_document_notify(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |model| {
                        let Some(document) = model.document().filter(|d| obj.support_document(d))
                        else {
                            return;
                        };

                        let job = JobLayers::new(&document);

                        job.connect_finished(move |job| {
                            let model = job.model().unwrap();

                            for o in &model {
                                let layer = o.ok().and_downcast::<Layer>().unwrap();
                                let rb_group = layer.rb_group() as usize;

                                if rb_group > 0 {
                                    let mut groups = obj.groups.borrow_mut();

                                    if let Some(v) = groups.get_mut(&rb_group) {
                                        v.push(layer);
                                    } else {
                                        groups.insert(rb_group, vec![layer]);
                                    }
                                }
                            }

                            let tree_model = gtk::TreeListModel::new(model, false, false, |l| {
                                l.downcast_ref::<Layer>().unwrap().children()
                            });

                            obj.selection_model.set_model(Some(&tree_model));
                        });

                        job.scheduler_push_job(JobPriority::PriorityNone);
                    }
                ));
            }
        }
    }

    impl BinImpl for PpsSidebarLayers {}

    impl WidgetImpl for PpsSidebarLayers {}

    impl PpsSidebarPageImpl for PpsSidebarLayers {
        fn support_document(&self, document: &Document) -> bool {
            document
                .dynamic_cast_ref::<DocumentLayers>()
                .map(|d| d.has_layers())
                .unwrap_or(false)
        }
    }

    #[gtk::template_callbacks]
    impl PpsSidebarLayers {
        #[template_callback]
        fn list_view_factory_setup(
            &self,
            item: &gtk::ListItem,
            _factory: &gtk::SignalListItemFactory,
        ) {
            let box_ = gtk::Box::builder()
                .orientation(gtk::Orientation::Horizontal)
                .spacing(6)
                .build();

            // let toggle = gtk::ToggleButton::builder().icon_name("view-reveal-symbolic").build();
            let toggle = gtk::Image::builder()
                .icon_name("view-reveal-symbolic")
                .build();

            let title = gtk::Label::builder()
                .xalign(0.0)
                .ellipsize(gtk::pango::EllipsizeMode::End)
                .hexpand(true)
                .has_tooltip(true)
                .build();

            box_.append(&toggle);
            box_.append(&title);

            let expander = gtk::TreeExpander::builder().child(&box_).build();

            item.set_focusable(false);
            item.set_child(Some(&expander));
        }

        #[template_callback]
        fn list_view_factory_bind(
            &self,
            list_item: &gtk::ListItem,
            _factory: &gtk::SignalListItemFactory,
        ) {
            let tree_item = list_item.item().and_downcast::<gtk::TreeListRow>().unwrap();
            let layer = tree_item.item().and_downcast::<Layer>().unwrap();
            let expander = list_item
                .child()
                .and_downcast::<gtk::TreeExpander>()
                .unwrap();
            let box_ = expander.child().unwrap();
            let toggle = box_.first_child().and_downcast::<gtk::Image>().unwrap();
            let title = box_.last_child().and_downcast::<gtk::Label>().unwrap();

            if layer.is_enabled() {
                toggle.set_icon_name(Some("view-reveal-symbolic"));
            } else {
                toggle.set_icon_name(None);
            }

            title.set_markup(layer.title().unwrap_or_default().as_str());

            let id = layer.connect_enabled_notify(move |layer| {
                if layer.is_enabled() {
                    toggle.set_icon_name(Some("view-reveal-symbolic"));
                } else {
                    toggle.set_icon_name(None);
                }
            });

            self.sig_handlers.borrow_mut().insert(layer, id);

            expander.set_list_row(Some(&tree_item));
        }

        #[template_callback]
        fn list_view_factory_unbind(
            &self,
            list_item: &gtk::ListItem,
            _factory: &gtk::SignalListItemFactory,
        ) {
            let expander = list_item
                .child()
                .and_downcast::<gtk::TreeExpander>()
                .unwrap();

            let tree_item = expander.list_row().unwrap();
            let layer = tree_item.item().and_downcast::<Layer>().unwrap();

            if let Some(id) = self.sig_handlers.borrow_mut().remove(&layer) {
                layer.disconnect(id)
            }

            expander.set_list_row(None);
        }

        fn document_layers(&self) -> Option<DocumentLayers> {
            self.obj()
                .document_model()
                .and_then(|m| m.document())
                .and_dynamic_cast::<DocumentLayers>()
                .ok()
        }

        #[template_callback]
        fn list_view_activate(&self, position: u32) {
            let tree_item = self
                .selection_model
                .item(position)
                .and_downcast::<gtk::TreeListRow>()
                .unwrap();

            let document = self.document_layers().unwrap();
            let layer = tree_item.item().and_downcast::<Layer>().unwrap();

            if !layer.is_title_only() {
                document.show_layer(&layer);

                let enabled = layer.is_enabled();

                if !enabled {
                    let group = layer.rb_group() as usize;

                    if let Some(v) = self.groups.borrow().get(&group) {
                        for l in v {
                            if l != &layer {
                                document.hide_layer(l);
                                l.set_enabled(false);
                            }
                        }
                    }
                } else {
                    document.hide_layer(&layer)
                }

                layer.set_enabled(!enabled);
                self.obj().emit_by_name::<()>("visibility-changed", &[]);
            }
        }

        fn _update_visibility(&self, model: &gio::ListModel) {
            let document = self.document_layers().unwrap();

            for layer in model {
                let layer = layer.ok().and_downcast::<Layer>().unwrap();

                if !layer.is_title_only() {
                    let enabled = document.layer_is_visible(&layer);

                    layer.set_enabled(enabled);

                    if let Some(children) = layer.children() {
                        self._update_visibility(&children);
                    }
                }
            }
        }

        pub(super) fn update_layers_visibility(&self) {
            let tree_model = self
                .selection_model
                .model()
                .and_dynamic_cast::<gtk::TreeListModel>()
                .unwrap();

            let model = tree_model.model();

            self._update_visibility(&model);
        }
    }
}

glib::wrapper! {
    pub struct PpsSidebarLayers(ObjectSubclass<imp::PpsSidebarLayers>)
        @extends PpsSidebarPage, adw::Bin, gtk::Widget,
        @implements gtk::Accessible, gtk::Buildable, gtk::ConstraintTarget;
}

impl PpsSidebarLayers {
    pub fn new() -> Self {
        glib::Object::builder().build()
    }

    pub fn update_visibility(&self) {
        self.imp().update_layers_visibility();
    }
}

impl Default for PpsSidebarLayers {
    fn default() -> Self {
        Self::new()
    }
}
