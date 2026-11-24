import assert from 'node:assert/strict';
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
console.log('converter validator: ok');
