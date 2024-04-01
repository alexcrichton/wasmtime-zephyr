#![no_std]

extern crate alloc;

use alloc::boxed::Box;

mod global_allocator;
mod panic;

static mut X: u32 = 4;

#[no_mangle]
pub unsafe extern "C" fn rust_foo() -> *mut u32 {
    Box::into_raw(Box::new(X))
}

#[no_mangle]
pub unsafe extern "C" fn rust_bar(x: *mut u32) {
    assert_eq!(*x, 4);
    let _ = Box::from_raw(x);
}

#[no_mangle]
pub unsafe extern "C" fn rust_set_foo(x: u32) {
    X = x;
}

mod bindings {
    include!(env!("BINDINGS"));
}
