#![no_std]

mod panic;

static mut X: u32 = 4;

#[no_mangle]
pub unsafe extern "C" fn rust_foo() -> u32 {
    X
}

#[no_mangle]
pub unsafe extern "C" fn rust_set_foo(x: u32) {
    X = x;
}

mod bindings {
    include!(env!("BINDINGS"));
}
