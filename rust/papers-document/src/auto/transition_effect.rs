// This file was generated by gir (https://github.com/gtk-rs/gir)
// from ../gir-files
// from ../pps-girs
// DO NOT EDIT

use crate::{ffi, TransitionEffectAlignment, TransitionEffectDirection, TransitionEffectType};
use glib::{
    prelude::*,
    signal::{connect_raw, SignalHandlerId},
    translate::*,
};
use std::boxed::Box as Box_;

glib::wrapper! {
    #[doc(alias = "PpsTransitionEffect")]
    pub struct TransitionEffect(Object<ffi::PpsTransitionEffect, ffi::PpsTransitionEffectClass>);

    match fn {
        type_ => || ffi::pps_transition_effect_get_type(),
    }
}

impl TransitionEffect {
    pub const NONE: Option<&'static TransitionEffect> = None;

    //#[doc(alias = "pps_transition_effect_new")]
    //pub fn new(type_: TransitionEffectType, first_property_name: &str, : /*Unknown conversion*//*Unimplemented*/Basic: VarArgs) -> TransitionEffect {
    //    unsafe { TODO: call ffi:pps_transition_effect_new() }
    //}
}

pub trait TransitionEffectExt: IsA<TransitionEffect> + 'static {
    fn alignment(&self) -> TransitionEffectAlignment {
        ObjectExt::property(self.as_ref(), "alignment")
    }

    fn set_alignment(&self, alignment: TransitionEffectAlignment) {
        ObjectExt::set_property(self.as_ref(), "alignment", alignment)
    }

    fn angle(&self) -> i32 {
        ObjectExt::property(self.as_ref(), "angle")
    }

    fn set_angle(&self, angle: i32) {
        ObjectExt::set_property(self.as_ref(), "angle", angle)
    }

    fn direction(&self) -> TransitionEffectDirection {
        ObjectExt::property(self.as_ref(), "direction")
    }

    fn set_direction(&self, direction: TransitionEffectDirection) {
        ObjectExt::set_property(self.as_ref(), "direction", direction)
    }

    fn duration(&self) -> i32 {
        ObjectExt::property(self.as_ref(), "duration")
    }

    fn set_duration(&self, duration: i32) {
        ObjectExt::set_property(self.as_ref(), "duration", duration)
    }

    #[doc(alias = "duration-real")]
    fn duration_real(&self) -> f64 {
        ObjectExt::property(self.as_ref(), "duration-real")
    }

    #[doc(alias = "duration-real")]
    fn set_duration_real(&self, duration_real: f64) {
        ObjectExt::set_property(self.as_ref(), "duration-real", duration_real)
    }

    fn is_rectangular(&self) -> bool {
        ObjectExt::property(self.as_ref(), "rectangular")
    }

    fn set_rectangular(&self, rectangular: bool) {
        ObjectExt::set_property(self.as_ref(), "rectangular", rectangular)
    }

    fn scale(&self) -> f64 {
        ObjectExt::property(self.as_ref(), "scale")
    }

    fn set_scale(&self, scale: f64) {
        ObjectExt::set_property(self.as_ref(), "scale", scale)
    }

    #[doc(alias = "type")]
    fn type_(&self) -> TransitionEffectType {
        ObjectExt::property(self.as_ref(), "type")
    }

    #[doc(alias = "type")]
    fn set_type(&self, type_: TransitionEffectType) {
        ObjectExt::set_property(self.as_ref(), "type", type_)
    }

    #[doc(alias = "alignment")]
    fn connect_alignment_notify<F: Fn(&Self) + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn notify_alignment_trampoline<
            P: IsA<TransitionEffect>,
            F: Fn(&P) + 'static,
        >(
            this: *mut ffi::PpsTransitionEffect,
            _param_spec: glib::ffi::gpointer,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(TransitionEffect::from_glib_borrow(this).unsafe_cast_ref())
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                c"notify::alignment".as_ptr() as *const _,
                Some(std::mem::transmute::<*const (), unsafe extern "C" fn()>(
                    notify_alignment_trampoline::<Self, F> as *const (),
                )),
                Box_::into_raw(f),
            )
        }
    }

    #[doc(alias = "angle")]
    fn connect_angle_notify<F: Fn(&Self) + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn notify_angle_trampoline<
            P: IsA<TransitionEffect>,
            F: Fn(&P) + 'static,
        >(
            this: *mut ffi::PpsTransitionEffect,
            _param_spec: glib::ffi::gpointer,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(TransitionEffect::from_glib_borrow(this).unsafe_cast_ref())
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                c"notify::angle".as_ptr() as *const _,
                Some(std::mem::transmute::<*const (), unsafe extern "C" fn()>(
                    notify_angle_trampoline::<Self, F> as *const (),
                )),
                Box_::into_raw(f),
            )
        }
    }

    #[doc(alias = "direction")]
    fn connect_direction_notify<F: Fn(&Self) + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn notify_direction_trampoline<
            P: IsA<TransitionEffect>,
            F: Fn(&P) + 'static,
        >(
            this: *mut ffi::PpsTransitionEffect,
            _param_spec: glib::ffi::gpointer,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(TransitionEffect::from_glib_borrow(this).unsafe_cast_ref())
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                c"notify::direction".as_ptr() as *const _,
                Some(std::mem::transmute::<*const (), unsafe extern "C" fn()>(
                    notify_direction_trampoline::<Self, F> as *const (),
                )),
                Box_::into_raw(f),
            )
        }
    }

    #[doc(alias = "duration")]
    fn connect_duration_notify<F: Fn(&Self) + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn notify_duration_trampoline<
            P: IsA<TransitionEffect>,
            F: Fn(&P) + 'static,
        >(
            this: *mut ffi::PpsTransitionEffect,
            _param_spec: glib::ffi::gpointer,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(TransitionEffect::from_glib_borrow(this).unsafe_cast_ref())
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                c"notify::duration".as_ptr() as *const _,
                Some(std::mem::transmute::<*const (), unsafe extern "C" fn()>(
                    notify_duration_trampoline::<Self, F> as *const (),
                )),
                Box_::into_raw(f),
            )
        }
    }

    #[doc(alias = "duration-real")]
    fn connect_duration_real_notify<F: Fn(&Self) + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn notify_duration_real_trampoline<
            P: IsA<TransitionEffect>,
            F: Fn(&P) + 'static,
        >(
            this: *mut ffi::PpsTransitionEffect,
            _param_spec: glib::ffi::gpointer,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(TransitionEffect::from_glib_borrow(this).unsafe_cast_ref())
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                c"notify::duration-real".as_ptr() as *const _,
                Some(std::mem::transmute::<*const (), unsafe extern "C" fn()>(
                    notify_duration_real_trampoline::<Self, F> as *const (),
                )),
                Box_::into_raw(f),
            )
        }
    }

    #[doc(alias = "rectangular")]
    fn connect_rectangular_notify<F: Fn(&Self) + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn notify_rectangular_trampoline<
            P: IsA<TransitionEffect>,
            F: Fn(&P) + 'static,
        >(
            this: *mut ffi::PpsTransitionEffect,
            _param_spec: glib::ffi::gpointer,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(TransitionEffect::from_glib_borrow(this).unsafe_cast_ref())
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                c"notify::rectangular".as_ptr() as *const _,
                Some(std::mem::transmute::<*const (), unsafe extern "C" fn()>(
                    notify_rectangular_trampoline::<Self, F> as *const (),
                )),
                Box_::into_raw(f),
            )
        }
    }

    #[doc(alias = "scale")]
    fn connect_scale_notify<F: Fn(&Self) + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn notify_scale_trampoline<
            P: IsA<TransitionEffect>,
            F: Fn(&P) + 'static,
        >(
            this: *mut ffi::PpsTransitionEffect,
            _param_spec: glib::ffi::gpointer,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(TransitionEffect::from_glib_borrow(this).unsafe_cast_ref())
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                c"notify::scale".as_ptr() as *const _,
                Some(std::mem::transmute::<*const (), unsafe extern "C" fn()>(
                    notify_scale_trampoline::<Self, F> as *const (),
                )),
                Box_::into_raw(f),
            )
        }
    }

    #[doc(alias = "type")]
    fn connect_type_notify<F: Fn(&Self) + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn notify_type_trampoline<
            P: IsA<TransitionEffect>,
            F: Fn(&P) + 'static,
        >(
            this: *mut ffi::PpsTransitionEffect,
            _param_spec: glib::ffi::gpointer,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(TransitionEffect::from_glib_borrow(this).unsafe_cast_ref())
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                c"notify::type".as_ptr() as *const _,
                Some(std::mem::transmute::<*const (), unsafe extern "C" fn()>(
                    notify_type_trampoline::<Self, F> as *const (),
                )),
                Box_::into_raw(f),
            )
        }
    }
}

impl<O: IsA<TransitionEffect>> TransitionEffectExt for O {}
