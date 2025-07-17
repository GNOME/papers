use crate::deps::*;
use papers_document::DocumentLicense;

mod imp {
    use super::*;

    fn set_uri_to_label(label: &gtk::Label, uri: &str) {
        if glib::uri_parse_scheme(uri).is_some() {
            let escaped = glib::markup_escape_text(uri);
            let markup = glib::gformat!("<a href=\"{}\">{}</a>", escaped, escaped);
            label.set_markup(markup.as_str());
        } else {
            label.set_text(uri);
        }
    }

    #[derive(CompositeTemplate, Debug, Default)]
    #[template(resource = "/org/gnome/papers/ui/properties-license.ui")]
    pub struct PpsPropertiesLicense {
        #[template_child(id = "license_box")]
        license_box: TemplateChild<gtk::Box>,
        #[template_child(id = "license")]
        license: TemplateChild<gtk::Label>,
        #[template_child(id = "uri_box")]
        uri_box: TemplateChild<gtk::Box>,
        #[template_child(id = "uri")]
        uri: TemplateChild<gtk::Label>,
        #[template_child(id = "web_statement_box")]
        web_statement_box: TemplateChild<gtk::Box>,
        #[template_child(id = "web_statement")]
        web_statement: TemplateChild<gtk::Label>,
    }

    impl PpsPropertiesLicense {
        pub fn set_license(&self, license: &mut DocumentLicense) {
            match license.clone().text() {
                Some(text) => {
                    self.license_box.set_visible(true);
                    self.license.set_text(text.as_str())
                }
                None => self.license_box.set_visible(false),
            }

            match license.clone().uri() {
                Some(uri) => {
                    self.uri_box.set_visible(true);
                    set_uri_to_label(&self.uri, uri.as_str());
                }
                None => self.uri_box.set_visible(false),
            }

            match license.clone().web_statement() {
                Some(web_statement) => {
                    self.web_statement_box.set_visible(true);
                    set_uri_to_label(&self.web_statement, web_statement.as_str())
                }
                None => self.web_statement_box.set_visible(false),
            }
        }
    }

    #[glib::object_subclass]
    impl ObjectSubclass for PpsPropertiesLicense {
        const NAME: &'static str = "PpsPropertiesLicense";
        type Type = super::PpsPropertiesLicense;
        type ParentType = adw::Bin;

        fn class_init(klass: &mut Self::Class) {
            klass.bind_template();
        }

        fn instance_init(obj: &InitializingObject<Self>) {
            obj.init_template();
        }
    }

    impl ObjectImpl for PpsPropertiesLicense {}
    impl WidgetImpl for PpsPropertiesLicense {}
    impl BinImpl for PpsPropertiesLicense {}
}

glib::wrapper! {
    pub struct PpsPropertiesLicense(ObjectSubclass<imp::PpsPropertiesLicense>)
    @extends gtk::Widget, adw::Bin,
    @implements gtk::Accessible, gtk::Buildable, gtk::ConstraintTarget;
}

impl Default for PpsPropertiesLicense {
    fn default() -> Self {
        Self::new()
    }
}

impl PpsPropertiesLicense {
    fn new() -> PpsPropertiesLicense {
        glib::Object::builder().build()
    }
}
