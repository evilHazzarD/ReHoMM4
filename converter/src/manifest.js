// The browser converter consumes the same small, auditable manifest as the
// native lifter. It intentionally supports only directives used by ReHoMM4;
// unknown directives fail conversion instead of silently dropping a hook.

const DIRECTIVES = new Set(['entry', 'name', 'probe', 'resume', 'replace']);

function address(word, line) {
  if (!/^[0-9a-f]{6,8}$/i.test(word)) throw new Error(`manifest line ${line}: invalid address ${word}`);
  return Number.parseInt(word, 16);
}

export function parseManifest(text) {
  const rules = [];
  for (const [offset, raw] of String(text).split(/\r?\n/).entries()) {
    const line = offset + 1;
    const words = raw.replace(/#.*/, '').trim().split(/\s+/).filter(Boolean);
    if (!words.length) continue;
    const [kind, at, symbol] = words;
    if (!DIRECTIVES.has(kind)) throw new Error(`manifest line ${line}: unsupported directive ${kind}`);
    if (!at) throw new Error(`manifest line ${line}: ${kind} needs an address`);
    if (kind === 'resume' && symbol) throw new Error(`manifest line ${line}: resume has no symbol`);
    if (kind !== 'resume' && !symbol) throw new Error(`manifest line ${line}: ${kind} needs a symbol`);
    rules.push(Object.freeze({kind, address: address(at, line), symbol: symbol || null}));
  }
  if (!rules.some(rule => rule.kind === 'replace')) throw new Error('manifest has no ReHoMM4 replacements');
  return Object.freeze(rules);
}

export function manifestPlan(rules) {
  const count = kind => rules.filter(rule => rule.kind === kind).length;
  return Object.freeze({
    entries: count('entry'),
    replacements: count('replace'),
    resumes: count('resume'),
    rules,
  });
}
