// This closure is created on every Emscripten worker. Native code publishes an
// immutable byte buffer in shared memory before the guest creates any threads.
Module['h4InstallLifted'] = (address, length, hooks, count, stride) => {
  const bytes = new Uint8Array(wasmMemory.buffer, address >>> 0, length >>> 0);
  const compiled = new WebAssembly.Module(bytes);
  const instance = instantiateLiftedModule(compiled, wasmMemory, Module);
  const view = new DataView(wasmMemory.buffer);
  const required = [];
  for (let index = 0; index < count; ++index) {
    required.push(view.getUint32((hooks >>> 0) + index * stride, true));
  }
  return (Module['h4InvokeLifted'] = createLiftedRegistry(instance, required));
};
