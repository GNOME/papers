// This file was generated by gir (https://github.com/gtk-rs/gir)
// from ../gir-files
// from ../pps-girs
// DO NOT EDIT

use crate::{ffi, CertificateInfo, Rectangle, SignatureStatus};
use glib::{prelude::*, translate::*};

glib::wrapper! {
    #[doc(alias = "PpsSignature")]
    pub struct Signature(Object<ffi::PpsSignature, ffi::PpsSignatureClass>);

    match fn {
        type_ => || ffi::pps_signature_get_type(),
    }
}

impl Signature {
    pub const NONE: Option<&'static Signature> = None;

    #[doc(alias = "pps_signature_new")]
    pub fn new(status: SignatureStatus, info: &impl IsA<CertificateInfo>) -> Signature {
        skip_assert_initialized!();
        unsafe {
            from_glib_full(ffi::pps_signature_new(
                status.into_glib(),
                info.as_ref().to_glib_none().0,
            ))
        }
    }
}

pub trait SignatureExt: IsA<Signature> + 'static {
    #[doc(alias = "pps_signature_get_background_color")]
    #[doc(alias = "get_background_color")]
    fn background_color(&self, color: &mut gdk::RGBA) {
        unsafe {
            ffi::pps_signature_get_background_color(
                self.as_ref().to_glib_none().0,
                color.to_glib_none_mut().0,
            );
        }
    }

    #[doc(alias = "pps_signature_get_border_color")]
    #[doc(alias = "get_border_color")]
    fn border_color(&self, color: &mut gdk::RGBA) {
        unsafe {
            ffi::pps_signature_get_border_color(
                self.as_ref().to_glib_none().0,
                color.to_glib_none_mut().0,
            );
        }
    }

    #[doc(alias = "pps_signature_get_border_width")]
    #[doc(alias = "get_border_width")]
    fn border_width(&self) -> i32 {
        unsafe { ffi::pps_signature_get_border_width(self.as_ref().to_glib_none().0) }
    }

    #[doc(alias = "pps_signature_get_destination_file")]
    #[doc(alias = "get_destination_file")]
    fn destination_file(&self) -> Option<glib::GString> {
        unsafe {
            from_glib_none(ffi::pps_signature_get_destination_file(
                self.as_ref().to_glib_none().0,
            ))
        }
    }

    #[doc(alias = "pps_signature_get_font_color")]
    #[doc(alias = "get_font_color")]
    fn font_color(&self, color: &mut gdk::RGBA) {
        unsafe {
            ffi::pps_signature_get_font_color(
                self.as_ref().to_glib_none().0,
                color.to_glib_none_mut().0,
            );
        }
    }

    #[doc(alias = "pps_signature_get_font_size")]
    #[doc(alias = "get_font_size")]
    fn font_size(&self) -> i32 {
        unsafe { ffi::pps_signature_get_font_size(self.as_ref().to_glib_none().0) }
    }

    #[doc(alias = "pps_signature_get_left_font_size")]
    #[doc(alias = "get_left_font_size")]
    fn left_font_size(&self) -> i32 {
        unsafe { ffi::pps_signature_get_left_font_size(self.as_ref().to_glib_none().0) }
    }

    #[doc(alias = "pps_signature_get_owner_password")]
    #[doc(alias = "get_owner_password")]
    fn owner_password(&self) -> Option<glib::GString> {
        unsafe {
            from_glib_none(ffi::pps_signature_get_owner_password(
                self.as_ref().to_glib_none().0,
            ))
        }
    }

    #[doc(alias = "pps_signature_get_page")]
    #[doc(alias = "get_page")]
    fn page(&self) -> i32 {
        unsafe { ffi::pps_signature_get_page(self.as_ref().to_glib_none().0) }
    }

    #[doc(alias = "pps_signature_get_password")]
    #[doc(alias = "get_password")]
    fn password(&self) -> Option<glib::GString> {
        unsafe {
            from_glib_none(ffi::pps_signature_get_password(
                self.as_ref().to_glib_none().0,
            ))
        }
    }

    #[doc(alias = "pps_signature_get_rect")]
    #[doc(alias = "get_rect")]
    fn rect(&self) -> Option<Rectangle> {
        unsafe { from_glib_full(ffi::pps_signature_get_rect(self.as_ref().to_glib_none().0)) }
    }

    #[doc(alias = "pps_signature_get_signature")]
    #[doc(alias = "get_signature")]
    fn signature(&self) -> Option<glib::GString> {
        unsafe {
            from_glib_none(ffi::pps_signature_get_signature(
                self.as_ref().to_glib_none().0,
            ))
        }
    }

    #[doc(alias = "pps_signature_get_signature_left")]
    #[doc(alias = "get_signature_left")]
    fn signature_left(&self) -> Option<glib::GString> {
        unsafe {
            from_glib_none(ffi::pps_signature_get_signature_left(
                self.as_ref().to_glib_none().0,
            ))
        }
    }

    #[doc(alias = "pps_signature_get_user_password")]
    #[doc(alias = "get_user_password")]
    fn user_password(&self) -> Option<glib::GString> {
        unsafe {
            from_glib_none(ffi::pps_signature_get_user_password(
                self.as_ref().to_glib_none().0,
            ))
        }
    }

    #[doc(alias = "pps_signature_is_valid")]
    fn is_valid(&self) -> bool {
        unsafe { from_glib(ffi::pps_signature_is_valid(self.as_ref().to_glib_none().0)) }
    }

    #[doc(alias = "pps_signature_set_background_color")]
    fn set_background_color(&self, color: &mut gdk::RGBA) {
        unsafe {
            ffi::pps_signature_set_background_color(
                self.as_ref().to_glib_none().0,
                color.to_glib_none_mut().0,
            );
        }
    }

    #[doc(alias = "pps_signature_set_border_color")]
    fn set_border_color(&self, color: &mut gdk::RGBA) {
        unsafe {
            ffi::pps_signature_set_border_color(
                self.as_ref().to_glib_none().0,
                color.to_glib_none_mut().0,
            );
        }
    }

    #[doc(alias = "pps_signature_set_border_width")]
    fn set_border_width(&self, width: i32) {
        unsafe {
            ffi::pps_signature_set_border_width(self.as_ref().to_glib_none().0, width);
        }
    }

    #[doc(alias = "pps_signature_set_destination_file")]
    fn set_destination_file(&self, file: &str) {
        unsafe {
            ffi::pps_signature_set_destination_file(
                self.as_ref().to_glib_none().0,
                file.to_glib_none().0,
            );
        }
    }

    #[doc(alias = "pps_signature_set_font_color")]
    fn set_font_color(&self, color: &mut gdk::RGBA) {
        unsafe {
            ffi::pps_signature_set_font_color(
                self.as_ref().to_glib_none().0,
                color.to_glib_none_mut().0,
            );
        }
    }

    #[doc(alias = "pps_signature_set_font_size")]
    fn set_font_size(&self, size: i32) {
        unsafe {
            ffi::pps_signature_set_font_size(self.as_ref().to_glib_none().0, size);
        }
    }

    #[doc(alias = "pps_signature_set_left_font_size")]
    fn set_left_font_size(&self, size: i32) {
        unsafe {
            ffi::pps_signature_set_left_font_size(self.as_ref().to_glib_none().0, size);
        }
    }

    #[doc(alias = "pps_signature_set_owner_password")]
    fn set_owner_password(&self, password: &str) {
        unsafe {
            ffi::pps_signature_set_owner_password(
                self.as_ref().to_glib_none().0,
                password.to_glib_none().0,
            );
        }
    }

    #[doc(alias = "pps_signature_set_page")]
    fn set_page(&self, page: u32) {
        unsafe {
            ffi::pps_signature_set_page(self.as_ref().to_glib_none().0, page);
        }
    }

    #[doc(alias = "pps_signature_set_password")]
    fn set_password(&self, password: &str) {
        unsafe {
            ffi::pps_signature_set_password(
                self.as_ref().to_glib_none().0,
                password.to_glib_none().0,
            );
        }
    }

    #[doc(alias = "pps_signature_set_rect")]
    fn set_rect(&self, rect: &Rectangle) {
        unsafe {
            ffi::pps_signature_set_rect(self.as_ref().to_glib_none().0, rect.to_glib_none().0);
        }
    }

    #[doc(alias = "pps_signature_set_signature")]
    fn set_signature(&self, signature: &str) {
        unsafe {
            ffi::pps_signature_set_signature(
                self.as_ref().to_glib_none().0,
                signature.to_glib_none().0,
            );
        }
    }

    #[doc(alias = "pps_signature_set_signature_left")]
    fn set_signature_left(&self, signature_left: &str) {
        unsafe {
            ffi::pps_signature_set_signature_left(
                self.as_ref().to_glib_none().0,
                signature_left.to_glib_none().0,
            );
        }
    }

    #[doc(alias = "pps_signature_set_user_password")]
    fn set_user_password(&self, password: &str) {
        unsafe {
            ffi::pps_signature_set_user_password(
                self.as_ref().to_glib_none().0,
                password.to_glib_none().0,
            );
        }
    }

    #[doc(alias = "certificate-info")]
    fn certificate_info(&self) -> Option<CertificateInfo> {
        ObjectExt::property(self.as_ref(), "certificate-info")
    }

    #[doc(alias = "signature-time")]
    fn signature_time(&self) -> Option<glib::DateTime> {
        ObjectExt::property(self.as_ref(), "signature-time")
    }

    fn status(&self) -> SignatureStatus {
        ObjectExt::property(self.as_ref(), "status")
    }
}

impl<O: IsA<Signature>> SignatureExt for O {}
