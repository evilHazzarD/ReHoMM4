// Host imports for dynamically lifted PE32 code. The caller runs on the guest
// thread and passes its live Ctx in the shell's shared WebAssembly memory.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>
#include <sys/stat.h>

#include <emscripten.h>

#include "install/hash.h"
#include "runtime/lifted.h"

using recomp::Ctx;
namespace x87 = recomp::x87;

EM_JS(int, h4_worker_original, (uint32_t address, Ctx* ctx,
                                  const uint8_t* bytes, uint32_t size), {
  let invoke = Module['h4InvokeLifted'];
  if (!invoke && size) invoke = Module['h4InstallLifted'](bytes, size, 0, 0, 0);
  return invoke ? invoke(address >>> 0, ctx >>> 0) : 0;
});

#if H4_WEB_DYNAMIC
EM_JS(int, h4_prepare_worker, (const uint8_t* bytes, uint32_t size,
                              const recomp::LiftedFn* hooks, uint32_t count, uint32_t stride), {
  try {
    Module['h4InstallLifted'](bytes, size, hooks, count, stride);
    return 1;
  } catch (error) {
    Module.printErr('h4-web: unable to load lifted code: ' + error.message);
    return 0;
  }
});

static std::vector<uint8_t> dynamic_code;

bool h4_prepare_dynamic_game(const char* executable) {
  FILE* input = std::fopen(executable, "rb");
  if (!input) {
    std::fprintf(stderr, "h4-web: unable to read game executable: %s\n", executable);
    return false;
  }
  h4::install::Sha256 hash;
  uint8_t buffer[65536];
  while (const auto size = std::fread(buffer, 1, sizeof(buffer), input)) {
    hash.Update(buffer, size);
  }
  const bool complete_input = !std::ferror(input);
  std::fclose(input);
  if (!complete_input) {
    std::fprintf(stderr, "h4-web: incomplete game executable: %s\n", executable);
    return false;
  }
  const std::string digest = hash.HexDigest();
  const std::string path = "/persist/ReHoMM4/code-" + digest + ".wasm";
  struct stat info {};
  // Bound the allocation and require a regular file in the mounted filesystem.
  if (stat(path.c_str(), &info)) {
    MAIN_THREAD_EM_ASM({
      if (Module.h4Status) Module.h4Status('compile', UTF8ToString($0));
    }, digest.c_str());
    return false;
  }
  if (!S_ISREG(info.st_mode) || info.st_size <= 0 ||
      info.st_size > (512LL << 20)) {
    std::fprintf(stderr, "h4-web: invalid lifted code: %s\n", path.c_str());
    return false;
  }
  FILE* file = std::fopen(path.c_str(), "rb");
  if (!file) {
    std::fprintf(stderr, "h4-web: unable to open lifted code: %s\n", path.c_str());
    return false;
  }
  dynamic_code.resize(static_cast<size_t>(info.st_size));
  const bool complete =
      std::fread(dynamic_code.data(), 1, dynamic_code.size(), file) == dynamic_code.size();
  std::fclose(file);
  if (!complete) {
    std::fprintf(stderr, "h4-web: incomplete lifted code: %s\n", path.c_str());
    return false;
  }
  return h4_prepare_worker(dynamic_code.data(), dynamic_code.size(), recomp::kLiftedFunctions,
                           recomp::kLiftedFunctionCount, sizeof(recomp::LiftedFn)) != 0;
}
#endif
constexpr x87::Op x87_arithmetic[] = {x87::Op::Add, x87::Op::Mul, x87::Op::Sub,
                                     x87::Op::SubR, x87::Op::Div, x87::Op::DivR};

template<typename T>
static T read_operand(const void* address) {
  T value;
  std::memcpy(&value, address, sizeof(value));
  return value;
}

template<typename T>
static void write_operand(void* address, T value) {
  std::memcpy(address, &value, sizeof(value));
}

// ABI v1 exposes the integer prefix of Ctx. FPU state remains opaque and is
// accessed through the same x87 implementation as the statically lifted code.
static_assert(sizeof(void*) == 4);
static_assert(offsetof(Ctx, eax) == 0 && offsetof(Ctx, ecx) == 8 &&
              offsetof(Ctx, edx) == 16 && offsetof(Ctx, ebx) == 24 &&
              offsetof(Ctx, esp) == 32 && offsetof(Ctx, ebp) == 40 &&
              offsetof(Ctx, esi) == 48 && offsetof(Ctx, edi) == 56);
static_assert(offsetof(Ctx, eflags) == 128 && offsetof(Ctx, fs_base) == 132);

extern "C" {

int h4_call_worker_original(uint32_t address, Ctx* ctx) {
#if H4_WEB_DYNAMIC
  return h4_worker_original(address, ctx, dynamic_code.data(), dynamic_code.size());
#else
  return h4_worker_original(address, ctx, nullptr, 0);
#endif
}

void h4_dynamic_original(Ctx* ctx, uint32_t address) {
  if (!h4_call_worker_original(address, ctx)) {
    recomp::Unimplemented(*ctx, address, "missing original body in dynamic module");
  }
}

EMSCRIPTEN_KEEPALIVE uint32_t h4_dynamic_abi_version() { return 1; }

EMSCRIPTEN_KEEPALIVE uint32_t h4_dynamic_guest_base() {
  return reinterpret_cast<uintptr_t>(recomp::g_guest_base);
}

EMSCRIPTEN_KEEPALIVE void h4_dynamic_dispatch(Ctx* ctx, uint32_t target, uint32_t from) {
  recomp::Dispatch(*ctx, target, from);
}

EMSCRIPTEN_KEEPALIVE uint32_t h4_dynamic_dispatch_owned(Ctx* ctx, uint32_t target,
                                                        uint32_t from, uint32_t excluded_origin) {
  const uint32_t frame_ebp = ctx->ebp;
  try {
    recomp::Dispatch(*ctx, target, from);
  } catch (const recomp::GuestResume& resume) {
    if (resume.ebp != frame_ebp || resume.from == excluded_origin) throw;
    recomp::Resume(*ctx, resume);
    return 1;
  }
  return 0;
}

EMSCRIPTEN_KEEPALIVE void h4_dynamic_divide_error(Ctx* ctx, uint32_t from) {
  recomp::RaiseDivideError(*ctx, from);
}

EMSCRIPTEN_KEEPALIVE void h4_dynamic_interrupt(Ctx* ctx, uint32_t from) {
  recomp::Trap(*ctx, from);
}

EMSCRIPTEN_KEEPALIVE int64_t h4_dynamic_signed_divide(Ctx* ctx, uint32_t from,
                                                    int64_t dividend, int64_t divisor) {
  if (divisor == 0 ||
      (dividend == std::numeric_limits<int64_t>::min() && divisor == -1)) {
    recomp::RaiseDivideError(*ctx, from);
    __builtin_trap();
  }
  return dividend / divisor;
}

EMSCRIPTEN_KEEPALIVE void h4_dynamic_resume(uint32_t target, uint32_t ebp, uint32_t from) {
  throw recomp::GuestResume{target, ebp, from};
}

EMSCRIPTEN_KEEPALIVE void h4_dynamic_fninit(Ctx* ctx) {
  recomp::x87::Init(ctx->fpu);
}

EMSCRIPTEN_KEEPALIVE void h4_dynamic_fld32(Ctx* ctx, uint32_t bits) {
  recomp::x87::LoadF32(ctx->fpu, bits);
}

EMSCRIPTEN_KEEPALIVE uint32_t h4_dynamic_fst32(Ctx* ctx) {
  return recomp::x87::StoreF32(ctx->fpu);
}

EMSCRIPTEN_KEEPALIVE uint32_t h4_dynamic_fstsw(const Ctx* ctx) {
  return recomp::x87::StatusWord(ctx->fpu);
}

EMSCRIPTEN_KEEPALIVE void h4_dynamic_fpop(Ctx* ctx) {
  recomp::x87::Pop(ctx->fpu);
}

EMSCRIPTEN_KEEPALIVE uint32_t h4_dynamic_fstcw(const Ctx* ctx) {
  return x87::ControlWord(ctx->fpu);
}

EMSCRIPTEN_KEEPALIVE void h4_dynamic_fldcw(Ctx* ctx, uint32_t word) {
  x87::SetControlWord(ctx->fpu, static_cast<uint16_t>(word));
}

// Opcode 0 loads, 1 stores, 2 compares, and 3–8 perform arithmetic.
// Format 0 is binary32, 1 binary64, 2–4 signed integer16/32/64, and 5 binary80.
// Opcodes 11–14 manage the complete x87 environment or state.
EMSCRIPTEN_KEEPALIVE void h4_dynamic_x87_memory(Ctx* ctx, uint32_t operation,
                                                uint32_t format, void* address) {
  auto& s = ctx->fpu;
  switch (operation) {
    case 0:
      switch (format) {
        case 0: return x87::LoadF32(s, read_operand<uint32_t>(address));
        case 1: return x87::LoadF64(s, read_operand<uint64_t>(address));
        case 2: return x87::LoadI16(s, read_operand<int16_t>(address));
        case 3: return x87::LoadI32(s, read_operand<int32_t>(address));
        case 4: return x87::LoadI64(s, read_operand<int64_t>(address));
        case 5: {
          const auto* bytes = static_cast<const uint8_t*>(address);
          return x87::LoadF80(s, {read_operand<uint64_t>(bytes),
                                  read_operand<uint16_t>(bytes + 8)});
        }
      }
      break;
    case 1:
      switch (format) {
        case 0: return write_operand(address, x87::StoreF32(s));
        case 1: return write_operand(address, x87::StoreF64(s));
        case 2: return write_operand(address, x87::StoreI16(s));
        case 3: return write_operand(address, x87::StoreI32(s));
        case 4: return write_operand(address, x87::StoreI64(s));
        case 5: {
          const auto value = x87::StoreF80(s);
          auto* bytes = static_cast<uint8_t*>(address);
          write_operand(bytes, value.sig);
          return write_operand(bytes + 8, value.se);
        }
      }
      break;
    case 2:
      switch (format) {
        case 0: return x87::CompareF32(s, read_operand<uint32_t>(address));
        case 1: return x87::CompareF64(s, read_operand<uint64_t>(address));
        case 2: return x87::CompareI16(s, read_operand<int16_t>(address));
        case 3: return x87::CompareI32(s, read_operand<int32_t>(address));
      }
      break;
    case 11: return x87::StoreEnv(s, static_cast<uint8_t*>(address));
    case 12: return x87::LoadEnv(s, static_cast<const uint8_t*>(address));
    case 13: return x87::Save(s, static_cast<uint8_t*>(address));
    case 14: return x87::Restore(s, static_cast<const uint8_t*>(address));
    default: {
      if (operation < 3 || operation > 8) break;
      const auto op = x87_arithmetic[operation - 3];
      switch (format) {
        case 0: return x87::ArithF32(s, op, read_operand<uint32_t>(address));
        case 1: return x87::ArithF64(s, op, read_operand<uint64_t>(address));
        case 2: return x87::ArithI16(s, op, read_operand<int16_t>(address));
        case 3: return x87::ArithI32(s, op, read_operand<int32_t>(address));
      }
      break;
    }
  }
  __builtin_trap();
}

EMSCRIPTEN_KEEPALIVE void h4_dynamic_x87_register(Ctx* ctx, uint32_t operation,
                                                  uint32_t dst, uint32_t src) {
  auto& s = ctx->fpu;
  if (dst > 7 || src > 7) __builtin_trap();
  switch (operation) {
    case 0: return x87::LoadSt(s, src);
    case 1: return x87::StoreSt(s, src);
    case 2: return x87::CompareSt(s, src, false);
    case 9: return x87::Xch(s, src);
    case 10: return x87::Free(s, src);
    default:
      if (operation < 3 || operation > 8 || (dst && src)) __builtin_trap();
      const auto op = x87_arithmetic[operation - 3];
      if (dst) return x87::ArithStiSt0(s, op, dst);
      return x87::ArithSt0Sti(s, op, src);
  }
}

EMSCRIPTEN_KEEPALIVE void h4_dynamic_x87_misc(Ctx* ctx, uint32_t operation) {
  constexpr auto operations = std::to_array<void (*)(x87::State&)>({
      x87::Chs, x87::Abs, x87::Sqrt, x87::Sin, x87::Cos, x87::RndInt, x87::F2xm1,
      x87::Test, x87::Examine, x87::Patan, x87::Yl2x, x87::Scale, x87::Prem, x87::Prem1,
      x87::Ptan, x87::IncStp, x87::DecStp, x87::ClearExceptions,
  });
  constexpr auto constants = std::to_array({
      x87::Const::One, x87::Const::Zero, x87::Const::Pi, x87::Const::L2E,
      x87::Const::L2T, x87::Const::LG2, x87::Const::LN2,
  });
  if (operation < operations.size()) return operations[operation](ctx->fpu);
  if (operation >= 32 && operation - 32 < constants.size()) {
    return x87::LoadConst(ctx->fpu, constants[operation - 32]);
  }
  __builtin_trap();
}

}  // extern "C"
