use crate::deps::*;

use glib::property::PropertySet;
use papers_document::DocumentFonts;

mod imp {
    use super::*;

    #[derive(Properties, Default, Debug, CompositeTemplate)]
    #[properties(wrapper_type = super::PpsPropertiesWindow)]
    #[template(resource = "/org/gnome/papers/ui/properties-window.ui")]
    pub struct PpsPropertiesWindow {
        #[template_child]
        pub(super) stack: TemplateChild<adw::ViewStack>,
        #[template_child]
        pub(super) fonts: TemplateChild<PpsPropertiesFonts>,
        #[template_child]
        pub(super) license: TemplateChild<PpsPropertiesLicense>,
        #[template_child]
        pub(super) signatures: TemplateChild<PpsPropertiesSignatures>,
        #[template_child]
        pub(super) fonts_page: TemplateChild<adw::ViewStackPage>,
        #[template_child]
        pub(super) license_page: TemplateChild<adw::ViewStackPage>,
        #[template_child]
        pub(super) signatures_page: TemplateChild<adw::ViewStackPage>,
        #[template_child]
        pub(super) view_switcher: TemplateChild<adw::ViewSwitcher>,
        #[template_child]
        pub(super) header_bar: TemplateChild<adw::HeaderBar>,
        #[property(get, nullable, set = Self::set_document)]
        pub(super) document: RefCell<Option<Document>>,
        #[property(get, nullable, set = Self::set_visible_page_name)]
        pub(super) visible_page_name: RefCell<String>,
    }

    #[glib::object_subclass]
    impl ObjectSubclass for PpsPropertiesWindow {
        const NAME: &'static str = "PpsPropertiesWindow";
        type Type = super::PpsPropertiesWindow;
        type ParentType = adw::Dialog;

        fn class_init(klass: &mut Self::Class) {
            PpsPropertiesFonts::ensure_type();
            PpsPropertiesGeneral::ensure_type();
            PpsPropertiesLicense::ensure_type();
            PpsPropertiesSignatures::ensure_type();

            klass.bind_template();
        }

        fn instance_init(obj: &InitializingObject<Self>) {
            obj.init_template();
        }
    }

    #[glib::derived_properties]
    impl ObjectImpl for PpsPropertiesWindow {}

    impl AdwDialogImpl for PpsPropertiesWindow {}

    impl WindowImpl for PpsPropertiesWindow {}

    impl WidgetImpl for PpsPropertiesWindow {}

    impl PpsPropertiesWindow {
        fn set_document(&self, document: Option<Document>) {
            if document == self.document.borrow().clone() {
                return;
            }

            if let Some(ref document) = document {
                let license = document.info().and_then(|i| i.license());
                let has_license = license.is_some();
                let has_fonts = document.dynamic_cast_ref::<DocumentFonts>().is_some();
                let mut has_signatures = false;

                if has_fonts {
                    self.fonts.imp().set_document(document.clone());
                }

                if let Some(mut license) = license {
                    self.license.imp().set_license(&mut license);
                }

                if let Some(signatures) = document.dynamic_cast_ref::<DocumentSignatures>() {
                    has_signatures = signatures.has_signatures();
                    if has_signatures {
                        self.signatures.imp().set_document(document.clone());
                    }
                }

                self.fonts_page.set_visible(has_fonts);
                self.license_page.set_visible(has_license);
                self.signatures_page.set_visible(has_signatures);
            } else {
                self.fonts_page.set_visible(false);
                self.license_page.set_visible(false);
                self.signatures_page.set_visible(false);
            }

            if self.fonts_page.is_visible()
                || self.license_page.is_visible()
                || self.signatures_page.is_visible()
            {
                self.header_bar
                    .set_title_widget(Some(&self.view_switcher.get()));
            } else {
                self.header_bar.set_title_widget(gtk::Widget::NONE);
            }

            self.document.replace(document);
        }

        fn set_visible_page_name(&self, page_name: String) {
            if page_name.as_str() == "fonts"
                || page_name.as_str() == "license"
                || page_name.as_str() == "signatures"
            {
                self.stack.set_visible_child_name(page_name.as_str());
                self.visible_page_name.set(page_name);
            }
        }
    }
}

glib::wrapper! {
    pub struct PpsPropertiesWindow(ObjectSubclass<imp::PpsPropertiesWindow>)
        @extends adw::Dialog, gtk::Widget, gtk::Window,
        @implements gtk::Accessible, gtk::Buildable, gtk::ConstraintTarget, gtk::ShortcutManager, gtk::Root, gtk::Native;
}

impl Default for PpsPropertiesWindow {
    fn default() -> Self {
        Self::new()
    }
}

impl PpsPropertiesWindow {
    pub fn new() -> Self {
        glib::Object::builder().build()
    }
}
