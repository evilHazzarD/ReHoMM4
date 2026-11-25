import {mkdir, readFile, writeFile} from 'node:fs/promises';
import {join} from 'node:path';
import {manifestPlan, parseManifest} from '../src/manifest.js';

const output = process.argv[2];
if (!output) throw new Error('Usage: node stage_manifest.mjs OUTPUT_DIRECTORY');

const [target, manifest] = await Promise.all([
  readFile(new URL('../h4/game.toml', import.meta.url), 'utf8'),
  readFile(new URL('../../h4/lifter.manifest', import.meta.url), 'utf8'),
]);
const {discoverySeeds, rules} = manifestPlan(parseManifest(manifest));

// The marked array holds only hints that come from GameRecomp's own discovery.
// Add ReHoMM4's entry, probe and replacement addresses to the same array.
const marker = /^seeds = \[([^\]]*)\] # rehomm4-lifter-seeds$/gm;
const matches = [...target.matchAll(marker)];
if (matches.length !== 1) throw new Error('Expected one discovery seed list in game.toml');
const hints = matches[0][1].match(/0x[\da-f]+/gi) ?? [];
const seeds = [...new Set([...hints.map(Number), ...discoverySeeds])].sort((a, b) => a - b);
const staged = target.replace(marker, `seeds = [${seeds.map(va => `0x${va.toString(16)}`).join(', ')}]`);
const gameDir = join(output, 'h4');
await mkdir(gameDir, {recursive: true});
await writeFile(join(gameDir, 'game.toml'), staged);
const resumeSites = rules.filter(rule => rule.kind === 'resume')
  .map(rule => rule.address.toString(16));
await writeFile(join(gameDir, 'resume-sites.txt'), resumeSites.map(site => site + '\n').join(''));
// The addresses ReHoMM4 replaces: calls to them go through the runtime, the rest are direct.
const hookedSites = rules.filter(rule => rule.kind === 'replace').map(rule => rule.address.toString(16));
await writeFile(join(gameDir, 'hooked-sites.txt'), hookedSites.map(site => site + '\n').join(''));
