#define BPF_NO_GLOBAL_DATA
#include <vmlinux.h>
#include "nginx_test.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

struct {
  __uint(type, BPF_MAP_TYPE_PERCPU_HASH);
  __uint(max_entries, 1024);
  __type(key, struct metric_key);
  __type(value, u64);
} metrics SEC(".maps");

int __always_inline strcmp_my(const char *a, const char *b) {
  while (*a && *b) {
    if (*a != *b)
      break;
    a++;
    b++;
  }
  return *a - *b;
}

void __always_inline strcpy_range(char *dest, const char *src_begin,
                                  const char *src_end) {
  for (; src_begin <= src_end; src_begin++) {
    *dest = *src_begin;
    dest++;
  }
  *dest = 0;
}

void __always_inline strcpy_my(char *dest, const char *src) {
  while (*src) {
    *dest = *src;
    src++;
    dest++;
  }
  *dest = 0;
}
void __always_inline empty(void *src, int sz) {
  for (int i = 0; i < sz; i++) {
    ((char *)src)[i] = 0;
  }
}

void __always_inline add_metric(char *sec1, char *sec2) {
  struct metric_key key;
  empty(&key, sizeof(key));
  strcpy_my(key.sec1, sec1);
  strcpy_my(key.sec2, sec2);
  u64 raw_val = 0;
  u64 *count = bpf_map_lookup_elem(&metrics, &key);
  if (count)
    raw_val = *count;
  raw_val++;
  bpf_map_update_elem(&metrics, &key, &raw_val, 0);
}

#define LENGTH_OF(a) (sizeof(a) / sizeof(a[0]))

static const char entries[][4][32] = {
    {"yunwei", "aaaa", "bbbb", ""},
    {"yyw", "abab", "1234", ""},
    {"mnfe", "notfound", "fe", ""},
    {"mikunot", "foundexception", "foundexception", ""}};

int __always_inline try_match_url(const char *str) {

  // str should be a path like /x/y/z
  const char *last = NULL, *curr = str;
  char sec1[32];
  char sec2[32];
  int sec1_used = 0, sec2_used = 0;
  while (1) {
    char p = *curr;
    if (p == '/' || p == 0) {
      if (last != NULL) {
        if (!sec1_used) {
          strcpy_range(sec1, last + 1, curr - 1);
          sec1_used = 1;
        } else if (!sec2_used) {
          strcpy_range(sec2, last + 1, curr - 1);
          sec2_used = 1;
        }
      }
      last = curr;
    }
    if (p)
      curr++;
    else
      break;
  };
  if (!sec1_used || !sec2_used)
    return 0;
  for (int i = 0; i < (int)LENGTH_OF(entries); i++) {
    if (strcmp_my(entries[i][0], sec1) == 0) {
      for (int j = 1; j < 4; j++) {
        if (strcmp_my(entries[i][j], sec2) == 0) {
          add_metric(sec1, sec2);
          return 1;
        }
      }
    }
  }
  return 0;
}

SEC("uprobe/http_url")
int nginx(const char *str) { return !try_match_url(str); }

char LICENSE[] SEC("license") = "GPL";
