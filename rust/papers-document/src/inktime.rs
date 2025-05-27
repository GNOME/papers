use crate::InkTime;

impl InkTime {
    #[inline]
    pub fn x(&self) -> f64 {
        self.inner.x
    }

    #[inline]
    pub fn y(&self) -> f64 {
        self.inner.y
    }

    #[inline]
    pub fn time(&self) -> u32 {
        self.inner.time
    }
}
