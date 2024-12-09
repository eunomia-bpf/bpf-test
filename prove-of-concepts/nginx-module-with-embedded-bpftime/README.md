# nginx-bpftime module

A simple nginx module that could run ebpf program through bpftime when handling requests.

## How to compile

- Modify CMakelists.txt, change the nginx URL in `ExternalProject_Add`, if your nginx version doesn't match it
- Build the target `nginx` of the cmake project at root
- Run `make -j` in bpf-prog

## How to use

- Run syscall server through `bpftime load ./nginx_test` in folder `bpf-prog`
- Run nginx using `LD_LIBRARY_PATH=./build nginx -p . -c nginx.conf` in the project root
- Send requests to the server. Such as `curl http://127.0.0.1:9023` or `curl http://127.0.0.1:9023/X`, see if the request was forbidden

## How to use wasm module

- Build the rust project in folder `simple-wasmtime-aux` with `cargo build --release`
- Modify `module/config`, comment out line 6 and uncomment line 7
- Remove directory `build` and rebuild target `nginx`
- Use `LD_LIBRARY_PATH=./simple-wasmtime-aux/target/release/ nginx -p . -c nginx.conf` to run the nginx server

## How to test with lua module
- Similar to wasm module, but uncomment line 8 in `module/config` and run `cargo build --release` in `simple-luajit-wrapper`

## With docker contaner

- Run `docker build .`
- In the container, just follow the `how to use` section
