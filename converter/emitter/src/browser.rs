use std::collections::BTreeSet;
use wasm_bindgen::prelude::*;

/// Convert a previously extracted local, allowlisted executable on a browser worker.
#[wasm_bindgen]
pub fn emit_web(exe: &[u8], manifest: &str, resume_sites: &[u32]) -> Result<Vec<u8>, JsValue> {
    let error = |message: String| JsValue::from_str(&message);
    let functions = crate::discovery::lift_main(manifest, exe).map_err(error)?;
    let sites: BTreeSet<_> = resume_sites.iter().copied().collect();
    crate::emit_with_resume_sites(&functions, &sites).map_err(error)
}
