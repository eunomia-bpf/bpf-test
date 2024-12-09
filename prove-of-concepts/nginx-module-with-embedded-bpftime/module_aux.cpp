#include "bpf_attach_ctx.hpp"
#include "bpftime_helper_group.hpp"
#include "bpftime_prog.hpp"
#include "bpftime_shm_internal.hpp"
#include "handler/prog_handler.hpp"
#include <bpftime_shm.hpp>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ostream>
#include <variant>
using namespace bpftime;

extern "C" int plus(int a, int b) { return a + b; }

static int selected_handler_id = -1;
static bpftime_prog *prog;
union bpf_attach_ctx_holder {
  bpf_attach_ctx ctx;
  bpf_attach_ctx_holder() {}
  ~bpf_attach_ctx_holder() {}
  void destroy() { ctx.~bpf_attach_ctx(); }
  void init() { new (&ctx) bpf_attach_ctx; }
};
static bpf_attach_ctx_holder ctx_holder;

extern "C" int module_init() {
  bpftime_initialize_global_shm(shm_open_type::SHM_OPEN_ONLY);
  ctx_holder.init();
  const handler_manager *manager =
      shm_holder.global_shared_memory.get_manager();
  for (size_t i = 0; i < manager->size(); i++) {
    if (std::holds_alternative<bpf_prog_handler>(manager->get_handler(i))) {
      const auto &prog = std::get<bpf_prog_handler>(manager->get_handler(i));
      if (prog.name == "nginx") {
        ::prog = new bpftime_prog(prog.insns.data(), prog.insns.size(),
                                  prog.name.c_str());
        bpftime_helper_group::get_kernel_utils_helper_group()
            .add_helper_group_to_prog(::prog);
        bpftime_helper_group::get_shm_maps_helper_group()
            .add_helper_group_to_prog(::prog);
        ::prog->bpftime_prog_load(true);
        return 0;
      }
    }
  }
  return -1;
}

extern "C" int module_run_at_handler(void *mem, uint64_t mem_size,
                                     uint64_t *ret, uint64_t uri_offset,
                                     void (*uri_extract)(const void *, char *,
                                                         size_t, size_t *)) {
  auto str = (const void *)((uintptr_t)mem + uri_offset);
  char buf[512];
  assert(prog != nullptr);

  size_t len;
  uri_extract(str, buf, sizeof(buf), &len);
  return prog->bpftime_prog_exec(buf, len + 1, ret);
}
