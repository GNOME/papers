use crate::deps::*;
use papers_document::{AnnotationMarkup, DocumentAnnotations};
use papers_view::AnnotationsContext;

mod imp {
    use super::*;

    #[derive(CompositeTemplate, Debug, Default, Properties)]
    #[properties(wrapper_type = super::PpsSidebarAnnotations)]
    #[template(resource = "/org/gnome/papers/ui/sidebar-annotations.ui")]
    pub struct PpsSidebarAnnotations {
        #[property(set = Self::set_annotations_context, get)]
        annotations_context: RefCell<Option<AnnotationsContext>>,

        #[template_child]
        list_view: TemplateChild<gtk::ListView>,
        #[template_child]
        stack: TemplateChild<adw::ViewStack>,
    }

    #[glib::object_subclass]
    impl ObjectSubclass for PpsSidebarAnnotations {
        const NAME: &'static str = "PpsSidebarAnnotations";
        type Type = super::PpsSidebarAnnotations;
        type ParentType = PpsSidebarPage;

        fn class_init(klass: &mut Self::Class) {
            klass.bind_template();
            klass.bind_template_callbacks();
        }

        fn instance_init(obj: &InitializingObject<Self>) {
            obj.init_template();
        }
    }

    #[glib::derived_properties]
    impl ObjectImpl for PpsSidebarAnnotations {}

    impl WidgetImpl for PpsSidebarAnnotations {}

    impl BinImpl for PpsSidebarAnnotations {}

    impl PpsSidebarPageImpl for PpsSidebarAnnotations {
        fn support_document(&self, document: &Document) -> bool {
            document.is::<DocumentAnnotations>()
        }
    }

    #[gtk::template_callbacks]
    impl PpsSidebarAnnotations {
        fn annotations_context(&self) -> Option<AnnotationsContext> {
            self.annotations_context.borrow().clone()
        }

        fn set_annotations_context(&self, context: Option<AnnotationsContext>) {
            if self.annotations_context() == context {
                return;
            }

            let model = context
                .as_ref()
                .and_then(|context| context.annots_model())
                .and_downcast::<gtk::SingleSelection>()
                .unwrap();

            model.connect_items_changed(glib::clone!(
                #[weak(rename_to = obj)]
                self,
                move |model, _, _, _| {
                    if model.n_items() > 0 {
                        obj.stack.set_visible_child_name("annot");
                    } else {
                        obj.stack.set_visible_child_name("empty");
                    }
                }
            ));

            model.connect_selected_item_notify(glib::clone!(
                #[weak(rename_to = obj)]
                self,
                #[weak]
                model,
                move |_| {
                    let position = model.selected();
                    if position != gtk::INVALID_LIST_POSITION {
                        obj.list_view
                            .scroll_to(
                                position,
                                gtk::ListScrollFlags::FOCUS | gtk::ListScrollFlags::SELECT,
                                None,
                            );
                    }
                }
            ));

            self.list_view.set_model(Some(&model));

            self.annotations_context.replace(context);
        }

        #[template_callback]
        fn list_view_factory_setup(&self, item: &gtk::ListItem, _factory: &gtk::ListItemFactory) {
            let row = PpsSidebarAnnotationsRow::new();
            item.set_child(Some(&row));
        }

        #[template_callback]
        fn list_view_factory_bind(&self, item: &gtk::ListItem, _factory: &gtk::ListItemFactory) {
            let row = item
                .child()
                .and_downcast::<PpsSidebarAnnotationsRow>()
                .unwrap();
            let document = self
                .obj()
                .document_model()
                .and_then(|m| m.document())
                .unwrap();
            let annot = item.item().and_downcast::<AnnotationMarkup>().unwrap();

            row.set_document(document);
            row.set_annotation(annot);
        }

        #[template_callback]
        fn list_view_row_activated(&self, position: u32) {
            let model = self
                .annotations_context()
                .and_then(|context| context.annots_model())
                .and_downcast::<gtk::SingleSelection>()
                .unwrap();

            // HACK: re-select item to let the view scroll after activation
            let freeze_guard = model.freeze_notify();
            model.set_selected(gtk::INVALID_LIST_POSITION);
            self.list_view.scroll_to(
                position,
                gtk::ListScrollFlags::FOCUS | gtk::ListScrollFlags::SELECT,
                None,
            );
            drop(freeze_guard);

            self.obj().navigate_to_view();
        }
    }
}

glib::wrapper! {
    pub struct PpsSidebarAnnotations(ObjectSubclass<imp::PpsSidebarAnnotations>)
    @extends gtk::Widget, adw::Bin, PpsSidebarPage,
    @implements gtk::Accessible, gtk::Buildable, gtk::ConstraintTarget;
}

impl Default for PpsSidebarAnnotations {
    fn default() -> Self {
        Self::new()
    }
}

impl PpsSidebarAnnotations {
    pub fn new() -> Self {
        glib::Object::builder().build()
    }
}
