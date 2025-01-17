use std::{
    ffi::{c_char, c_int},
    os::raw::c_void,
    sync::atomic::{AtomicUsize, Ordering},
};
use wasmtime::{Engine, Instance, Module, Store};
static WASM_VM: AtomicUsize = AtomicUsize::new(0);

#[allow(unused)]
struct StatePack {
    engine: Engine,
    module: Module,
    store: Store<()>,
    instance: Instance,
}
#[allow(unused)]
#[no_mangle]
extern "C" fn module_init() -> c_int {
    let wasm_bytes = include_bytes!("../handler.wasm");
    let engine = Engine::default();
    let module = Module::new(&engine, wasm_bytes).unwrap();
    let mut store = Store::new(&engine, ());
    let instance = Instance::new(&mut store, &module, &[]).unwrap();

    let state = Box::new(StatePack {
        engine,
        module,
        store,
        instance,
    });
    let ptr = Box::leak(state) as *mut StatePack;
    WASM_VM.store(ptr as usize, Ordering::Release);
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
    let mut sp = unsafe { &mut *(WASM_VM.load(Ordering::Acquire) as *mut StatePack) };

    let check_func = sp
        .instance
        .get_typed_func::<(i32,), (i32,)>(&mut sp.store, "test_url")
        .unwrap();
    let memory = sp.instance.get_memory(&mut sp.store, "memory").unwrap();
    // Dump the memory
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

    memory.write(&mut sp.store, 8, &str_buf);

    let (ret_f,) = check_func.call(&mut sp.store, (8,)).unwrap();
    *(unsafe { &mut *ret }) = ret_f as u64;
    0
}
