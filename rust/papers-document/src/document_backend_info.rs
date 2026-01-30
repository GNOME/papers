use crate::DocumentBackendInfo;
use glib::translate::*;
use std::fmt;

impl DocumentBackendInfo {
    #[inline]
    pub fn name(&self) -> Option<String> {
        unsafe { from_glib_none((*self.as_ptr()).name) }
    }

    #[inline]
    pub fn version(&self) -> Option<String> {
        unsafe { from_glib_none((*self.as_ptr()).version) }
    }
}

impl fmt::Debug for DocumentBackendInfo {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        f.debug_struct("DocumentBackendInfo")
            .field("name", &self.name())
            .field("version", &self.version())
            .finish()
    }
}
