# test how syscount affect the system

Syscount: Tracing syscalls, printing top 10...

The similar approach is used widely in observation daemons.

The test result shows that:

- Syscount to trace all results: Mean: 32053.66
- Syscount filter other pid: Mean: 35716.34
- No effect: Mean: 39335.21

If we want to use the tracepoint to trace system calls for a target process, it will also affect other process, bring about 10% unnecessary overhead. This is because the syscall tracepoint is a global tracepoint, and it will be triggered for all processes. If you want to trace a specific process, you need to filter the pid in the kernel.

What's more, if you want to filter the pid in userspace, it will bring 20% overhead.

The code is at bpftime/example/libbpf-tools/syscount

Test env is similar to ssl-nginx.

Test nginx:

```sh
sudo nginx -c $(pwd)/nginx.conf -p $(pwd)
make
```

## Test nginx with syscount for 20 times

Run with syscount.

```sh
$ sudo ./example/libbpf-tools/syscount/syscount
Tracing syscalls, printing top 10... Ctrl+C to quit.
```

res:

```sh
Requests/sec statistics:
Mean: 32053.66
Median: 32288.62
Stdev: 3092.01
Min: 27080.95
Max: 37208.48

Transfer/sec statistics:
Mean: 9.97
Median: 10.04
Stdev: 0.96
Min: 8.42
Max: 11.57
```

## Test nginx with syscount fiilter other pid for 20 times

Run with an unexisting pid to test the effect of syscount.

```sh
$ sudo ./example/libbpf-tools/syscount/syscount -p 123
Tracing syscalls, printing top 10... Ctrl+C to quit.
```

res:

```sh
Requests/sec statistics:
Mean: 35716.34
Median: 35967.76
Stdev: 2479.69
Min: 31142.76
Max: 39226.01

Transfer/sec statistics:
Mean: 11.10
Median: 11.18
Stdev: 0.77
Min: 9.68
Max: 12.20
```

## Test nginx without syscount for 20 times

```sh
len:  20
Requests/sec statistics:
Mean: 39335.21
Median: 39495.93
Stdev: 2765.97
Min: 31005.75
Max: 42489.23

Transfer/sec statistics:
Mean: 12.23
Median: 12.28
Stdev: 0.86
Min: 9.64
Max: 13.21
```