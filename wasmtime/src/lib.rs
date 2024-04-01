#![no_std]

extern crate alloc;

mod global_allocator;
mod panic;

const CWASM: &[u8] = include_bytes!("sample.cwasm");

#[no_mangle]
pub unsafe extern "C" fn rust_foo() {
    let engine = wasmtime::Engine::default();
    wasmtime::Module::deserialize(&engine, CWASM).unwrap();
}

mod bindings {
    include!(env!("BINDINGS"));
}
