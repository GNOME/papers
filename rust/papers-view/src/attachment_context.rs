use crate::ffi;
use crate::AttachmentContext;
use glib::{prelude::*, translate::*};
use std::boxed::Box as Box_;
use std::pin::Pin;

impl AttachmentContext {
    #[doc(alias = "pps_attachment_context_save_attachments_async")]
    pub fn save_attachments_async<P: FnOnce(Result<(), glib::Error>) + 'static>(
        &self,
        attachments: impl IsA<gio::ListModel>,
        parent: Option<&impl IsA<gtk::Window>>,
        cancellable: Option<&impl IsA<gio::Cancellable>>,
        callback: P,
    ) {
        let main_context = glib::MainContext::ref_thread_default();
        let is_main_context_owner = main_context.is_owner();
        let has_acquired_main_context = (!is_main_context_owner)
            .then(|| main_context.acquire().ok())
            .flatten();
        assert!(
            is_main_context_owner || has_acquired_main_context.is_some(),
            "Async operations only allowed if the thread is owning the MainContext"
        );

        let user_data: Box_<glib::thread_guard::ThreadGuard<P>> =
            Box_::new(glib::thread_guard::ThreadGuard::new(callback));
        unsafe extern "C" fn save_attachments_async_trampoline<
            P: FnOnce(Result<(), glib::Error>) + 'static,
        >(
            _source_object: *mut glib::gobject_ffi::GObject,
            res: *mut gio::ffi::GAsyncResult,
            user_data: glib::ffi::gpointer,
        ) {
            let mut error = std::ptr::null_mut();
            let _ = ffi::pps_attachment_context_save_attachments_finish(
                _source_object as *mut _,
                res,
                &mut error,
            );
            let result = if error.is_null() {
                Ok(())
            } else {
                Err(from_glib_full(error))
            };
            let callback: Box_<glib::thread_guard::ThreadGuard<P>> =
                Box_::from_raw(user_data as *mut _);
            let callback: P = callback.into_inner();
            callback(result);
        }
        let callback = save_attachments_async_trampoline::<P>;
        unsafe {
            ffi::pps_attachment_context_save_attachments_async(
                self.to_glib_none().0,
                attachments.upcast().into_glib_ptr(),
                parent.map(|p| p.as_ref()).to_glib_none().0,
                cancellable.map(|p| p.as_ref()).to_glib_none().0,
                Some(callback),
                Box_::into_raw(user_data) as *mut _,
            );
        }
    }

    pub fn save_attachments_future(
        &self,
        attachments: impl IsA<gio::ListModel> + Clone + 'static,
        parent: Option<&(impl IsA<gtk::Window> + Clone + 'static)>,
    ) -> Pin<Box_<dyn std::future::Future<Output = Result<(), glib::Error>> + 'static>> {
        let attachments = attachments.clone();
        let parent = parent.map(ToOwned::to_owned);
        Box_::pin(gio::GioFuture::new(self, move |obj, cancellable, send| {
            obj.save_attachments_async(
                attachments,
                parent.as_ref().map(::std::borrow::Borrow::borrow),
                Some(cancellable),
                move |res| {
                    send.resolve(res);
                },
            );
        }))
    }
}
