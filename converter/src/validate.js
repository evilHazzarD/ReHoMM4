// Browser-side input validation for the ReHoMM4 converter.  This module has no
// game data: it only recognises the public hash of the supported GOG executable.

export const SUPPORTED_GOG_30 = Object.freeze({
  b171788633a63c38e28016b379ae97e033c880d259a5103fd006adcfcd4e4c57: {language: 'de', variant: 'gog-de-76223'},
  b7c2f6caeeba640b1c5f1d812b172938bb4dd4ec62e34ec93ad827bd1ffaec87: {language: 'en', variant: 'gog-en-76223'},
  a6f8d14af6add9e630addd37df29a7803f074f88fbc58a744817ae860cfa5701: {language: 'fr', variant: 'gog-fr-76223'},
  '6a3c699d2d3d6e96d8f6fdc9c17fde6e03126a3073d1b7a754cb51bd5b32a023': {language: 'it', variant: 'gog-it-76223'},
  bfc9eb079fbc3c74c879a9e90486479970f2a7cd450a6adad31ca12372809e93: {language: 'es', variant: 'gog-es-76223'},
});

const POLISH = /\b(polish|polski|polska)\b/i;

export function hex(bytes) {
  return Array.from(new Uint8Array(bytes), b => b.toString(16).padStart(2, '0')).join('');
}

export async function sha256(file) {
  if (!file || typeof file.arrayBuffer !== 'function') throw new TypeError('a browser File is required');
  return hex(await crypto.subtle.digest('SHA-256', await file.arrayBuffer()));
}

// The installer is allowed to have GOG's changing version suffix, but conversion
// needs both halves. Polish is intentionally unsupported (rather than silently
// producing a build with an incompatible executable/data pair).
export function validateInstallerNames(files) {
  const names = [...files].map(f => String(f.name || '')).filter(Boolean);
  if (names.some(name => POLISH.test(name)))
    return {ok: false, error: 'Polish GOG installers are not supported by the browser converter.'};
  const setup = names.find(name => /^setup_heroes_of_might_and_magic_4_complete_.*\.exe$/i.test(name));
  const bin = names.find(name => /^setup_heroes_of_might_and_magic_4_complete_.*-1\.bin$/i.test(name));
  if (!setup || !bin)
    return {ok: false, error: 'Choose both GOG offline-installer files: setup_….exe and setup_…-1.bin.'};
  return {ok: true, setup, bin};
}

// A serialisable hand-off from the unpacker to the dynamic lifter. Keeping this
// deliberately declarative ensures the Pages worker cannot accidentally upload
// the player-owned executable or generated result.
export async function makeBuildPlan({exe, installerFiles}) {
  const names = validateInstallerNames(installerFiles);
  if (!names.ok) throw new Error(names.error);
  const digest = await sha256(exe);
  const supported = SUPPORTED_GOG_30[digest];
  if (!supported)
    throw new Error('heroes4.exe is not a supported GOG Complete 3.0 executable (DE, EN, FR, IT or ES).');
  return Object.freeze({
    format: 1,
    game: `heroes4-gog-complete-3.0-${supported.language}`,
    language: supported.language,
    variant: supported.variant,
    exeSha256: digest,
    manifest: 'h4/lifter.manifest',
    targets: ['web', 'windows', 'android', 'linux'],
  });
}
