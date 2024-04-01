use crate::bindings;
use core::fmt::{self, Write};
use core::panic::PanicInfo;

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    let _ = writeln!(AppStderr, "{}", info);
    unsafe {
        bindings::app_abort();
    }
}

struct AppStderr;

impl Write for AppStderr {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        unsafe {
            bindings::app_printk(s.as_ptr(), s.len());
        }
        Ok(())
    }
}
