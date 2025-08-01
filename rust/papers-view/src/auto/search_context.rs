// This file was generated by gir (https://github.com/gtk-rs/gir)
// from ../gir-files
// from ../pps-girs
// DO NOT EDIT

use crate::{ffi, DocumentModel, SearchResult};
use glib::{
    object::ObjectType as _,
    prelude::*,
    signal::{connect_raw, SignalHandlerId},
    translate::*,
};
use std::boxed::Box as Box_;

glib::wrapper! {
    #[doc(alias = "PpsSearchContext")]
    pub struct SearchContext(Object<ffi::PpsSearchContext, ffi::PpsSearchContextClass>);

    match fn {
        type_ => || ffi::pps_search_context_get_type(),
    }
}

impl SearchContext {
    pub const NONE: Option<&'static SearchContext> = None;

    #[doc(alias = "pps_search_context_new")]
    pub fn new(model: &DocumentModel) -> SearchContext {
        skip_assert_initialized!();
        unsafe { from_glib_full(ffi::pps_search_context_new(model.to_glib_none().0)) }
    }
}

pub trait SearchContextExt: IsA<SearchContext> + 'static {
    #[cfg(feature = "v48")]
    #[cfg_attr(docsrs, doc(cfg(feature = "v48")))]
    #[doc(alias = "pps_search_context_activate")]
    fn activate(&self) {
        unsafe {
            ffi::pps_search_context_activate(self.as_ref().to_glib_none().0);
        }
    }

    #[cfg(feature = "v48")]
    #[cfg_attr(docsrs, doc(cfg(feature = "v48")))]
    #[doc(alias = "pps_search_context_autoselect_result")]
    fn autoselect_result(&self, result: &impl IsA<SearchResult>) {
        unsafe {
            ffi::pps_search_context_autoselect_result(
                self.as_ref().to_glib_none().0,
                result.as_ref().to_glib_none().0,
            );
        }
    }

    #[cfg(feature = "v48")]
    #[cfg_attr(docsrs, doc(cfg(feature = "v48")))]
    #[doc(alias = "pps_search_context_get_active")]
    #[doc(alias = "get_active")]
    #[doc(alias = "active")]
    fn is_active(&self) -> bool {
        unsafe {
            from_glib(ffi::pps_search_context_get_active(
                self.as_ref().to_glib_none().0,
            ))
        }
    }

    #[doc(alias = "pps_search_context_get_options")]
    #[doc(alias = "get_options")]
    fn options(&self) -> papers_document::FindOptions {
        unsafe {
            from_glib(ffi::pps_search_context_get_options(
                self.as_ref().to_glib_none().0,
            ))
        }
    }

    #[doc(alias = "pps_search_context_get_result_model")]
    #[doc(alias = "get_result_model")]
    fn result_model(&self) -> Option<gtk::SingleSelection> {
        unsafe {
            from_glib_none(ffi::pps_search_context_get_result_model(
                self.as_ref().to_glib_none().0,
            ))
        }
    }

    #[cfg(feature = "v48")]
    #[cfg_attr(docsrs, doc(cfg(feature = "v48")))]
    #[doc(alias = "pps_search_context_get_results_on_page")]
    #[doc(alias = "get_results_on_page")]
    fn results_on_page(&self, page: u32) -> Vec<SearchResult> {
        unsafe {
            FromGlibPtrContainer::from_glib_full(ffi::pps_search_context_get_results_on_page(
                self.as_ref().to_glib_none().0,
                page,
            ))
        }
    }

    #[doc(alias = "pps_search_context_get_search_term")]
    #[doc(alias = "get_search_term")]
    #[doc(alias = "search-term")]
    fn search_term(&self) -> Option<glib::GString> {
        unsafe {
            from_glib_none(ffi::pps_search_context_get_search_term(
                self.as_ref().to_glib_none().0,
            ))
        }
    }

    #[doc(alias = "pps_search_context_has_results_on_page")]
    fn has_results_on_page(&self, page: u32) -> bool {
        unsafe {
            from_glib(ffi::pps_search_context_has_results_on_page(
                self.as_ref().to_glib_none().0,
                page,
            ))
        }
    }

    #[cfg(feature = "v48")]
    #[cfg_attr(docsrs, doc(cfg(feature = "v48")))]
    #[doc(alias = "pps_search_context_release")]
    fn release(&self) {
        unsafe {
            ffi::pps_search_context_release(self.as_ref().to_glib_none().0);
        }
    }

    #[doc(alias = "pps_search_context_restart")]
    fn restart(&self) {
        unsafe {
            ffi::pps_search_context_restart(self.as_ref().to_glib_none().0);
        }
    }

    #[doc(alias = "pps_search_context_set_options")]
    fn set_options(&self, options: papers_document::FindOptions) {
        unsafe {
            ffi::pps_search_context_set_options(
                self.as_ref().to_glib_none().0,
                options.into_glib(),
            );
        }
    }

    #[doc(alias = "pps_search_context_set_search_term")]
    #[doc(alias = "search-term")]
    fn set_search_term(&self, search_term: &str) {
        unsafe {
            ffi::pps_search_context_set_search_term(
                self.as_ref().to_glib_none().0,
                search_term.to_glib_none().0,
            );
        }
    }

    #[cfg(not(feature = "v48"))]
    #[cfg_attr(docsrs, doc(cfg(not(feature = "v48"))))]
    fn is_active(&self) -> bool {
        ObjectExt::property(self.as_ref(), "active")
    }

    #[doc(alias = "cleared")]
    fn connect_cleared<F: Fn(&Self) + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn cleared_trampoline<P: IsA<SearchContext>, F: Fn(&P) + 'static>(
            this: *mut ffi::PpsSearchContext,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(SearchContext::from_glib_borrow(this).unsafe_cast_ref())
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                c"cleared".as_ptr() as *const _,
                Some(std::mem::transmute::<*const (), unsafe extern "C" fn()>(
                    cleared_trampoline::<Self, F> as *const (),
                )),
                Box_::into_raw(f),
            )
        }
    }

    #[doc(alias = "finished")]
    fn connect_finished<F: Fn(&Self, i32) + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn finished_trampoline<
            P: IsA<SearchContext>,
            F: Fn(&P, i32) + 'static,
        >(
            this: *mut ffi::PpsSearchContext,
            object: std::ffi::c_int,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(
                SearchContext::from_glib_borrow(this).unsafe_cast_ref(),
                object,
            )
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                c"finished".as_ptr() as *const _,
                Some(std::mem::transmute::<*const (), unsafe extern "C" fn()>(
                    finished_trampoline::<Self, F> as *const (),
                )),
                Box_::into_raw(f),
            )
        }
    }

    #[doc(alias = "result-activated")]
    fn connect_result_activated<F: Fn(&Self, &SearchResult) + 'static>(
        &self,
        f: F,
    ) -> SignalHandlerId {
        unsafe extern "C" fn result_activated_trampoline<
            P: IsA<SearchContext>,
            F: Fn(&P, &SearchResult) + 'static,
        >(
            this: *mut ffi::PpsSearchContext,
            object: *mut ffi::PpsSearchResult,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(
                SearchContext::from_glib_borrow(this).unsafe_cast_ref(),
                &from_glib_borrow(object),
            )
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                c"result-activated".as_ptr() as *const _,
                Some(std::mem::transmute::<*const (), unsafe extern "C" fn()>(
                    result_activated_trampoline::<Self, F> as *const (),
                )),
                Box_::into_raw(f),
            )
        }
    }

    #[doc(alias = "started")]
    fn connect_started<F: Fn(&Self) + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn started_trampoline<P: IsA<SearchContext>, F: Fn(&P) + 'static>(
            this: *mut ffi::PpsSearchContext,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(SearchContext::from_glib_borrow(this).unsafe_cast_ref())
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                c"started".as_ptr() as *const _,
                Some(std::mem::transmute::<*const (), unsafe extern "C" fn()>(
                    started_trampoline::<Self, F> as *const (),
                )),
                Box_::into_raw(f),
            )
        }
    }

    #[doc(alias = "active")]
    fn connect_active_notify<F: Fn(&Self) + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn notify_active_trampoline<
            P: IsA<SearchContext>,
            F: Fn(&P) + 'static,
        >(
            this: *mut ffi::PpsSearchContext,
            _param_spec: glib::ffi::gpointer,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(SearchContext::from_glib_borrow(this).unsafe_cast_ref())
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                c"notify::active".as_ptr() as *const _,
                Some(std::mem::transmute::<*const (), unsafe extern "C" fn()>(
                    notify_active_trampoline::<Self, F> as *const (),
                )),
                Box_::into_raw(f),
            )
        }
    }

    #[doc(alias = "search-term")]
    fn connect_search_term_notify<F: Fn(&Self) + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn notify_search_term_trampoline<
            P: IsA<SearchContext>,
            F: Fn(&P) + 'static,
        >(
            this: *mut ffi::PpsSearchContext,
            _param_spec: glib::ffi::gpointer,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(SearchContext::from_glib_borrow(this).unsafe_cast_ref())
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                c"notify::search-term".as_ptr() as *const _,
                Some(std::mem::transmute::<*const (), unsafe extern "C" fn()>(
                    notify_search_term_trampoline::<Self, F> as *const (),
                )),
                Box_::into_raw(f),
            )
        }
    }
}

impl<O: IsA<SearchContext>> SearchContextExt for O {}
