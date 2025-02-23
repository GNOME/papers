use super::*;

use gtk::gdk::gdk_pixbuf;
use papers_document::{AnnotationTextMarkupType, DocumentImages, DocumentSignatures};
use papers_view::annotations_context::AddAnnotationData;

fn gdk_pixbuf_format_by_extension(uri: &str) -> Option<gdk_pixbuf::PixbufFormat> {
    for format in gdk_pixbuf::Pixbuf::formats()
        .iter()
        .filter(|f| !f.is_disabled() && f.is_writable())
    {
        for ext in format.extensions() {
            if uri.ends_with(ext.as_str()) {
                return Some(format.clone());
            }
        }
    }

    None
}

impl imp::PpsDocumentView {
    pub(crate) fn set_action_enabled(&self, name: &str, enabled: bool) {
        let action = self
            .document_action_group
            .lookup_action(name)
            .and_downcast::<gio::SimpleAction>()
            .unwrap_or_else(|| panic!("there is no action named {}", name));

        action.set_enabled(enabled);
    }

    pub(crate) fn set_action_state(&self, name: &str, state: &glib::Variant) {
        self.document_action_group.change_action_state(name, state)
    }

    pub(crate) fn set_default_actions(&self) {
        let dual_mode = self.model.page_layout() == PageLayout::Dual;
        let document = self.document().unwrap();
        let info = document.info();

        if info.is_none() {
            self.set_action_enabled("show-properties", false);
        }

        if !document.is::<papers_document::Selection>() {
            self.set_action_enabled("select-all", false);
        }

        if !document.is::<papers_document::DocumentFind>() {
            self.set_action_enabled("find", false);
            self.set_action_enabled("toggle-find", false);
        }

        if document
            .dynamic_cast_ref::<DocumentAnnotations>()
            .is_some_and(|d| d.can_add_annotation())
        {
            let item = gio::MenuItem::new(None, None);

            item.set_attribute_value("custom", Some(&"palette".into()));
            self.annot_menu.insert_item(0, &item);

            self.view_popup
                .add_child(&self.annot_menu_child.clone(), "palette");
        } else {
            self.set_action_enabled("add-text-annotation", false);
        }

        if let Some(document) = document.dynamic_cast_ref::<DocumentSignatures>() {
            self.set_action_enabled("digital-signing", document.can_sign());
        }

        self.set_action_enabled("dual-odd-left", dual_mode);

        self.set_action_enabled("zoom-in", self.view.can_zoom_in());
        self.set_action_enabled("zoom-out", self.view.can_zoom_out());

        // Set enabled state for go-back-history and go-forward-history
        self.set_action_enabled(
            "go-back-history",
            !self.history.is_frozen() && self.history.can_go_back(),
        );

        self.set_action_enabled(
            "go-forward-history",
            !self.history.is_frozen() && self.history.can_go_forward(),
        );

        // Set enabled state for caret-navigation
        self.set_action_enabled("caret-navigation", self.view.supports_caret_navigation());

        self.doc_restrictions_changed();
    }

    pub fn setup_actions(&self) {
        let actions = [
            gio::ActionEntryBuilder::new("open-copy")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.open_copy_at_dest(None);
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("escape")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.cmd_escape();
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("show-sidebar")
                .state(glib::Variant::from(true))
                .change_state(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, action, state| {
                        let show_side_pane = state.unwrap().get::<bool>().unwrap();
                        action.set_state(&glib::Variant::from(show_side_pane));
                        obj.split_view.set_show_sidebar(show_side_pane);
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("select-page")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.page_selector.grab_focus();
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("print")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.action_menu_button.popdown();
                        obj.print_range(1, obj.document().unwrap().n_pages());
                    }
                ))
                .build(),
            // Document related actions
            gio::ActionEntryBuilder::new("save-as")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.cmd_save_as();
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("show-properties")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.cmd_file_properties();
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("continuous")
                .state(true.into())
                .change_state(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, action, state| {
                        let state = state.unwrap();
                        let continuous = state.get::<bool>().unwrap();

                        obj.model.set_continuous(continuous);
                        action.set_state(state);
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("rtl")
                .state(false.into())
                .change_state(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, action, state| {
                        let state = state.unwrap();
                        let rtl = state.get::<bool>().unwrap();

                        obj.model.set_rtl(rtl);
                        action.set_state(state);
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("dual-odd-left")
                .state(false.into())
                .change_state(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, action, state| {
                        let state = state.unwrap();
                        let dual_odd_left = state.get::<bool>().unwrap();

                        obj.model.set_dual_page_odd_pages_left(dual_odd_left);
                        action.set_state(state);
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("dual-page")
                .state(false.into())
                .change_state(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, action, state| {
                        let state = state.unwrap();
                        let dual_page = state.get::<bool>().unwrap();

                        obj.model.set_page_layout(if dual_page {
                            PageLayout::Dual
                        } else {
                            PageLayout::Single
                        });

                        let has_pages = obj.document().map(|d| d.n_pages() > 0).unwrap_or_default();

                        obj.set_action_enabled("dual-odd-left", has_pages);

                        action.set_state(state);
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("select-all")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.view.select_all();
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("zoom-in")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.model.set_sizing_mode(papers_view::SizingMode::Free);
                        obj.view.zoom_in();
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("zoom-out")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.model.set_sizing_mode(papers_view::SizingMode::Free);
                        obj.view.zoom_out();
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("zoom")
                .parameter_type(Some(glib::VariantTy::DOUBLE))
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, scale| {
                        let scale = scale.and_then(|d| d.get::<f64>()).unwrap();
                        obj.model.set_sizing_mode(papers_view::SizingMode::Free);
                        obj.model.set_scale(scale);
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("sizing-mode")
                .parameter_type(Some(glib::VariantTy::STRING))
                .state(glib::Variant::from("free"))
                .change_state(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, action, state| {
                        let mode = state.and_then(|s| s.str()).unwrap();

                        let mode = match mode {
                            "fit-width" => SizingMode::FitWidth,
                            "fit-page" => SizingMode::FitPage,
                            "free" => SizingMode::Free,
                            "automatic" => SizingMode::Automatic,
                            _ => panic!(),
                        };

                        obj.model.set_sizing_mode(mode);
                        action.set_state(state.unwrap());
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("annot-color")
                .parameter_type(Some(glib::VariantTy::STRING))
                .state(glib::Variant::from("yellow"))
                .change_state(glib::clone!(
                    #[weak(rename_to = _obj)]
                    self,
                    move |_, action, state| {
                        action.set_state(state.unwrap());
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("annot-style")
                .parameter_type(Some(glib::VariantTy::STRING))
                .state(glib::Variant::from("highlight"))
                .change_state(glib::clone!(
                    #[weak(rename_to = _obj)]
                    self,
                    move |_, action, state| {
                        action.set_state(state.unwrap());
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("enable-spellchecking")
                .state(false.into())
                .change_state(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, action, state| {
                        let enabled = state.and_then(|v| v.get::<bool>()).unwrap();

                        obj.view.set_enable_spellchecking(enabled);
                        action.set_state(state.unwrap());
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("go-previous-page")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.view.previous_page();
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("go-next-page")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.view.next_page();
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("go-first-page")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        let old_page = obj.model.page();
                        obj.model.set_page(0);
                        if old_page >= 0 {
                            obj.history.add_page(old_page);
                            obj.history.add_page(0);
                        }
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("go-last-page")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        let old_page = obj.model.page();
                        let new_page = obj.document().unwrap().n_pages();

                        obj.model.set_page(new_page);

                        if old_page >= 0 && new_page >= 0 {
                            obj.history.add_page(old_page);
                            obj.history.add_page(new_page);
                        }
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("rotate-left")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| obj.cmd_rotate_left()
                ))
                .build(),
            gio::ActionEntryBuilder::new("rotate-right")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| obj.cmd_rotate_right()
                ))
                .build(),
            gio::ActionEntryBuilder::new("copy")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| obj.view.copy()
                ))
                .build(),
            gio::ActionEntryBuilder::new("caret-navigation")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.cmd_view_toggle_caret_navigation();
                    }
                ))
                .build(),
            // popup menu actions
            gio::ActionEntryBuilder::new("open-link")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.view.handle_link(obj.link.borrow().as_ref().unwrap());
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("go-to-link")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.view.handle_link(obj.link.borrow().as_ref().unwrap());
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("open-link-new-window")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        if let Some(dest) = obj
                            .link
                            .borrow()
                            .as_ref()
                            .and_then(|l| l.action())
                            .and_then(|action| action.dest())
                        {
                            obj.open_copy_at_dest(Some(&dest));
                        };
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("copy-link-address")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        if let Some(action) = obj.link.borrow().as_ref().and_then(|l| l.action()) {
                            obj.view.copy_link_address(&action);
                        };
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("copy-image")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.cmd_copy_image();
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("save-image")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.cmd_save_image_as();
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("remove-annot")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        if let Some(annot) = obj.annot.borrow().clone() {
                            obj.annots_context.remove_annotation(&annot);
                        };
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("digital-signing")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.create_certificate_selection();
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("add-highlight-annotation")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.cmd_add_highlight_annotation();
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("add-text-annotation")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.cmd_add_text_annotation();
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("open-attachment")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.cmd_open_attachment();
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("save-attachment")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.cmd_save_attachment_as();
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("annot-properties")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.cmd_annot_properties();
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("open-with")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        if let Some(file) = obj.file.borrow().clone() {
                            let launcher = gtk::FileLauncher::new(Some(&file));

                            launcher.set_always_ask(true);

                            launcher.launch(
                                Some(&obj.parent_window()),
                                gio::Cancellable::NONE,
                                |_| {},
                            );
                        };
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("toggle-find")
                .state(false.into())
                .change_state(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, action, state| {
                        let state = state.unwrap();
                        let show = state.get::<bool>().unwrap();
                        let is_shown = action.state().unwrap().get::<bool>().unwrap();
                        let search_context = obj.search_context.borrow();

                        if show {
                            obj.show_find_bar();
                            if !is_shown {
                                search_context.as_ref().unwrap().activate();
                            }
                        } else {
                            obj.close_find_bar();
                            if is_shown {
                                search_context.as_ref().unwrap().release();
                            }
                        }

                        action.set_state(state);
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("find")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        let selected_text = obj.view.selected_text().filter(|t| !t.is_empty());

                        if let Some(selected_text) = selected_text {
                            obj.search_context
                                .borrow()
                                .as_ref()
                                .unwrap()
                                .set_search_term(&selected_text);
                            obj.document_action_group
                                .change_action_state("toggle-find", &true.into());
                            obj.find_restart();
                        } else {
                            let active = obj
                                .sidebar_stack
                                .visible_child()
                                .is_some_and(|w| w == *obj.find_sidebar)
                                && obj.find_sidebar.focus_child().is_some();

                            obj.document_action_group
                                .change_action_state("toggle-find", &(!active).into());
                        }
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("find-next")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.find_sidebar.next();
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("find-previous")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.find_sidebar.previous();
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("go-back-history")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        let page = obj.model.page();

                        if page >= 0 {
                            obj.history.add_page(page);
                        }

                        obj.history.go_back();
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("go-forward-history")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        obj.history.go_forward();
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("go-forward")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        let n_pages = obj.document().unwrap().n_pages();
                        let current_page = obj.model.page();

                        if current_page + 10 < n_pages {
                            obj.model.set_page(current_page + 10);
                        } else {
                            obj.model.set_page(n_pages - 1);
                        }
                    }
                ))
                .build(),
            gio::ActionEntryBuilder::new("go-backwards")
                .activate(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _, _| {
                        let current_page = obj.model.page();

                        if current_page - 10 >= 0 {
                            obj.model.set_page(current_page - 10);
                        } else {
                            obj.model.set_page(0);
                        }
                    }
                ))
                .build(),
        ];

        let group = self.document_action_group.clone();
        group.add_action_entries(actions);
        self.obj().insert_action_group("doc", Some(&group));
    }

    fn cmd_file_properties(&self) {
        let properties = PpsPropertiesWindow::new();

        properties.set_document(self.model.document());
        properties.present(Some(&self.obj().clone()));
    }

    fn cmd_escape(&self) {
        if self
            .sidebar_stack
            .visible_child()
            .is_some_and(|w| w == *self.find_sidebar)
            && self.find_sidebar.focus_child().is_some()
        {
            WidgetExt::activate_action(&self.obj().clone(), "doc.toggle-find", None).unwrap();
        } else if self.parent_window().is_fullscreen() {
            self.parent_window()
                .dynamic_cast::<gio::ActionGroup>()
                .unwrap()
                .change_action_state("fullscreen", &false.into());
        } else if self.split_view.is_collapsed() && self.split_view.shows_sidebar() {
            self.split_view.set_show_sidebar(false);
        }
    }

    fn rotate(&self, degree: i32) {
        let rotation = self.model.rotation() + degree;
        self.model.set_rotation(rotation);
    }

    fn cmd_rotate_left(&self) {
        self.rotate(-90);
    }

    fn cmd_rotate_right(&self) {
        self.rotate(90);
    }

    fn cmd_save_as(&self) {
        self.save_as();
    }

    fn create_file_from_uri_for_format(uri: &str, format: &gdk_pixbuf::PixbufFormat) -> gio::File {
        let extensions = format.extensions();

        for ext in extensions.iter() {
            if uri.ends_with(ext.as_str()) {
                return gio::File::for_uri(uri);
            }
        }

        gio::File::for_uri(&format!("{}.{}", uri, extensions[0]))
    }

    fn cmd_save_image_as(&self) {
        let Some(image) = self.image.borrow().clone() else {
            return;
        };

        // We simply give user a default name here. The extension is not hardcoded
        // and we will detect the target file extension to determine the format.
        // We will fallback to png or jpeg when no extension is specified.
        let initial_name = glib::DateTime::now_local()
            .unwrap()
            .format("%c.png")
            .unwrap();

        let dialog = gtk::FileDialog::builder()
            .title(gettext("Save Image"))
            .modal(true)
            .initial_name(initial_name)
            .build();

        self.file_dialog_restore_folder(&dialog, UserDirectory::Pictures);

        glib::spawn_future_local(glib::clone!(
            #[weak(rename_to = obj)]
            self,
            async move {
                let Ok(file) = dialog.save_future(Some(&obj.parent_window())).await else {
                    return;
                };

                obj.file_dialog_save_folder(Some(&file), UserDirectory::Pictures);

                let uri = file.uri();
                let mut format = gdk_pixbuf_format_by_extension(&uri);

                if format.is_none()
                    && file
                        .path()
                        .map(|p| p.extension().is_some())
                        .unwrap_or_default()
                {
                    // no extension found and no extension provided within uri
                    format = gdk_pixbuf_format_by_extension(".png").or(
                        // no .png support, try .jpeg
                        gdk_pixbuf_format_by_extension(".jpeg"),
                    );
                }

                let Some(format) = format else {
                    obj.error_message(
                        None,
                        &gettext("Couldn’t find appropriate format to save image"),
                    );
                    return;
                };

                let target_file = Self::create_file_from_uri_for_format(&uri, &format);

                let document = obj.document().unwrap();

                document.doc_mutex_lock();

                let pixbuf = obj
                    .document()
                    .and_dynamic_cast::<DocumentImages>()
                    .ok()
                    .and_then(|d| d.image(&image))
                    .unwrap();

                document.doc_mutex_unlock();

                if let Err(e) = obj.save_file(&target_file, |target| {
                    pixbuf.savev(target.path().unwrap(), &format.name().unwrap(), &[])
                }) {
                    obj.error_message(Some(&e), &gettext("The image could not be saved."));
                    return;
                }
            }
        ));
    }

    fn cmd_copy_image(&self) {
        let Some(image) = self.image.borrow().clone() else {
            return;
        };
        let clipboard = self.obj().clipboard();
        let document = self.document().unwrap();

        document.doc_mutex_lock();

        let pixbuf = document
            .dynamic_cast_ref::<papers_document::DocumentImages>()
            .unwrap()
            .image(&image);

        document.doc_mutex_unlock();

        clipboard.set_texture(&gdk::Texture::for_pixbuf(&pixbuf.unwrap()))
    }

    fn annot_properties_dialog_response(&self, dialog: &PpsAnnotationPropertiesDialog) {
        let annot = self.annot.borrow().clone().unwrap();
        let author = dialog.author();
        let rgba = dialog.rgba();
        let opacity = dialog.opacity();
        let popup_is_open = dialog.popup_open();
        let mut mask = AnnotationsSaveMask::NONE;

        // Set annotations changes
        if let Some(annot) = annot.dynamic_cast_ref::<AnnotationMarkup>() {
            if annot.set_label(&author) {
                mask |= AnnotationsSaveMask::LABEL;
            }
            if annot.set_rgba(&rgba) {
                mask |= AnnotationsSaveMask::COLOR;
            }
            if annot.set_opacity(opacity) {
                mask |= AnnotationsSaveMask::OPACITY;
            }
            if annot.set_popup_is_open(popup_is_open) {
                mask |= AnnotationsSaveMask::POPUP_IS_OPEN;
            }
        }

        if let Some(annot) = annot.dynamic_cast_ref::<AnnotationText>() {
            let icon = dialog.text_icon();
            if annot.set_icon(icon) {
                mask |= AnnotationsSaveMask::TEXT_ICON;
            }
        }

        if let Some(annot) = annot.dynamic_cast_ref::<AnnotationTextMarkup>() {
            let markup_type = dialog.markup_type();
            if annot.set_markup_type(markup_type) {
                mask |= AnnotationsSaveMask::TEXT_MARKUP_TYPE;
            }
        }

        if !mask.is_empty() {
            if let Some(document) = self.document() {
                if let Some(doc_annot) = document.dynamic_cast_ref::<DocumentAnnotations>() {
                    document.doc_mutex_lock();
                    doc_annot.save_annotation(&annot, mask);
                    document.doc_mutex_unlock();
                }
            }
        }
    }

    fn cmd_annot_properties(&self) {
        let Some(annot) = self.annot.borrow().clone() else {
            return;
        };

        let dialog = PpsAnnotationPropertiesDialog::new(&annot);

        dialog.connect_response(
            Some("apply"),
            glib::clone!(
                #[weak(rename_to = obj)]
                self,
                move |dialog, _| obj.annot_properties_dialog_response(dialog)
            ),
        );

        dialog.present(Some(&self.obj().clone()));
    }

    fn cmd_view_toggle_caret_navigation(&self) {
        let enabled = self.view.is_caret_navigation_enabled();

        self.set_caret_navigation_enabled(!enabled);

        let msg = if !enabled {
            gettext("Caret navigation mode is now enabled, press F7 to disable.")
        } else {
            gettext("Caret navigation mode is now disabled, press F7 to enable.")
        };

        let toast = adw::Toast::builder().title(&msg).timeout(5).build();

        self.toast_overlay.add_toast(toast);
    }

    fn annot_color_rgba(&self) -> gdk::RGBA {
        let binding = self
            .document_action_group
            .lookup_action("annot-color")
            .unwrap()
            .state()
            .unwrap();
        let color = binding.str().unwrap();
        match color {
            "yellow" => gdk::RGBA::parse("#f5c211").unwrap(),
            "orange" => gdk::RGBA::parse("#ff7800").unwrap(),
            "red" => gdk::RGBA::parse("#ed333b").unwrap(),
            "purple" => gdk::RGBA::parse("#c061cb").unwrap(),
            "blue" => gdk::RGBA::parse("#3584e4").unwrap(),
            "green" => gdk::RGBA::parse("#33d17a").unwrap(),
            _ => panic!("unknown color {}", color),
        }
    }

    fn annot_style(&self) -> AnnotationTextMarkupType {
        use papers_document::AnnotationTextMarkupType::*;

        let binding = self
            .document_action_group
            .lookup_action("annot-style")
            .unwrap()
            .state()
            .unwrap();
        let style = binding.str().unwrap();

        match style {
            "highlight" => Highlight,
            "squiggly" => Squiggly,
            "strikethrough" => StrikeOut,
            "underline" => Underline,
            _ => panic!("unknown style {}", style),
        }
    }

    fn cmd_add_highlight_annotation(&self) {
        let selections = self.view.selections();
        for sel in selections.iter() {
            let mut start_point = papers_document::Point::new();
            let mut end_point = papers_document::Point::new();
            start_point.set_x(sel.rect().x1());
            start_point.set_y(sel.rect().y1());
            end_point.set_x(sel.rect().x2());
            end_point.set_y(sel.rect().y2());
            _ = self.annots_context.add_annotation_sync(
                sel.page(),
                papers_document::AnnotationType::TextMarkup,
                &start_point,
                &end_point,
                &self.annot_color_rgba(),
                AddAnnotationData::TextMarkup(self.annot_style()),
            );
        }
    }

    fn cmd_add_text_annotation(&self) {
        let (x, y);
        if let Some((px, py)) = Document::misc_get_pointer_position(&self.view.clone()) {
            x = px;
            y = py
        } else {
            // Check if the pointer is not over the current surface, then
            // it should be in the popover, and we should get the point
            // from where the popover is pointing

            let (_, rect) = self.view_popup.pointing_to();
            x = rect.x();
            y = rect.y();
        };

        if let Some(mark) = self.view.mark_for_view_point(x.into(), y.into()) {
            _ = self.annots_context.add_annotation_sync(
                mark.page_index(),
                papers_document::AnnotationType::Text,
                &mark.doc_point(),
                &mark.doc_point(),
                &self.annot_color_rgba(),
                AddAnnotationData::None,
            );
        };
    }

    fn cmd_open_attachment(&self) {
        let Some(attachment) = self.attachment.take() else {
            return;
        };
        let context = self.obj().display().app_launch_context();

        if let Err(e) = attachment.open(&context) {
            if !e.matches(gtk::DialogError::Dismissed) {
                self.error_message(Some(&e), &gettext("Unable to Open Attachment"));
            }
        }
    }

    fn cmd_save_attachment_as(&self) {
        let Some(attachment) = self.attachment.borrow().clone() else {
            return;
        };
        let attachments = gio::ListStore::new::<Attachment>();

        attachments.append(&attachment);

        self.reset_progress_cancellable();

        let dialog = gtk::FileDialog::builder()
            .title(gettext("Save Attachment"))
            .modal(true)
            .build();

        self.file_dialog_restore_folder(&dialog, UserDirectory::Documents);

        let window = self.parent_window();

        glib::spawn_future_local(glib::clone!(
            #[weak(rename_to = obj)]
            self,
            async move {
                if let Err(e) = obj
                    .attachment_context
                    .save_attachments_future(attachments, Some(&window))
                    .await
                {
                    obj.error_message(Some(&e), &gettext("The attachment could not be saved."));
                }
            }
        ));
    }
}
