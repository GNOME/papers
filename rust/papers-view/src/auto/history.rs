// This file was generated by gir (https://github.com/gtk-rs/gir)
// from ../gir-files
// from ../pps-girs
// DO NOT EDIT

use crate::{ffi, DocumentModel};
use glib::{
    object::ObjectType as _,
    prelude::*,
    signal::{connect_raw, SignalHandlerId},
    translate::*,
};
use std::boxed::Box as Box_;

glib::wrapper! {
    #[doc(alias = "PpsHistory")]
    pub struct History(Object<ffi::PpsHistory, ffi::PpsHistoryClass>);

    match fn {
        type_ => || ffi::pps_history_get_type(),
    }
}

impl History {
    pub const NONE: Option<&'static History> = None;

    #[doc(alias = "pps_history_new")]
    pub fn new(model: &DocumentModel) -> History {
        skip_assert_initialized!();
        unsafe { from_glib_full(ffi::pps_history_new(model.to_glib_none().0)) }
    }
}

pub trait HistoryExt: IsA<History> + 'static {
    #[doc(alias = "pps_history_add_link")]
    fn add_link(&self, link: &papers_document::Link) {
        unsafe {
            ffi::pps_history_add_link(self.as_ref().to_glib_none().0, link.to_glib_none().0);
        }
    }

    #[doc(alias = "pps_history_add_page")]
    fn add_page(&self, page: i32) {
        unsafe {
            ffi::pps_history_add_page(self.as_ref().to_glib_none().0, page);
        }
    }

    #[doc(alias = "pps_history_can_go_back")]
    fn can_go_back(&self) -> bool {
        unsafe { from_glib(ffi::pps_history_can_go_back(self.as_ref().to_glib_none().0)) }
    }

    #[doc(alias = "pps_history_can_go_forward")]
    fn can_go_forward(&self) -> bool {
        unsafe {
            from_glib(ffi::pps_history_can_go_forward(
                self.as_ref().to_glib_none().0,
            ))
        }
    }

    #[doc(alias = "pps_history_freeze")]
    fn freeze(&self) {
        unsafe {
            ffi::pps_history_freeze(self.as_ref().to_glib_none().0);
        }
    }

    #[doc(alias = "pps_history_get_back_list")]
    #[doc(alias = "get_back_list")]
    fn back_list(&self) -> Vec<papers_document::Link> {
        unsafe {
            FromGlibPtrContainer::from_glib_container(ffi::pps_history_get_back_list(
                self.as_ref().to_glib_none().0,
            ))
        }
    }

    #[doc(alias = "pps_history_get_forward_list")]
    #[doc(alias = "get_forward_list")]
    fn forward_list(&self) -> Vec<papers_document::Link> {
        unsafe {
            FromGlibPtrContainer::from_glib_container(ffi::pps_history_get_forward_list(
                self.as_ref().to_glib_none().0,
            ))
        }
    }

    #[doc(alias = "pps_history_go_back")]
    fn go_back(&self) {
        unsafe {
            ffi::pps_history_go_back(self.as_ref().to_glib_none().0);
        }
    }

    #[doc(alias = "pps_history_go_forward")]
    fn go_forward(&self) {
        unsafe {
            ffi::pps_history_go_forward(self.as_ref().to_glib_none().0);
        }
    }

    #[doc(alias = "pps_history_go_to_link")]
    fn go_to_link(&self, link: &papers_document::Link) -> bool {
        unsafe {
            from_glib(ffi::pps_history_go_to_link(
                self.as_ref().to_glib_none().0,
                link.to_glib_none().0,
            ))
        }
    }

    #[doc(alias = "pps_history_is_frozen")]
    fn is_frozen(&self) -> bool {
        unsafe { from_glib(ffi::pps_history_is_frozen(self.as_ref().to_glib_none().0)) }
    }

    #[doc(alias = "pps_history_thaw")]
    fn thaw(&self) {
        unsafe {
            ffi::pps_history_thaw(self.as_ref().to_glib_none().0);
        }
    }

    #[doc(alias = "activate-link")]
    fn connect_activate_link<F: Fn(&Self, &glib::Object) + 'static>(
        &self,
        f: F,
    ) -> SignalHandlerId {
        unsafe extern "C" fn activate_link_trampoline<
            P: IsA<History>,
            F: Fn(&P, &glib::Object) + 'static,
        >(
            this: *mut ffi::PpsHistory,
            object: *mut glib::gobject_ffi::GObject,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(
                History::from_glib_borrow(this).unsafe_cast_ref(),
                &from_glib_borrow(object),
            )
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                c"activate-link".as_ptr() as *const _,
                Some(std::mem::transmute::<*const (), unsafe extern "C" fn()>(
                    activate_link_trampoline::<Self, F> as *const (),
                )),
                Box_::into_raw(f),
            )
        }
    }

    fn emit_activate_link(&self, object: &glib::Object) {
        self.emit_by_name::<()>("activate-link", &[&object]);
    }

    #[doc(alias = "changed")]
    fn connect_changed<F: Fn(&Self) + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn changed_trampoline<P: IsA<History>, F: Fn(&P) + 'static>(
            this: *mut ffi::PpsHistory,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(History::from_glib_borrow(this).unsafe_cast_ref())
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                c"changed".as_ptr() as *const _,
                Some(std::mem::transmute::<*const (), unsafe extern "C" fn()>(
                    changed_trampoline::<Self, F> as *const (),
                )),
                Box_::into_raw(f),
            )
        }
    }

    fn emit_changed(&self) {
        self.emit_by_name::<()>("changed", &[]);
    }
}

impl<O: IsA<History>> HistoryExt for O {}
