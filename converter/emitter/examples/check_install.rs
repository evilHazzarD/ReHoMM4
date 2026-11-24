use std::{
    collections::{BTreeMap, BTreeSet},
    env, fs, io,
    io::Write,
    path::Path,
};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut paths = env::args_os().skip(1);
    let usage = "usage: check_install STAGED_GAME_TOML HEROES4_EXE [--coverage | --output PATH]";
    let (Some(manifest), Some(exe)) = (paths.next(), paths.next()) else {
        return Err(usage.into());
    };
    let (coverage, output) = match (paths.next(), paths.next(), paths.next()) {
        (None, None, None) => (false, None),
        (Some(option), None, None) if option == "--coverage" => (true, None),
        (Some(option), Some(output), None) if option == "--output" => (false, Some(output)),
        _ => return Err(usage.into()),
    };
    let manifest = Path::new(&manifest);
    let resume_sites = fs::read_to_string(manifest.with_file_name("resume-sites.txt"))?
        .lines()
        .map(|line| u32::from_str_radix(line, 16))
        .collect::<Result<BTreeSet<_>, _>>()?;
    let functions =
        rehomm4_emit_wasm::discovery::lift_main(&fs::read_to_string(manifest)?, &fs::read(exe)?)
            .map_err(io::Error::other)?;
    println!("Discovered {} lifted entries", functions.len());
    let mut undiscovered = resume_sites.clone();
    for instruction in functions.iter().flat_map(|function| &function.insns) {
        undiscovered.remove(&instruction.addr);
        if undiscovered.is_empty() {
            break;
        }
    }
    if !undiscovered.is_empty() {
        return Err(format!("resume sites missing from discovery: {undiscovered:#x?}").into());
    }
    if coverage {
        let mut supported = 0;
        let mut unsupported = BTreeMap::<String, (u32, String)>::new();
        for function in &functions {
            match rehomm4_emit_wasm::emit_with_resume_sites(
                std::slice::from_ref(function),
                &resume_sites,
            ) {
                Ok(_) => supported += 1,
                Err(error) => {
                    let reason = error.rsplit_once(": ").map_or_else(
                        || {
                            if error.starts_with("exception frame ") {
                                "exception frame"
                            } else if error.ends_with(" has no return") {
                                "function has no return"
                            } else {
                                error.as_str()
                            }
                        },
                        |(_, reason)| reason,
                    );
                    let entry = unsupported.entry(reason.to_owned()).or_insert((0, error));
                    entry.0 += 1;
                }
            }
        }
        println!(
            "Encoded {supported}/{} entries individually",
            functions.len()
        );
        for (_, (count, example)) in unsupported {
            println!("{count:6}  {example}");
        }
        return Ok(());
    }
    let code = rehomm4_emit_wasm::emit_with_resume_sites(&functions, &resume_sites)
        .map_err(io::Error::other)?;
    println!("Encoded {} bytes of WebAssembly", code.len());
    if let Some(output) = output {
        fs::File::create_new(output)?.write_all(&code)?;
    }
    Ok(())
}
