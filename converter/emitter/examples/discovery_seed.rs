//! Writes the discovery seed of a supported heroes4.exe (converter/h4/discover-<sha256>.txt):
//! the browser converter starts its discovery from it (one round instead of about sixteen).
//! Checks that the seeded discovery gives the same code, and prints the times.
//!   discovery_seed <heroes4.exe> <staged h4 dir (game.toml)> <out dir>
use std::time::Instant;

fn main() -> Result<(), String> {
    let args: Vec<String> = std::env::args().collect();
    let [_, exe, staged, out] = args.as_slice() else {
        return Err("usage: discovery_seed <heroes4.exe> <staged h4 dir> <out dir>".into());
    };
    if rehomm4_emit_wasm::discovery::DISCOVERY_VERSION.is_empty() {
        return Err("H4_DISCOVERY_VERSION is not set (converter/tools/discovery_seed.sh sets it)".into());
    }
    let exe = std::fs::read(exe).map_err(|e| e.to_string())?;
    let manifest = std::fs::read_to_string(format!("{staged}/game.toml")).map_err(|e| e.to_string())?;
    let t = Instant::now();
    let (full, seed) = rehomm4_emit_wasm::discovery::lift_main_seeded(&manifest, &exe, "")?;
    let full_time = t.elapsed();
    let t = Instant::now();
    let (seeded, again) = rehomm4_emit_wasm::discovery::lift_main_seeded(&manifest, &exe, &seed)?;
    let seeded_time = t.elapsed();
    let sites = |name: &str| -> Result<std::collections::BTreeSet<u32>, String> {
        let text = std::fs::read_to_string(format!("{staged}/{name}")).map_err(|e| e.to_string())?;
        text.split_whitespace().map(|a| u32::from_str_radix(a, 16).map_err(|e| e.to_string())).collect()
    };
    let (resume, hooked) = (sites("resume-sites.txt")?, sites("hooked-sites.txt")?);
    if again != seed
        || rehomm4_emit_wasm::emit_linked(&full, &resume, &hooked)?
            != rehomm4_emit_wasm::emit_linked(&seeded, &resume, &hooked)?
    {
        return Err("the seeded discovery gives other code".into());
    }
    let sha = seed.lines().find_map(|l| l.strip_prefix("sha256 ")).unwrap_or_default();
    let path = format!("{out}/discover-{sha}.txt");
    std::fs::write(&path, &seed).map_err(|e| e.to_string())?;
    println!(
        "{path}: {} bytes; {} functions; whole discovery {:.1} s, seeded {:.1} s",
        seed.len(),
        full.len(),
        full_time.as_secs_f64(),
        seeded_time.as_secs_f64()
    );
    Ok(())
}
