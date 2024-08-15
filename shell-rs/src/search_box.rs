use crate::deps::*;

use papers_document::FindOptions;
use papers_view::SearchContext;

mod imp {
    use super::*;

    #[derive(Properties, Default, Debug, CompositeTemplate)]
    #[properties(wrapper_type = super::PpsSearchBox)]
    #[template(resource = "/org/gnome/papers/ui/search-box.ui")]
    pub struct PpsSearchBox {
        #[template_child]
        pub(super) entry: TemplateChild<gtk::SearchEntry>,
        #[property(nullable, set = Self::set_context)]
        pub(super) context: RefCell<Option<SearchContext>>,
        pub(super) search_term_handler: RefCell<Option<SignalHandlerId>>,
        pub(super) context_signal_handlers: RefCell<Vec<SignalHandlerId>>,
    }

    #[glib::object_subclass]
    impl ObjectSubclass for PpsSearchBox {
        const NAME: &'static str = "PpsSearchBox";
        type Type = super::PpsSearchBox;
        type ParentType = adw::Bin;

        fn class_init(klass: &mut Self::Class) {
            klass.bind_template();
            klass.bind_template_callbacks();
        }

        fn instance_init(obj: &InitializingObject<Self>) {
            obj.init_template();
        }
    }

    #[glib::derived_properties]
    impl ObjectImpl for PpsSearchBox {
        fn constructed(&self) {
            let actions = [
                gio::ActionEntryBuilder::new("whole-words-only")
                    .state(false.into())
                    .change_state(glib::clone!(
                        #[weak(rename_to = obj)]
                        self,
                        move |_, action, state| {
                            let state = state.unwrap();
                            let active = state.get::<bool>().unwrap();

                            action.set_state(state);

                            if let Some(context) = obj.context() {
                                let mut options = context.options();

                                if active {
                                    options.insert(FindOptions::WHOLE_WORDS_ONLY);
                                } else {
                                    options.remove(FindOptions::WHOLE_WORDS_ONLY);
                                }

                                context.set_options(options);
                            }
                        }
                    ))
                    .build(),
                gio::ActionEntryBuilder::new("case-sensitive")
                    .state(false.into())
                    .change_state(glib::clone!(
                        #[weak(rename_to = obj)]
                        self,
                        move |_, action, state| {
                            let state = state.unwrap();
                            let active = state.get::<bool>().unwrap();

                            action.set_state(state);

                            if let Some(context) = obj.context() {
                                let mut options = context.options();

                                if active {
                                    options.insert(FindOptions::CASE_SENSITIVE);
                                } else {
                                    options.remove(FindOptions::CASE_SENSITIVE);
                                }

                                context.set_options(options);
                            }
                        }
                    ))
                    .build(),
            ];

            let group = gio::SimpleActionGroup::new();
            group.add_action_entries(actions);

            self.obj().insert_action_group("search", Some(&group));

            let id = self.entry.connect_search_changed(glib::clone!(
                #[weak(rename_to = obj)]
                self,
                move |_| {
                    if let Some(context) = obj.context.borrow().clone() {
                        context.set_search_term(obj.entry.text().as_str());
                    };
                }
            ));

            self.search_term_handler.replace(Some(id));
        }

        fn dispose(&self) {
            self.clear_context();
        }
    }

    impl BinImpl for PpsSearchBox {}

    impl WidgetImpl for PpsSearchBox {
        fn grab_focus(&self) -> bool {
            self.entry.grab_focus()
        }
    }

    impl PpsSearchBox {
        fn context(&self) -> Option<SearchContext> {
            self.context.borrow().clone()
        }

        fn clear_context(&self) {
            if let Some(context) = self.context.take() {
                for id in self.context_signal_handlers.take() {
                    context.disconnect(id);
                }
            }
        }

        fn set_context(&self, context: Option<SearchContext>) {
            if self.context() == context {
                return;
            }

            self.clear_context();

            if let Some(ref context) = context {
                let mut handlers = self.context_signal_handlers.borrow_mut();

                handlers.push(context.connect_started(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _| {
                        obj.search_changed();
                    }
                )));

                handlers.push(context.connect_cleared(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_| {
                        obj.search_changed();
                    }
                )));

                handlers.push(context.connect_search_term_notify(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_| {
                        obj.search_changed();
                    }
                )));

                handlers.push(context.connect_finished(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |context, _, _| {
                        let has_result = context
                            .result_model()
                            .map(|m| m.n_items() != 0)
                            .unwrap_or_default();

                        if !has_result {
                            obj.entry.add_css_class("error");
                        }
                    }
                )));
            }

            self.context.replace(context);
        }

        fn search_changed(&self) {
            self.entry.remove_css_class("error");

            if let Some(context) = self.context() {
                let search_term = context.search_term();
                let handler_bind = self.search_term_handler.borrow();
                let handler = handler_bind.as_ref().unwrap();

                self.entry.block_signal(handler);
                if Some(self.entry.text()) != search_term {
                    self.entry.set_text(
                        search_term
                            .as_ref()
                            .map(|gs| gs.as_str())
                            .unwrap_or_default(),
                    );
                }
                self.entry.unblock_signal(handler);
            }
        }
    }

    #[gtk::template_callbacks]
    impl PpsSearchBox {
        #[template_callback]
        fn entry_activated(&self) {
            self.obj().activate_action("doc.find-next", None).unwrap();
        }

        #[template_callback]
        fn entry_next_matched(&self) {
            self.obj().activate_action("doc.find-next", None).unwrap();
        }

        #[template_callback]
        fn entry_previous_matched(&self) {
            self.obj()
                .activate_action("doc.find-previous", None)
                .unwrap();
        }

        #[template_callback]
        fn stopped_search(&self) {
            self.obj().activate_action("doc.toggle-find", None).unwrap();
        }
    }
}

glib::wrapper! {
    pub struct PpsSearchBox(ObjectSubclass<imp::PpsSearchBox>)
        @extends adw::Bin, gtk::Widget;
}

impl PpsSearchBox {
    pub fn new() -> Self {
        glib::Object::builder().build()
    }
}

impl Default for PpsSearchBox {
    fn default() -> Self {
        Self::new()
    }
}
