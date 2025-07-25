use papers_document::{DocumentSignatures, SignatureStatus};
use papers_view::{JobPriority, JobSignatures};

use super::*;

impl imp::PpsDocumentView {
    pub(crate) fn message_area(&self) -> Option<PpsProgressMessageArea> {
        self.message_area.borrow().clone()
    }

    pub(crate) fn set_message_area(&self, area: Option<&PpsProgressMessageArea>) {
        let area = area.cloned();
        let message_area = self.message_area();

        if message_area == area {
            return;
        }

        if let Some(message_area) = message_area {
            self.document_toolbar_view.remove(&message_area);
        }

        if let Some(ref area) = area {
            self.document_toolbar_view.add_top_bar(area);
        }

        self.message_area.replace(area);
    }

    pub(super) fn clear_save_job(&self) {
        if let Some(job) = self.save_job.take() {
            if !job.is_finished() {
                job.cancel();
            }

            if let Some(id) = self.save_job_handler.take() {
                job.disconnect(id);
            }
        }
    }

    pub(super) fn reload_document(&self, document: Option<&Document>) {
        self.set_document(document);

        // Restart the search after reloading
        if self
            .sidebar_stack
            .visible_child()
            .is_some_and(|child| child == *self.find_sidebar)
        {
            if let Some(search_context) = self.search_context.borrow().clone() {
                search_context.restart();
            }
        }
    }

    pub(super) fn open_document(
        &self,
        document: &Document,
        metadata: Option<&papers_view::Metadata>,
        dest: Option<&LinkDest>,
    ) {
        if self.model.document().is_some_and(|d| d == *document) {
            return;
        }

        self.metadata.replace(metadata.cloned());

        let file = document.uri().map(|uri| gio::File::for_uri(&uri));

        self.file.replace(file);

        self.setup_model_from_metadata();
        self.set_document(Some(document));

        self.setup_document();
        self.setup_view_from_metadata();

        if let Some(dest) = dest {
            self.handle_link(dest);
        }
    }

    pub(super) fn open_copy_at_dest(&self, dest: Option<&LinkDest>) {
        self.parent_window()
            .downcast::<PpsWindow>()
            .unwrap()
            .open_copy(
                self.metadata().as_ref(),
                dest,
                self.display_name.borrow().as_str(),
                self.edit_name.borrow().as_str(),
            );
    }

    pub(super) fn document_modified(&self) {
        if self.modified.get() {
            return;
        }

        self.modified.set(true);

        let window_title = self
            .header_bar
            .title_widget()
            .and_downcast::<adw::WindowTitle>()
            .unwrap();
        let title = window_title.title();

        let new_title = if self.obj().direction() == gtk::TextDirection::Rtl {
            format!("{title} •")
        } else {
            format!("• {title}")
        };

        window_title.set_title(&new_title);
    }

    pub(super) fn set_document_metadata(&self) {
        let (Some(info), Some(metadata)) =
            (self.document().and_then(|d| d.info()), self.metadata())
        else {
            return;
        };

        metadata.set_string("title", &info.title().unwrap_or_default());
        metadata.set_string("author", &info.author().unwrap_or_default());
    }

    fn doc_title(&self) -> Option<String> {
        let backend = self.document().map(|d| d.type_().name())?;

        const BACKEND_PS: &str = "PSDocument";
        const BACKEND_PDF: &str = "PdfDocument";

        const BAD_EXTENSIONS: &[(&str, &str)] = &[
            (BACKEND_PS, ".dvi"),
            (BACKEND_PDF, ".doc"),
            (BACKEND_PDF, ".dvi"),
            (BACKEND_PDF, ".indd"),
            (BACKEND_PDF, ".rtf"),
        ];

        const BAD_PREFIXES: &[(&str, &str)] = &[
            (BACKEND_PDF, "Microsoft Word - "),
            (BACKEND_PDF, "Microsoft PowerPoint - "),
        ];

        self.document()
            .and_then(|d| d.title())
            .filter(|s| !s.trim().is_empty())
            .map(|title| {
                let mut title = title.as_str();

                for (target_backend, extension) in BAD_EXTENSIONS {
                    if *target_backend == backend {
                        title = title.trim_end_matches(extension);
                    }
                }

                for (target_backend, prefix) in BAD_PREFIXES {
                    if *target_backend == backend {
                        title = title.trim_start_matches(prefix);
                    }
                }

                title.to_string()
            })
    }

    fn update_title(&self) {
        let ltr = self.obj().direction() == gtk::TextDirection::Ltr;
        let doc_title = self.doc_title();
        let display_name = self.display_name.borrow();

        let window = self.parent_window();
        let title_widget = self
            .header_bar
            .title_widget()
            .and_downcast::<adw::WindowTitle>()
            .unwrap();

        if !display_name.is_empty() {
            if let Some(doc_title) = doc_title.filter(|title| !title.is_empty()) {
                let title = if ltr {
                    format!("{doc_title} – {display_name}")
                } else {
                    format!("{display_name} – {doc_title}")
                };

                let title = title.replace('\n', " ");

                window.set_title(Some(&title));
                title_widget.set_title(&doc_title);
                title_widget.set_subtitle(&display_name);
            } else {
                window.set_title(Some(&display_name));
                title_widget.set_title(&display_name);
            }
        } else {
            window.set_title(Some(&gettext("Papers")));
            title_widget.set_title(&gettext("Papers"));
        }
    }

    // This function detects the schema dynamically, since not only
    // linux installations have the schemas available
    pub(crate) fn setup_lockdown(&self) {
        const GS_LOCKDOWN_SCHEMA_NAME: &str = "org.gnome.desktop.lockdown";

        if self.lockdown_settings.borrow().is_some() {
            return;
        }

        let Some(schema) = gio::SettingsSchemaSource::default()
            .and_then(|source| source.lookup(GS_LOCKDOWN_SCHEMA_NAME, true))
        else {
            return;
        };

        let settings = gio::Settings::new_full(&schema, gio::SettingsBackend::NONE, None);

        settings.connect_changed(
            None,
            glib::clone!(
                #[weak(rename_to = obj)]
                self,
                move |_, _| {
                    obj.doc_restrictions_changed();
                }
            ),
        );

        self.lockdown_settings.replace(Some(settings));
    }

    pub(super) fn set_document(&self, document: Option<&Document>) {
        if self.model.document() == document.cloned() {
            return;
        }

        self.model.set_document(document);

        self.set_message_area(None);

        self.set_document_metadata();

        let Some(document) = document else {
            return;
        };

        if document.is::<DocumentSignatures>() {
            let job = JobSignatures::new(document);

            job.connect_finished(glib::clone!(
                #[weak(rename_to = obj)]
                self,
                move |job| {
                    let sigs = job.signatures();

                    if !sigs.is_empty() {
                        if sigs.iter().all(|s| s.is_valid()) {
                            obj.signature_banner.remove_css_class("error");
                            obj.signature_banner
                                .set_title(&gettext("Document has been digitally signed."));
                        } else {
                            obj.signature_banner.add_css_class("error");
                            obj.signature_banner
                                .set_title(&gettext("Digital signature is invalid"));
                        }
                        obj.signature_banner.set_revealed(true);
                    }
                }
            ));

            job.scheduler_push_job(JobPriority::PriorityNone);
        }

        if document.n_pages() <= 0 {
            self.warning_message(&gettext("The Document Contains No Pages"));
        } else if !document.check_dimensions() {
            self.warning_message(&gettext("The Document Contains Only Empty Pages"));
        }

        self.set_default_actions();

        self.modified.set(false);

        let id = document.connect_modified_notify(glib::clone!(
            #[weak(rename_to = obj)]
            self,
            move |_| {
                obj.document_modified();
            }
        ));

        self.modified_handler_id.replace(Some(id));

        // This cannot be done in pps_document_view_setup_default because before
        // having a document, we don't know which sidebars are supported
        self.setup_sidebar();

        // Set password callback
        if let Some(document) = document.dynamic_cast_ref::<DocumentSignatures>() {
            document.set_password_callback(glib::clone!(
                #[weak(rename_to = obj)]
                self,
                #[upgrade_or_default]
                move |name| obj.on_signature_password(name)
            ));
        }

        self.view.grab_focus();

        self.update_title();
    }

    pub(super) fn reset_progress_cancellable(&self) {
        self.progress_cancellable
            .replace(Some(gio::Cancellable::new()));
    }

    pub(super) fn set_filenames(&self, display_name: &str, edit_name: &str) {
        self.display_name.replace(display_name.to_owned());
        self.edit_name.replace(edit_name.to_owned());

        self.update_title();
    }

    pub(super) fn save_file<F>(&self, dest: &gio::File, f: F) -> Result<(), glib::Error>
    where
        F: FnOnce(&gio::File) -> Result<(), glib::Error>,
    {
        if dest.is_native() {
            f(dest)?;
        } else {
            let temp = papers_document::mkstemp_file("savefile.XXXXXX")?;

            f(&temp)?;
            self.save_remote(&temp, dest);
        }

        Ok(())
    }

    fn save_remote(&self, src: &gio::File, dest: &gio::File) {
        self.reset_progress_cancellable();

        glib::spawn_future_local(glib::clone!(
            #[weak(rename_to = obj)]
            self,
            #[strong]
            src,
            #[strong]
            dest,
            async move {
                let (result, _) = src.copy_future(
                    &dest,
                    gio::FileCopyFlags::OVERWRITE,
                    glib::Priority::DEFAULT,
                );

                if let Err(e) = result.await {
                    if !e.matches(gio::IOErrorEnum::Cancelled) {
                        obj.error_message(
                            Some(&e),
                            &gettext_f(
                                "The file could not be saved as “{}”.",
                                [dest.basename().unwrap_or_default().display().to_string()],
                            ),
                        );
                    }
                }
                papers_document::tmp_file_unlink(&src);
            }
        ));
    }

    // save as
    fn default_save_directory(&self) -> PathBuf {
        let default_dir = glib::user_special_dir(UserDirectory::Documents)
            .filter(|d| d.is_dir())
            .unwrap_or_else(glib::home_dir);

        let document_dir = self
            .file
            .borrow()
            .as_ref()
            .and_then(|f| f.parent())
            .and_then(|p| p.path());

        let tmp_dirs = [
            PathBuf::from("/tmp"),
            PathBuf::from("/var/tmp"),
            glib::tmp_dir(),
        ];

        document_dir
            .filter(|dir| !tmp_dirs.iter().any(|tmp| dir.starts_with(tmp)))
            .unwrap_or(default_dir)
    }

    pub(super) fn parent_window(&self) -> gtk::Window {
        self.obj().native().and_downcast::<gtk::Window>().unwrap()
    }

    pub(super) fn save_as(&self) {
        let dialog = gtk::FileDialog::builder()
            .title(gettext("Save As…"))
            .modal(true)
            .initial_name(self.edit_name.borrow().clone())
            .initial_folder(&gio::File::for_path(self.default_save_directory()))
            .build();

        Document::factory_add_filters(&dialog, self.document().as_ref());

        glib::spawn_future_local(glib::clone!(
            #[weak(rename_to = obj)]
            self,
            async move {
                let result = dialog.save_future(Some(&obj.parent_window())).await;

                match result {
                    Err(_) => obj.close_after_save.set(false),
                    Ok(file) => {
                        obj.file_dialog_save_folder(Some(&file), UserDirectory::Documents);

                        obj.clear_save_job();

                        let document = obj.document().unwrap();
                        let uri = file.uri();
                        let document_uri = obj.file.borrow().as_ref().unwrap().uri();

                        let save_job = papers_view::JobSave::new(&document, &uri, &document_uri);

                        let id = save_job.connect_finished(glib::clone!(
                            #[weak]
                            obj,
                            move |job| {
                                match job.is_succeeded() {
                                    Err(e) => {
                                        obj.close_after_save.set(false);
                                        obj.error_message(
                                            Some(&e),
                                            &gettext_f(
                                                "The file could not be saved as “{}”.",
                                                [job.uri().unwrap_or_default()],
                                            ),
                                        );
                                    }
                                    Ok(_) => {
                                        if let Some(uri) = job.uri() {
                                            gtk::RecentManager::default().add_item(&uri);
                                        }
                                    }
                                }

                                obj.clear_save_job();

                                if obj.close_after_save.get() {
                                    glib::idle_add_local_once(glib::clone!(
                                        #[weak]
                                        obj,
                                        move || {
                                            obj.parent_window().destroy();
                                        }
                                    ));
                                }
                            }
                        ));

                        obj.save_job.replace(Some(save_job.clone()));
                        obj.save_job_handler.replace(Some(id));

                        // The priority doesn't matter for this job
                        save_job.scheduler_push_job(JobPriority::PriorityNone);
                    }
                }
            }
        ));
    }

    fn draw_rect_action(&self) {
        self.banner
            .set_title(&gettext("Draw a rectangle to insert a signature field"));
        self.banner.set_button_label(Some(&gettext("_Cancel")));

        self.banner.set_revealed(true);
        self.view.start_signature_rect();
    }

    fn certificate_selection_response(&self) {
        let Some(certificate_info) = self.certificate_info.borrow().clone() else {
            return;
        };

        let signature =
            papers_document::Signature::new(SignatureStatus::Invalid, &certificate_info);

        let subject_common_name = certificate_info.subject_common_name().unwrap_or_default();
        let time = glib::DateTime::now_local()
            .unwrap()
            .format_iso8601()
            .unwrap();

        signature.set_signature(&gettext_f(
            "Digitally signed by {}\nDate: {}",
            [subject_common_name.as_str(), time.as_str()],
        ));
        signature.set_signature_left(&subject_common_name);

        self.signature.replace(Some(signature));

        self.draw_rect_action();
    }

    pub(crate) fn create_certificate_selection(&self) {
        let certs = self
            .document()
            .and_dynamic_cast_ref::<DocumentSignatures>()
            .map(|d| d.available_signing_certificates())
            .unwrap_or_default();
        let dialog = adw::AlertDialog::new(Some(&gettext("Certificate Required")), None);

        if !certs.is_empty() {
            dialog.set_body(&gettext("Select signing certificate"));
            dialog.add_response("select", &gettext("Select"));
            dialog.set_response_appearance("select", adw::ResponseAppearance::Suggested);

            dialog.add_response("cancel", &gettext("_Cancel"));
            dialog.set_close_response("cancel");
            dialog.set_default_response(Some("select"));

            let list_box = gtk::ListBox::builder()
                .selection_mode(gtk::SelectionMode::None)
                .css_classes(["content"])
                .build();

            let mut check_button_group = None;

            for cert in certs {
                let row = adw::ActionRow::builder()
                    .title(cert.id().unwrap_or_default())
                    .subtitle(cert.subject_common_name().unwrap_or_default())
                    .build();

                let check_button = gtk::CheckButton::builder()
                    .valign(gtk::Align::Center)
                    .build();

                row.add_prefix(&check_button);
                row.set_activatable_widget(Some(&check_button));

                check_button.connect_toggled(glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |button| {
                        if button.is_active() {
                            let row = button
                                .ancestor(adw::ActionRow::static_type())
                                .and_downcast::<adw::ActionRow>()
                                .unwrap();
                            let nick = row.title();

                            let document = obj
                                .document()
                                .and_dynamic_cast::<DocumentSignatures>()
                                .unwrap();
                            let info = document.certificate_info(&nick);

                            obj.certificate_info.replace(info);
                        }
                    }
                ));

                check_button.set_group(check_button_group.as_ref());

                if check_button_group.is_none() {
                    check_button.set_active(true);
                    check_button_group = Some(check_button);
                }

                list_box.insert(&row, -1);
            }

            list_box.select_row(list_box.row_at_index(0).as_ref());
            dialog.set_extra_child(Some(&list_box));

            dialog.connect_response(
                Some("select"),
                glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move |_, _| {
                        obj.certificate_selection_response();
                    }
                ),
            );
        } else {
            dialog.set_body(&gettext("A certificate is required to sign this document"));
            // show close if the certificate is null, or if no cert is found
            dialog.add_response("close", &gettext("_Close"));
            dialog.set_close_response("close");
        }

        dialog.present(Some(self.obj().as_ref()));
    }

    fn certificate_save_file(&self, path: impl AsRef<std::path::Path>) {
        let signature = self.signature.borrow().clone().unwrap();

        let filename = path.as_ref().as_os_str().to_str().unwrap();
        signature.set_destination_file(filename);
        signature.set_page(self.model.page() as u32);

        let document = self
            .document()
            .and_dynamic_cast::<DocumentSignatures>()
            .unwrap();

        glib::spawn_future_local(glib::clone!(
            #[weak(rename_to = obj)]
            self,
            async move {
                let result = document.sign_future(&signature).await;

                match result {
                    Err(e) => {
                        glib::g_warning!("", "Could not sign document: {}", e);
                        obj.signature.take();
                    }
                    Ok(_) => {
                        glib::spawn_future_local(glib::clone!(
                            #[weak]
                            obj,
                            async move {
                                let uri = glib::filename_to_uri(
                                    signature.destination_file().unwrap(),
                                    None,
                                )
                                .unwrap();

                                crate::application::spawn(
                                    Some(&gio::File::for_uri(&uri)),
                                    None,
                                    None,
                                );

                                obj.signature.take();
                            }
                        ));
                    }
                }
            }
        ));
    }

    pub(crate) fn certificate_save_as_dialog(&self) {
        let edit_name = self.edit_name.borrow().clone();

        let name = match edit_name.rsplit_once(".") {
            None => edit_name,
            Some((filename, ext)) => {
                format!("{filename}-signed.{ext}")
            }
        };

        let dialog = gtk::FileDialog::builder()
            .title(gettext("Save Signed File"))
            .initial_name(name)
            .initial_folder(&gio::File::for_path(self.default_save_directory()))
            .build();

        glib::spawn_future_local(glib::clone!(
            #[weak(rename_to = obj)]
            self,
            async move {
                let result = dialog.save_future(Some(&obj.parent_window())).await;

                match result {
                    Ok(file) => obj.certificate_save_file(file.path().unwrap()),
                    Err(e) => {
                        if !e.matches(gio::IOErrorEnum::Cancelled) {
                            glib::g_warning!("", "Could not save signed file: {}", e.message());
                        }

                        obj.certificate_info.take();
                    }
                }
            }
        ));
    }

    fn on_signature_password(&self, name: &str) -> Option<String> {
        use std::sync::{Arc, Mutex};

        let password: Arc<Mutex<Option<String>>> = Arc::new(Mutex::new(None));

        let body = gettext_f("Please enter password for {}", [name]);

        let entry = gtk::Entry::builder()
            .activates_default(true)
            .visibility(false)
            .build();

        let dialog = adw::AlertDialog::builder()
            .heading(gettext("Password Required"))
            .body(body)
            .default_response("login")
            .extra_child(&entry)
            .build();

        dialog.add_responses(&[
            ("cancel", &gettext("_Cancel")),
            ("login", &gettext("_Login")),
        ]);
        dialog.set_response_appearance("login", adw::ResponseAppearance::Suggested);

        // Note: Poppler (NSS) requires this function to return the requested value sync.
        // There is no async API so we need a nested loop here -.-

        let lp = Arc::new(glib::MainLoop::new(None, false));
        let remote_password = password.clone();
        let remote_lp = lp.clone();

        dialog.connect_response(
            None,
            glib::clone!(
                #[weak]
                entry,
                move |_, response| {
                    if response == "login" {
                        remote_password
                            .lock()
                            .unwrap()
                            .replace(entry.text().to_string());
                    }

                    remote_lp.quit();
                }
            ),
        );

        dialog.present(Some(&self.parent_window()));
        entry.grab_focus();
        lp.run();

        let password = password.lock().unwrap().take();

        password
    }
}
