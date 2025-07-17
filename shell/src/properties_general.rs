use crate::deps::*;
use papers_document::DocumentContainsJS;

mod imp {
    use super::*;

    #[derive(Properties, Default, Debug)]
    #[properties(wrapper_type = super::PpsPropertiesGeneral)]
    pub struct PpsPropertiesGeneral {
        #[property(set = Self::set_document, nullable)]
        pub(super) document: RefCell<Option<Document>>,
    }

    #[glib::object_subclass]
    impl ObjectSubclass for PpsPropertiesGeneral {
        const NAME: &'static str = "PpsPropertiesGeneral";
        type Type = super::PpsPropertiesGeneral;
        type ParentType = adw::Bin;
    }

    #[glib::derived_properties]
    impl ObjectImpl for PpsPropertiesGeneral {}

    impl BinImpl for PpsPropertiesGeneral {}

    impl WidgetImpl for PpsPropertiesGeneral {}

    impl PpsPropertiesGeneral {
        fn set_document(&self, document: Option<Document>) {
            if document == self.document.borrow().clone() {
                return;
            }

            self.obj().set_child(None::<&gtk::Widget>);

            if let Some(ref document) = document {
                let uri = document.uri().unwrap_or_default();
                let uri = glib::uri_unescape_string(uri, None::<&str>).unwrap_or_default();

                if let Some(info) = document.info() {
                    self.refresh_properties(uri.as_str(), document.size(), &info);
                }
            }

            self.document.replace(document);
        }

        fn create_copy_button(&self, text: &str) -> gtk::Button {
            let copy_button = gtk::Button::builder()
                .tooltip_text(gettext("Copy to Clipboard"))
                .icon_name("edit-copy-symbolic")
                .valign(gtk::Align::Center)
                .css_classes(["flat"])
                .build();

            let text_to_copy = text.to_owned();
            copy_button.connect_clicked(move |button| {
                let display = button.display();
                let clipboard = display.clipboard();
                clipboard.set_text(&text_to_copy);
            });

            copy_button
        }

        fn add_list_box_item_with_copy_button(
            &self,
            _group: &impl IsA<adw::PreferencesGroup>,
            label: &str,
            text: Option<&str>,
            text_to_copy: &str,
        ) -> adw::ActionRow {
            let row = adw::ActionRow::builder()
                .title(gettext(label))
                .use_markup(text.is_none())
                .css_classes(["property"])
                .subtitle_selectable(true)
                .build();

            // translators: This is used when a document property does not have
            // a value.  Examples:
            // Author: None
            // Keywords: None
            let display_text = match text {
                Some("") | None => gettext("None"),
                Some(text) => text.to_owned(),
            };

            row.set_subtitle(&display_text);

            // Add copy button
            if !text_to_copy.is_empty() {
                let copy_button = self.create_copy_button(text_to_copy);
                row.add_suffix(&copy_button);
            }

            row
        }

        fn refresh_properties(&self, uri: &str, size: u64, info: &DocumentInfo) {
            macro_rules! insert_string_field {
                ($group:ident, $title:expr, $field:ident) => {{
                    if let Some(value) = info.$field() {
                        self.add_list_box_item(&$group, &$title, Some(value.to_string().as_str()));
                    }

                    info.$field().is_some()
                }};
            }

            macro_rules! insert_property_time {
                ($group:ident, $title:expr, $field:ident) => {{
                    if let Some(time) = info.$field() {
                        self.add_list_box_item(
                            &$group,
                            &$title,
                            Document::misc_format_datetime(&time).as_deref(),
                        );
                    }

                    info.$field().is_some()
                }};
            }

            let page = adw::PreferencesPage::new();

            let file = gio::File::for_uri(uri);

            // Create buttons box with linked style
            let button_box = gtk::Box::builder()
                .orientation(gtk::Orientation::Horizontal)
                .css_classes(["linked"])
                .build();

            // Open With button
            let open_with_button = gtk::Button::builder()
                .tooltip_text(gettext("Open With…"))
                .icon_name("external-link-symbolic")
                .valign(gtk::Align::Center)
                .css_classes(["flat"])
                .build();

            let file_for_open = file.clone();
            open_with_button.connect_clicked(move |_| {
                let file_launcher = gtk::FileLauncher::new(Some(&file_for_open));
                file_launcher.set_always_ask(true);
                file_launcher.launch(gtk::Window::NONE, gio::Cancellable::NONE, |_| {});
            });

            // Create open folder button
            let folder_button = gtk::Button::builder()
                .tooltip_text(gettext("Open File Location"))
                .icon_name("folder-open-symbolic")
                .valign(gtk::Align::Center)
                .css_classes(["flat"])
                .build();

            let uri_clone = uri.to_string();
            let file_for_folder = file.clone();

            folder_button.connect_clicked(glib::clone!(
                #[weak(rename_to = obj)]
                self,
                move |_| {
                    let native = obj.obj().native();
                    let window = native.and_dynamic_cast_ref::<gtk::Window>();
                    let uri = uri_clone.clone();

                    // FIXME: It's broken on MacOS due to lack of support in GTK4
                    gtk::FileLauncher::new(Some(&file_for_folder)).open_containing_folder(
                        window,
                        gio::Cancellable::NONE,
                        move |result| {
                            if let Err(e) = result {
                                glib::g_warning!(
                                    "",
                                    "Could not show containing folder for \"{}\": {}",
                                    uri,
                                    e.message()
                                );
                            }
                        },
                    );
                }
            ));

            // Add buttons to the box
            button_box.append(&open_with_button);
            button_box.append(&folder_button);

            // File group
            let group = adw::PreferencesGroup::builder()
                .title(gettext("File"))
                .header_suffix(&button_box)
                .build();

            // Add filename with copy button as first item
            if let Some(filename) = file.basename() {
                let filename_str = filename.display().to_string();
                let filename_row = self.add_list_box_item_with_copy_button(
                    &group,
                    &gettext("Name"),
                    Some(&filename_str),
                    &filename_str,
                );
                group.add(&filename_row);
            }

            if let Some(parent) = file.parent() {
                // This is a extended attributes exported by document portal
                const HOST_PATH_ATTR: &str = "xattr::document-portal.host-path";

                // Get host path from file attributes (if running in Flatpak)
                let host_path = file
                    .query_info(
                        HOST_PATH_ATTR,
                        gio::FileQueryInfoFlags::NONE,
                        gio::Cancellable::NONE,
                    )
                    .ok()
                    .and_then(|r| r.attribute_string(HOST_PATH_ATTR));

                let host_parent = host_path
                    .as_ref()
                    .map(gio::File::for_path)
                    .and_then(|f| f.parent());

                // Default to regular parent if no host path available
                let folder_path = host_parent.as_ref().or(Some(&parent));

                // Get folder name for display
                let folder_name = folder_path
                    .and_then(|p| p.basename())
                    .map(|p| p.display().to_string())
                    .unwrap_or_else(|| gettext("None"));

                // Get folder path for copy operation
                let folder_path_str = folder_path
                    .and_then(|p| p.path())
                    .map(|p| p.display().to_string());

                // Create the folder row
                let row = adw::ActionRow::builder()
                    .title(gettext("Folder"))
                    .css_classes(["property"])
                    .subtitle_selectable(true)
                    .build();

                // Set folder name as subtitle
                row.set_subtitle(&folder_name);

                // Add copy button if we have a folder path
                if let Some(ref path) = folder_path_str {
                    let copy_button = self.create_copy_button(path);
                    row.add_suffix(&copy_button);
                }

                // Add folder row to group
                group.add(&row);

                // Add file path row with copy button
                if let (Some(folder_path), Some(fname)) = (folder_path_str, file.basename()) {
                    let path = std::path::Path::new(&folder_path).join(fname);
                    let full_path = path.display().to_string();

                    let path_row = self.add_list_box_item_with_copy_button(
                        &group,
                        &gettext("File Path"),
                        Some(&full_path),
                        &full_path,
                    );
                    group.add(&path_row);
                } else if let Some(file_path) = file.path() {
                    // Fallback to file.path() if we don't have a host path
                    let full_path = file_path.display().to_string();
                    let path_row = self.add_list_box_item_with_copy_button(
                        &group,
                        &gettext("File Path"),
                        Some(&full_path),
                        &full_path,
                    );
                    group.add(&path_row);
                }
            }

            if size != 0 {
                self.add_list_box_item(
                    &group,
                    &gettext("Size"),
                    Some(glib::format_size(size).as_str()),
                );
            }
            page.add(&group);

            // Content group
            let mut has_field = false;
            let group = adw::PreferencesGroup::builder()
                .title(gettext("Content"))
                .build();
            has_field |= insert_string_field!(group, gettext("Title"), title);
            has_field |= insert_string_field!(group, gettext("Subject"), subject);
            has_field |= insert_string_field!(group, gettext("Author"), author);
            has_field |= insert_string_field!(group, gettext("Keywords"), keywords);
            has_field |= insert_string_field!(group, gettext("Producer"), producer);
            has_field |= insert_string_field!(group, gettext("Creator"), creator);

            if has_field {
                page.add(&group);
            }

            // Date and time group
            let mut has_field = false;
            let group = adw::PreferencesGroup::builder()
                .title(gettext("Date &amp; Time"))
                .build();
            has_field |= insert_property_time!(group, gettext("Created"), created_datetime);
            has_field |= insert_property_time!(group, gettext("Modified"), modified_datetime);

            if has_field {
                page.add(&group);
            }

            // Format group
            let mut has_field = false;
            let group = adw::PreferencesGroup::builder()
                .title(gettext("Format"))
                .build();
            has_field |= insert_string_field!(group, gettext("Format"), format);
            has_field |= insert_string_field!(group, gettext("Number of Pages"), pages);
            has_field |= insert_string_field!(group, gettext("Optimized"), linearized);
            has_field |= insert_string_field!(group, gettext("Security"), security);
            has_field |= insert_string_field!(group, gettext("Paper Size"), regular_paper_size);

            if let Some(contain_js) = info.contains_js() {
                let text = match contain_js {
                    DocumentContainsJS::Yes => gettext("Yes"),
                    DocumentContainsJS::No => gettext("No"),
                    _ => gettext("Unknown"),
                };
                self.add_list_box_item(&group, &gettext("Contains Javascript"), Some(&text));

                has_field |= true;
            }

            if has_field {
                page.add(&group);
            }

            self.obj().set_child(Some(&page));
        }

        fn add_list_box_item(
            &self,
            group: &impl IsA<adw::PreferencesGroup>,
            label: &str,
            text: Option<&str>,
        ) {
            let row = adw::ActionRow::builder()
                .title(gettext(label))
                .use_markup(text.is_none())
                .css_classes(["property"])
                .subtitle_selectable(true)
                .build();

            // translators: This is used when a document property does not have
            // a value.  Examples:
            // Author: None
            // Keywords: None
            let text = match text {
                Some("") | None => gettext("None"),
                Some(text) => text.to_owned(),
            };

            row.set_subtitle(&text);
            group.add(&row);
        }
    }
}

glib::wrapper! {
    pub struct PpsPropertiesGeneral(ObjectSubclass<imp::PpsPropertiesGeneral>)
        @extends adw::Bin, gtk::Widget,
        @implements gtk::Accessible, gtk::Buildable, gtk::ConstraintTarget;
}

impl Default for PpsPropertiesGeneral {
    fn default() -> Self {
        Self::new()
    }
}

impl PpsPropertiesGeneral {
    pub fn new() -> Self {
        glib::Object::builder().build()
    }
}
