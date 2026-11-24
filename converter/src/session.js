import {manifestPlan, parseManifest} from './manifest.js';
import {parsePe32, validateManifestAddresses} from './pe.js';
import {makeBuildPlan} from './validate.js';

// The worker's last pre-lift boundary: after this resolves, all input is known
// to be the supported player-owned executable and every ReHoMM4 hook maps to
// its PE image. The lifter may only consume this returned plan and local bytes.
export async function prepareConversion({exe, installerFiles, manifestText}) {
  if (!exe || typeof exe.arrayBuffer !== 'function') throw new TypeError('heroes4.exe File is required');
  const bytes = await exe.arrayBuffer();
  const pe = parsePe32(bytes);
  const rules = parseManifest(manifestText);
  validateManifestAddresses(pe, rules);
  const build = await makeBuildPlan({exe, installerFiles});
  return Object.freeze({...build, pe: Object.freeze({imageBase: pe.imageBase, sections: pe.sections}),
                        lift: manifestPlan(rules)});
}
