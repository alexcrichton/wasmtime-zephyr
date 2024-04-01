use crate::bindings;
use alloc::alloc::{GlobalAlloc, Layout};

#[global_allocator]
static ALLOC: Allocator = Allocator;

struct Allocator;

unsafe impl GlobalAlloc for Allocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        bindings::app_alloc(layout.size(), layout.align()).cast()
    }
    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
        bindings::app_dealloc(ptr.cast())
    }
}
