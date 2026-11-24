//! Connect the staged ReHoMM4 target to GameRecomp's PE discovery and IR.

use std::path::Path;

use rc_convert::{lift, rc_manifest};
use rc_ir::Function;

/// Lift the selected main executable. The caller passes a staged ReHoMM4 manifest and the
/// bytes of the executable it selected locally.
pub fn lift_main(manifest_text: &str, exe: &[u8]) -> Result<Vec<Function>, String> {
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
    let loaded = lift::load(exe, &main.name, Some(known), None, true)?;
    Ok(lift::lift_map(&loaded).functions)
}
