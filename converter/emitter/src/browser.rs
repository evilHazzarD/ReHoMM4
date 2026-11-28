use std::collections::BTreeSet;
use wasm_bindgen::prelude::*;

#[wasm_bindgen]
extern "C" {
    /// How far the conversion is, 0 to 1 (the worker shows it: emit_worker.mjs).
    #[wasm_bindgen(js_namespace = globalThis, js_name = h4EmitProgress)]
    fn emit_progress(fraction: f64);
    /// A piece of the converted module at its offset (the worker writes it to the file at once:
    /// a view of this module's memory, good until the converter goes on).
    #[wasm_bindgen(js_namespace = globalThis, js_name = h4EmitWrite)]
    fn emit_write(at: f64, bytes: &[u8]);
    /// A panic's message (the worker adds it to the error: the browser says only "unreachable").
    #[wasm_bindgen(js_namespace = globalThis, js_name = h4EmitPanic)]
    fn emit_panic(message: &str);
}

/// Convert a previously extracted local, allowlisted executable on a browser worker.
#[wasm_bindgen]
pub fn emit_web(
    exe: &[u8],
    manifest: &str,
    resume_sites: &[u32],
    hooked_sites: &[u32],
    discovery_seed: &str,
) -> Result<f64, JsValue> {
    std::panic::set_hook(Box::new(|info| emit_panic(&info.to_string())));
    let error = |message: String| JsValue::from_str(&message);
    // The seed (h4/discover-<sha256>.txt, "" if there is none) saves nine tenths of the time.
    // Discovery about half the time; then each function lifted and lowered in turn (the
    // whole game's IR at once took most of a gigabyte: a phone's browser stopped the worker).
    emit_progress(0.02);
    let prepared = crate::discovery::prepare_main(
        manifest,
        exe,
        discovery_seed,
        Box::new(|p| emit_progress(0.02 + 0.48 * p)),
    )
    .map_err(error)?;
    emit_progress(0.5);
    let sites: BTreeSet<_> = resume_sites.iter().copied().collect();
    let hooked: BTreeSet<_> = hooked_sites.iter().copied().collect();
    // (handed to the worker in pieces: h4EmitWrite; the module's size given back)
    crate::emit_linked_each_to(
        &prepared.addrs(),
        &mut |i| Ok(std::borrow::Cow::Owned(prepared.lift(i))),
        &sites,
        &hooked,
        &mut |p| emit_progress(0.5 + 0.47 * p),
        &mut |at, bytes| emit_write(at as f64, bytes),
    )
    .map(|size| size as f64)
    .map_err(error)
}
