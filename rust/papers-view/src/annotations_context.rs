use crate::ffi;
use crate::AnnotationsContext;
use glib::translate::*;
use libc::c_void;
use papers_document::{AnnotationType, Point};

pub enum AddAnnotationData {
    None,
    TextMarkup(papers_document::AnnotationTextMarkupType),
}

impl AnnotationsContext {
    #[doc(alias = "pps_annotations_context_add_annotation")]
    pub fn add_annotation_sync(
        &self,
        page_index: i32,
        type_: AnnotationType,
        start: &Point,
        end: &Point,
        color: &gdk::RGBA,
        user_data: AddAnnotationData,
    ) -> Option<papers_document::Annotation> {
        match type_ {
            AnnotationType::TextMarkup => {
                if let AddAnnotationData::TextMarkup(markup_type) = user_data {
                    unsafe {
                        from_glib_none(ffi::pps_annotations_context_add_annotation_sync(
                            self.to_glib_none().0,
                            page_index,
                            type_.into_glib(),
                            start.to_glib_none().0,
                            end.to_glib_none().0,
                            color.to_glib_none().0,
                            &markup_type as *const _ as *mut c_void,
                        ))
                    }
                } else {
                    panic!("You must pass an AnnotationTextMarkupType for TextMarkupAnnotations")
                }
            }
            _ => unsafe {
                from_glib_none(ffi::pps_annotations_context_add_annotation_sync(
                    self.to_glib_none().0,
                    page_index,
                    type_.into_glib(),
                    start.to_glib_none().0,
                    end.to_glib_none().0,
                    color.to_glib_none().0,
                    &user_data as *const _ as *mut c_void,
                ))
            },
        }
    }
}
