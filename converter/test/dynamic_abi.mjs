import assert from 'node:assert/strict';
import { inspectDynamicWasm } from '../src/dynamic_abi.js';

// An empty, valid WASM module proves the checker reports the ABI gap instead
// of accepting arbitrary WASM as game code.
const empty = new Uint8Array([0, 97, 115, 109, 1, 0, 0, 0]);
const result = inspectDynamicWasm(empty);
assert.equal(result.missing.length, 14);
assert.match(result.missing[0], /^rc\.dispatch/);
console.log('dynamic ABI contract: ok');
