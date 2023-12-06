// SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause)
/* Copyright (c) 2020 Facebook */
#include "nginx_test.h"
#include "nginx_test.skel.h"
#include <asm-generic/errno-base.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>
#define warn(...) fprintf(stderr, __VA_ARGS__)

static int libbpf_print_fn(enum libbpf_print_level level, const char *format,
                           va_list args) {
  return vfprintf(stderr, format, args);
}

static volatile bool exiting = false;

static void sig_handler(int sig) { exiting = true; }

static int print_stat(struct nginx_test_bpf *skel) {
  int mfd = bpf_map__fd(skel->maps.metrics);
  time_t t;
  struct tm *tm;
  char ts[16];
  time(&t);
  tm = localtime(&t);
  strftime(ts, sizeof(ts), "%H:%M:%S", tm);
  printf("%-9s\n", ts);
  int err = 0;
  struct metric_key key, *prev_key = NULL;
  const int ncpu = sysconf(_SC_NPROCESSORS_ONLN);
  uint64_t value[ncpu];
  while (1) {
    err = bpf_map_get_next_key(mfd, prev_key, &key);
    if (err) {
      if (errno == ENOENT) {
        err = 0;
        break;
      }
      warn("bpf_map_get_next_key failed: %s\n", strerror(errno));
      return err;
    }
    err = bpf_map_lookup_elem(mfd, &key, &value);
    prev_key = &key;
    if (err) {
      warn("bpf_map_lookup_elem failed: %s\n", strerror(errno));
      return err;
    }
    printf("%s/%s: ", key.sec1, key.sec2);
    uint64_t sum = 0;
    for (int i = 0; i < ncpu; i++)
      sum += value[i];
    printf("%" PRIu64 " \n", sum);

    fflush(stdout);
  }
  return err;
}

int main(int argc, char **argv) {
  struct nginx_test_bpf *skel;
  int err;

  /* Set up libbpf errors and debug info callback */
  libbpf_set_print(libbpf_print_fn);

  /* Cleaner handling of Ctrl-C */
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  /* Load and verify BPF application */
  skel = nginx_test_bpf__open();
  if (!skel) {
    fprintf(stderr, "Failed to open and load BPF skeleton\n");
    return 1;
  }

  /* Load & verify BPF programs */
  err = nginx_test_bpf__load(skel);
  if (err) {
    fprintf(stderr, "Failed to load and verify BPF skeleton\n");
    goto cleanup;
  }
  LIBBPF_OPTS(bpf_uprobe_opts, attach_opts, .func_name = "dummy",
              .retprobe = false);
  struct bpf_link *attach = bpf_program__attach_uprobe_opts(
      skel->progs.nginx, -1, "../module-output/ngx_http_bpftime_module.so", 0,
      &attach_opts);
  if (!attach) {
    fprintf(stderr, "Failed to attach BPF skeleton\n");
    err = -1;
    goto cleanup;
  }
  while (!exiting) {
    sleep(1);
  }
  print_stat(skel);
cleanup:
  /* Clean up */
  nginx_test_bpf__destroy(skel);

  return err < 0 ? -err : 0;
}
