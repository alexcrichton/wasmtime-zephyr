#![no_std]

extern crate alloc;

use wasmtime::component::bindgen;
use wasmtime::{Config, Engine, Instance, Store};

mod global_allocator;
mod panic;

const CWASM: &[u8] = include_bytes!("sample.cwasm");

bindgen!({
    world: "my-world",
});

fn engine() -> Engine {
    let mut config = Config::new();
    config.static_memory_maximum_size(0);
    config.dynamic_memory_guard_size(0);
    config.guard_before_linear_memory(false);
    Engine::new(&config).unwrap()
}

#[no_mangle]
pub unsafe extern "C" fn rust_add(a: i32, b: i32) -> i32 {
    let engine = engine();
    let module = wasmtime::Module::deserialize(&engine, CWASM).unwrap();
    let mut store = Store::new(&engine, ());
    let i = Instance::new(&mut store, &module, &[]).unwrap();
    let f = i
        .get_typed_func::<(i32, i32), i32>(&mut store, "add")
        .unwrap();
    f.call(&mut store, (a, b)).unwrap()
}

mod bindings {
    include!(env!("BINDINGS"));
}
