use crate::deps::*;

mod imp {
    use super::*;

    #[derive(Properties, Default, Debug, CompositeTemplate)]
    #[properties(wrapper_type = super::PpsSidebar)]
    #[template(resource = "/org/gnome/papers/ui/sidebar.ui")]
    pub struct PpsSidebar {
        #[template_child]
        pub(super) stack: TemplateChild<adw::ViewStack>,
        #[property(name = "document-model", set = Self::set_model, construct_only)]
        pub(super) model: RefCell<Option<DocumentModel>>,
        #[property(name= "visible-child-name", type = Option<String>, set = Self::set_visible_child_name, get = Self::visible_child_name)]
        pub(super) _stub: (),
    }

    #[glib::object_subclass]
    impl ObjectSubclass for PpsSidebar {
        const NAME: &'static str = "PpsSidebar";
        type Type = super::PpsSidebar;
        type ParentType = adw::Bin;
        type Interfaces = (gtk::Buildable,);

        fn class_init(klass: &mut Self::Class) {
            klass.bind_template();
            klass.bind_template_callbacks();
        }

        fn instance_init(obj: &InitializingObject<Self>) {
            obj.init_template();
        }
    }

    #[glib::derived_properties]
    impl ObjectImpl for PpsSidebar {
        fn signals() -> &'static [Signal] {
            static SIGNALS: OnceLock<Vec<Signal>> = OnceLock::new();
            SIGNALS.get_or_init(|| vec![Signal::builder("navigated-to-view").run_last().build()])
        }
    }

    impl BinImpl for PpsSidebar {}

    impl WidgetImpl for PpsSidebar {}

    impl BuildableImpl for PpsSidebar {
        fn internal_child(&self, builder: &gtk::Builder, name: &str) -> Option<glib::Object> {
            if name == "stack" {
                return Some(self.stack.clone().into());
            }

            self.parent_internal_child(builder, name)
        }
    }

    impl PpsSidebar {
        fn document_changed(&self) {
            let Some(document) = self.document() else {
                return;
            };

            let mut first_supported_page = None;

            for page in self.stack.pages().iter::<adw::ViewStackPage>() {
                let page = page.unwrap();
                let sidebar_page = page.child();
                let supported = sidebar_page
                    .dynamic_cast_ref::<PpsSidebarPage>()
                    .unwrap()
                    .support_document(&document);

                page.set_visible(supported);

                if supported && first_supported_page.is_none() {
                    first_supported_page = Some(sidebar_page);
                }
            }

            if let Some(page) = first_supported_page {
                if !self
                    .stack
                    .visible_child()
                    .and_dynamic_cast::<PpsSidebarPage>()
                    .map(|p| p.support_document(&document))
                    .unwrap_or_default()
                {
                    self.stack.set_visible_child(&page);
                }
            } else {
                self.obj().set_visible(false);
            }
        }

        fn set_model(&self, model: DocumentModel) {
            model.connect_document_notify(glib::clone!(
                #[weak(rename_to = obj)]
                self,
                move |_| {
                    obj.document_changed();
                }
            ));

            self.model.replace(Some(model));
        }

        fn document(&self) -> Option<Document> {
            self.model.borrow().as_ref().and_then(|m| m.document())
        }

        fn visible_child_name(&self) -> Option<String> {
            self.stack.visible_child_name().map(|gs| gs.to_string())
        }

        fn set_visible_child_default(&self) {
            let Some(document) = self.document() else {
                return;
            };
            if self
                .stack
                .child_by_name("links")
                .unwrap()
                .dynamic_cast_ref::<PpsSidebarPage>()
                .unwrap()
                .support_document(&document)
            {
                self.stack.set_visible_child_name("links");
            } else {
                self.stack.set_visible_child_name("thumbnails");
            }
        }

        fn set_visible_child_name(&self, name: Option<String>) {
            let Some(document) = self.document() else {
                return;
            };
            let Some(name) = name else {
                self.set_visible_child_default();
                return;
            };

            if ![
                "annotations",
                "attachments",
                "layers",
                "links",
                "thumbnails",
            ]
            .contains(&name.as_str())
            {
                self.set_visible_child_default();
                return;
            }

            let page = self.stack.child_by_name(&name).unwrap();

            if page
                .dynamic_cast_ref::<PpsSidebarPage>()
                .unwrap()
                .support_document(&document)
            {
                self.stack.set_visible_child(&page);
            } else {
                self.set_visible_child_default();
            }
        }
    }

    #[gtk::template_callbacks]
    impl PpsSidebar {
        #[template_callback]
        fn visible_child_changed(&self) {
            if self.stack.visible_child_name().is_some() {
                self.obj().notify_visible_child_name();
            }
        }
    }
}

glib::wrapper! {
    pub struct PpsSidebar(ObjectSubclass<imp::PpsSidebar>)
        @extends adw::Bin, gtk::Widget,
        @implements gtk::Accessible, gtk::Buildable, gtk::ConstraintTarget;
}

impl PpsSidebar {
    pub fn new() -> Self {
        glib::Object::builder().build()
    }
}

impl Default for PpsSidebar {
    fn default() -> Self {
        Self::new()
    }
}
