use self::bindings::wasi::i2c::delay::{self, HostDelay};
use self::bindings::wasi::i2c::i2c::{self, ErrorCode, HostI2c, Operation};
use crate::bindings as ffi;
use alloc::vec;
use alloc::vec::Vec;
use wasmtime::component::{Linker, Resource, ResourceTable, ResourceTableError};
use wasmtime::{Error, Result};

mod bindings {
    use wasmtime::component::bindgen;

    bindgen!({
        world: "my-world",
        with: {
            "wasi:i2c/i2c/i2c": super::I2c,
            "wasi:i2c/delay/delay": super::Delay,
        },
        trappable_error_type: {
            "wasi:i2c/i2c/error-code" => super::I2cError,
        },
        trappable_imports: true,
    });
}

pub use bindings::MyWorld;

#[derive(Default)]
pub struct StoreState {
    table: ResourceTable,
}

impl StoreState {
    pub fn add_device(&mut self, device: *const ffi::device) -> Resource<I2c> {
        self.table.push(I2c { device }).unwrap()
    }
}

pub fn add_to_linker(linker: &mut Linker<StoreState>) -> Result<()> {
    bindings::MyWorld::add_to_linker(linker, |d| d)?;
    Ok(())
}

fn cvt(rc: i32) -> Result<(), ErrorCode> {
    match rc {
        0 => Ok(()),
        // TODO: actually match on `ffi::E*` errors and return more specific
        // errors here. Unsure what maps to what.
        _ => Err(ErrorCode::Other),
    }
}

pub struct I2c {
    device: *const ffi::device,
}

// TODO: comment this
unsafe impl Send for I2c {}
unsafe impl Sync for I2c {}

pub enum Delay {}

pub enum I2cError {
    Normal(ErrorCode),
    Resource(ResourceTableError),
}

pub type I2cResult<T, E = I2cError> = Result<T, E>;

impl From<ErrorCode> for I2cError {
    fn from(err: ErrorCode) -> I2cError {
        I2cError::Normal(err)
    }
}

impl From<ResourceTableError> for I2cError {
    fn from(err: ResourceTableError) -> I2cError {
        I2cError::Resource(err)
    }
}

impl delay::Host for StoreState {}

impl HostDelay for StoreState {
    fn delay_ns(&mut self, resource: Resource<Delay>, _ns: u32) -> Result<()> {
        let delay = self.table.get(&resource).map_err(Error::msg)?;
        match *delay {}
    }

    fn drop(&mut self, resource: Resource<Delay>) -> Result<()> {
        let delay = self.table.get(&resource).map_err(Error::msg)?;
        match *delay {}
    }
}

impl i2c::Host for StoreState {
    fn convert_error_code(&mut self, err: I2cError) -> Result<ErrorCode> {
        match err {
            I2cError::Normal(code) => Ok(code),
            I2cError::Resource(err) => Err(Error::msg(err)),
        }
    }
}

impl HostI2c for StoreState {
    fn transaction(
        &mut self,
        i2c: Resource<I2c>,
        address: u16,
        operations: Vec<Operation>,
    ) -> I2cResult<Vec<Vec<u8>>> {
        let i2c = self.table.get(&i2c)?;
        let mut msgs = Vec::new();
        let mut bufs = Vec::new();
        for op in operations.iter() {
            match op {
                Operation::Read(len) => {
                    let mut buf = vec![0; *len as usize];
                    msgs.push(ffi::i2c_msg {
                        buf: buf.as_mut_ptr(),
                        len: buf.len() as u32,
                        flags: ffi::_I2C_MSG_READ,
                    });
                    bufs.push(buf);
                }
                Operation::Write(buf) => {
                    msgs.push(ffi::i2c_msg {
                        buf: buf.as_ptr().cast_mut(),
                        len: buf.len() as u32,
                        flags: ffi::I2C_MSG_WRITE as u8,
                    });
                }
            }
        }
        cvt(unsafe {
            ffi::i2c_transfer(i2c.device, msgs.as_mut_ptr(), msgs.len() as u8, address)
        })?;
        Ok(bufs)
    }

    fn read(&mut self, i2c: Resource<I2c>, address: u16, size: u64) -> I2cResult<Vec<u8>> {
        let i2c = self.table.get(&i2c)?;
        let mut buf = vec![0; size as usize];
        cvt(unsafe { ffi::i2c_read(i2c.device, buf.as_mut_ptr(), buf.len() as u32, address) })?;
        Ok(buf)
    }

    fn write(&mut self, i2c: Resource<I2c>, address: u16, buf: Vec<u8>) -> I2cResult<()> {
        let i2c = self.table.get(&i2c)?;
        cvt(unsafe { ffi::i2c_write(i2c.device, buf.as_ptr(), buf.len() as u32, address) })?;
        Ok(())
    }

    fn write_read(
        &mut self,
        i2c: Resource<I2c>,
        address: u16,
        write: Vec<u8>,
        read_len: u64,
    ) -> I2cResult<Vec<u8>> {
        let i2c = self.table.get(&i2c)?;
        let mut buf = vec![0; read_len as usize];
        cvt(unsafe {
            ffi::i2c_write_read(
                i2c.device,
                address,
                write.as_ptr().cast(),
                write.len(),
                buf.as_mut_ptr().cast(),
                buf.len(),
            )
        })?;
        Ok(buf)
    }

    fn drop(&mut self, i2c: Resource<I2c>) -> Result<()> {
        let _i2c = self.table.delete(i2c).map_err(Error::msg)?;
        Ok(())
    }
}
