import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {manifestPlan, parseManifest} from '../src/manifest.js';
import {validateInstallerNames} from '../src/validate.js';

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

assert.equal(validateInstallerNames([]).ok, false);

const manifest = parseManifest(await readFile(new URL('../../h4/lifter.manifest', import.meta.url), 'utf8'));
const plan = manifestPlan(manifest);
assert.ok(plan.replacements > 20);
assert.ok(manifest.some(rule => rule.kind === 'replace' && rule.symbol === 'set_mode'));
assert.throws(() => parseManifest('polish 008D3750 nope'), /unsupported directive/);
console.log('converter validator: ok');
