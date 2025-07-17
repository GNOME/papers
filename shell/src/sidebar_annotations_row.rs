use crate::deps::*;

use papers_document::AnnotationMarkup;
use papers_document::AnnotationTextMarkup;
use papers_document::AnnotationTextMarkupType;
use papers_document::AnnotationType;

mod imp {
    use super::*;

    #[derive(Properties, CompositeTemplate, Default, Debug)]
    #[properties(wrapper_type = super::PpsSidebarAnnotationsRow)]
    #[template(resource = "/org/gnome/papers/ui/sidebar-annotations-row.ui")]
    pub struct PpsSidebarAnnotationsRow {
        #[property(set = Self::set_annotation)]
        pub(super) annotation: RefCell<Option<AnnotationMarkup>>,
        #[property(set)]
        pub(super) document: RefCell<Option<Document>>,

        pub(super) annot_signal_handlers: RefCell<Vec<SignalHandlerId>>,

        #[template_child]
        image: TemplateChild<gtk::Image>,
        #[template_child]
        page_label: TemplateChild<gtk::Label>,
        #[template_child]
        author_label: TemplateChild<gtk::Label>,
        #[template_child]
        reference_label: TemplateChild<gtk::Label>,
        #[template_child]
        content_label: TemplateChild<gtk::Label>,
    }

    #[glib::object_subclass]
    impl ObjectSubclass for PpsSidebarAnnotationsRow {
        const NAME: &'static str = "PpsSidebarAnnotationsRow";
        type Type = super::PpsSidebarAnnotationsRow;
        type ParentType = gtk::Box;

        fn class_init(klass: &mut Self::Class) {
            klass.bind_template();
            klass.set_css_name("pps-sidebar-annotations-row")
        }

        fn instance_init(obj: &InitializingObject<Self>) {
            obj.init_template();
        }
    }

    #[glib::derived_properties]
    impl ObjectImpl for PpsSidebarAnnotationsRow {
        fn dispose(&self) {
            self.clear_annotation()
        }
    }

    impl WidgetImpl for PpsSidebarAnnotationsRow {}

    impl BoxImpl for PpsSidebarAnnotationsRow {}

    impl PpsSidebarAnnotationsRow {
        fn set_row_tooltip(&self, annot: Option<&AnnotationMarkup>) {
            let tooltip = annot.filter(|annot| annot.label().is_some()).map(|annot| {
                let label = annot.label().unwrap();
                annot.modified().map_or(
                    format!("<span weight=\"bold\">{label}</span>"),
                    |modified| format!("<span weight=\"bold\">{label}</span>\n{modified}"),
                )
            });
            self.obj().set_tooltip_markup(tooltip.as_deref());
        }

        fn set_page_label(&self, annot: Option<&AnnotationMarkup>) {
            let author = annot
                .map(|annot| {
                    let page = annot.page_index();
                    let page_label = self
                        .document()
                        .and_then(|document| document.page_label(page as i32))
                        .map(|gstr| gstr.to_string())
                        .unwrap_or(page.to_string());
                    gettext_f("Page {}", [page_label])
                })
                .unwrap_or_default();
            self.page_label.set_label(&author);
        }

        fn set_author_label(&self, annot: Option<&AnnotationMarkup>) {
            let author = annot
                .and_then(|annot| annot.label())
                .map(|gstr| gstr.to_string())
                .unwrap_or_default();
            self.author_label.set_label(&author);
        }

        fn set_content_label(&self, annot: Option<&AnnotationMarkup>) {
            self.content_label.set_visible(false);
            if let Some(markup) = annot
                .and_then(|annot| annot.contents())
                .filter(|s| !s.is_empty())
                .map(|content| glib::markup_escape_text(&content).trim().to_string())
            {
                self.content_label.set_label(&markup);
                self.content_label.set_visible(true);
            }
        }

        fn set_reference_label(&self, annot: Option<&AnnotationMarkup>) {
            let reference_text = annot
                .map(|annot| (annot.page_index(), annot.area()))
                .and_then(|(page_index, mut area)| {
                    self.document().and_then(|document| {
                        let page = document.page(page_index as i32);
                        document
                            .dynamic_cast_ref::<DocumentText>()
                            .and_then(|document_text| {
                                page.and_then(|page| document_text.text_in_area(&page, &mut area))
                            })
                            .map(|gstr| gstr.to_string())
                    })
                });

            if let Some(label) = &reference_text {
                self.reference_label.set_label(label);
            }
            self.reference_label.set_visible(reference_text.is_some());
        }

        fn set_color(&self, annot: Option<&AnnotationMarkup>) {
            let color = annot
                .map(|annot| annot.rgba().to_str())
                .map(|gstr| gstr.to_string())
                .unwrap_or("rgb(100 0 0 / 100%)".to_string());
            let name = annot
                .and_then(|annot| annot.name())
                .map(|gstr| gstr.as_str().to_string())
                .unwrap_or("default".to_string());
            let page_index = annot.map(|annot| annot.page_index() as i32).unwrap_or(-1);
            let annotation_id_class = format!("annotation-{page_index}-{name}");
            let css = format!(
                "pps-sidebar-annotations-row.{annotation_id_class} {{ --annotation-color: {color}; }}"
            );
            let provider = gtk::CssProvider::new();

            self.obj().add_css_class(annotation_id_class.as_str());

            provider.load_from_string(&css);

            match gdk::Display::default() {
                Some(display) => {
                    gtk::style_context_add_provider_for_display(
                        &display,
                        &provider,
                        gtk::STYLE_PROVIDER_PRIORITY_APPLICATION,
                    );
                }
                _ => glib::g_critical!("", "Could not find a display"),
            }
        }

        fn markup_type_to_icon_name(&self, markup_type: AnnotationTextMarkupType) -> Option<&str> {
            match markup_type {
                AnnotationTextMarkupType::StrikeOut => Some("format-text-strikethrough-symbolic"),
                AnnotationTextMarkupType::Underline
                | AnnotationTextMarkupType::Squiggly
                | AnnotationTextMarkupType::Highlight => None,
                _ => unimplemented!(),
            }
        }

        fn set_markup_icon_name(&self, annot: Option<&AnnotationTextMarkup>) {
            if let Some(annot) = annot {
                let icon_name = self.markup_type_to_icon_name(annot.markup_type());
                self.image.set_icon_name(icon_name);
                self.image.set_visible(icon_name.is_some());
            }
        }

        fn document(&self) -> Option<Document> {
            self.document.borrow().clone()
        }

        fn clear_annotation(&self) {
            if let Some(annot) = self.annotation.take() {
                for id in self.annot_signal_handlers.take() {
                    annot.disconnect(id);
                }
            }
        }

        fn set_annotation(&self, annot: Option<&AnnotationMarkup>) {
            if self.annotation.borrow().as_ref() == annot {
                return;
            }

            self.clear_annotation();

            // setup the new one
            self.set_row_tooltip(annot);
            self.set_page_label(annot);
            self.set_author_label(annot);
            self.set_content_label(annot);
            self.set_reference_label(annot);
            self.set_color(annot);

            if let Some(annot) = annot {
                let mut handlers = self.annot_signal_handlers.borrow_mut();

                handlers.push(annot.connect_label_notify(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |annot| {
                        obj.set_row_tooltip(Some(annot));
                        obj.set_author_label(Some(annot));
                    }
                )));

                handlers.push(annot.connect_modified_notify(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |annot| {
                        obj.set_row_tooltip(Some(annot));
                    }
                )));

                handlers.push(annot.connect_contents_notify(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |annot| {
                        obj.set_content_label(Some(annot));
                    }
                )));

                handlers.push(annot.connect_rgba_notify(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |annot| {
                        obj.set_color(Some(annot));
                    }
                )));

                if let Some(markup_annot) = annot.dynamic_cast_ref::<AnnotationTextMarkup>() {
                    handlers.push(markup_annot.connect_type_notify(glib::clone!(
                        #[weak(rename_to = obj)]
                        self,
                        move |markup_annot| {
                            obj.set_markup_icon_name(Some(markup_annot));
                        }
                    )))
                }
            }

            let icon_name = annot.and_then(|annot| match annot.annotation_type() {
                AnnotationType::Attachment => Some("mail-attachment-symbolic"),
                AnnotationType::TextMarkup => self.markup_type_to_icon_name(
                    annot
                        .dynamic_cast_ref::<AnnotationTextMarkup>()
                        .unwrap()
                        .markup_type(),
                ),
                AnnotationType::Text | AnnotationType::FreeText | AnnotationType::Stamp => None,
                _ => unimplemented!(),
            });
            self.image.set_icon_name(icon_name);
            self.image.set_visible(icon_name.is_some());

            self.annotation.replace(annot.cloned());
        }
    }
}

glib::wrapper! {
    pub struct PpsSidebarAnnotationsRow(ObjectSubclass<imp::PpsSidebarAnnotationsRow>)
        @extends gtk::Box, gtk::Widget,
        @implements gtk::Accessible, gtk::Buildable, gtk::ConstraintTarget;
}

impl PpsSidebarAnnotationsRow {
    pub fn new() -> Self {
        glib::Object::builder().build()
    }
}

impl Default for PpsSidebarAnnotationsRow {
    fn default() -> Self {
        Self::new()
    }
}
