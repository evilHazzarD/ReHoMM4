import init, {emit_web} from './rehomm4_emit_wasm.js';

async function assetText(relativePath) {
  const response = await fetch(new URL(relativePath, import.meta.url));
  if (!response.ok) throw new Error(`Unable to load converter asset ${relativePath}`);
  return response.text();
}

self.addEventListener('message', async ({data}) => {
  if (data?.type !== 'emit') return;
  try {
    if (!(data.exe instanceof File)) throw new Error('Select a local heroes4.exe.');
    const [exe, manifest, sites] = await Promise.all([
      data.exe.arrayBuffer(), assetText('h4/game.toml'), assetText('h4/resume-sites.txt'), init(),
    ]);
    const resume = Uint32Array.from(sites.trim().split(/\s+/).filter(Boolean),
                                  site => Number.parseInt(site, 16));
    const digestBytes = new Uint8Array(await crypto.subtle.digest('SHA-256', exe));
    const digest = Array.from(digestBytes, byte => byte.toString(16).padStart(2, '0')).join('');
    if (data.expectedDigest && digest !== data.expectedDigest) {
      throw new Error('The installed executable changed. Restart and try again.');
    }
    const code = emit_web(new Uint8Array(exe), manifest, resume);
    const root = await navigator.storage.getDirectory();
    const app = await root.getDirectoryHandle('ReHoMM4', {create: true});
    const file = await app.getFileHandle(`code-${digest}.wasm`, {create: true});
    const writer = await file.createWritable();
    try {
      await writer.write(code);
      await writer.close();
    } catch (error) {
      await writer.abort().catch(() => {});
      throw error;
    }
    self.postMessage({type: 'done', size: code.byteLength, digest});
  } catch (error) {
    self.postMessage({type: 'error', message: String(error?.message ?? error)});
  }
}, {once: true});
