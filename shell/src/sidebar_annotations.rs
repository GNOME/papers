use crate::deps::*;
use papers_document::{Annotation, AnnotationMarkup, DocumentAnnotations};
use papers_view::AnnotationsContext;

use gtk::graphene;

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
        #[template_child]
        popup: TemplateChild<gtk::PopoverMenu>,
        #[template_child]
        selection_model: TemplateChild<gtk::SingleSelection>,
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
    impl ObjectImpl for PpsSidebarAnnotations {
        fn signals() -> &'static [Signal] {
            static SIGNALS: OnceLock<Vec<Signal>> = OnceLock::new();
            SIGNALS.get_or_init(|| {
                vec![Signal::builder("annot-activated")
                    .run_last()
                    .action()
                    .param_types([Annotation::static_type()])
                    .build()]
            })
        }

        fn constructed(&self) {
            self.obj().connect_closure(
                "annot-activated",
                true,
                glib::closure_local!(move |obj: super::PpsSidebarAnnotations, _: Annotation| {
                    obj.navigate_to_view();
                }),
            );
        }
    }

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

            let binding = context.as_ref().and_then(|context| context.annots_model());
            let model = binding.as_ref().unwrap();

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

            self.selection_model.set_model(Some(model));

            self.annotations_context.replace(context);
        }

        #[template_callback]
        fn list_view_factory_setup(&self, item: &gtk::ListItem, _factory: &gtk::ListItemFactory) {
            let row = PpsSidebarAnnotationsRow::new();

            let gesture = gtk::GestureClick::builder().button(0).build();

            gesture.connect_pressed(glib::clone!(
                #[weak(rename_to = obj)]
                self,
                #[weak]
                item,
                move |gesture, _, x, y| {
                    let annot = item.item().and_downcast::<AnnotationMarkup>().unwrap();

                    match gesture.current_button() {
                        gdk::BUTTON_PRIMARY => {
                            obj.obj().emit_by_name::<()>("annot-activated", &[&annot])
                        }
                        gdk::BUTTON_SECONDARY => {
                            let document_view = obj
                                .obj()
                                .ancestor(PpsDocumentView::static_type())
                                .and_downcast::<PpsDocumentView>()
                                .unwrap();
                            let row = item.child().unwrap();

                            document_view.handle_annot_popup(&annot);

                            let point = row
                                .compute_point(
                                    &obj.popup.parent().unwrap(),
                                    &graphene::Point::new(x as f32, y as f32),
                                )
                                .unwrap();

                            obj.popup.set_pointing_to(Some(&gdk::Rectangle::new(
                                point.x() as i32,
                                point.y() as i32,
                                1,
                                1,
                            )));
                            obj.popup.popup();
                        }
                        _ => (),
                    }
                }
            ));

            row.add_controller(gesture);
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
