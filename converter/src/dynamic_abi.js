// ABI expected by the dynamic core's generated code.wasm.  Keeping this list
// here makes the boundary explicit: a module is not usable by ReHoMM4 until
// the web runtime provides every import below from the same WebAssembly
// memory/table that the game uses.
export const DYNAMIC_WASM_IMPORTS = Object.freeze([
  ["rc", "dispatch", "function"], ["rc", "trap", "function"],
  ["rc", "cpuid", "function"], ["rc", "rdtsc", "function"],
  ["rc", "fpu", "function"], ["rc", "fstsw", "function"],
  ["rc", "point", "function"], ["rc", "mmx", "function"],
  ["rc", "dispatch_jump", "function"], ["rc", "unwind_claim", "function"],
  ["rc", "unwind_begin", "function"], ["env", "memory", "memory"],
  ["env", "table", "table"], ["env", "table_base", "global"],
]);

export function inspectDynamicWasm(bytes) {
  let module;
  try {
    module = new WebAssembly.Module(bytes);
  } catch (error) {
    throw new Error(`generated code.wasm is invalid: ${error.message}`);
  }
  const found = new Set(WebAssembly.Module.imports(module)
    .map(({ module: mod, name, kind }) => `${mod}:${name}:${kind}`));
  const missing = DYNAMIC_WASM_IMPORTS
    .filter(([mod, name, kind]) => !found.has(`${mod}:${name}:${kind}`))
    .map(([mod, name, kind]) => `${mod}.${name} (${kind})`);
  return { module, missing };
}

export function requireDynamicRuntime(bytes) {
  const inspected = inspectDynamicWasm(bytes);
  if (inspected.missing.length) {
    throw new Error(`ReHoMM4 dynamic runtime adapter is missing: ${inspected.missing.join(", ")}`);
  }
  return inspected.module;
}
