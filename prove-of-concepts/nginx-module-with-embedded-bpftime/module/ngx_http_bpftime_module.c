#include "ngx_conf_file.h"
#include "ngx_log.h"
#include "ngx_string.h"
#include <stdio.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <stddef.h>

typedef struct {
  ngx_flag_t enable;
} ngx_http_bpftime_loc_conf_t;

static ngx_int_t ngx_http_bpftime_handler(ngx_http_request_t *r);
static void *ngx_http_bpftime_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_bpftime_merge_loc_conf(ngx_conf_t *cf, void *parent,
                                             void *child);
static ngx_int_t ngx_http_bpftime_init(ngx_conf_t *cf);

static ngx_command_t ngx_http_bpftime_commands[] = {

    {ngx_string("bpftime"),
     NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_FLAG,
     ngx_conf_set_flag_slot, NGX_HTTP_LOC_CONF_OFFSET,
     offsetof(ngx_http_bpftime_loc_conf_t, enable), NULL},

    ngx_null_command};

static ngx_http_module_t ngx_http_bpftime_module_ctx = {
    NULL,                  /* preconfiguration */
    ngx_http_bpftime_init, /* postconfiguration */

    NULL, /* create main configuration */
    NULL, /* init main configuration */

    NULL, /* create server configuration */
    NULL, /* merge server configuration */

    ngx_http_bpftime_create_loc_conf, /* create location configuration */
    ngx_http_bpftime_merge_loc_conf   /* merge location configuration */
};

ngx_module_t ngx_http_bpftime_module = {
    NGX_MODULE_V1,
    &ngx_http_bpftime_module_ctx, /* module context */
    ngx_http_bpftime_commands,    /* module directives */
    NGX_HTTP_MODULE,              /* module type */
    NULL,                         /* init master */
    NULL,                         /* init module */
    NULL,                         /* init process */
    NULL,                         /* init thread */
    NULL,                         /* exit thread */
    NULL,                         /* exit process */
    NULL,                         /* exit master */
    NGX_MODULE_V1_PADDING};

int module_run_at_handler(void *mem, uint64_t mem_size, uint64_t *ret,
                          uint64_t uri_offset,
                          void (*uri_extract)(const void *, char *, size_t,
                                              size_t *));

static void extract_uri(const void *value, char *out_buf, size_t buf_size,
                        size_t *out_len) {
  ngx_str_t *str = (ngx_str_t *)value;
  snprintf(out_buf, buf_size, "%*s", (int)str->len, str->data);
  int len = strlen(out_buf);
  for (int i = len - 1; i >= 0; i--)
    if (out_buf[i] == ' ') {
      out_buf[i] = '\0';
      break;
    }
  len = strlen(out_buf);
  *out_len = len;
}

static ngx_int_t ngx_http_bpftime_handler(ngx_http_request_t *r) {
  ngx_http_bpftime_loc_conf_t *ulcf;
  ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0,
                "http ua access handler");
  ulcf = ngx_http_get_module_loc_conf(r, ngx_http_bpftime_module);
  if (ulcf->enable) {
    uint64_t ret = 0;
    int err = module_run_at_handler(
        r, sizeof(*r), &ret, offsetof(ngx_http_request_t, uri), &extract_uri);

    if (err < 0) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                    "Failed to run ebpf program, err=%d", err);
      return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0, "Ebpf ret: %d",
                  (int)ret);
    if (ret) {
      ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0,
                    "Rejected access by ebpf");
      return NGX_HTTP_FORBIDDEN;
    }
  }

  return NGX_DECLINED;
}

static void *ngx_http_bpftime_create_loc_conf(ngx_conf_t *cf) {
  ngx_http_bpftime_loc_conf_t *conf;

  conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_bpftime_loc_conf_t));
  if (conf == NULL) {
    return NULL;
  }

  conf->enable = NGX_CONF_UNSET;

  return conf;
}

static char *ngx_http_bpftime_merge_loc_conf(ngx_conf_t *cf, void *parent,
                                             void *child) {
  ngx_http_bpftime_loc_conf_t *prev = parent;
  ngx_http_bpftime_loc_conf_t *conf = child;

  ngx_conf_merge_value(conf->enable, prev->enable, 0);

  return NGX_CONF_OK;
}

static ngx_int_t ngx_http_bpftime_init(ngx_conf_t *cf) {
  ngx_http_handler_pt *h;
  ngx_http_core_main_conf_t *cmcf;

  cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

  h = ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);
  if (h == NULL) {
    return NGX_ERROR;
  }

  *h = ngx_http_bpftime_handler;
  int module_init();
  int err = module_init();
  ngx_log_error(NGX_LOG_ERR, cf->log, 0, "Module init: %d", err);
  return NGX_OK;
}

void dummy() {}
