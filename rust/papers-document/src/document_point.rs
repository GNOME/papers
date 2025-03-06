use crate::DocumentPoint;
use crate::Point;
use std::fmt;

impl DocumentPoint {
    #[inline]
    pub fn page_index(&self) -> i32 {
        self.inner.page_index
    }

    #[inline]
    pub fn point_on_page(&self) -> Point {
        Point {
            inner: self.inner.point_on_page,
        }
    }
}

impl fmt::Debug for DocumentPoint {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        f.debug_struct("Mark")
            .field("page_index", &self.page_index())
            .field("point_on_page", &self.point_on_page())
            .finish()
    }
}
