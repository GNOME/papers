use crate::ffi;
use crate::Path;
use crate::Point;

use glib::translate::*;

/* somehow gir gets confused between Point and &Point so we have to
let this here */
impl Path {
    #[doc(alias = "pps_path_new_for_array")]
    #[doc(alias = "new_for_array")]
    pub fn for_array(points: &[Point]) -> Path {
        assert_initialized_main_thread!();
        let n_points = points.len() as _;
        unsafe {
            from_glib_full(ffi::pps_path_new_for_array(
                points.to_glib_none().0,
                n_points,
            ))
        }
    }
}
