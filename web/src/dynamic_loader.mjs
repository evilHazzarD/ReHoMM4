// Instantiate lifted code on the guest worker with the shell's own memory.
// wasmMemory is passed by the worker, where the host exports are callable.
export async function instantiateLiftedCode(bytes, wasmMemory, shell) {
  return instantiateLiftedModule(await WebAssembly.compile(bytes), wasmMemory, shell);
}

export function instantiateLiftedModule(module, wasmMemory, shell) {
  if (!(wasmMemory instanceof WebAssembly.Memory) ||
      typeof SharedArrayBuffer === 'undefined' ||
      !(wasmMemory.buffer instanceof SharedArrayBuffer)) {
    throw new Error('The ReHoMM4 runtime needs shared WebAssembly memory.');
  }
  if (shell._h4_dynamic_abi_version?.() !== 1) {
    throw new Error('The ReHoMM4 dynamic ABI version is unsupported.');
  }

  const h4 = {memory: wasmMemory};
  let importsMemory = false;
  for (const {module: namespace, name, kind} of WebAssembly.Module.imports(module)) {
    if (namespace === 'h4' && name === 'memory' && kind === 'memory') {
      importsMemory = true;
      continue;
    }
    if (namespace !== 'h4' || kind !== 'function' ||
        !['guest_base', 'dispatch', 'dispatch_owned', 'divide_error', 'signed_divide', 'interrupt', 'resume',
          'fninit', 'fld32', 'fst32', 'fstsw', 'fpop', 'fstcw', 'fldcw',
          'x87_memory', 'x87_register', 'x87_misc'].includes(name)) {
      throw new Error(`Unsupported ReHoMM4 dynamic import ${namespace}.${name}.`);
    }
    const handler = shell[`_h4_dynamic_${name}`];
    if (typeof handler !== 'function') {
      throw new Error(`The ReHoMM4 runtime is missing h4.${name}.`);
    }
    h4[name] = handler;
  }
  if (!importsMemory) throw new Error('Lifted code must import h4.memory.');
  return new WebAssembly.Instance(module, {h4});
}

// Each worker owns the instance it invokes, even when workers share memory.
export function createLiftedRegistry(instance, requiredAddresses) {
  const entries = new Map();
  for (const [name, entry] of Object.entries(instance.exports)) {
    if (!/^f_[0-9a-f]{8}$/.test(name) || typeof entry !== 'function') continue;
    entries.set(Number.parseInt(name.slice(2), 16), entry);
  }
  if (!entries.size) throw new Error('The lifted module has no guest entries.');
  for (const address of requiredAddresses) {
    if (!entries.has(address)) {
      throw new Error(`Missing required lifted guest entry 0x${address.toString(16)}.`);
    }
  }
  return (address, context) => {
    const entry = entries.get(address >>> 0);
    if (!entry) return false;
    entry(context);
    return true;
  };
}
