// Minimal PE32 reader used before dynamic lifting.  It reads metadata only;
// the original executable stays in the browser worker's memory/OPFS.

function fail(message) { throw new Error(`PE: ${message}`); }
function range(view, at, bytes) {
  if (!Number.isSafeInteger(at) || at < 0 || at + bytes > view.byteLength) fail('truncated file');
}
function ascii(view, at, bytes) {
  range(view, at, bytes);
  let text = '';
  for (let i = 0; i < bytes && view.getUint8(at + i); ++i) text += String.fromCharCode(view.getUint8(at + i));
  return text;
}

export function parsePe32(bytes) {
  const buffer = bytes instanceof ArrayBuffer ? bytes : bytes.buffer;
  const offset = bytes instanceof ArrayBuffer ? 0 : bytes.byteOffset;
  const length = bytes instanceof ArrayBuffer ? bytes.byteLength : bytes.byteLength;
  if (!(buffer instanceof ArrayBuffer)) fail('an ArrayBuffer or Uint8Array is required');
  const view = new DataView(buffer, offset, length);
  range(view, 0, 0x40);
  if (view.getUint16(0, true) !== 0x5a4d) fail('missing MZ signature');
  const pe = view.getUint32(0x3c, true);
  range(view, pe, 24);
  if (view.getUint32(pe, true) !== 0x00004550) fail('missing PE signature');
  const sections = view.getUint16(pe + 6, true);
  const optionalBytes = view.getUint16(pe + 20, true);
  const optional = pe + 24;
  range(view, optional, optionalBytes);
  if (view.getUint16(optional, true) !== 0x10b) fail('only PE32 executables are supported');
  const imageBase = view.getUint32(optional + 28, true);
  const sectionTable = optional + optionalBytes;
  range(view, sectionTable, sections * 40);
  const out = [];
  for (let i = 0; i < sections; ++i) {
    const at = sectionTable + i * 40;
    const virtualSize = view.getUint32(at + 8, true);
    const virtualAddress = view.getUint32(at + 12, true);
    const rawSize = view.getUint32(at + 16, true);
    const rawOffset = view.getUint32(at + 20, true);
    if (rawSize) range(view, rawOffset, rawSize);
    out.push(Object.freeze({name: ascii(view, at, 8), virtualAddress, virtualSize, rawOffset, rawSize}));
  }
  return Object.freeze({imageBase, sections: Object.freeze(out)});
}

export function fileOffsetForVa(pe, va) {
  const rva = va - pe.imageBase;
  if (!Number.isInteger(rva) || rva < 0) return null;
  for (const section of pe.sections) {
    const span = Math.max(section.virtualSize, section.rawSize);
    if (rva >= section.virtualAddress && rva < section.virtualAddress + span) {
      const delta = rva - section.virtualAddress;
      return delta < section.rawSize ? section.rawOffset + delta : null;
    }
  }
  return null;
}

export function validateManifestAddresses(pe, rules) {
  const missing = rules.filter(rule => fileOffsetForVa(pe, rule.address) === null).map(rule => rule.address);
  if (missing.length) fail(`manifest addresses outside executable sections: ${missing.map(a => a.toString(16)).join(', ')}`);
  return true;
}
