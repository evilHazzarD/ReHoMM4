import {readFile, writeFile} from 'node:fs/promises';
import {parseManifest} from '../../converter/src/manifest.js';

const [manifestPath, outputPath] = process.argv.slice(2);
if (!manifestPath || !outputPath) {
  throw new Error('Usage: node generate_dynamic_hooks.mjs LIFTER_MANIFEST OUTPUT_CPP');
}

const hooks = parseManifest(await readFile(manifestPath, 'utf8'))
  .filter(rule => rule.kind === 'replace')
  .map(rule => rule.address)
  .sort((a, b) => a - b);
if (new Set(hooks).size !== hooks.length) {
  throw new Error('Duplicate replacement addresses in lifter manifest');
}

const hex = address => address.toString(16).toUpperCase().padStart(8, '0');
const wrappers = hooks.map(address => {
  const suffix = hex(address);
  return `void f_${suffix}(recomp::Ctx&);
void f_${suffix}_lifted(recomp::Ctx& ctx) {
  h4_dynamic_original(&ctx, 0x${suffix}u);
}`;
});
const entries = hooks.map(address => `    {0x${hex(address)}u, gen::f_${hex(address)}},`);
await writeFile(outputPath, `// Generated from h4/lifter.manifest.
#include "runtime/lifted.h"

extern "C" void h4_dynamic_original(recomp::Ctx*, uint32_t address);

namespace recomp::gen {
${wrappers.join('\n\n')}
}  // namespace recomp::gen

namespace recomp {
extern const LiftedFn kLiftedFunctions[] = {
${entries.join('\n')}
};
extern const uint32_t kLiftedFunctionCount = sizeof(kLiftedFunctions) / sizeof(kLiftedFunctions[0]);
}  // namespace recomp
`);
