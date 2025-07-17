use crate::deps::*;

use papers_document::DocumentFonts;
use papers_view::{JobFonts, JobPriority};

mod imp {
    use super::*;

    #[derive(CompositeTemplate, Debug, Default)]
    #[template(resource = "/org/gnome/papers/ui/properties-fonts.ui")]
    pub struct PpsPropertiesFonts {
        #[template_child]
        list_box: TemplateChild<gtk::ListBox>,
        #[template_child]
        fonts_page: TemplateChild<adw::PreferencesPage>,
        #[template_child]
        stack: TemplateChild<adw::ViewStack>,

        fonts_job: RefCell<Option<JobFonts>>,
        document: RefCell<Option<Document>>,
        job_handler_id: RefCell<Option<SignalHandlerId>>,
    }

    impl PpsPropertiesFonts {
        fn document(&self) -> Option<Document> {
            self.document.borrow().clone()
        }

        pub fn set_document(&self, document: Document) {
            if self.document().is_some_and(|d| d == document) {
                return;
            }

            let job = JobFonts::new(&document);

            let job_handler_id = job.connect_finished(glib::clone!(
                #[weak(rename_to = obj)]
                self,
                move |job| {
                    if let Some(id) = obj.job_handler_id.take() {
                        job.disconnect(id);
                    }

                    if let Some(doc_fonts) = job.document().and_dynamic_cast_ref::<DocumentFonts>()
                    {
                        let model = doc_fonts.model();
                        obj.list_box.bind_model(model.as_ref(), |obj| {
                            let row = adw::ActionRow::new();
                            let font = obj
                                .downcast_ref::<papers_document::FontDescription>()
                                .unwrap();

                            row.set_title(&font.name().unwrap_or_default());
                            row.set_subtitle(&font.details().unwrap_or_default());

                            row.into()
                        });

                        if let Some(model) = model {
                            obj.list_box.set_visible(model.n_items() != 0);
                        }

                        let fonts_summary = doc_fonts.fonts_summary().unwrap_or_default();
                        obj.fonts_page.set_description(fonts_summary.as_str());

                        obj.stack.set_visible_child_name("fonts");
                    }
                }
            ));

            self.stack.set_visible_child_name("load");

            job.scheduler_push_job(JobPriority::PriorityNone);

            self.document.replace(Some(document));
            self.fonts_job.replace(Some(job));
            self.job_handler_id.replace(Some(job_handler_id));
        }
    }

    #[glib::object_subclass]
    impl ObjectSubclass for PpsPropertiesFonts {
        const NAME: &'static str = "PpsPropertiesFonts";
        type Type = super::PpsPropertiesFonts;
        type ParentType = adw::Bin;

        fn class_init(klass: &mut Self::Class) {
            klass.bind_template();
        }

        fn instance_init(obj: &InitializingObject<Self>) {
            obj.init_template();
        }
    }

    impl ObjectImpl for PpsPropertiesFonts {
        fn dispose(&self) {
            if let Some(job) = self.fonts_job.borrow().as_ref() {
                if let Some(id) = self.job_handler_id.take() {
                    job.disconnect(id);
                }

                job.cancel();
            }
        }
    }

    impl WidgetImpl for PpsPropertiesFonts {}

    impl BinImpl for PpsPropertiesFonts {}
}

glib::wrapper! {
    pub struct PpsPropertiesFonts(ObjectSubclass<imp::PpsPropertiesFonts>)
    @extends gtk::Widget, adw::Bin,
    @implements gtk::Accessible, gtk::Buildable, gtk::ConstraintTarget;
}

impl Default for PpsPropertiesFonts {
    fn default() -> Self {
        Self::new()
    }
}

impl PpsPropertiesFonts {
    fn new() -> PpsPropertiesFonts {
        glib::Object::builder().build()
    }
}
