//! Connect the staged ReHoMM4 target to GameRecomp's PE discovery and IR.

use std::collections::BTreeSet;
use std::path::Path;

use rc_convert::{lift, rc_manifest};
use rc_discover::Seed;
use rc_ir::Function;

/// The discovery code the seeds were written with: a seed (discover-<sha256>.txt) of another
/// version is not used (its map could differ). Set by the build (build_web_emitter.sh: the
/// game-recomp commit and its patches' hash).
pub const DISCOVERY_VERSION: &str = match option_env!("H4_DISCOVERY_VERSION") {
    Some(v) => v,
    None => "",
};

/// Lift the selected main executable. The caller passes a staged ReHoMM4 manifest and the
/// bytes of the executable it selected locally.
pub fn lift_main(manifest_text: &str, exe: &[u8]) -> Result<Vec<Function>, String> {
    lift_main_seeded(manifest_text, exe, "").map(|(functions, _)| functions)
}

/// As [`lift_main`], from `seed_text` ([`seed_text`] of an earlier discovery of the same
/// executable by the same code; "" or another one: the whole discovery, which takes ten times as
/// long). Also gives the seed text of this discovery.
pub fn lift_main_seeded(
    manifest_text: &str,
    exe: &[u8],
    seed_text: &str,
) -> Result<(Vec<Function>, String), String> {
    lift_main_progress(manifest_text, exe, seed_text, &mut |_| {})
}

/// [`lift_main_seeded`], telling `progress` when the discovery is done (0.5: about half the time
/// with a seed) and the lifting (1).
pub fn lift_main_progress(
    manifest_text: &str,
    exe: &[u8],
    seed_text: &str,
    progress: &mut dyn FnMut(f64),
) -> Result<(Vec<Function>, String), String> {
    let manifest = rc_manifest::parse_str(manifest_text, Path::new("h4/game.toml"))
        .map_err(|error| error.to_string())?;
    let digest = rc_manifest::sha256_hex(exe);
    let main = manifest
        .binaries
        .iter()
        .find(|binary| {
            binary.role == rc_manifest::Role::Main
                && binary.treatment == rc_manifest::Treatment::Lift
                && binary.matches(&digest)
        })
        .ok_or("heroes4.exe is not listed in the ReHoMM4 manifest")?;
    let known = lift::Known {
        game: manifest.game.id.clone(),
        binary: main.clone(),
    };
    // lift::load, with the discovery seeded.
    let pe = rc_pe::parse(exe).map_err(|e| format!("{}: {e}", main.name))?;
    let mut image =
        rc_discover::Image::load(exe, &pe, main.base).map_err(|e| format!("{}: {e}", main.name))?;
    for c in &main.patch.code {
        let (from, to) = c.bytes()?;
        image.patch(c.at, &from, &to)?;
    }
    let mut opts = lift::options(Some(&known));
    opts.use_relocs = true;
    let manifest_sha = rc_manifest::sha256_hex(manifest_text.as_bytes());
    let (map, seed) = match parse_seed(seed_text, &digest, &manifest_sha) {
        Some(seed) => (
            rc_discover::discover_seeded(&image, &opts, &main.name, &digest, &seed),
            seed,
        ),
        None => rc_discover::discover_seed(&image, &opts, &main.name, &digest),
    };
    progress(0.5);
    let loaded = lift::Loaded {
        image,
        map,
        known: Some(known),
        sha256: digest.clone(),
    };
    let functions = lift::lift_map(&loaded).functions;
    progress(1.0);
    Ok((functions, seed_text_of(&seed, &digest, &manifest_sha)))
}

/// A seed as text: the discovery version, executable and staged manifest it is for, then its
/// address lists.
fn seed_text_of(seed: &Seed, digest: &str, manifest_sha: &str) -> String {
    let list = |s: &BTreeSet<u32>| {
        s.iter().map(|a| format!("{a:x}")).collect::<Vec<_>>().join(" ")
    };
    format!(
        "version {DISCOVERY_VERSION}\nsha256 {digest}\nmanifest {manifest_sha}\ndeny {}\nno_return {}\njoins {}\n",
        list(&seed.deny),
        list(&seed.no_return),
        list(&seed.joins)
    )
}

/// The seed in `text` if it is one for `digest` and the manifest by this discovery code.
fn parse_seed(text: &str, digest: &str, manifest_sha: &str) -> Option<Seed> {
    if DISCOVERY_VERSION.is_empty() {
        return None;
    }
    let mut seed = Seed::default();
    let (mut version, mut sha, mut manifest) = (false, false, false);
    for line in text.lines() {
        let (key, rest) = line.split_once(' ').unwrap_or((line, ""));
        let list = || -> Option<BTreeSet<u32>> {
            rest.split_whitespace().map(|a| u32::from_str_radix(a, 16).ok()).collect()
        };
        match key {
            "version" => version = rest == DISCOVERY_VERSION,
            "sha256" => sha = rest == digest,
            "manifest" => manifest = rest == manifest_sha,
            "deny" => seed.deny = list()?,
            "no_return" => seed.no_return = list()?,
            "joins" => seed.joins = list()?,
            _ => {}
        }
    }
    (version && sha && manifest).then_some(seed)
}
