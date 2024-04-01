#![no_std]

use core::panic::PanicInfo;

#[no_mangle]
pub extern "C" fn rust_foo() -> u32 {
    4
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    // let mut host_stderr = HStderr::new();

    // // logs "panicked at '$reason', src/main.rs:27:4" to the host stderr
    // writeln!(host_stderr, "{}", info).ok();

    loop {}
}
