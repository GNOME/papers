use crate::deps::*;

use gdk::{ScrollDirection, ScrollEvent};

mod imp {
    use super::*;

    #[derive(Properties, Default, Debug, CompositeTemplate)]
    #[properties(wrapper_type = super::PpsPageSelector)]
    #[template(resource = "/org/gnome/papers/ui/page-selector.ui")]
    pub struct PpsPageSelector {
        #[template_child]
        pub(super) entry: TemplateChild<gtk::Entry>,
        #[template_child]
        pub(super) label: TemplateChild<gtk::Label>,
        #[property(nullable, set = Self::set_model)]
        pub(super) model: RefCell<Option<DocumentModel>>,
        pub(super) document_notify_handler: RefCell<Option<SignalHandlerId>>,
        pub(super) page_changed_handler: RefCell<Option<SignalHandlerId>>,
    }

    #[glib::object_subclass]
    impl ObjectSubclass for PpsPageSelector {
        const NAME: &'static str = "PpsPageSelector";
        type Type = super::PpsPageSelector;
        type ParentType = gtk::Box;

        fn class_init(klass: &mut Self::Class) {
            klass.bind_template();
            klass.bind_template_callbacks();
        }

        fn instance_init(obj: &InitializingObject<Self>) {
            obj.init_template();
        }
    }

    #[glib::derived_properties]
    impl ObjectImpl for PpsPageSelector {
        fn signals() -> &'static [Signal] {
            static SIGNALS: OnceLock<Vec<Signal>> = OnceLock::new();
            SIGNALS.get_or_init(|| {
                vec![Signal::builder("activate-link")
                    .param_types([papers_document::Link::static_type()])
                    .run_last()
                    .build()]
            })
        }
    }

    impl BoxImpl for PpsPageSelector {}

    impl WidgetImpl for PpsPageSelector {
        fn grab_focus(&self) -> bool {
            self.entry.grab_focus()
        }
    }

    #[gtk::template_callbacks]
    impl PpsPageSelector {
        #[template_callback]
        fn activated(&self) {
            let Some(model) = self.model() else {
                return;
            };

            let current_page = model.page();

            // Convert utf8 fullwidth numbers (eg. japanese) to halfwidth - fixes #1518
            let text = glib::normalize(self.entry.text(), glib::NormalizeMode::All);
            self.entry.set_text(text.as_str());

            let text = self.entry.text();

            let link_dest = papers_document::LinkDest::new_page_label(text.as_str());
            let link_action = papers_document::LinkAction::new_dest(&link_dest);
            let link_text = gettext_f("Page {}", [text]);
            let link = papers_document::Link::new(Some(&link_text), &link_action);

            self.obj().emit_by_name::<()>("activate-link", &[&link]);

            if current_page == model.page() {
                self.set_current_page(current_page);
            }
        }

        #[template_callback]
        fn page_scrolled(&self, _: f64, _: f64, controller: gtk::EventControllerScroll) -> bool {
            let Some(direction) = controller
                .current_event()
                .and_then(|e| e.downcast::<ScrollEvent>().ok())
                .map(|e| e.direction())
            else {
                return false;
            };

            let Some(model) = self.model() else {
                return false;
            };
            let Some(document) = self.document() else {
                return false;
            };

            let n_pages = document.n_pages();
            let mut page = model.page();

            if direction == ScrollDirection::Down && page < n_pages - 1 {
                page += 1;
            }

            if direction == ScrollDirection::Up && page > 0 {
                page -= 1;
            }

            model.set_page(page);

            true
        }

        #[template_callback]
        fn focused_out(&self) {
            if let Some(model) = self.model() {
                self.set_current_page(model.page());
            }

            EntryExt::set_alignment(&self.entry.get(), 0.9);
            self.update_max_width();
        }
    }

    impl PpsPageSelector {
        fn model(&self) -> Option<DocumentModel> {
            self.model.borrow().clone()
        }

        fn document(&self) -> Option<Document> {
            self.model().and_then(|m| m.document())
        }

        fn clear_model(&self) {
            if let Some(model) = self.model.take() {
                if let Some(id) = self.document_notify_handler.take() {
                    model.disconnect(id);
                }

                if let Some(id) = self.page_changed_handler.take() {
                    model.disconnect(id);
                }
            }
        }

        fn set_model(&self, model: DocumentModel) {
            self.clear_model();

            let id = model.connect_document_notify(glib::clone!(
                #[weak(rename_to = obj)]
                self,
                move |_| {
                    obj.update_document();
                }
            ));

            self.document_notify_handler.replace(Some(id));

            let id = model.connect_page_changed(glib::clone!(
                #[weak(rename_to = obj)]
                self,
                move |_, _, new_page| {
                    obj.set_current_page(new_page);
                }
            ));

            self.page_changed_handler.replace(Some(id));

            self.model.replace(Some(model));
        }

        fn update_document(&self) {
            let document = self.document();

            self.obj()
                .set_sensitive(document.is_some_and(|d| d.n_pages() > 0));

            if let Some(model) = self.model() {
                self.set_current_page(model.page());
            }

            self.update_max_width();
        }

        fn set_current_page(&self, page: i32) {
            if page >= 0 {
                if let Some(document) = self.document() {
                    let page_label = document.page_label(page);

                    self.entry.set_text(&page_label.unwrap_or_default());
                    self.entry.set_position(-1);
                } else {
                    self.entry.set_text("");
                }

                self.update_pages_label(page);
            }
        }

        fn show_page_number_in_pages_label(&self, page: i32) -> bool {
            let Some(document) = self.document() else {
                return false;
            };

            if !document.has_text_page_labels() {
                return false;
            }

            let page_label = format!("{}", page + 1);
            let entry_text = self.entry.text();

            page_label != entry_text.as_str()
        }

        fn update_pages_label(&self, page: i32) {
            if let Some(document) = self.document() {
                let n_pages = document.n_pages();

                let label_text = if self.show_page_number_in_pages_label(page) {
                    gettext_fd(
                        // Translators: Do NOT translate the content between
                        // '{' and '}' they are variable names. Changing their
                        // order is possible
                        "({pagenum} of {totalpages})",
                        &[
                            ("pagenum", &(page + 1).to_string()),
                            ("totalpages", &n_pages.to_string()),
                        ],
                    )
                } else {
                    // Translators: the placeholder is the total amount of pages
                    gettext_f("of {}", [n_pages.to_string()])
                };

                self.label.set_text(&label_text);
            }
        }

        fn update_max_width(&self) {
            let Some(document) = self.document() else {
                return;
            };

            let n_pages = document.n_pages();
            let max_page_numeric_label = format!("{n_pages}").len() as i32;

            let max_label_len = if document.has_text_page_labels() {
                gettext_fd(
                    // Translators: Do NOT translate the content between
                    // '{' and '}' they are variable names. Changing their
                    // order is possible
                    "({pagenum} of {totalpages})",
                    &[
                        ("pagenum", &n_pages.to_string()),
                        ("totalpages", &n_pages.to_string()),
                    ],
                )
                .len()
                    - 2
            } else {
                // Translators: the placeholder is the total amount of pages
                gettext_f("of {}", [n_pages.to_string()]).len()
            } as i32;

            self.label.set_width_chars(max_label_len);

            let max_label_len = document.max_label_len().clamp(max_page_numeric_label, 12);
            self.entry.set_width_chars(max_label_len);
        }
    }
}

glib::wrapper! {
    pub struct PpsPageSelector(ObjectSubclass<imp::PpsPageSelector>)
        @extends gtk::Box, gtk::Widget,
        @implements gtk::Accessible, gtk::Buildable, gtk::ConstraintTarget;
}

impl PpsPageSelector {
    pub fn new() -> Self {
        glib::Object::builder().build()
    }
}

impl Default for PpsPageSelector {
    fn default() -> Self {
        Self::new()
    }
}
