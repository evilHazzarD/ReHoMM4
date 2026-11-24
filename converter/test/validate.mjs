import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {manifestPlan, parseManifest} from '../src/manifest.js';
import {fileOffsetForVa, parsePe32, validateManifestAddresses} from '../src/pe.js';
import {prepareConversion} from '../src/session.js';
import {SUPPORTED_GOG_30, validateInstallerNames} from '../src/validate.js';

const valid = validateInstallerNames([
  {name: 'setup_heroes_of_might_and_magic_4_complete_3.0_win11_gog_0.2_(76223).exe'},
  {name: 'setup_heroes_of_might_and_magic_4_complete_3.0_win11_gog_0.2_(76223)-1.bin'},
]);
assert.equal(valid.ok, true);

const polish = validateInstallerNames([
  {name: 'setup_heroes_of_might_and_magic_4_complete_3.0_(polish)_(76223).exe'},
  {name: 'setup_heroes_of_might_and_magic_4_complete_3.0_(polish)_(76223)-1.bin'},
]);
assert.equal(polish.ok, false);
assert.match(polish.error, /Polish/);
assert.match(validateInstallerNames([{name: 'setup_heroes_of_might_and_magic_4_complete_polski_76223.exe'}]).error, /Polish/);
assert.match(validateInstallerNames([
  {name: valid.setup},
  {name: 'setup_heroes_of_might_and_magic_4_complete_3.0_(german)_(76223)-1.bin'},
]).error, /same GOG offline installer/);

assert.deepEqual(Object.values(SUPPORTED_GOG_30).map(v => v.language).sort(), ['de', 'en', 'es', 'fr', 'it']);
assert.equal(Object.keys(SUPPORTED_GOG_30).length, 5);

assert.equal(validateInstallerNames([]).ok, false);

const manifest = parseManifest(await readFile(new URL('../../h4/lifter.manifest', import.meta.url), 'utf8'));
const plan = manifestPlan(manifest);
assert.ok(plan.replacements > 20);
assert.ok(manifest.some(rule => rule.kind === 'replace' && rule.symbol === 'set_mode'));
const optionalNames = manifestPlan(parseManifest(
  'entry 00401000\nprobe 00401000\nname 00401000 a diagnostic label\nreplace 00402000\nresume 00403000',
));
assert.deepEqual(optionalNames.discoverySeeds, [0x401000, 0x402000]);
assert.equal(optionalNames.rules[1].symbol, 'probe');
assert.equal(optionalNames.rules[2].symbol, 'a diagnostic label');
assert.throws(() => parseManifest('resume 00403000 unexpected'), /resume has no symbol/);
assert.throws(() => parseManifest('polish 008D3750 nope'), /unsupported directive/);

const fixture = new Uint8Array(0x400);
const view = new DataView(fixture.buffer);
view.setUint16(0, 0x5a4d, true); view.setUint32(0x3c, 0x80, true);
view.setUint32(0x80, 0x4550, true); view.setUint16(0x86, 1, true); view.setUint16(0x94, 0xe0, true);
view.setUint16(0x98, 0x10b, true); view.setUint32(0xb4, 0x400000, true);
fixture.set(new TextEncoder().encode('.text'), 0x178);
view.setUint32(0x180, 0x200, true); view.setUint32(0x184, 0x1000, true);
view.setUint32(0x188, 0x200, true); view.setUint32(0x18c, 0x200, true);
const pe = parsePe32(fixture);
assert.equal(fileOffsetForVa(pe, 0x401020), 0x220);
assert.equal(fileOffsetForVa(pe, 0x402000), null);
assert.equal(validateManifestAddresses(pe, [{address: 0x401020}]), true);
assert.throws(() => validateManifestAddresses(pe, [{address: 0x402000}]), /outside executable/);
await assert.rejects(
  prepareConversion({exe: {arrayBuffer: async () => fixture.buffer}, installerFiles: [], manifestText: 'replace 00401020 test'}),
  /Choose both GOG offline-installer files/,
);
console.log('converter validator: ok');
