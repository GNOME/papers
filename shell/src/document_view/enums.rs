use super::*;

pub enum AnnotationColor {
    Yellow,
    Orange,
    Red,
    Purple,
    Blue,
    Green,
    Unknown,
}

impl From<&str> for AnnotationColor {
    fn from(color: &str) -> Self {
        match color {
            "yellow" => AnnotationColor::Yellow,
            "orange" => AnnotationColor::Orange,
            "red" => AnnotationColor::Red,
            "purple" => AnnotationColor::Purple,
            "blue" => AnnotationColor::Blue,
            "green" => AnnotationColor::Green,
            _ => AnnotationColor::Unknown,
        }
    }
}

impl std::fmt::Display for AnnotationColor {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            AnnotationColor::Yellow => write!(f, "yellow"),
            AnnotationColor::Orange => write!(f, "orange"),
            AnnotationColor::Red => write!(f, "red"),
            AnnotationColor::Purple => write!(f, "purple"),
            AnnotationColor::Blue => write!(f, "blue"),
            AnnotationColor::Green => write!(f, "green"),
            AnnotationColor::Unknown => write!(f, "unknown"),
        }
    }
}

impl From<gdk::RGBA> for AnnotationColor {
    fn from(color: gdk::RGBA) -> Self {
        if color == AnnotationColor::Yellow.to_rgba() {
            return AnnotationColor::Yellow;
        }
        if color == AnnotationColor::Orange.to_rgba() {
            return AnnotationColor::Orange;
        }
        if color == AnnotationColor::Red.to_rgba() {
            return AnnotationColor::Red;
        }
        if color == AnnotationColor::Purple.to_rgba() {
            return AnnotationColor::Purple;
        }
        if color == AnnotationColor::Blue.to_rgba() {
            return AnnotationColor::Blue;
        }
        if color == AnnotationColor::Green.to_rgba() {
            return AnnotationColor::Green;
        }
        AnnotationColor::Unknown
    }
}

impl AnnotationColor {
    pub fn to_rgba(&self) -> gdk::RGBA {
        match self {
            AnnotationColor::Yellow => gdk::RGBA::parse("#f5c211").unwrap(),
            AnnotationColor::Orange => gdk::RGBA::parse("#ff7800").unwrap(),
            AnnotationColor::Red => gdk::RGBA::parse("#ed333b").unwrap(),
            AnnotationColor::Purple => gdk::RGBA::parse("#c061cb").unwrap(),
            AnnotationColor::Blue => gdk::RGBA::parse("#3584e4").unwrap(),
            AnnotationColor::Green => gdk::RGBA::parse("#33d17a").unwrap(),
            // TODO: Does yellow here makes sense?
            AnnotationColor::Unknown => gdk::RGBA::parse("#f5c211").unwrap(),
        }
    }
}
