#![allow(deprecated)]

// FIXME: Remove gtk::InfoBar

use crate::deps::*;
use std::cell::Cell;

mod imp {
    use super::*;

    #[derive(Properties, Default, Debug, CompositeTemplate)]
    #[properties(wrapper_type = super::PpsProgressMessageArea)]
    #[template(resource = "/org/gnome/papers/ui/progress-message-area.ui")]
    pub struct PpsProgressMessageArea {
        #[template_child]
        pub(super) info_bar: TemplateChild<gtk::InfoBar>,
        #[template_child]
        pub(super) image: TemplateChild<gtk::Image>,
        #[template_child]
        pub(super) label: TemplateChild<gtk::Label>,
        #[template_child]
        pub(super) secondary_label: TemplateChild<gtk::Label>,
        #[template_child]
        pub(super) progress_bar: TemplateChild<gtk::ProgressBar>,
        #[template_child]
        pub(super) progress_label: TemplateChild<gtk::Label>,

        #[property(get, set = Self::set_text)]
        pub(super) text: RefCell<String>,
        #[property(get, set = Self::set_secondary_text)]
        pub(super) secondary_text: RefCell<String>,
        #[property(get, set = Self::set_status)]
        pub(super) status: RefCell<String>,
        #[property(get, set = Self::set_fraction)]
        pub(super) fraction: Cell<f64>,
    }

    #[glib::object_subclass]
    impl ObjectSubclass for PpsProgressMessageArea {
        const NAME: &'static str = "PpsProgressMessageArea";
        type Type = super::PpsProgressMessageArea;
        type ParentType = adw::Bin;

        fn class_init(klass: &mut Self::Class) {
            klass.bind_template();
        }

        fn instance_init(obj: &InitializingObject<Self>) {
            obj.init_template();
        }
    }

    #[glib::derived_properties]
    impl ObjectImpl for PpsProgressMessageArea {
        fn signals() -> &'static [Signal] {
            static SIGNALS: OnceLock<Vec<Signal>> = OnceLock::new();
            SIGNALS.get_or_init(|| vec![Signal::builder("navigated-to-view").run_last().build()])
        }
    }

    impl BinImpl for PpsProgressMessageArea {}

    impl WidgetImpl for PpsProgressMessageArea {}

    impl PpsProgressMessageArea {
        fn set_text(&self, text: &str) {
            let text = if !text.is_empty() {
                format!("<b>{}</b>", glib::markup_escape_text(text))
            } else {
                "".to_owned()
            };

            self.label.set_markup(&text);
            self.text.replace(text);
        }

        fn set_secondary_text(&self, text: &str) {
            let text = if !text.is_empty() {
                format!("<small>{}</small>", glib::markup_escape_text(text))
            } else {
                "".to_owned()
            };

            self.secondary_label.set_markup(&text);
            self.secondary_label.set_visible(true);
            self.secondary_text.replace(text);
        }

        fn set_status(&self, status: &str) {
            self.progress_label.set_text(status);
            self.status.replace(status.to_owned());
        }

        fn set_fraction(&self, fraction: f64) {
            self.progress_bar.set_fraction(fraction);
            self.fraction.set(fraction);
        }
    }
}

glib::wrapper! {
    pub struct PpsProgressMessageArea(ObjectSubclass<imp::PpsProgressMessageArea>)
        @extends adw::Bin, gtk::Widget,
        @implements gtk::Accessible, gtk::Buildable, gtk::ConstraintTarget;
}

impl PpsProgressMessageArea {
    pub fn new(icon_name: &str, text: &str) -> Self {
        let area: PpsProgressMessageArea = glib::Object::builder().property("text", text).build();

        area.set_image_from_icon_name(icon_name);

        area
    }

    pub fn set_image_from_icon_name(&self, icon_name: &str) {
        self.imp().image.set_icon_name(Some(icon_name));
    }

    pub fn add_button(&self, button_text: &str, response_id: gtk::ResponseType) {
        self.imp().info_bar.add_button(button_text, response_id);
    }

    pub fn info_bar(&self) -> gtk::InfoBar {
        self.imp().info_bar.clone()
    }
}
