use std::{
    ffi::{c_char, c_int, c_void},
    sync::atomic::{AtomicUsize, Ordering},
};

use mlua::{Function, Lua};

static LUA_VM: AtomicUsize = AtomicUsize::new(0);

#[allow(unused)]
struct StatePack {
    lua: Lua,
    func: Function,
}

#[allow(unused)]
#[no_mangle]
extern "C" fn module_init() -> c_int {
    let script_str = include_str!("../../filter.lua");
    let lua = Lua::new();
    let function = lua.load(script_str).eval::<mlua::Function>().unwrap();

    let state = Box::new(StatePack {
        func: function,
        lua,
    });
    let ptr = Box::leak(state) as *mut StatePack;
    LUA_VM.store(ptr as usize, Ordering::SeqCst);
    0
}

#[allow(unused)]
#[no_mangle]
extern "C" fn module_run_at_handler(
    mem: *const u8,
    mem_size: u64,
    ret: *mut u64,
    uri_offset: u64,
    uri_extract: unsafe extern "C" fn(*const c_void, *mut c_char, usize, *mut usize),
) -> c_int {
    let mut sp = unsafe { &mut *(LUA_VM.load(Ordering::SeqCst) as *mut StatePack) };

    let copied_mem = unsafe { std::slice::from_raw_parts(mem, mem_size as usize) }
        .iter()
        .map(|x| *x)
        .collect::<Vec<u8>>();
    let mut str_buf = vec![0u8; 512];
    let mut out_len: usize = 0;
    unsafe {
        uri_extract(
            mem.add(uri_offset as usize) as *const c_void,
            str_buf.as_mut_ptr() as *mut i8,
            str_buf.len(),
            &mut out_len as *mut usize,
        )
    };

    let str_ref = sp.lua.create_string(&str_buf[..out_len]).unwrap();
    let result = sp.func.call::<mlua::Value>(str_ref).unwrap();
    *(unsafe { &mut *ret }) = (!result.as_boolean().unwrap()) as u64;
    0
}
