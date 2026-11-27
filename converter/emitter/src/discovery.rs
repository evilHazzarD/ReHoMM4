//! Connect the staged ReHoMM4 target to GameRecomp's PE discovery and IR.

use std::collections::{BTreeMap, BTreeSet};
use std::path::Path;

use rc_convert::{lift, rc_manifest};
use rc_discover::{Seed, Stage};
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
    let (loaded, seed) = load_main(manifest_text, exe, seed_text, Box::new(|_| {}))?;
    progress(0.5);
    let functions = lift::lift_map(&loaded).functions;
    progress(1.0);
    Ok((functions, seed))
}

/// The selected main executable discovered (as [`lift_main_seeded`]), its functions not lifted
/// yet: [`Prepared::lift`] makes each one, in the order and with the fields of
/// `lift::lift_map`'s, so that they can be lowered one at a time.
pub struct Prepared {
    loaded: lift::Loaded,
    /// Each function's address and the code map function whose body it is (an entry at a label
    /// inside it when the address is not that function's own), ascending.
    items: Vec<(u32, usize)>,
    excepts: BTreeMap<u32, u32>,
    continuations: BTreeMap<u32, u32>,
    /// The seed text of this discovery.
    pub seed: String,
}

impl Prepared {
    pub fn addrs(&self) -> Vec<u32> {
        self.items.iter().map(|(addr, _)| *addr).collect()
    }

    pub fn lift(&self, i: usize) -> Function {
        let (addr, body) = self.items[i];
        let map = &self.loaded.map;
        let mut f = rc_x86::lift_entry(&self.loaded.image, map, &map.functions[body], addr);
        f.except_block = self.excepts.contains_key(&f.addr);
        let own: BTreeSet<u32> = f.insns.iter().map(|i| i.addr).collect();
        f.continuations = self
            .continuations
            .iter()
            .filter(|(_, frame)| own.contains(frame))
            .map(|(c, _)| *c)
            .collect();
        f
    }
}

/// `progress`: how far the discovery is (0 to 1), now and then, when the seed says how many
/// instructions it ends with.
pub fn prepare_main(
    manifest_text: &str,
    exe: &[u8],
    seed_text: &str,
    progress: Box<dyn FnMut(f64)>,
) -> Result<Prepared, String> {
    let (loaded, seed) = load_main(manifest_text, exe, seed_text, progress)?;
    let map = &loaded.map;
    let mut items: Vec<(u32, usize)> =
        map.functions.iter().enumerate().map(|(i, f)| (f.entry.0, i)).collect();
    let position: std::collections::HashMap<*const rc_discover::Function, usize> =
        map.functions.iter().enumerate().map(|(i, f)| (f as *const _, i)).collect();
    for (va, f) in lift::label_entries(map) {
        items.push((va, position[&(f as *const _)]));
    }
    // (stable: a function before a label entry at the same address, as lift_map's sort)
    items.sort_by_key(|(addr, _)| *addr);
    let excepts = lift::except_blocks(&loaded);
    let continuations = lift::continuations(&loaded);
    Ok(Prepared { loaded, items, excepts, continuations, seed })
}

/// The executable loaded and discovered (lift::load, with the discovery seeded), and the seed
/// text of that discovery.
fn load_main(
    manifest_text: &str,
    exe: &[u8],
    seed_text: &str,
    mut progress: Box<dyn FnMut(f64)>,
) -> Result<(lift::Loaded, String), String> {
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
    let parsed = parse_seed(seed_text, &digest, &manifest_sha);
    // The instructions the discovery ends with (the seed's "insns"): progress is the share of
    // them decoded so far.
    let expected = parsed.as_ref().map_or(0, |(_, insns)| *insns);
    let decoded = std::rc::Rc::new(std::cell::Cell::new(0usize));
    let hook = {
        let decoded = decoded.clone();
        let mut shown = 0.0f64;
        Box::new(move |stage: Stage, done: usize, total: usize| {
            // Where each stage starts, and its share (seeded, in a browser: decoding a third of
            // the time, the bodies a third, the code map the rest).
            let (start, share, part) = match stage {
                Stage::Decode => {
                    decoded.set(done);
                    let part = if expected > 0 { done as f64 / expected as f64 } else { 0.0 };
                    (0.0, 0.35, part)
                }
                Stage::Bodies => (0.35, 0.35, done as f64 / total.max(1) as f64),
                Stage::MapFunctions => (0.7, 0.15, done as f64 / total.max(1) as f64),
                Stage::MapInsns => (0.85, 0.15, done as f64 / total.max(1) as f64),
            };
            let now = start + share * part.min(1.0);
            if now > shown {
                shown = now;
                progress(now);
            }
        })
    };
    let (map, seed) = rc_discover::with_progress(hook, || match parsed {
        Some((seed, _)) => (
            rc_discover::discover_seeded(&image, &opts, &main.name, &digest, &seed),
            seed,
        ),
        None => rc_discover::discover_seed(&image, &opts, &main.name, &digest),
    });
    let seed = seed_text_of(&seed, &digest, &manifest_sha, decoded.get());
    let loaded = lift::Loaded {
        image,
        map,
        known: Some(known),
        sha256: digest,
    };
    Ok((loaded, seed))
}

/// A seed as text: the discovery version, executable and staged manifest it is for, then its
/// address lists.
fn seed_text_of(seed: &Seed, digest: &str, manifest_sha: &str, insns: usize) -> String {
    let list = |s: &BTreeSet<u32>| {
        s.iter().map(|a| format!("{a:x}")).collect::<Vec<_>>().join(" ")
    };
    format!(
        "version {DISCOVERY_VERSION}\nsha256 {digest}\nmanifest {manifest_sha}\ninsns {insns}\ndeny {}\nno_return {}\njoins {}\n",
        list(&seed.deny),
        list(&seed.no_return),
        list(&seed.joins)
    )
}

/// The seed in `text` if it is one for `digest` and the manifest by this discovery code, and the
/// instructions its discovery ended with (0: not said).
fn parse_seed(text: &str, digest: &str, manifest_sha: &str) -> Option<(Seed, usize)> {
    if DISCOVERY_VERSION.is_empty() {
        return None;
    }
    let mut seed = Seed::default();
    let (mut version, mut sha, mut manifest, mut insns) = (false, false, false, 0);
    for line in text.lines() {
        let (key, rest) = line.split_once(' ').unwrap_or((line, ""));
        let list = || -> Option<BTreeSet<u32>> {
            rest.split_whitespace().map(|a| u32::from_str_radix(a, 16).ok()).collect()
        };
        match key {
            "version" => version = rest == DISCOVERY_VERSION,
            "sha256" => sha = rest == digest,
            "manifest" => manifest = rest == manifest_sha,
            "insns" => insns = rest.parse().unwrap_or(0),
            "deny" => seed.deny = list()?,
            "no_return" => seed.no_return = list()?,
            "joins" => seed.joins = list()?,
            _ => {}
        }
    }
    (version && sha && manifest).then_some((seed, insns))
}
