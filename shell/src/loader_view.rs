use crate::deps::*;

use std::cell::Cell;

mod imp {
    use super::*;

    #[derive(Debug, Default, CompositeTemplate, Properties)]
    #[properties(wrapper_type = super::PpsLoaderView)]
    #[template(resource = "/org/gnome/papers/ui/loader-view.ui")]
    pub struct PpsLoaderView {
        #[template_child]
        stack: TemplateChild<gtk::Stack>,
        #[template_child]
        progress_bar: TemplateChild<gtk::ProgressBar>,
        #[template_child]
        spinner: TemplateChild<adw::Spinner>,
        #[property(get, set = Self::set_fraction)]
        fraction: Cell<f64>,
        #[property(get, set)]
        uri: RefCell<String>,
    }

    #[glib::object_subclass]
    impl ObjectSubclass for PpsLoaderView {
        const NAME: &'static str = "PpsLoaderView";
        type Type = super::PpsLoaderView;
        type ParentType = adw::Bin;

        fn class_init(klass: &mut Self::Class) {
            klass.bind_template();
            klass.bind_template_callbacks();
        }

        fn instance_init(obj: &InitializingObject<Self>) {
            obj.init_template();
        }
    }

    impl BinImpl for PpsLoaderView {}

    impl WidgetImpl for PpsLoaderView {}

    #[glib::derived_properties]
    impl ObjectImpl for PpsLoaderView {
        fn signals() -> &'static [Signal] {
            static SIGNALS: OnceLock<Vec<Signal>> = OnceLock::new();

            SIGNALS.get_or_init(|| vec![Signal::builder("cancel").run_last().build()])
        }
    }

    impl PpsLoaderView {
        fn set_fraction(&self, fraction: f64) {
            if fraction < 0.0 {
                self.stack.set_visible_child_name("spinner");
            } else {
                self.progress_bar.set_fraction(fraction);
                self.stack.set_visible_child_name("bar");
            }
            self.fraction.set(fraction);
        }
    }

    #[gtk::template_callbacks]
    impl PpsLoaderView {
        #[template_callback]
        fn cancel_clicked(&self) {
            self.obj().emit_by_name::<()>("cancel", &[]);
        }
    }
}

glib::wrapper! {
    pub struct PpsLoaderView(ObjectSubclass<imp::PpsLoaderView>)
        @extends gtk::Widget, adw::Bin,
        @implements gtk::Accessible, gtk::Buildable, gtk::ConstraintTarget;
}
