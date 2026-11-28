import init, {emit_web} from './rehomm4_emit_wasm.js';

async function assetText(relativePath) {
  const response = await fetch(new URL(relativePath, import.meta.url));
  if (!response.ok) throw new Error(`Unable to load converter asset ${relativePath}`);
  return response.text();
}

self.addEventListener('message', async ({data}) => {
  if (data?.type !== 'emit') return;
  // The converter's progress (0 to 1; its last 3% saving the code), to the page.
  // How far it got, its panic's message and its memory: said with an error (a panic, or memory
  // the browser would not give, is only "Unreachable code should not be executed" otherwise).
  let reached = 0, panicked = '', wasm = null;
  globalThis.h4EmitProgress = fraction => {
    reached = fraction;
    self.postMessage({type: 'progress', fraction});
  };
  globalThis.h4EmitPanic = message => { panicked = message; };
  try {
    if (!(data.exe instanceof File)) throw new Error('Select a local heroes4.exe.');
    const [exe, manifest, sites, hookedSites] = await Promise.all([
      data.exe.arrayBuffer(), assetText('h4/game.toml'), assetText('h4/resume-sites.txt'),
      assetText('h4/hooked-sites.txt'), init().then(w => { wasm = w; }),
    ]);
    const addresses = text => Uint32Array.from(text.trim().split(/\s+/).filter(Boolean),
                                               site => Number.parseInt(site, 16));
    const resume = addresses(sites);
    const hooked = addresses(hookedSites);
    const digestBytes = new Uint8Array(await crypto.subtle.digest('SHA-256', exe));
    const digest = Array.from(digestBytes, byte => byte.toString(16).padStart(2, '0')).join('');
    if (data.expectedDigest && digest !== data.expectedDigest) {
      throw new Error('The installed executable changed. Restart and try again.');
    }
    // Where the discovery of this executable ends (converter/tools/discovery_seed.sh): addresses
    // only. Without it (another executable) the conversion takes ten times as long.
    const seed = await assetText(`h4/discover-${digest}.txt`).catch(() => '');
    const root = await navigator.storage.getDirectory();
    const app = await root.getDirectoryHandle('ReHoMM4', {create: true});
    // The emitter's output version, as web/src/dynamic_host.cpp: another version is converted again.
    const name = `code-3-${digest}.wasm`;
    const file = await app.getFileHandle(name, {create: true});
    // The code written as the converter makes it, in pieces (h4EmitWrite): never whole in this
    // worker's memory. (a worker: the sync access handle every browser with this storage has;
    // Safari before 26 has no createWritable. Without one: the pieces kept, then written.)
    const h = typeof file.createSyncAccessHandle === 'function' ? await file.createSyncAccessHandle() : null;
    const kept = [];
    let size = 0;
    try {
      if (h) h.truncate(0);
      globalThis.h4EmitWrite = (at, bytes) => {
        if (!h) { kept.push([at, bytes.slice()]); return; }
        for (let off = 0; off < bytes.length;) {
          const n = h.write(bytes.subarray(off), {at: at + off});
          if (!(n > 0)) throw new Error('The browser did not write the game code (no space?).');
          off += n;
        }
      };
      size = emit_web(new Uint8Array(exe), manifest, resume, hooked, seed);
      self.postMessage({type: 'progress', fraction: 0.97});
      if (h) {
        h.flush();
        if (h.getSize() !== size) throw new Error(`The game code was written short (${h.getSize()} of ${size} bytes).`);
        h.close();
      } else {
        const code = new Uint8Array(size);
        for (const [at, bytes] of kept) code.set(bytes, at);
        kept.length = 0;
        const writer = await file.createWritable();
        try {
          await writer.write(code);
          await writer.close();
        } catch (error) {
          await writer.abort().catch(() => {});
          throw error;
        }
      }
    } catch (error) {
      try { h?.close(); } catch { /* closed */ }
      await app.removeEntry(name).catch(() => {});  // (no half-written code left to load)
      throw error;
    }
    // Earlier versions' code (and other executables'): about 140 MB each.
    for await (const [entry] of app.entries()) {
      if (entry !== name && /^code-.*\.wasm$/.test(entry)) await app.removeEntry(entry).catch(() => {});
    }
    self.postMessage({type: 'done', size, digest});
  } catch (error) {
    let message = String(error?.message ?? error);
    if (panicked) message += ` (${panicked})`;
    if (wasm?.memory) {
      message += ` at ${Math.round(reached * 100)}%, the converter's memory ` +
                 `${Math.round(wasm.memory.buffer.byteLength / 1048576)} MiB`;
    }
    self.postMessage({type: 'error', message});
  }
}, {once: true});
