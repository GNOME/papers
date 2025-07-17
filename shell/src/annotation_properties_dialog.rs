use crate::deps::*;
use std::cell::OnceCell;

use glib::translate::{FromGlib, IntoGlib};

use papers_document::{
    Annotation, AnnotationMarkup, AnnotationText, AnnotationTextIcon, AnnotationTextMarkup,
    AnnotationTextMarkupType,
};

mod imp {
    use super::*;

    #[derive(Debug, Default, CompositeTemplate, Properties)]
    #[properties(wrapper_type = super::PpsAnnotationPropertiesDialog)]
    #[template(resource = "/org/gnome/papers/ui/annotation-properties-dialog.ui")]
    pub struct PpsAnnotationPropertiesDialog {
        #[template_child]
        author: TemplateChild<adw::EntryRow>,
        #[template_child]
        color: TemplateChild<gtk::ColorDialogButton>,
        #[template_child]
        opacity: TemplateChild<adw::SpinRow>,
        #[template_child]
        popup_state: TemplateChild<adw::SwitchRow>,

        // Text Annotations
        #[template_child]
        icon: TemplateChild<adw::ComboRow>,

        // Text Markup Annotations
        #[template_child]
        markup_type: TemplateChild<adw::ComboRow>,

        #[property(construct_only, set = Self::set_annotation, get)]
        annotation: OnceCell<Annotation>,
        #[property(name = "author", type = String, get = Self::author)]
        _1: (),
        #[property(name = "rgba", type = gdk::RGBA, get = Self::rgba)]
        _2: (),
        #[property(name = "opacity", type = f64, get = Self::opacity)]
        _3: (),
        #[property(name = "popup-open", type = bool, get = Self::is_popup_open)]
        _4: (),
        #[property(name = "text-icon", type = AnnotationTextIcon, get = Self::text_icon, builder(AnnotationTextIcon::Unknown))]
        _5: (),
        #[property(name = "markup-type", type = AnnotationTextMarkupType, get = Self::markup_type, builder(AnnotationTextMarkupType::Highlight))]
        _6: (),
    }

    #[glib::object_subclass]
    impl ObjectSubclass for PpsAnnotationPropertiesDialog {
        const NAME: &'static str = "PpsAnnotationPropertiesDialog";
        type Type = super::PpsAnnotationPropertiesDialog;
        type ParentType = adw::Dialog;

        fn class_init(klass: &mut Self::Class) {
            klass.bind_template();
            klass.bind_template_callbacks();
        }

        fn instance_init(obj: &InitializingObject<Self>) {
            obj.init_template();
        }
    }

    impl AdwDialogImpl for PpsAnnotationPropertiesDialog {}

    impl WidgetImpl for PpsAnnotationPropertiesDialog {}

    #[glib::derived_properties]
    impl ObjectImpl for PpsAnnotationPropertiesDialog {
        fn signals() -> &'static [Signal] {
            static SIGNALS: OnceLock<Vec<Signal>> = OnceLock::new();
            SIGNALS.get_or_init(|| vec![Signal::builder("changed").run_last().action().build()])
        }
    }

    impl PpsAnnotationPropertiesDialog {
        fn set_annotation(&self, annot: &Annotation) {
            if let Some(label) = annot
                .dynamic_cast_ref::<AnnotationMarkup>()
                .and_then(|annot| annot.label())
            {
                self.author.set_text(&label);
            }

            self.color.set_rgba(&annot.rgba());

            if let Some(opacity) = annot
                .dynamic_cast_ref::<AnnotationMarkup>()
                .map(|annot| annot.opacity())
            {
                self.opacity.set_value(opacity * 100.);
            }

            if let Some(is_open) = annot
                .dynamic_cast_ref::<AnnotationMarkup>()
                .map(|annot| annot.is_popup_is_open())
            {
                self.popup_state.set_active(is_open);
            }

            if let Some(icon) = annot
                .dynamic_cast_ref::<AnnotationText>()
                .map(|annot| annot.icon())
            {
                self.icon.set_selected(icon.into_glib() as u32);
            }

            if let Some(markup_type) = annot
                .dynamic_cast_ref::<AnnotationTextMarkup>()
                .map(|annot| annot.markup_type())
            {
                self.markup_type
                    .set_selected(markup_type.into_glib() as u32);
            }

            if annot.is::<AnnotationText>() {
                self.icon.set_visible(true);
            }

            if annot.is::<AnnotationTextMarkup>() {
                self.markup_type.set_visible(true);
            }

            self.annotation.set(annot.clone()).unwrap();
        }

        fn author(&self) -> String {
            self.author.text().to_string()
        }

        fn rgba(&self) -> gdk::RGBA {
            self.color.rgba()
        }

        fn opacity(&self) -> f64 {
            self.opacity.value() / 100.
        }

        fn is_popup_open(&self) -> bool {
            self.popup_state.is_active()
        }

        fn text_icon(&self) -> AnnotationTextIcon {
            unsafe { AnnotationTextIcon::from_glib(self.icon.selected() as i32) }
        }

        fn markup_type(&self) -> AnnotationTextMarkupType {
            unsafe { AnnotationTextMarkupType::from_glib(self.markup_type.selected() as i32) }
        }
    }

    #[gtk::template_callbacks]
    impl PpsAnnotationPropertiesDialog {
        #[template_callback]
        fn property_changed(&self) {
            self.obj().emit_by_name::<()>("changed", &[]);
        }
    }
}

glib::wrapper! {
    pub struct PpsAnnotationPropertiesDialog(ObjectSubclass<imp::PpsAnnotationPropertiesDialog>)
        @extends gtk::Widget, adw::Dialog,
        @implements gtk::Accessible, gtk::Buildable, gtk::ConstraintTarget;
}

impl PpsAnnotationPropertiesDialog {
    pub fn new(annot: &impl IsA<papers_document::Annotation>) -> Self {
        glib::Object::builder()
            .property("annotation", annot)
            .build()
    }
}
