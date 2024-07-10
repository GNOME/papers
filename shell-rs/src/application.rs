use crate::deps::*;

use papers_document::{LinkDest, LinkDestType};
use papers_view::Job;

use std::ffi::OsString;

use std::env;

mod imp {

    use super::*;

    #[derive(Default)]
    pub struct PpsApplication;

    #[glib::object_subclass]
    impl ObjectSubclass for PpsApplication {
        const NAME: &'static str = "PpsApplication";
        type Type = super::PpsApplication;
        type ParentType = adw::Application;
    }

    impl ObjectImpl for PpsApplication {
        fn constructed(&self) {
            self.parent_constructed();
            self.setup_command_line();
        }
    }

    impl AdwApplicationImpl for PpsApplication {}

    impl GtkApplicationImpl for PpsApplication {}

    impl ApplicationImpl for PpsApplication {
        fn startup(&self) {
            self.parent_startup();
            papers_document::init();

            // Manually set name and icon
            glib::set_application_name(&gettext("Papers"));
            gtk::Window::set_default_icon_name(APP_ID);

            self.setup_actions();

            // Listen to the dbus interface
            glib::spawn_future_local(glib::clone!(@weak self as obj => async move {
                if let Err(e) = obj.dbus_service().await {
                    glib::g_warning!("", "Failed to launch the dbus service: {}", e);
                }
            }));
        }

        fn shutdown(&self) {
            papers_document::shutdown();
            Job::scheduler_wait();
            self.parent_shutdown();
        }

        fn activate(&self) {
            for window in self.obj().windows() {
                if let Ok(window) = window.downcast::<papers_shell::Window>() {
                    window.present();
                }
            }
        }

        fn handle_local_options(&self, options: &glib::VariantDict) -> glib::ExitCode {
            // print the version in local instance rather than sending it to primary
            if options.contains("version") {
                glib::g_print!("{} {}", gettext("Papers"), crate::config::VERSION);
                return 0.into();
            }

            (-1).into()
        }

        fn command_line(&self, command_line: &gio::ApplicationCommandLine) -> glib::ExitCode {
            let options = command_line.options_dict();
            let mut mode = WindowRunMode::Normal;

            if options.contains("fullscreen") {
                mode = WindowRunMode::Fullscreen;
            } else if options.contains("presentation") {
                mode = WindowRunMode::Presentation;
            }

            let page_index = options.lookup::<i32>("page-index").unwrap();
            let page_label = options.lookup::<String>("page-label").unwrap();
            let named_dest = options.lookup::<String>("named-dest").unwrap();
            let files = options
                .lookup::<Vec<OsString>>(glib::OPTION_REMAINING)
                .unwrap();

            let mut dest = None;

            if let Some(page_label) = page_label {
                dest = Some(LinkDest::new_page_label(&page_label));
            } else if let Some(page_index) = page_index {
                dest = Some(LinkDest::new_page(i32::max(0, page_index - 1)));
            } else if let Some(named_dest) = named_dest {
                dest = Some(LinkDest::new_named(&named_dest));
            }

            match files {
                None => self.open_start_view(),
                Some(files) => {
                    for arg in files {
                        let f = arg.to_string_lossy();
                        let (f, label) = Self::split_label(&f);

                        if let Some(label) = label {
                            dest = Some(LinkDest::new_page_label(label));
                        }

                        let f = gio::File::for_commandline_arg(f);
                        self.open_uri_at_dest(&f.uri(), dest.as_ref(), mode);
                    }
                }
            }

            0.into()
        }

        fn open(&self, files: &[gio::File], _hint: &str) {
            for f in files {
                let uri = f.uri();

                self.open_uri_at_dest(uri.as_str(), None, WindowRunMode::Normal);
            }
        }
    }

    #[zbus::interface(name = "org.gnome.Papers")]
    impl PpsApplication {}

    impl PpsApplication {
        async fn dbus_service(&self) -> zbus::Result<()> {
            let connection = zbus::Connection::session().await?;

            connection
                .object_server()
                .at(
                    format!("/org/gnome/Papers/Papers{}", OBJECT_PROFILE),
                    PpsApplication,
                )
                .await?;

            connection.request_name("org.gnome.Papers").await?;

            loop {
                // do something else, wait forever or timeout here:
                // handling D-Bus messages is done in the background
                std::future::pending::<()>().await;
            }
        }
    }

    impl PpsApplication {
        fn split_label(arg: &str) -> (&str, Option<&str>) {
            if let Some((arg, label)) = arg.rsplit_once('#') {
                // Filename contains a #, check whether it's part of the path
                // or a label.
                let file = gio::File::for_commandline_arg(arg);
                if !file.query_exists(gio::Cancellable::NONE) {
                    return (arg, Some(label));
                }
            }
            (arg, None)
        }

        fn open_start_view(&self) {
            papers_shell::Window::default().present();
        }

        fn open_uri_at_dest(
            &self,
            uri: &str,
            dest: Option<&papers_document::LinkDest>,
            mode: WindowRunMode,
        ) {
            let obj = self.obj();
            let mut n_window = 0;
            let mut window = None;

            for w in obj
                .windows()
                .into_iter()
                .filter_map(|w| w.downcast::<papers_shell::Window>().ok())
            {
                if w.is_empty() || w.uri().map(|gs| gs.as_str() == uri).unwrap_or_default() {
                    window = Some(w.clone());
                }

                n_window += 1;
            }

            if n_window != 0 && window.is_none() {
                // There are windows, but they hold a different document.
                // Since we don't have security between documents, then
                // spawn a new process! See:
                // https://gitlab.gnome.org/GNOME/Incubator/papers/-/issues/104
                spawn(Some(uri), dest, mode);
                return;
            }

            let window = window.unwrap_or_default();

            // We need to load uri before showing the window, so
            // we can restore window size without flickering
            window.open_uri(uri, dest, mode);
            window.present();
        }

        fn show_about(&self) {
            let about =
                adw::AboutDialog::from_appdata("/org/gnome/papers/metainfo.xml", Some(VERSION));

            about.set_copyright(&gettext("© 1996–2024 The Papers authors"));
            about.set_translator_credits(&gettext("translator-credits"));

            about.set_developers(&[
                "Martin Kretzschmar <m_kretzschmar@gmx.net>",
                "Jonathan Blandford <jrb@gnome.org>",
                "Marco Pesenti Gritti <marco@gnome.org>",
                "Nickolay V. Shmyrev <nshmyrev@yandex.ru>",
                "Bryan Clark <clarkbw@gnome.org>",
                "Carlos Garcia Campos <carlosgc@gnome.org>",
                "Wouter Bolsterlee <wbolster@gnome.org>",
                "Christian Persch <chpe\u{0040}src.gnome.org>",
                "Germán Poo-Caamaño <gpoo\u{0040}gnome.org>",
                "Qiu Wenbo <qiuwenbo\u{0040}gnome.org>",
                "Pablo Correa Gómez <ablocorrea\u{0040}hotmail.com>",
            ]);

            about.set_documenters(&[
                "Nickolay V. Shmyrev <nshmyrev@yandex.ru>",
                "Phil Bull <philbull@gmail.com>",
                "Tiffany Antpolski <tiffany.antopolski@gmail.com>",
            ]);

            // Force set the version for the development release
            about.set_version(VERSION);

            about.present(self.obj().active_window().as_ref().unwrap());
        }

        fn show_help(&self) {
            gtk::UriLauncher::new("help:papers").launch(
                self.obj().active_window().as_ref(),
                gio::Cancellable::NONE,
                |_| {},
            );
        }

        fn setup_actions(&self) {
            let actions = [
                gio::ActionEntryBuilder::new("about")
                    .activate(glib::clone!(@weak self as obj => move |_, _, _| {
                        obj.show_about();
                    }))
                    .build(),
                gio::ActionEntryBuilder::new("help")
                    .activate(glib::clone!(@weak self as obj => move |_, _, _| obj.show_help()))
                    .build(),
                gio::ActionEntryBuilder::new("quit")
                    .activate(glib::clone!(@weak self as obj => move |_, _, _| obj.obj().quit()))
                    .build(),
                gio::ActionEntryBuilder::new("new")
                    .activate(glib::clone!(@weak self as obj => move |_ , _, _| {
                        // spawn an empty window
                        spawn(None, None, WindowRunMode::Normal);
                    }))
                    .build(),
            ];

            let obj = self.obj();

            obj.add_action_entries(actions);

            obj.set_accels_for_action("app.help", &["F1"]);
            obj.set_accels_for_action("app.new", &["<Ctrl>N"]);

            obj.set_accels_for_action("win.open", &["<Ctrl>O"]);
            obj.set_accels_for_action("win.close", &["<Ctrl>W"]);
            obj.set_accels_for_action("win.fullscreen", &["F11"]);
            obj.set_accels_for_action("win.escape", &["Escape"]);
            obj.set_accels_for_action("win.show-sidebar", &["F9"]);
            obj.set_accels_for_action("win.inverted-colors", &["<Ctrl>I"]);

            // TODO: move to doc group
            obj.set_accels_for_action("win.open-copy", &["<Ctrl><Shift>N"]);
            obj.set_accels_for_action("win.save-as", &["<Ctrl>S"]);
            obj.set_accels_for_action("win.print", &["<Ctrl>P"]);
            obj.set_accels_for_action("win.show-properties", &["<alt>Return"]);
            obj.set_accels_for_action("win.copy", &["<Ctrl>C", "<Ctrl>Insert"]);
            obj.set_accels_for_action("win.select-page", &["<Ctrl>L"]);
            obj.set_accels_for_action("win.caret-navigation", &["F7"]);
            obj.set_accels_for_action("win.presentation", &["F5", "<Shift>F5"]);
            obj.set_accels_for_action("win.rotate-left", &["<Ctrl>Left"]);
            obj.set_accels_for_action("win.rotate-right", &["<Ctrl>Right"]);
            obj.set_accels_for_action("win.add-highlight-annotation", &["<Ctrl>H"]);
        }

        fn setup_command_line(&self) {
            use glib::{OptionArg, OptionFlags};

            let obj = self.obj();

            obj.set_option_context_parameter_string(Some(&gettext("Papers")));

            obj.add_main_option(
                "page-label",
                b'p'.into(),
                OptionFlags::NONE,
                OptionArg::String,
                &gettext("The page label of the document to display."),
                Some(&gettext("PAGE")),
            );

            obj.add_main_option(
                "page-index",
                b'i'.into(),
                OptionFlags::NONE,
                OptionArg::Int,
                &gettext("The page number of the document to display."),
                Some(&gettext("NUMBER")),
            );

            obj.add_main_option(
                "named-dest",
                b'n'.into(),
                OptionFlags::NONE,
                OptionArg::String,
                &gettext("Named destination to display."),
                Some(&gettext("DEST")),
            );

            obj.add_main_option(
                "fullscreen",
                b'f'.into(),
                OptionFlags::NONE,
                OptionArg::None,
                &gettext("Run papers in fullscreen mode."),
                None,
            );

            obj.add_main_option(
                "presentation",
                b's'.into(),
                OptionFlags::NONE,
                OptionArg::None,
                &gettext("Run papers in presentation mode."),
                None,
            );

            obj.add_main_option(
                "version",
                0.into(),
                OptionFlags::NONE,
                OptionArg::None,
                &gettext("Show the version of the program."),
                None,
            );

            obj.add_main_option(
                glib::OPTION_REMAINING,
                0.into(),
                OptionFlags::NONE,
                OptionArg::FilenameArray,
                "",
                Some(&gettext("[FILE…]")),
            );
        }
    }
}

glib::wrapper! {
    pub struct PpsApplication(ObjectSubclass<imp::PpsApplication>)
        @extends adw::Application, gtk::Application, gio::Application,
        @implements gio::ActionGroup, gio::ActionMap;
}

impl Default for PpsApplication {
    fn default() -> Self {
        Self::new()
    }
}

impl PpsApplication {
    pub fn new() -> Self {
        let flags = gio::ApplicationFlags::HANDLES_COMMAND_LINE
            | gio::ApplicationFlags::NON_UNIQUE
            | gio::ApplicationFlags::HANDLES_OPEN;

        glib::Object::builder()
            .property("application-id", APP_ID)
            .property("flags", flags)
            .property("resource-base-path", "/org/gnome/papers")
            .property("register-session", true)
            .build()
    }
}

pub fn spawn(uri: Option<&str>, dest: Option<&LinkDest>, mode: WindowRunMode) {
    let mut cmd = String::new();

    match env::current_exe() {
        Ok(path) => {
            cmd.push_str(&format!(" {}", &path.to_string_lossy()));

            // Page label
            if let Some(dest) = dest {
                match dest.dest_type() {
                    LinkDestType::PageLabel => {
                        cmd.push_str(" --page-label=");
                        cmd.push_str(&dest.page_label().unwrap_or_default());
                    }
                    LinkDestType::Page
                    | LinkDestType::Xyz
                    | LinkDestType::Fit
                    | LinkDestType::Fith
                    | LinkDestType::Fitv
                    | LinkDestType::Fitr => {
                        cmd.push_str(&format!(" --page-index={}", dest.page() + 1))
                    }
                    LinkDestType::Named => {
                        cmd.push_str(" --named-dest=");
                        cmd.push_str(&dest.named_dest().unwrap_or_default())
                    }
                    _ => (),
                }
            }
        }
        Err(e) => glib::g_critical!("", "Failed to find current executable: {}", e),
    }

    // Mode
    match mode {
        WindowRunMode::Fullscreen => cmd.push_str(" -f"),
        WindowRunMode::Presentation => cmd.push_str(" -s"),
        _ => (),
    }

    let app =
        gio::AppInfo::create_from_commandline(&cmd, None, gio::AppInfoCreateFlags::SUPPORTS_URIS);

    let result = app.and_then(|app| {
        let ctx = gdk::Display::default().map(|display| display.app_launch_context());
        // Some URIs can be changed when passed through a GFile
        // (for instance unsupported uris with strange formats like mailto:),
        // so if you have a textual uri you want to pass in as argument,
        // consider using g_app_info_launch_uris() instead.
        // See https://bugzilla.gnome.org/show_bug.cgi?id=644604
        let mut uris = vec![];

        if let Some(uri) = uri {
            uris.push(uri);
        }

        app.launch_uris(&uris, ctx.as_ref())
    });

    if let Err(e) = result {
        debug!("fallback to plain spawn: {}", e.message());

        if let Some(uri) = uri {
            cmd.push(' ');
            cmd.push_str(uri);
        }

        // MacOS take this path since GAppInfo doesn't support created by
        // command line on MacOS.
        if let Err(e) = glib::spawn_command_line_async(cmd) {
            glib::g_printerr!(
                "Error launching papers {}: {}\n",
                uri.unwrap_or_default(),
                e.message()
            );
        }
    }
}
