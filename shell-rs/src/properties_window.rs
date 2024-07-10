use crate::deps::*;

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
        pub(super) fonts: TemplateChild<crate::properties_fonts::PpsPropertiesFonts>,
        #[template_child]
        pub(super) license: TemplateChild<crate::properties_license::PpsPropertiesLicense>,
        #[template_child]
        pub(super) fonts_page: TemplateChild<adw::ViewStackPage>,
        #[template_child]
        pub(super) license_page: TemplateChild<adw::ViewStackPage>,
        #[template_child]
        pub(super) view_switcher: TemplateChild<adw::ViewSwitcher>,
        #[template_child]
        pub(super) header_bar: TemplateChild<adw::HeaderBar>,
        #[property(get, nullable, set = Self::set_document)]
        pub(super) document: RefCell<Option<Document>>,
    }

    #[glib::object_subclass]
    impl ObjectSubclass for PpsPropertiesWindow {
        const NAME: &'static str = "PpsPropertiesWindow";
        type Type = super::PpsPropertiesWindow;
        type ParentType = adw::Dialog;

        fn class_init(klass: &mut Self::Class) {
            crate::properties_fonts::PpsPropertiesFonts::ensure_type();
            crate::properties_general::PpsPropertiesGeneral::ensure_type();
            crate::properties_license::PpsPropertiesLicense::ensure_type();

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

                if has_fonts {
                    self.fonts.imp().set_document(document.clone());
                }

                if let Some(mut license) = license {
                    self.license.imp().set_license(&mut license);
                }

                self.fonts_page.set_visible(has_fonts);
                self.license_page.set_visible(has_license);
            } else {
                self.fonts_page.set_visible(false);
                self.license_page.set_visible(false);
            }

            if self.fonts_page.is_visible() || self.license_page.is_visible() {
                self.header_bar
                    .set_title_widget(Some(&self.view_switcher.clone()));
            } else {
                self.header_bar.set_title_widget(gtk::Widget::NONE);
            }

            self.document.replace(document);
        }
    }
}

glib::wrapper! {
    pub struct PpsPropertiesWindow(ObjectSubclass<imp::PpsPropertiesWindow>)
        @extends adw::Dialog, gtk::Widget;
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
