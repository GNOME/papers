#![cfg_attr(docsrs, feature(doc_cfg))]

/// No-op.
macro_rules! skip_assert_initialized {
    () => {};
}

/// No-op.
macro_rules! assert_initialized_main_thread {
    () => {};
}

pub mod prelude;

#[allow(unused_imports)]
mod auto;

mod document_point;
mod point;
mod rectangle;

pub use auto::functions::*;
pub use auto::*;
pub use papers_document_sys as ffi;
