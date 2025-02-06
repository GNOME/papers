use crate::deps::*;

use papers_view::SearchContext;

mod imp {
    use super::*;

    #[derive(Properties, Default, Debug, CompositeTemplate)]
    #[properties(wrapper_type = super::PpsFindSidebar)]
    #[template(resource = "/org/gnome/papers/ui/find-sidebar.ui")]
    pub struct PpsFindSidebar {
        #[template_child]
        pub(super) results_stack: TemplateChild<gtk::Stack>,
        #[template_child]
        pub(super) search_box: TemplateChild<PpsSearchBox>,
        #[template_child]
        pub(super) list_view: TemplateChild<gtk::ListView>,

        #[property(name="search-context", nullable, set = Self::set_search_context)]
        pub(super) context: RefCell<Option<papers_view::SearchContext>>,

        pub(super) context_signal_handlers: RefCell<Vec<SignalHandlerId>>,
    }

    #[glib::object_subclass]
    impl ObjectSubclass for PpsFindSidebar {
        const NAME: &'static str = "PpsFindSidebar";
        type Type = super::PpsFindSidebar;
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
    impl ObjectImpl for PpsFindSidebar {
        fn dispose(&self) {
            self.clear_context();
        }
    }

    impl BinImpl for PpsFindSidebar {}

    impl WidgetImpl for PpsFindSidebar {
        fn grab_focus(&self) -> bool {
            self.search_box.grab_focus()
        }
    }

    impl PpsFindSidebar {
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

        fn set_search_context(&self, context: Option<SearchContext>) {
            if self.context() == context {
                return;
            }

            self.clear_context();

            self.list_view
                .set_model(context.as_ref().and_then(|c| c.result_model()).as_ref());

            if let Some(ref context) = context {
                let mut handlers = self.context_signal_handlers.borrow_mut();

                handlers.push(context.connect_started(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_| {
                        obj.start();
                    }
                )));

                handlers.push(context.connect_cleared(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_| {
                        obj.clear();
                    }
                )));

                handlers.push(context.connect_finished(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |context, page| {
                        let has_result = context
                            .result_model()
                            .map(|m| m.n_items() != 0)
                            .unwrap_or_default();

                        if !has_result {
                            obj.results_stack.set_visible_child_name("no-results");
                        } else {
                            obj.results_stack.set_visible_child_name("results");
                        }

                        if page != -1 {
                            obj.highlight_first_match_of_page(page as u32);
                        }
                    }
                )));
            }

            self.search_box.set_context(context.clone());
            self.context.replace(context);
        }

        fn highlight_first_match_of_page(&self, page: u32) {
            let result_model = self.context().and_then(|c| c.result_model()).unwrap();
            let mut lower_bound = 0;
            let mut upper_bound = result_model.n_items();
            let mut index = 0;

            // Binary search is a fast algorithm, here. However, in the future
            // the search context should deal with this, and ideally handle this
            // with a map. Just, that would need to a few more changes on its end.

            while lower_bound <= upper_bound {
                index = (upper_bound + lower_bound) / 2;

                let result = result_model
                    .item(index)
                    .and_downcast::<papers_view::SearchResult>()
                    .unwrap();

                let result_page = result.page();
                let result_index_in_page = result.index();

                if result_page == page && result_index_in_page == 0 {
                    break;
                } else if result_page < page {
                    lower_bound = index + 1;
                } else {
                    upper_bound = index - 1;
                }
            }

            self.list_view
                .scroll_to(index, gtk::ListScrollFlags::SELECT, None);
        }

        pub(super) fn restart(&self, page: u32) {
            let mut first_match_page = None;
            let model = self.context().and_then(|c| c.result_model()).unwrap();

            for result in model.iter::<papers_view::SearchResult>() {
                let result = result.unwrap();

                if result.page() >= page {
                    first_match_page = Some(result.page());
                    break;
                }
            }

            if let Some(page) = first_match_page {
                self.highlight_first_match_of_page(page);
            }
        }

        fn start(&self) {
            self.results_stack.set_visible_child_name("loading");
        }

        fn clear(&self) {
            self.results_stack.set_visible_child_name("initial");
        }

        pub(super) fn previous(&self) {
            let result_model = self.context().and_then(|c| c.result_model()).unwrap();
            let pos = result_model.selected() - 1;

            self.list_view
                .scroll_to(pos, gtk::ListScrollFlags::SELECT, None);
        }

        pub(super) fn next(&self) {
            let result_model = self.context().and_then(|c| c.result_model()).unwrap();
            let selected = result_model.selected();

            let pos = if selected != gtk::INVALID_LIST_POSITION {
                selected + 1
            } else {
                0
            };

            self.list_view
                .scroll_to(pos, gtk::ListScrollFlags::SELECT, None);
        }
    }

    #[gtk::template_callbacks]
    impl PpsFindSidebar {
        #[template_callback]
        fn list_view_factory_setup(&self, item: &gtk::ListItem) {
            let box_ = gtk::Box::builder()
                .orientation(gtk::Orientation::Horizontal)
                .spacing(6)
                .build();

            item.set_child(Some(&box_));
        }

        #[template_callback]
        fn list_view_factory_bind(&self, item: &gtk::ListItem) {
            let box_ = item.child().and_downcast::<gtk::Box>().unwrap();
            let result = item
                .item()
                .and_downcast::<papers_view::SearchResult>()
                .unwrap();

            let result_label = gtk::Label::builder()
                .label(result.markup().unwrap_or_default())
                .use_markup(true)
                .ellipsize(gtk::pango::EllipsizeMode::End)
                .hexpand(true)
                .halign(gtk::Align::Start)
                .build();

            let page_label = gtk::Label::new(result.label().as_ref().map(|gs| gs.as_str()));

            box_.append(&result_label);
            box_.append(&page_label);
        }

        #[template_callback]
        fn list_view_factory_unbind(&self, item: &gtk::ListItem) {
            let box_ = item.child().and_downcast::<gtk::Box>().unwrap();

            while let Some(child) = box_.first_child() {
                box_.remove(&child);
            }
        }
    }
}

glib::wrapper! {
    pub struct PpsFindSidebar(ObjectSubclass<imp::PpsFindSidebar>)
        @extends adw::Bin, gtk::Widget;
}

impl PpsFindSidebar {
    pub fn new() -> Self {
        glib::Object::builder().build()
    }

    pub fn previous(&self) {
        self.imp().previous()
    }

    pub fn next(&self) {
        self.imp().next()
    }

    pub fn restart(&self, page: u32) {
        self.imp().restart(page);
    }
}

impl Default for PpsFindSidebar {
    fn default() -> Self {
        Self::new()
    }
}
