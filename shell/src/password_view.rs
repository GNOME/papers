use crate::deps::*;

use gio::PasswordSave;

const CAN_SAVE_PASSWORD: bool = cfg!(feature = "with-keyring");

const PASSWORD_PREFERENCE_NEVER: &str = "never";
const PASSWORD_PREFERENCE_PERMANENTLY: &str = "permanently";

const RESPONSE_UNLOCK: &str = "unlock";
const RESPONSE_CANCELLED: &str = "cancelled";

mod imp {
    use super::*;

    #[derive(Debug, Clone)]
    struct PasswordDialog {
        main: adw::AlertDialog,
        password_entry: gtk::PasswordEntry,
    }

    impl PasswordDialog {
        fn new(password_view: &super::PpsPasswordView, error: bool) -> Self {
            let builder = gtk::Builder::new();

            let scope = gtk::BuilderRustScope::new();
            PpsPasswordView::add_callbacks_to_scope(&scope);

            builder.set_scope(Some(&scope));
            builder.set_current_object(Some(password_view));

            builder
                .add_from_resource("/org/gnome/papers/ui/password-dialog.ui")
                .unwrap();

            if CAN_SAVE_PASSWORD {
                builder
                    .object::<gtk::Widget>("password_choice")
                    .unwrap()
                    .set_visible(true);
            }

            let password_entry: gtk::PasswordEntry = builder.object("password_entry").unwrap();

            if error {
                password_entry.add_css_class("error");

                builder
                    .object::<gtk::Label>("error_message")
                    .unwrap()
                    .set_visible(true);
            }

            PasswordDialog {
                main: builder.object("dialog").unwrap(),
                password_entry,
            }
        }
    }

    #[derive(Debug, Default, CompositeTemplate, Properties)]
    #[properties(wrapper_type = super::PpsPasswordView)]
    #[template(resource = "/org/gnome/papers/ui/password-view.ui")]
    pub struct PpsPasswordView {
        #[property(get, set)]
        filename: RefCell<String>,
        dialog: RefCell<Option<PasswordDialog>>,
        #[template_child]
        pub(super) action_group: TemplateChild<gio::SimpleActionGroup>,
    }

    #[glib::object_subclass]
    impl ObjectSubclass for PpsPasswordView {
        const NAME: &'static str = "PpsPasswordView";
        type Type = super::PpsPasswordView;
        type ParentType = adw::Bin;

        fn class_init(klass: &mut Self::Class) {
            klass.bind_template();
            klass.bind_template_callbacks();
        }

        fn instance_init(obj: &InitializingObject<Self>) {
            obj.init_template();
        }
    }

    impl BinImpl for PpsPasswordView {}

    impl WidgetImpl for PpsPasswordView {}

    #[glib::derived_properties]
    impl ObjectImpl for PpsPasswordView {
        fn constructed(&self) {
            self.parent_constructed();

            let preferences_action = gio::SimpleAction::new_stateful(
                "preference",
                Some(glib::VariantTy::STRING),
                &glib::Variant::from(PASSWORD_PREFERENCE_NEVER),
            );

            self.action_group.add_action(&preferences_action);
        }

        fn signals() -> &'static [Signal] {
            static SIGNALS: OnceLock<Vec<Signal>> = OnceLock::new();

            SIGNALS.get_or_init(|| {
                vec![
                    Signal::builder("unlock")
                        .param_types([glib::Type::STRING, PasswordSave::static_type()])
                        .run_last()
                        .build(),
                    Signal::builder("cancelled").run_last().build(),
                ]
            })
        }
    }

    #[gtk::template_callbacks]
    impl PpsPasswordView {
        pub(super) fn ask_password(&self, error: bool) {
            if self.dialog.borrow().is_none() {
                self.create_dialog(error);
            }
        }

        #[template_callback]
        fn open_unlock_dialog(&self) {
            self.create_dialog(false);
        }

        fn create_dialog(&self, error: bool) {
            let dialog = PasswordDialog::new(&self.obj(), error);

            dialog.main.set_close_response(RESPONSE_CANCELLED);
            dialog.main.set_default_response(Some(RESPONSE_UNLOCK));

            dialog
                .main
                .insert_action_group("password", Some(&self.action_group.get()));

            let body = gettext_f(
                "The document “{}” is locked and requires a password before it can be opened",
                [self.filename.borrow().clone()],
            );

            dialog.main.set_body(&body);

            let main = dialog.main.clone();

            self.dialog.replace(Some(dialog));

            main.choose(
                self.obj().as_ref(),
                gio::Cancellable::NONE,
                glib::clone![
                    #[weak(rename_to = obj)]
                    self,
                    move |response| {
                        obj.handle_response(&response);
                    }
                ],
            );
        }

        #[template_callback]
        fn update_password(&self, password_entry: gtk::PasswordEntry) {
            let has_password = !password_entry.text().is_empty();

            if let Some(dialog) = &*self.dialog.borrow() {
                dialog
                    .main
                    .set_response_enabled(RESPONSE_UNLOCK, has_password);
            }
        }

        fn handle_response(&self, response: &str) {
            let dialog = self.dialog.borrow_mut().take().unwrap();

            let preferences_action = self
                .action_group
                .lookup_action("preference")
                .and_downcast::<gio::SimpleAction>()
                .unwrap();

            match response {
                RESPONSE_UNLOCK => {
                    let password = dialog.password_entry.text();

                    let preference = preferences_action
                        .state()
                        .filter(|_| CAN_SAVE_PASSWORD)
                        .and_then(|value| value.get::<String>())
                        .map(|pref_str| preference_choice_to_password_save(&pref_str[..]))
                        .unwrap_or(PasswordSave::Never);

                    preferences_action.set_state(&glib::Variant::from(PASSWORD_PREFERENCE_NEVER));

                    self.obj()
                        .emit_by_name::<()>("unlock", &[&password, &preference]);
                }
                RESPONSE_CANCELLED => {
                    self.obj().emit_by_name::<()>("cancelled", &[]);
                }
                _ => unreachable!(),
            }
        }
    }

    fn preference_choice_to_password_save(choice: &str) -> PasswordSave {
        match choice {
            PASSWORD_PREFERENCE_NEVER => PasswordSave::Never,
            PASSWORD_PREFERENCE_PERMANENTLY => PasswordSave::Permanently,
            _ => unreachable!(),
        }
    }
}

glib::wrapper! {
    pub struct PpsPasswordView(ObjectSubclass<imp::PpsPasswordView>)
        @extends gtk::Widget, adw::Bin,
        @implements gtk::Accessible, gtk::Buildable, gtk::ConstraintTarget;
}

impl PpsPasswordView {
    pub fn ask_password(&self, error: bool) {
        self.imp().ask_password(error);
    }
}
