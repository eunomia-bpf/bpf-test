use std::{
    ffi::c_int,
    slice,
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
extern "C" fn module_run_at_handler(mem: *const u8, mem_size: u64, ret: *mut u64) -> c_int {
    let mut sp = unsafe { &mut *(LUA_VM.load(Ordering::SeqCst) as *mut StatePack) };

    let str_ref = sp
        .lua
        .create_string(unsafe { slice::from_raw_parts(mem, mem_size as usize) })
        .unwrap();
    let result = sp.func.call::<mlua::Value>(str_ref).unwrap();
    *(unsafe { &mut *ret }) = (!result.as_boolean().unwrap()) as u64;
    0
}
