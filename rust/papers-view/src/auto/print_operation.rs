// This file was generated by gir (https://github.com/gtk-rs/gir)
// from ../gir-files
// from ../pps-girs
// DO NOT EDIT

use crate::ffi;
use glib::{
    object::ObjectType as _,
    prelude::*,
    signal::{connect_raw, SignalHandlerId},
    translate::*,
};
use std::boxed::Box as Box_;

glib::wrapper! {
    #[doc(alias = "PpsPrintOperation")]
    pub struct PrintOperation(Object<ffi::PpsPrintOperation, ffi::PpsPrintOperationClass>);

    match fn {
        type_ => || ffi::pps_print_operation_get_type(),
    }
}

impl PrintOperation {
    #[doc(alias = "pps_print_operation_new")]
    pub fn new(document: &impl IsA<papers_document::Document>) -> Option<PrintOperation> {
        assert_initialized_main_thread!();
        unsafe {
            from_glib_full(ffi::pps_print_operation_new(
                document.as_ref().to_glib_none().0,
            ))
        }
    }

    #[doc(alias = "pps_print_operation_cancel")]
    pub fn cancel(&self) {
        unsafe {
            ffi::pps_print_operation_cancel(self.to_glib_none().0);
        }
    }

    #[doc(alias = "pps_print_operation_get_default_page_setup")]
    #[doc(alias = "get_default_page_setup")]
    pub fn default_page_setup(&self) -> Option<gtk::PageSetup> {
        unsafe {
            from_glib_none(ffi::pps_print_operation_get_default_page_setup(
                self.to_glib_none().0,
            ))
        }
    }

    #[doc(alias = "pps_print_operation_get_embed_page_setup")]
    #[doc(alias = "get_embed_page_setup")]
    pub fn embeds_page_setup(&self) -> bool {
        unsafe {
            from_glib(ffi::pps_print_operation_get_embed_page_setup(
                self.to_glib_none().0,
            ))
        }
    }

    #[doc(alias = "pps_print_operation_get_error")]
    #[doc(alias = "get_error")]
    pub fn error(&self) -> Result<(), glib::Error> {
        unsafe {
            let mut error = std::ptr::null_mut();
            let _ = ffi::pps_print_operation_get_error(self.to_glib_none().0, &mut error);
            if error.is_null() {
                Ok(())
            } else {
                Err(from_glib_full(error))
            }
        }
    }

    #[doc(alias = "pps_print_operation_get_job_name")]
    #[doc(alias = "get_job_name")]
    pub fn job_name(&self) -> Option<glib::GString> {
        unsafe { from_glib_none(ffi::pps_print_operation_get_job_name(self.to_glib_none().0)) }
    }

    #[doc(alias = "pps_print_operation_get_print_settings")]
    #[doc(alias = "get_print_settings")]
    pub fn print_settings(&self) -> Option<gtk::PrintSettings> {
        unsafe {
            from_glib_none(ffi::pps_print_operation_get_print_settings(
                self.to_glib_none().0,
            ))
        }
    }

    #[doc(alias = "pps_print_operation_get_progress")]
    #[doc(alias = "get_progress")]
    pub fn progress(&self) -> f64 {
        unsafe { ffi::pps_print_operation_get_progress(self.to_glib_none().0) }
    }

    #[doc(alias = "pps_print_operation_get_status")]
    #[doc(alias = "get_status")]
    pub fn status(&self) -> Option<glib::GString> {
        unsafe { from_glib_none(ffi::pps_print_operation_get_status(self.to_glib_none().0)) }
    }

    #[doc(alias = "pps_print_operation_run")]
    pub fn run(&self, parent: &impl IsA<gtk::Window>) {
        unsafe {
            ffi::pps_print_operation_run(self.to_glib_none().0, parent.as_ref().to_glib_none().0);
        }
    }

    #[doc(alias = "pps_print_operation_set_current_page")]
    pub fn set_current_page(&self, current_page: i32) {
        unsafe {
            ffi::pps_print_operation_set_current_page(self.to_glib_none().0, current_page);
        }
    }

    #[doc(alias = "pps_print_operation_set_default_page_setup")]
    pub fn set_default_page_setup(&self, page_setup: &gtk::PageSetup) {
        unsafe {
            ffi::pps_print_operation_set_default_page_setup(
                self.to_glib_none().0,
                page_setup.to_glib_none().0,
            );
        }
    }

    #[doc(alias = "pps_print_operation_set_embed_page_setup")]
    pub fn set_embed_page_setup(&self, embed: bool) {
        unsafe {
            ffi::pps_print_operation_set_embed_page_setup(self.to_glib_none().0, embed.into_glib());
        }
    }

    #[doc(alias = "pps_print_operation_set_job_name")]
    pub fn set_job_name(&self, job_name: &str) {
        unsafe {
            ffi::pps_print_operation_set_job_name(self.to_glib_none().0, job_name.to_glib_none().0);
        }
    }

    #[doc(alias = "pps_print_operation_set_print_settings")]
    pub fn set_print_settings(&self, print_settings: &gtk::PrintSettings) {
        unsafe {
            ffi::pps_print_operation_set_print_settings(
                self.to_glib_none().0,
                print_settings.to_glib_none().0,
            );
        }
    }

    #[doc(alias = "pps_print_operation_exists_for_document")]
    pub fn exists_for_document(document: &impl IsA<papers_document::Document>) -> bool {
        assert_initialized_main_thread!();
        unsafe {
            from_glib(ffi::pps_print_operation_exists_for_document(
                document.as_ref().to_glib_none().0,
            ))
        }
    }

    #[doc(alias = "begin-print")]
    pub fn connect_begin_print<F: Fn(&Self) + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn begin_print_trampoline<F: Fn(&PrintOperation) + 'static>(
            this: *mut ffi::PpsPrintOperation,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(&from_glib_borrow(this))
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                c"begin-print".as_ptr() as *const _,
                Some(std::mem::transmute::<*const (), unsafe extern "C" fn()>(
                    begin_print_trampoline::<F> as *const (),
                )),
                Box_::into_raw(f),
            )
        }
    }

    #[doc(alias = "done")]
    pub fn connect_done<F: Fn(&Self, gtk::PrintOperationResult) + 'static>(
        &self,
        f: F,
    ) -> SignalHandlerId {
        unsafe extern "C" fn done_trampoline<
            F: Fn(&PrintOperation, gtk::PrintOperationResult) + 'static,
        >(
            this: *mut ffi::PpsPrintOperation,
            object: gtk::ffi::GtkPrintOperationResult,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(&from_glib_borrow(this), from_glib(object))
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                c"done".as_ptr() as *const _,
                Some(std::mem::transmute::<*const (), unsafe extern "C" fn()>(
                    done_trampoline::<F> as *const (),
                )),
                Box_::into_raw(f),
            )
        }
    }

    #[doc(alias = "status-changed")]
    pub fn connect_status_changed<F: Fn(&Self) + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn status_changed_trampoline<F: Fn(&PrintOperation) + 'static>(
            this: *mut ffi::PpsPrintOperation,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(&from_glib_borrow(this))
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                c"status-changed".as_ptr() as *const _,
                Some(std::mem::transmute::<*const (), unsafe extern "C" fn()>(
                    status_changed_trampoline::<F> as *const (),
                )),
                Box_::into_raw(f),
            )
        }
    }
}
