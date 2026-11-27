import init, {emit_web} from './rehomm4_emit_wasm.js';

async function assetText(relativePath) {
  const response = await fetch(new URL(relativePath, import.meta.url));
  if (!response.ok) throw new Error(`Unable to load converter asset ${relativePath}`);
  return response.text();
}

self.addEventListener('message', async ({data}) => {
  if (data?.type !== 'emit') return;
  // The converter's progress (0 to 1; its last 3% saving the code), to the page.
  globalThis.h4EmitProgress = fraction => self.postMessage({type: 'progress', fraction});
  try {
    if (!(data.exe instanceof File)) throw new Error('Select a local heroes4.exe.');
    const [exe, manifest, sites, hookedSites] = await Promise.all([
      data.exe.arrayBuffer(), assetText('h4/game.toml'), assetText('h4/resume-sites.txt'),
      assetText('h4/hooked-sites.txt'), init(),
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
    const code = emit_web(new Uint8Array(exe), manifest, resume, hooked, seed);
    const root = await navigator.storage.getDirectory();
    const app = await root.getDirectoryHandle('ReHoMM4', {create: true});
    // The emitter's output version, as web/src/dynamic_host.cpp: another version is converted again.
    const name = `code-3-${digest}.wasm`;
    const file = await app.getFileHandle(name, {create: true});
    self.postMessage({type: 'progress', fraction: 0.97});
    if (typeof file.createSyncAccessHandle === 'function') {
      // (a worker: the sync access handle every browser with this storage has; Safari before 26
      // has no createWritable)
      const h = await file.createSyncAccessHandle();
      try {
        h.truncate(0);
        for (let at = 0; at < code.byteLength;) {
          const n = h.write(code.subarray(at), {at});
          if (!(n > 0)) throw new Error('The browser did not write the game code (no space?).');
          at += n;
        }
        h.flush();
      } catch (error) {
        h.close();
        await app.removeEntry(name).catch(() => {});  // (no half-written code left to load)
        throw error;
      }
      h.close();
    } else {
      const writer = await file.createWritable();
      try {
        await writer.write(code);
        await writer.close();
      } catch (error) {
        await writer.abort().catch(() => {});
        throw error;
      }
    }
    // Earlier versions' code (and other executables'): about 140 MB each.
    for await (const [entry] of app.entries()) {
      if (entry !== name && /^code-.*\.wasm$/.test(entry)) await app.removeEntry(entry).catch(() => {});
    }
    self.postMessage({type: 'done', size: code.byteLength, digest});
  } catch (error) {
    self.postMessage({type: 'error', message: String(error?.message ?? error)});
  }
}, {once: true});
