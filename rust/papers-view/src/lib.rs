#![cfg_attr(docsrs, feature(doc_cfg))]

/// No-op.
macro_rules! skip_assert_initialized {
    () => {};
}

/// No-op.
macro_rules! assert_initialized_main_thread {
    () => {};
}

pub use papers_view_sys as ffi;

pub use auto::*;
pub mod annotations_context;
mod attachment_context;

#[allow(unused_imports)]
mod auto;

pub mod prelude;
mod view_selection;
