// Prepended to heroes4.js (--pre-js): what the game's threads ask the page for (web_platform.cpp).
// Runs on the page's own thread (MAIN_THREAD_ASYNC_EM_ASM). Module.h4Status, when the page sets
// it (shell/index.html), shows what happens.

// The game's log (stdout, stderr) is written on its workers, where it would only reach each
// worker's own console: it goes to the page too, which shows it in its console and keeps the
// last lines (index.html: shown when the game stops).
if (typeof ENVIRONMENT_IS_PTHREAD !== 'undefined' && ENVIRONMENT_IS_PTHREAD && typeof BroadcastChannel === 'function') {
  const log = new BroadcastChannel('h4-log');
  Module.print = (t) => log.postMessage({ err: false, t });
  Module.printErr = (t) => log.postMessage({ err: true, t });
}

// The game's switches (H4_TRACE=1, H4_LANG=uk, ...: the same as the apps' environment), from the
// page's address: index.html?env=H4_TRACE=1,H4_MAX_FPS=30 (Module.h4Env, set by the page).
Module.preRun = [].concat(Module.preRun || [], () => {
  for (const [k, v] of Object.entries(Module.h4Env || {})) ENV[k] = v;
});

// SDL closing its window (the installer's, before the game opens its own) removes the folder of
// dropped files with FS.rmdir, which WasmFS's FS has not got: the error stopped it there, and the
// page stayed black after an install. That empty folder (in memory, /tmp) is simply kept.
Module.preRun.push(() => {
  if (typeof FS === 'object' && typeof FS.rmdir !== 'function') FS.rmdir = () => {};
});

// The installer's "Install from GOG installer files" / "Use an installed game": the page's file
// input. What is picked is copied into the origin private file system, ReHoMM4/picked/ (the
// module's /persist/ReHoMM4/picked), then the list of it into picked/.done, which the installer
// waits for; an empty list means the picker was cancelled.
//
// The picker is opened by a button on the page: browsers open one only for the player's own tap or
// click (Safari ignores input.click() from here, and the installer then waited for ever). The
// files are written by a worker through sync access handles (Safari has no createWritable).
const PICK_WORKER = `
onmessage = async (e) => {
  const { files } = e.data;  // [{ file, path: [...] }]
  let h = null;
  try {
    const root = await navigator.storage.getDirectory();
    const app = await root.getDirectoryHandle('ReHoMM4', { create: true });
    const dir = await app.getDirectoryHandle('picked', { create: true });
    const names = [];
    const write = async (d, name, chunks) => {
      h = await (await d.getFileHandle(name, { create: true })).createSyncAccessHandle();
      h.truncate(0);
      let at = 0;
      for await (const value of chunks) {
        for (let off = 0; off < value.length;) off += h.write(value.subarray(off), { at: at + off });
        at += value.length;
        if (value.length) postMessage({ bytes: value.length });
      }
      h.flush();
      h.close();
      h = null;
    };
    const stream = async function* (f) {
      const reader = f.stream().getReader();
      for (;;) {
        const { value, done } = await reader.read();
        if (done) return;
        yield value;
      }
    };
    for (const { file, path } of files) {
      let d = dir;
      for (const p of path.slice(0, -1)) d = await d.getDirectoryHandle(p, { create: true });
      postMessage({ name: file.name });
      await write(d, path[path.length - 1], stream(file));
      names.push(path.join('/'));
    }
    postMessage({ names });
  } catch (err) {
    try { if (h) h.close(); } catch (e2) { /* closed */ }
    postMessage({ error: String((err && err.message) || err) });
  }
};
`;
const PICK_DONE_WORKER = `
onmessage = async (e) => {
  let h = null;
  try {
    const root = await navigator.storage.getDirectory();
    const app = await root.getDirectoryHandle('ReHoMM4', { create: true });
    const dir = await app.getDirectoryHandle('picked', { create: true });
    h = await (await dir.getFileHandle('.done', { create: true })).createSyncAccessHandle();
    const bytes = new TextEncoder().encode(e.data);
    h.truncate(0);
    h.write(bytes, { at: 0 });
    h.flush();
    h.close();
    postMessage({ ok: true });
  } catch (err) {
    try { if (h) h.close(); } catch (e2) { /* closed */ }
    postMessage({ error: String((err && err.message) || err) });
  }
};
`;
// The module's shared memory (web/build.sh puts this in place of Emscripten's own
// new WebAssembly.Memory): the largest maximum the browser will reserve, from Emscripten's 4 GiB
// down to what the game needs (its 1.25 GiB arena, and about 400 MiB more). iPad Safari would
// not reserve 4 GiB of shared memory ("Out of memory" before the game started).
// How much memory the module has (it grows as the game asks): in the console at each change.
if (typeof ENVIRONMENT_IS_PTHREAD === 'undefined' || !ENVIRONMENT_IS_PTHREAD) {
  let last = 0;
  setInterval(() => {
    const n = typeof wasmMemory === 'object' && wasmMemory ? wasmMemory.buffer.byteLength >> 20 : 0;
    if (n && n !== last) console.log('h4-web: memory ' + (last = n) + ' MiB');
  }, 5000);
}

Module["h4NewMemory"] = function (initial, maximum) {
  let last;
  // (Testing: ?h4maxmem=<pages> starts from a smaller maximum, as a browser that refuses more.)
  const cap = +(new URLSearchParams(globalThis.location?.search || '').get('h4maxmem') || 0);
  for (const max of [maximum, 36864, 32768, 28672]) {
    if (max > maximum || (cap && max > cap)) continue;
    try {
      const m = new WebAssembly.Memory({ initial, maximum: max, shared: true });
      if (max < maximum) console.warn('h4-web: shared memory of ' + (max / 16) + ' MiB at most');
      return m;
    } catch (e) {
      last = e;
    }
  }
  throw last;
};

function h4RunWorker(source, data, onMessage) {
  return new Promise((resolve) => {
    const url = URL.createObjectURL(new Blob([source], { type: 'text/javascript' }));
    const w = new Worker(url);
    w.onmessage = (e) => {
      if (onMessage) onMessage(e.data);
      if (e.data.names || e.data.error || e.data.ok) { w.terminate(); URL.revokeObjectURL(url); resolve(e.data); }
    };
    w.onerror = (e) => { w.terminate(); URL.revokeObjectURL(url); resolve({ error: e.message || 'worker failed' }); };
    w.postMessage(data);
  });
}
Module.h4PickFiles = function (folder) {
  const status = (kind, ...a) => { if (Module.h4Status) Module.h4Status(kind, ...a); };
  const apple = /iPad|iPhone|iPod/.test(navigator.userAgent) ||
    (navigator.platform === 'MacIntel' && navigator.maxTouchPoints > 1);
  const input = document.createElement('input');
  input.type = 'file';
  input.multiple = true;
  if (folder) input.webkitdirectory = true;
  else if (!apple) input.accept = '.exe,.bin';  // (iOS greys out files of types it does not know)
  input.style.display = 'none';
  document.body.appendChild(input);
  // The button the picker is opened from, over the game.
  const box = document.createElement('div');
  box.style.cssText = 'position:fixed;inset:0;z-index:5;display:grid;place-items:center;background:rgba(0,0,0,.6);' +
    'font:16px/1.5 system-ui,-apple-system,sans-serif;color:#f3e6c4';
  const card = document.createElement('div');
  card.style.cssText = 'max-width:30rem;margin:1rem;padding:1.5rem;border:1px solid #8a6a32;border-radius:10px;' +
    'background:rgba(20,13,5,.96);text-align:center';
  const text = document.createElement('p');
  text.style.margin = '0 0 1rem';
  text.textContent = folder ? 'Pick the folder of the installed game (the one with heroes4.exe).'
    : 'Pick both of GOG\'s installer files: setup_heroes_of_might_and_magic_4_complete_…exe and its -1.bin.';
  const button = (label, primary) => {
    const b = document.createElement('button');
    b.textContent = label;
    b.style.cssText = 'font:inherit;font-weight:600;border:0;border-radius:6px;padding:.65rem 1.4rem;margin:.3rem;cursor:pointer;' +
      (primary ? 'background:#d6ab57;color:#1b1205' : 'background:none;color:#b89c64;text-decoration:underline');
    return b;
  };
  const choose = button(folder ? 'Choose the folder' : 'Choose the files', true);
  const cancel = button('Cancel', false);
  card.append(text, choose, document.createElement('br'), cancel);
  box.appendChild(card);
  document.body.appendChild(box);
  choose.onclick = () => input.click();  // (the player's tap: the browser opens the picker)
  let finished = false;
  const finish = async (files) => {
    if (finished) return;
    finished = true;
    input.remove();
    box.remove();
    const list = [];
    for (const f of files) {
      // A folder's files by their path inside it (without the folder's own name).
      const rel = folder ? f.webkitRelativePath.split('/').slice(1).join('/') : f.name;
      if (rel) list.push({ file: f, path: rel.split('/') });
    }
    const total = list.reduce((n, x) => n + x.file.size, 0);
    let done = 0, name = '';
    const copy_start = performance.now();
    const r = list.length ? await h4RunWorker(PICK_WORKER, { files: list }, (m) => {
      if (m.name) name = m.name;
      if (m.bytes) { done += m.bytes; status('copy', total ? done / total : 1, name); }
    }) : { names: [] };
    console.log('h4-web: copying ' + (total / 1e6).toFixed(0) + ' MB took ' +
                ((performance.now() - copy_start) / 1000).toFixed(1) + ' s');
    if (r.error) {
      console.error('h4-web: copying the picked files failed', r.error);
      status('error', 'The files could not be copied into the browser\'s storage: ' + r.error +
             ' (is the disk full?)');
    }
    status('copy-done');
    const names = r.names || [];
    await h4RunWorker(PICK_DONE_WORKER, names.length ? names.join('\n') + '\n' : '');
  };
  input.addEventListener('change', () => { if (input.files && input.files.length) finish(Array.from(input.files)); });
  input.addEventListener('cancel', () => { /* the button stays: picked again, or Cancel */ });
  cancel.onclick = () => finish([]);
};
