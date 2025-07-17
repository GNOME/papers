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
        #[template_child]
        status_group: TemplateChild<adw::PreferencesGroup>,
        #[template_child]
        details_group: TemplateChild<adw::PreferencesGroup>,
        #[template_child]
        status_listbox: TemplateChild<gtk::ListBox>,
        #[template_child]
        listbox: TemplateChild<gtk::ListBox>,
        #[template_child]
        signers_drop_down: TemplateChild<gtk::DropDown>,
        #[template_child]
        details_button: TemplateChild<gtk::ToggleButton>,

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

                    obj.listbox.set_filter_func(glib::clone!(
                        #[weak]
                        obj,
                        #[upgrade_or]
                        true,
                        move |row| {
                            if obj.signers_drop_down.is_visible() {
                                unsafe {
                                    if let Some(signature_index_ptr) =
                                        row.data::<usize>("signature-index")
                                    {
                                        let signature_index: usize = *signature_index_ptr.as_ptr();
                                        if signature_index
                                            != obj.signers_drop_down.selected().try_into().unwrap()
                                        {
                                            return false;
                                        }
                                    }
                                }
                            }

                            if obj.details_button.is_active() {
                                return true;
                            }

                            unsafe {
                                if let Some(key) = row.data::<glib::GString>("key") {
                                    let k = key.as_ref();

                                    if k.as_str() == "certificate-issuer-common-name"
                                        || k.as_str() == "certificate-issuer-email"
                                        || k.as_str() == "certificate-issuer-organization"
                                        || k.as_str() == "certificate-issuance-time"
                                        || k.as_str() == "certificate-expiration-time"
                                    {
                                        return false;
                                    }
                                }
                            }

                            return true;
                        }
                    ));

                    obj.status_listbox.set_filter_func(glib::clone!(
                        #[weak]
                        obj,
                        #[upgrade_or]
                        true,
                        move |row| {
                            if obj.signers_drop_down.is_visible() {
                                unsafe {
                                    if let Some(signature_index_ptr) =
                                        row.data::<usize>("signature-index")
                                    {
                                        let signature_index: usize = *signature_index_ptr.as_ptr();
                                        if signature_index
                                            != obj.signers_drop_down.selected().try_into().unwrap()
                                        {
                                            return false;
                                        }
                                    }
                                }
                            }

                            return true;
                        }
                    ));

                    if let Some(doc_signatures) =
                        job.document().and_dynamic_cast_ref::<DocumentSignatures>()
                    {
                        let signatures = doc_signatures.signatures();

                        if signatures.len() == 1 {
                            let signature = &signatures[0];

                            obj.signers_drop_down.set_visible(false);

                            let mut signer_text = glib::gformat!("");
                            if let Some(ref certificate_info) = signature.certificate_info() {
                                if let Some(ref signer_name) =
                                    certificate_info.subject_common_name()
                                {
                                    signer_text = glib::gformat!(
                                        "{} {}",
                                        gettext("Signed by"),
                                        signer_name.as_str()
                                    );
                                    if let Some(ref signer_email) = certificate_info.subject_email()
                                    {
                                        if !signer_email.is_empty() {
                                            signer_text = glib::gformat!(
                                                "{} {} <{}>",
                                                gettext("Signed by"),
                                                signer_name,
                                                signer_email
                                            );
                                        }
                                    }
                                }
                            }
                            obj.status_group
                                .set_title(glib::markup_escape_text(signer_text.as_str()).as_str());
                        } else {
                            let signers = gtk::StringList::new(&[]);

                            for signature in signatures.iter() {
                                let mut signer_text = glib::gformat!("");
                                if let Some(ref certificate_info) = signature.certificate_info() {
                                    if let Some(ref signer_name) =
                                        certificate_info.subject_common_name()
                                    {
                                        signer_text = glib::gformat!("{}", signer_name.as_str());
                                        if let Some(ref signer_email) =
                                            certificate_info.subject_email()
                                        {
                                            if !signer_email.is_empty() {
                                                signer_text = glib::gformat!(
                                                    "{} <{}>",
                                                    signer_name,
                                                    signer_email
                                                );
                                            }
                                        }
                                    }
                                }
                                signers.append(signer_text.as_str());
                            }
                            obj.signers_drop_down.set_model(Some(&signers));
                        }

                        for (signature_index, signature) in signatures.iter().enumerate() {
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

                            obj.add_status_row(signature_index, icon_name, &text);

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

                                obj.add_status_row(signature_index, icon_name, &text);

                                if let Some(ref signer_organization) =
                                    certificate_info.subject_organization()
                                {
                                    obj.add_row(
                                        signature_index,
                                        "organization",
                                        &gettext("Organization"),
                                        signer_organization,
                                    );
                                }

                                if let Some(signature_time) = signature.signature_time() {
                                    obj.add_row(
                                        signature_index,
                                        "signature-time",
                                        &gettext("Date and Time"),
                                        &Document::misc_format_datetime(&signature_time).unwrap(),
                                    );
                                }

                                if let Some(ref certificate_issuer_common_name) =
                                    certificate_info.issuer_common_name()
                                {
                                    obj.add_row(
                                        signature_index,
                                        "certificate-issuer-common-name",
                                        &gettext("Certificate Issuer"),
                                        certificate_issuer_common_name,
                                    );
                                }

                                if let Some(ref certificate_issuer_email) =
                                    certificate_info.issuer_email()
                                {
                                    obj.add_row(
                                        signature_index,
                                        "certificate-issuer-email",
                                        &gettext("Certificate Issuer's Email"),
                                        certificate_issuer_email,
                                    );
                                }

                                if let Some(ref certificate_issuer_organization) =
                                    certificate_info.issuer_organization()
                                {
                                    obj.add_row(
                                        signature_index,
                                        "certificate-issuer-organization",
                                        &gettext("Certificate Issuer's Organization"),
                                        certificate_issuer_organization,
                                    );
                                }

                                if let Some(certificate_issuance_time) =
                                    certificate_info.issuance_time()
                                {
                                    obj.add_row(
                                        signature_index,
                                        "certificate-issuance-time",
                                        &gettext("Certificate's Issuance Time"),
                                        &Document::misc_format_datetime(&certificate_issuance_time)
                                            .unwrap(),
                                    );
                                }

                                if let Some(certificate_expiration_time) =
                                    certificate_info.expiration_time()
                                {
                                    obj.add_row(
                                        signature_index,
                                        "certificate-expiration-time",
                                        &gettext("Certificate's Expiration Time"),
                                        &Document::misc_format_datetime(
                                            &certificate_expiration_time,
                                        )
                                        .unwrap(),
                                    );
                                }
                            }
                        }
                    }
                }
            ));

            job.scheduler_push_job(JobPriority::PriorityNone);

            self.document.replace(Some(document));
            self.signatures_job.replace(Some(job));
            self.job_handler_id.replace(Some(job_handler_id));
        }

        fn add_row(
            &self,
            signature_index: usize,
            key: &str,
            display_key: &str,
            display_value: &str,
        ) {
            if !display_value.is_empty() {
                let row = adw::ActionRow::builder().css_classes(["property"]).build();

                unsafe {
                    row.set_data("key", glib::GString::from(key));
                    row.set_data("signature-index", signature_index);
                }

                row.set_title(display_key);
                row.set_subtitle(display_value);

                self.listbox.insert(&row, -1);
            }
        }

        fn add_status_row(&self, signature_index: usize, icon_name: &str, status_text: &str) {
            let row = adw::PreferencesRow::new();
            let hbox = gtk::Box::builder()
                .orientation(gtk::Orientation::Horizontal)
                .spacing(12)
                .margin_top(12)
                .margin_bottom(12)
                .margin_start(12)
                .margin_end(12)
                .build();

            unsafe {
                row.set_data("signature-index", signature_index);
            }

            let icon = gtk::Image::new();
            icon.set_icon_name(Some(icon_name));
            hbox.append(&icon);

            let status_label = gtk::Label::builder()
                .xalign(0.0)
                .ellipsize(gtk::pango::EllipsizeMode::End)
                .hexpand(true)
                .build();
            status_label.set_label(status_text);
            hbox.append(&status_label);

            row.set_child(Some(&hbox));
            self.status_listbox.insert(&row, -1);
        }
    }

    #[gtk::template_callbacks]
    impl PpsPropertiesSignatures {
        #[template_callback]
        fn details_button_toggled(&self) {
            self.listbox.invalidate_filter();
            self.status_listbox.invalidate_filter();

            if self.details_button.is_active() {
                self.details_button.set_label(&gettext("Hide Details…"));
            } else {
                self.details_button.set_label(&gettext("View Details…"));
            }
        }

        #[template_callback]
        fn signer_changed(&self) {
            self.listbox.invalidate_filter();
            self.status_listbox.invalidate_filter();
        }
    }

    #[glib::object_subclass]
    impl ObjectSubclass for PpsPropertiesSignatures {
        const NAME: &'static str = "PpsPropertiesSignatures";
        type Type = super::PpsPropertiesSignatures;
        type ParentType = adw::Bin;

        fn class_init(klass: &mut Self::Class) {
            klass.bind_template();
            klass.bind_template_callbacks();
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
