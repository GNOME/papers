use crate::deps::*;

use glib::translate::*;

mod imp {
    use super::*;

    #[derive(Properties, Default, Debug)]
    #[properties(wrapper_type = super::PpsSidebarPage)]
    pub struct PpsSidebarPage {
        #[property(name = "document-model", set, get, construct_only)]
        pub(super) model: RefCell<Option<DocumentModel>>,
        #[property(set, get, construct_only)]
        pub(super) sidebar: RefCell<Option<PpsSidebar>>,
    }

    #[glib::object_subclass]
    impl ObjectSubclass for PpsSidebarPage {
        const NAME: &'static str = "PpsSidebarPage";
        const ABSTRACT: bool = true;
        type Type = super::PpsSidebarPage;
        type ParentType = adw::Bin;
        type Class = super::Class;
    }

    #[glib::derived_properties]
    impl ObjectImpl for PpsSidebarPage {}

    impl BinImpl for PpsSidebarPage {}

    impl WidgetImpl for PpsSidebarPage {}

    impl PpsSidebarPage {}
}

glib::wrapper! {
    pub struct PpsSidebarPage(ObjectSubclass<imp::PpsSidebarPage>)
        @extends adw::Bin, gtk::Widget,
        @implements gtk::Accessible, gtk::Buildable, gtk::ConstraintTarget;
}

impl PpsSidebarPage {
    pub fn new() -> Self {
        glib::Object::builder().build()
    }
}

impl Default for PpsSidebarPage {
    fn default() -> Self {
        Self::new()
    }
}

pub trait PpsSidebarPageExt: IsA<PpsSidebarPage> {
    fn document_model(&self) -> Option<DocumentModel> {
        self.as_ref().document_model()
    }

    fn navigate_to_view(&self) {
        if let Some(sidebar) = self.as_ref().imp().sidebar.borrow().clone() {
            sidebar.emit_by_name::<()>("navigated-to-view", &[]);
        }
    }

    fn support_document(&self, document: &papers_document::Document) -> bool {
        let obj = self.as_ref().clone();

        (obj.class().as_ref().support_document)(&obj, document.to_glib_none().0)
    }
}

impl<O: IsA<PpsSidebarPage>> PpsSidebarPageExt for O {}

pub trait PpsSidebarPageImpl: BinImpl {
    fn support_document(&self, document: &papers_document::Document) -> bool;
}

unsafe impl<T: PpsSidebarPageImpl> IsSubclassable<T> for PpsSidebarPage {
    fn class_init(class: &mut glib::Class<Self>) {
        Self::parent_class_init::<T>(class);

        let klass = class.as_mut();
        klass.support_document = |obj, document| unsafe {
            let imp = obj.unsafe_cast_ref::<T::Type>().imp();
            imp.support_document(&from_glib_borrow(document))
        };
    }
}

#[repr(C)]
pub struct Class {
    pub parent_class: adw::ffi::AdwBinClass,

    pub support_document:
        fn(&PpsSidebarPage, document: *mut papers_document::ffi::PpsDocument) -> bool,
}

unsafe impl ClassStruct for Class {
    type Type = imp::PpsSidebarPage;
}

impl std::ops::Deref for Class {
    type Target = glib::Class<<<Self as ClassStruct>::Type as ObjectSubclass>::ParentType>;

    fn deref(&self) -> &Self::Target {
        unsafe { &*(&self.parent_class as *const _ as *const _) }
    }
}
