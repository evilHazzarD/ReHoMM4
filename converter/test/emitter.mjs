import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
import {createLiftedRegistry, instantiateLiftedCode} from '../../web/src/dynamic_loader.mjs';

const manifest = fileURLToPath(new URL('../emitter/Cargo.toml', import.meta.url));
const bytes = execFileSync('cargo', ['run', '--locked', '--quiet', '--manifest-path', manifest,
  '--example', 'smoke_fixture'], {stdio: ['ignore', 'pipe', 'inherit']});
const memory = new WebAssembly.Memory({initial: 1, maximum: 65536, shared: true});
const state = new DataView(memory.buffer);
const guestBase = 0x2000;
const context = 0x1000;
state.setUint32(context, 11, true);
state.setUint32(context + 32, 0x80, true);
state.setUint32(guestBase + 0x80, 0x402000, true);
state.setUint32(context + 128, 0x606, true);
state.setUint32(guestBase + 0x20, 31, true);
state.setUint32(guestBase + 0x24, 0x3fc00000, true);
state.setUint32(guestBase + 0x40, 3, true);
state.setFloat64(guestBase + 0x60, 1.25, true);
state.setFloat64(guestBase + 0x68, 0.5, true);
const fpu = [];
let controlWord;
let resumeCompleted = false;
const shell = {
  _h4_dynamic_abi_version: () => 1,
  _h4_dynamic_guest_base: () => guestBase,
  _h4_dynamic_interrupt(ctx, from) {
    assert.deepEqual([ctx, from], [context, 0x403000]);
    throw new WebAssembly.RuntimeError('guest interrupt');
  },
  _h4_dynamic_resume(target, ebp, from) {
    assert.deepEqual([target, ebp, from], [0x402000, 0x900, 0x404000]);
    assert.equal(state.getUint32(context + 32, true), 0x80);
    throw new WebAssembly.RuntimeError('guest resume');
  },
  _h4_dynamic_fstcw: () => controlWord,
  _h4_dynamic_fldcw(ctx, word) {
    assert.equal(ctx, context);
    controlWord = word;
  },
  _h4_dynamic_x87_memory(ctx, operation, format, address) {
    assert.equal(ctx, context);
    assert.equal(format, 1);
    switch (operation) {
      case 0: fpu.push(state.getFloat64(address, true)); break;
      case 1: state.setFloat64(address, fpu.at(-1), true); break;
      case 3: fpu[fpu.length - 1] += state.getFloat64(address, true); break;
      default: assert.fail(`Unexpected x87 opcode ${operation}`);
    }
  },
  _h4_dynamic_x87_register(ctx, operation, dst, src) {
    assert.equal(ctx, context);
    assert.equal(src, 0);
    if (operation === 0 && dst === 0) {
      fpu.push(fpu.at(-1));
    } else {
      assert.deepEqual([operation, dst], [3, 1]);
      fpu[fpu.length - 2] += fpu.at(-1);
    }
  },
  _h4_dynamic_x87_misc(ctx, operation) {
    assert.deepEqual([ctx, operation], [context, 0]);
    fpu[fpu.length - 1] *= -1;
  },
  _h4_dynamic_divide_error(ctx, from) {
    assert.deepEqual([ctx, from], [context, 0x401000]);
    throw new WebAssembly.RuntimeError('guest divide error');
  },
  _h4_dynamic_signed_divide(ctx, from, dividend, divisor) {
    assert.deepEqual([ctx, from, dividend, divisor], [context, 0x401000, -17n, 3n]);
    return dividend / divisor;
  },
  _h4_dynamic_dispatch(ctx, target, from) {
    assert.deepEqual([ctx, target, from], [context, 0x402000, 0x401001]);
    assert.equal(state.getUint32(ctx + 32, true), 0x7c);
    assert.equal(state.getUint32(guestBase + 0x7c, true), 0x401006);
    state.setUint32(ctx + 32, 0x80, true);
  },
  _h4_dynamic_dispatch_owned(ctx, target, from, excludedOrigin) {
    assert.equal(excludedOrigin, 0);
    shell._h4_dynamic_dispatch(ctx, target, from);
    return Number(resumeCompleted);
  },
  _h4_dynamic_fninit(ctx) {
    assert.equal(ctx, context);
    fpu.length = 0;
  },
  _h4_dynamic_fld32(ctx, bits) {
    assert.equal(ctx, context);
    fpu.push(bits);
  },
  _h4_dynamic_fst32(ctx) {
    assert.equal(ctx, context);
    return fpu.at(-1);
  },
  _h4_dynamic_fstsw: () => assert.fail('unexpected x87 status read'),
  _h4_dynamic_fpop(ctx) {
    assert.equal(ctx, context);
    fpu.pop();
  },
};
const instance = await instantiateLiftedCode(bytes, memory, shell);
const invoke = createLiftedRegistry(instance, [0x401000, 0x403000, 0x404000]);
assert.equal(invoke(0x401000, context), true);
assert.equal(invoke(0x4fffff, context), false);
assert.throws(() => createLiftedRegistry(instance, [0x4fffff]), /Missing required lifted guest entry/);
assert.equal(state.getUint32(context, true), 0xab2a);
assert.equal(state.getUint32(context + 8, true), 0);
assert.equal(state.getUint32(context + 32, true), 0x8c);
assert.equal(state.getUint32(context + 128, true), 0xe03);
assert.equal(state.getUint32(guestBase + 0x28, true), 0x3fc00000);
assert.equal(state.getUint8(guestBase + 0x2d), 0xab);
assert.equal(state.getUint32(guestBase + 0x30, true), 0x20000000);
assert.equal(state.getUint32(guestBase + 0x34, true), 0xffffffff);
assert.equal(state.getUint32(guestBase + 0x38, true), 0xffffffff);
assert.equal(state.getUint32(guestBase + 0x3c, true), 0);
assert.equal(state.getInt32(guestBase + 0x44, true), -5);
assert.equal(state.getInt32(guestBase + 0x48, true), -2);
assert.equal(state.getUint16(guestBase + 0x58, true), 0x27f);
assert.equal(state.getFloat64(guestBase + 0x70, true), -3.5);
assert.equal(fpu.length, 0);
state.setUint32(context + 32, 0x80, true);
state.setUint32(context + 128, 0x202, true);
instance.exports.f_00401000(context);
assert.equal(state.getUint32(context, true), 0);
assert.equal(state.getUint32(context + 8, true), 0);
assert.equal(state.getUint32(context + 32, true), 0x88);
resumeCompleted = true;
state.setUint32(context + 32, 0x80, true);
instance.exports.f_00401000(context);
assert.equal(state.getUint32(context + 32, true), 0x80);
state.setUint32(guestBase + 0x40, 0, true);
assert.throws(() => instance.exports.f_00401000(context), /guest divide error/);
assert.throws(() => instance.exports.f_00403000(context), /guest interrupt/);
state.setUint32(context + 32, 0x80, true);
state.setUint32(context + 40, 0x900, true);
assert.throws(() => instance.exports.f_00404000(context), /guest resume/);
await assert.rejects(instantiateLiftedCode(bytes, memory, {}), /ABI version/);
await assert.rejects(instantiateLiftedCode(bytes, memory, {_h4_dynamic_abi_version: () => 1}), /missing h4.guest_base/);
console.log('ReHoMM4 WASM emitter and loader: ok');
