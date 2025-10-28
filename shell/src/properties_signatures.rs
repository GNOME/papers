use crate::deps::*;

use papers_document::{CertificateStatus, DocumentSignatures, SignatureStatus};
use papers_view::{JobPriority, JobSignatures};

mod imp {
    use super::*;

    #[derive(CompositeTemplate, Debug, Default)]
    #[template(resource = "/org/gnome/papers/ui/properties-signatures.ui")]
    pub struct PpsPropertiesSignatures {
        #[template_child]
        signatures_page: TemplateChild<adw::PreferencesPage>,

        signatures_job: RefCell<Option<JobSignatures>>,
        document: RefCell<Option<Document>>,
        job_handler_id: RefCell<Option<SignalHandlerId>>,
    }

    impl PpsPropertiesSignatures {
        fn document(&self) -> Option<Document> {
            self.document.borrow().clone()
        }

        pub fn set_document(&self, document: Document) {
            if self.document().is_some_and(|d| d == document) {
                return;
            }

            let job = JobSignatures::new(&document);

            let job_handler_id = job.connect_finished(glib::clone!(
                #[weak(rename_to = obj)]
                self,
                move |job| {
                    if let Some(id) = obj.job_handler_id.take() {
                        job.disconnect(id);
                    }

                    if let Some(doc_signatures) =
                        job.document().and_dynamic_cast_ref::<DocumentSignatures>()
                    {
                        let signatures = doc_signatures.signatures();
                        let signature_count = signatures.len();

                        for (signature_index, signature) in signatures.iter().enumerate() {
                            // Create a PreferencesGroup for each signature
                            let signature_group = adw::PreferencesGroup::new();
                            // Set the title with signer information
                            let mut signer_text = String::new();
                            if let Some(ref certificate_info) = signature.certificate_info() &&
                               let Some(ref signer_name) = certificate_info.subject_common_name() {
                                if signature_count > 1 {
                                    signer_text = match certificate_info.subject_email() {
                                        Some(ref signer_email) if !signer_email.is_empty() =>
                                            formatx!(
                                                // Translators: Do NOT translate the content between
                                                // '{' and '}', they are variable names. Changing their
                                                // order is possible. {current} and {total} are numbers,
                                                // {name} and {email} are signer details
                                                gettext("Signature {current} of {total} by {name} <{email}>"),
                                                current = &(signature_index + 1).to_string(),
                                                total = &signature_count.to_string(),
                                                name = signer_name.as_str(),
                                                email = signer_email.as_str(),
                                            ).expect("Wrong format in translated string"),
                                        _ =>
                                            formatx!(
                                                // Translators: Do NOT translate the content between
                                                // '{' and '}', they are variable names. Changing their
                                                // order is possible. {current} and {total} are numbers,
                                                // {name} is the signer name
                                                gettext("Signature {current} of {total} by {name}"),
                                                current = &(signature_index + 1).to_string(),
                                                total = &signature_count.to_string(),
                                                name = signer_name.as_str(),
                                            ).expect("Wrong format in translated string")

                                    };
                                } else {
                                    signer_text = if let Some(ref signer_email) = certificate_info.subject_email() {
                                        if !signer_email.is_empty() {
                                            formatx!(
                                                // Translators: Do NOT translate the content between
                                                // '{' and '}', they are variable names. Changing their
                                                // order is possible. {name} and {email} are signer details
                                                gettext("Signed by {name} <{email}>"),
                                                name = signer_name.as_str(),
                                                email = signer_email.as_str(),
                                            ).expect("Wrong format in translated string")
                                        } else {
                                            formatx!(
                                                // Translators: Do NOT translate the content between
                                                // '{' and '}', they are variable names. {name} is the signer name
                                                gettext("Signed by {name}"),
                                                name = signer_name.as_str(),
                                            ).expect("Wrong format in translated string")
                                        }
                                    } else {
                                        formatx!(
                                            // Translators: Do NOT translate the content between
                                            // '{' and '}', they are variable names. {name} is the signer name
                                            gettext("Signed by {name}"),
                                            name = signer_name.as_str(),
                                        ).expect("Wrong format in translated string")
                                    };
                                }
                            }
                            signature_group.set_title(&glib::markup_escape_text(&signer_text));

                            // Create status listbox for this signature
                            let status_listbox = gtk::ListBox::builder()
                                .selection_mode(gtk::SelectionMode::None)
                                .can_focus(false)
                                .hexpand(true)
                                .vexpand(false)
                                .css_classes(["content"])
                                .build();
                            let (icon_name, text) = match signature.status() {
                                SignatureStatus::Valid => {
                                    ("emblem-ok", gettext("Signature is valid."))
                                }
                                SignatureStatus::Invalid => {
                                    ("dialog-error-symbolic", gettext("Signature is invalid."))
                                }
                                SignatureStatus::DigestMismatch => (
                                    "dialog-error-symbolic",
                                    gettext("Document has been changed after signing."),
                                ),
                                SignatureStatus::DecodingError => (
                                    "dialog-error-symbolic",
                                    gettext("Signature could not be decoded."),
                                ),
                                SignatureStatus::GenericError => (
                                    "dialog-error-symbolic",
                                    gettext("Signature verification error."),
                                ),
                                _ => panic!("unknown signature status"),
                            };

                            obj.add_status_row_to_listbox(&status_listbox, icon_name, &text);

                            if let Some(ref certificate_info) = signature.certificate_info() {
                                let (icon_name, text) = match certificate_info.status() {
                                    CertificateStatus::Trusted => (
                                        "emblem-ok",
                                        gettext(
                                            "Signed with a certificate issued by trusted issuer.",
                                        ),
                                    ),
                                    CertificateStatus::UntrustedIssuer => (
                                        "dialog-warning-symbolic",
                                        gettext(
                                            "Signed with a certificate issued by untrusted issuer.",
                                        ),
                                    ),
                                    CertificateStatus::UnknownIssuer => (
                                        "dialog-warning-symbolic",
                                        gettext(
                                            "Signed with a certificate issued by unknown issuer.",
                                        ),
                                    ),
                                    CertificateStatus::Revoked => (
                                        "dialog-error-symbolic",
                                        gettext("Signed with revoked certificate."),
                                    ),
                                    CertificateStatus::Expired => (
                                        "dialog-warning-symbolic",
                                        gettext("Signed with expired certificate."),
                                    ),
                                    CertificateStatus::GenericError => (
                                        "dialog-error-symbolic",
                                        gettext("Certificate verification error."),
                                    ),
                                    CertificateStatus::NotVerified => (
                                        "dialog-warning-symbolic",
                                        gettext("Certificate has not been verified."),
                                    ),
                                    _ => panic!("unknown certificate status"),
                                };

                                obj.add_status_row_to_listbox(&status_listbox, icon_name, &text);
                            }

                            signature_group.add(&status_listbox);

                            // Create details group for this signature
                            let details_group = adw::PreferencesGroup::builder()
                                .title("")
                                .build();

                            let details_listbox = gtk::ListBox::builder()
                                .selection_mode(gtk::SelectionMode::None)
                                .can_focus(false)
                                .hexpand(true)
                                .vexpand(false)
                                .css_classes(["content"])
                                .build();

                            // Add always-visible important information
                            if let Some(ref certificate_info) = signature.certificate_info() &&
                               let Some(ref signer_organization) = certificate_info.subject_organization()
                            {
                                obj.add_property_to_listbox(
                                    &details_listbox,
                                    &gettext("Organization"),
                                    signer_organization.as_str(),
                                );
                            }


                            if let Some(signature_time) = signature.signature_time() {
                                obj.add_property_to_listbox(
                                    &details_listbox,
                                    &gettext("Date and Time"),
                                    Document::misc_format_datetime(&signature_time).unwrap().as_str(),
                                );
                            }

                            // Add expandable row for additional details
                            let expander_row = adw::ExpanderRow::builder()
                                .title(gettext("Details"))
                                .build();

                            if let Some(ref certificate_info) = signature.certificate_info() {
                                if let Some(ref certificate_issuer_common_name) =
                                    certificate_info.issuer_common_name()
                                {
                                    obj.add_property_to_expander_row(
                                        &expander_row,
                                        &gettext("Certificate Issuer"),
                                        certificate_issuer_common_name.as_str()
                                    );
                                }

                                if let Some(ref certificate_issuer_email) =
                                    certificate_info.issuer_email()
                                {
                                    obj.add_property_to_expander_row(
                                        &expander_row,
                                        &gettext("Certificate Issuer's Email"),
                                        certificate_issuer_email.as_str()
                                    );
                                }

                                if let Some(ref certificate_issuer_organization) =
                                    certificate_info.issuer_organization()
                                {
                                    obj.add_property_to_expander_row(
                                        &expander_row,
                                        &gettext("Certificate Issuer's Organization"),
                                        certificate_issuer_organization.as_str()
                                    );
                                }

                                if let Some(certificate_issuance_time) =
                                    certificate_info.issuance_time()
                                {
                                    obj.add_property_to_expander_row(
                                        &expander_row,
                                        &gettext("Certificate's Issuance Time"),
                                        Document::misc_format_datetime(
                                            &certificate_issuance_time
                                        ).unwrap().as_str(),
                                    );
                                }

                                if let Some(certificate_expiration_time) =
                                    certificate_info.expiration_time()
                                {
                                    obj.add_property_to_expander_row(
                                        &expander_row,
                                        &gettext("Certificate's Expiration Time"),
                                        Document::misc_format_datetime(
                                            &certificate_expiration_time,
                                        ).unwrap().as_str(),
                                    );
                                }
                            }

                            details_listbox.append(&expander_row);
                            details_group.add(&details_listbox);

                            // Add both groups to the page
                            obj.signatures_page.add(&signature_group);
                            obj.signatures_page.add(&details_group);
                        }
                    }
                }
            ));

            job.scheduler_push_job(JobPriority::PriorityNone);

            self.document.replace(Some(document));
            self.signatures_job.replace(Some(job));
            self.job_handler_id.replace(Some(job_handler_id));
        }

        fn add_status_row_to_listbox(
            &self,
            listbox: &gtk::ListBox,
            icon_name: &str,
            status_text: &str,
        ) {
            let row = adw::PreferencesRow::new();
            let hbox = gtk::Box::builder()
                .orientation(gtk::Orientation::Horizontal)
                .spacing(12)
                .margin_top(12)
                .margin_bottom(12)
                .margin_start(12)
                .margin_end(12)
                .build();

            let icon = gtk::Image::new();
            icon.set_icon_name(Some(icon_name));
            hbox.append(&icon);

            let status_label = gtk::Label::builder()
                .xalign(0.0)
                .wrap(true)
                .hexpand(true)
                .build();
            status_label.set_label(status_text);
            hbox.append(&status_label);

            row.set_child(Some(&hbox));
            listbox.insert(&row, -1);
        }

        fn add_property_to_listbox(&self, listbox: &gtk::ListBox, title: &str, subtitle: &str) {
            if !subtitle.is_empty() {
                let row = adw::ActionRow::builder()
                    .css_classes(["property"])
                    .title(title)
                    .subtitle(subtitle)
                    .build();
                listbox.append(&row);
            }
        }

        fn add_property_to_expander_row(
            &self,
            expander_row: &adw::ExpanderRow,
            title: &str,
            subtitle: &str,
        ) {
            if !subtitle.is_empty() {
                let row = adw::ActionRow::builder()
                    .css_classes(["property"])
                    .title(title)
                    .subtitle(subtitle)
                    .build();
                expander_row.add_row(&row);
            }
        }
    }

    #[glib::object_subclass]
    impl ObjectSubclass for PpsPropertiesSignatures {
        const NAME: &'static str = "PpsPropertiesSignatures";
        type Type = super::PpsPropertiesSignatures;
        type ParentType = adw::Bin;

        fn class_init(klass: &mut Self::Class) {
            klass.bind_template();
        }

        fn instance_init(obj: &InitializingObject<Self>) {
            obj.init_template();
        }
    }

    impl ObjectImpl for PpsPropertiesSignatures {
        fn dispose(&self) {
            if let Some(job) = self.signatures_job.borrow().as_ref() {
                if let Some(id) = self.job_handler_id.take() {
                    job.disconnect(id);
                }

                job.cancel();
            }
        }
    }

    impl WidgetImpl for PpsPropertiesSignatures {}

    impl BinImpl for PpsPropertiesSignatures {}
}

glib::wrapper! {
    pub struct PpsPropertiesSignatures(ObjectSubclass<imp::PpsPropertiesSignatures>)
    @extends gtk::Widget, adw::Bin,
    @implements gtk::Accessible, gtk::Buildable, gtk::ConstraintTarget;
}

impl Default for PpsPropertiesSignatures {
    fn default() -> Self {
        Self::new()
    }
}

impl PpsPropertiesSignatures {
    fn new() -> PpsPropertiesSignatures {
        glib::Object::builder().build()
    }
}
