#![no_std]

extern crate alloc;

use core::ffi::c_void;
use wasmtime::component::{Component, Linker};
use wasmtime::{Config, Engine, Instance, Store};

mod global_allocator;
mod i2c;
mod panic;

const CWASM: &[u8] = include_bytes!(env!("CWASM"));

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

#[no_mangle]
pub unsafe extern "C" fn rust_run(device: *const c_void) {
    let engine = engine();
    let mut linker = Linker::new(&engine);
    i2c::add_to_linker(&mut linker).unwrap();
    let component = Component::deserialize(&engine, CWASM).unwrap();
    let mut data = i2c::StoreState::default();
    let i2c = data.add_device(device.cast());
    let mut store = Store::new(&engine, data);
    let (bindings, _) = i2c::MyWorld::instantiate(&mut store, &component, &linker).unwrap();
    bindings.call_run(&mut store, i2c).unwrap();
}

#[allow(warnings)]
mod bindings {
    include!(env!("BINDINGS"));
}
