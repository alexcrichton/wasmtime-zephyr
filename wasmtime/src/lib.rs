#![no_std]

mod panic;

#[no_mangle]
pub extern "C" fn rust_foo() -> u32 {
    4
}

mod bindings {
    include!(env!("BINDINGS"));
}
