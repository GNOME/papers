use crate::deps::*;
use papers_document::{Attachment, DocumentAttachments};
use papers_view::AttachmentContext;
use std::env;
use std::fs;
use std::io;
use std::path::Path;
use std::process;
mod imp {
    use super::*;

    #[derive(CompositeTemplate, Debug, Default, Properties)]
    #[properties(wrapper_type = super::PpsSidebarAttachments)]
    #[template(resource = "/org/gnome/papers/ui/sidebar-attachments.ui")]
    pub struct PpsSidebarAttachments {
        #[property(set = Self::set_attachment_context, get)]
        attachment_context: RefCell<Option<AttachmentContext>>,

        #[template_child]
        list_view: TemplateChild<gtk::ListView>,
        #[template_child]
        selection_model: TemplateChild<gtk::MultiSelection>,
    }

    #[gtk::template_callbacks]
    impl PpsSidebarAttachments {
        fn attachment_context(&self) -> Option<AttachmentContext> {
            self.attachment_context.borrow().clone()
        }

        fn set_attachment_context(&self, context: Option<AttachmentContext>) {
            if self.attachment_context() == context {
                return;
            }

            self.selection_model.set_model(
                context
                    .as_ref()
                    .and_then(|context| context.model())
                    .as_ref(),
            );

            self.attachment_context.replace(context);
        }

        #[template_callback]
        fn list_view_factory_setup(&self, item: &gtk::ListItem, _factory: &gtk::ListItemFactory) {
            let box_ = gtk::Box::builder()
                .orientation(gtk::Orientation::Horizontal)
                .spacing(6)
                .margin_top(8)
                .margin_bottom(8)
                .build();

            let image = gtk::Image::builder()
                .icon_size(gtk::IconSize::Normal)
                .css_classes(["symbolic-circular"])
                .build();

            let label = gtk::Label::builder()
                .ellipsize(gtk::pango::EllipsizeMode::Middle)
                .xalign(0.0)
                .build();

            let save_button = gtk::Button::builder()
                .icon_name("document-save-symbolic")
                .css_classes(["circular", "flat"])
                .tooltip_text("Save As…")
                .hexpand(true)
                .halign(gtk::Align::End)
                .build();

            save_button.connect_clicked(glib::clone!(
                #[weak(rename_to = obj)]
                self,
                #[weak]
                item,
                move |_| {
                    obj.save_attachment(&item);
                }
            ));

            box_.append(&image);
            box_.append(&label);
            box_.append(&save_button);

            let drag = gtk::DragSource::new();

            drag.connect_prepare(glib::clone!(
                #[weak(rename_to = obj)]
                self,
                #[weak]
                item,
                #[upgrade_or_default]
                move |_drag, _x, _y| obj.attachments_drag_prepare(&item)
            ));

            box_.add_controller(drag);

            item.set_child(Some(&box_));
        }

        #[template_callback]
        fn list_view_factory_bind(&self, item: &gtk::ListItem, _factory: &gtk::ListItemFactory) {
            let box_ = item.child().and_downcast::<gtk::Box>().unwrap();
            let image = box_.first_child().and_downcast::<gtk::Image>().unwrap();
            let label = image.next_sibling().and_downcast::<gtk::Label>().unwrap();
            let attachment = item.item().and_downcast::<Attachment>().unwrap();

            if let Some(description) = attachment
                .description()
                .map(|s| glib::gformat!("{}", glib::markup_escape_text(s.as_str())))
            {
                if !description.is_empty() {
                    box_.set_tooltip_text(Some(description.as_str()));
                }
            }

            if let Some(icon) = attachment
                .mime_type()
                .map(|mime| gio::content_type_get_symbolic_icon(mime.as_str()))
            {
                image.set_from_gicon(&icon);
            }

            if let Some(name) = attachment.name() {
                label.set_text(name.as_str());
            }
        }

        #[template_callback]
        fn list_view_item_activated(&self, position: u32, _list_view: &gtk::ListView) {
            let Some(attachment) = self
                .list_view
                .model()
                .and_then(|model| model.item(position))
                .and_downcast::<Attachment>()
            else {
                return;
            };

            let context = self.obj().display().app_launch_context();

            if let Err(e) = attachment.open(&context) {
                warn!("{e}");
            }
        }

        #[template_callback]
        fn button_clicked(&self, _n_press: i32, x: f64, y: f64, click: &gtk::GestureClick) {
            let Some(selection) = self.list_view.model() else {
                return;
            };

            let point = gtk::graphene::Point::new(x as f32, y as f32);

            if click
                .widget()
                .and_then(|w| w.compute_point(self.list_view.upcast_ref::<gtk::Widget>(), &point))
                .and_then(|point| {
                    self.list_view
                        .pick(point.x() as f64, point.y() as f64, gtk::PickFlags::DEFAULT)
                })
                .is_none()
            {
                selection.unselect_all();
            }
        }

        fn selected_attachment(&self) -> glib::List<Attachment> {
            let mut attachments = glib::List::new();

            let Some(selection) = &self.list_view.model() else {
                return attachments;
            };

            let bitset = selection.selection();

            let Some((iter, index)) = gtk::BitsetIter::init_first(&bitset) else {
                return attachments;
            };

            let attachment = selection.item(index).and_downcast::<Attachment>();
            if let Some(attachment) = attachment {
                attachments.push_front(attachment);
            }

            for attachment in iter
                .filter_map(|index| selection.item(index))
                .filter_map(|obj| obj.downcast::<Attachment>().ok())
            {
                attachments.push_front(attachment);
            }

            attachments
        }

        fn attachments_drag_prepare(&self, item: &gtk::ListItem) -> Option<gdk::ContentProvider> {
            let selection = self.list_view.model()?;

            if !selection.is_selected(item.position()) {
                return None;
            }

            let mut files = Vec::new();
            fn get_unique_temp_dir() -> Result<String, io::Error> {
                let exe_path = env::current_exe()?;

                // Extract the executable name from the path
                let exe_name = exe_path.file_name().unwrap_or_default().to_string_lossy();

                // "Build a unique directory path in the temp folder using the executable name and process ID

                let full_temp_path =
                    env::temp_dir().join(format!("{}-{}", exe_name, process::id()));

                // Ensure the directory exists (create it if necessary)
                if !full_temp_path.exists() {
                    fs::create_dir_all(&full_temp_path)?;
                }

                Ok(full_temp_path.to_string_lossy().to_string())
            }

            for attachment in self.selected_attachment() {
                let tempdir = match get_unique_temp_dir() {
                    Ok(s) => s,
                    Err(e) => {
                        warn!("{e}");
                        continue;
                    }
                };

                let Some(name) = attachment.name() else {
                    continue;
                };

                let template = Path::new(&tempdir).join(name);
                let file = gio::File::for_path(template);

                if let Err(e) = attachment.save(&file) {
                    warn!("{e}");
                    continue;
                }

                files.push(file);
            }

            if files.is_empty() {
                return None;
            }

            let file_list = gdk::FileList::from_array(&files);

            Some(gdk::ContentProvider::for_value(&file_list.to_value()))
        }

        fn save_attachment(&self, item: &gtk::ListItem) {
            if let Some(attachment) = item.item().and_downcast::<Attachment>() {
                let attachment_singleton = gio::ListStore::new::<Attachment>();
                attachment_singleton.append(&attachment);

                if let Some(attachment_context) = self.attachment_context() {
                    let window = self
                        .obj()
                        .root()
                        .and_then(|root| root.downcast::<gtk::Window>().ok());
                    attachment_context.save_attachments_async(
                        attachment_singleton,
                        window.as_ref(),
                        gio::Cancellable::NONE,
                        glib::clone!(
                            #[weak(rename_to = obj)]
                            self,
                            move |result| {
                                if let Err(error) = result {
                                    let document_view = obj
                                        .obj()
                                        .ancestor(PpsDocumentView::static_type())
                                        .and_downcast::<PpsDocumentView>()
                                        .unwrap();

                                    if !error.matches(gtk::DialogError::Dismissed) {
                                        document_view.error_message(
                                            Some(&error),
                                            &gettext("Unable to Open Attachment"),
                                        );
                                    }
                                }
                            }
                        ),
                    );
                }
            }
        }
    }

    #[glib::object_subclass]
    impl ObjectSubclass for PpsSidebarAttachments {
        const NAME: &'static str = "PpsSidebarAttachments";
        type Type = super::PpsSidebarAttachments;
        type ParentType = PpsSidebarPage;

        fn class_init(klass: &mut Self::Class) {
            klass.bind_template();
            klass.bind_template_callbacks();
        }

        fn instance_init(obj: &InitializingObject<Self>) {
            obj.init_template();
        }
    }

    #[glib::derived_properties]
    impl ObjectImpl for PpsSidebarAttachments {}

    impl WidgetImpl for PpsSidebarAttachments {}

    impl BinImpl for PpsSidebarAttachments {}

    impl PpsSidebarPageImpl for PpsSidebarAttachments {
        fn support_document(&self, document: &Document) -> bool {
            document
                .dynamic_cast_ref::<DocumentAttachments>()
                .map(|d| d.has_attachments())
                .unwrap_or_default()
        }
    }
}

glib::wrapper! {
    pub struct PpsSidebarAttachments(ObjectSubclass<imp::PpsSidebarAttachments>)
    @extends gtk::Widget, adw::Bin, PpsSidebarPage,
    @implements gtk::Accessible, gtk::Buildable, gtk::ConstraintTarget;
}

impl Default for PpsSidebarAttachments {
    fn default() -> Self {
        Self::new()
    }
}

impl PpsSidebarAttachments {
    fn new() -> Self {
        glib::Object::builder().build()
    }
}
