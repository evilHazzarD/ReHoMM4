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

// The installer's "Install from GOG installer files" / "Use an installed game": the page's file
// input. What is picked is copied into the origin private file system, ReHoMM4/picked/ (the
// module's /persist/ReHoMM4/picked), then the list of it into picked/.done, which the installer
// waits for; an empty list means the picker was cancelled.
Module.h4PickFiles = function (folder) {
  const status = (kind, ...a) => { if (Module.h4Status) Module.h4Status(kind, ...a); };
  const input = document.createElement('input');
  input.type = 'file';
  input.multiple = true;
  if (folder) input.webkitdirectory = true;
  else input.accept = '.exe,.bin';
  input.style.display = 'none';
  document.body.appendChild(input);
  let finished = false;
  const finish = async (files) => {
    if (finished) return;
    finished = true;
    input.remove();
    const root = await navigator.storage.getDirectory();
    const app = await root.getDirectoryHandle('ReHoMM4', { create: true });
    const dir = await app.getDirectoryHandle('picked', { create: true });
    const names = [];
    try {
      const total = files.reduce((n, f) => n + f.size, 0);
      let done = 0;
      for (const f of files) {
        // A folder's files by their path inside it (without the folder's own name).
        const rel = folder ? f.webkitRelativePath.split('/').slice(1).join('/') : f.name;
        if (!rel) continue;
        const parts = rel.split('/');
        let d = dir;
        for (const p of parts.slice(0, -1)) d = await d.getDirectoryHandle(p, { create: true });
        const handle = await d.getFileHandle(parts[parts.length - 1], { create: true });
        const w = await handle.createWritable();
        const reader = f.stream().getReader();
        for (;;) {
          const { value, done: end } = await reader.read();
          if (end) break;
          await w.write(value);
          done += value.length;
          status('copy', total ? done / total : 1, f.name);
        }
        await w.close();
        names.push(rel);
      }
    } catch (e) {
      console.error('h4-web: copying the picked files failed', e);
      status('error', 'The files could not be copied into the browser\'s storage: ' + e.message +
             ' (is the disk full?)');
      names.length = 0;
    }
    status('copy-done');
    const list = await dir.getFileHandle('.done', { create: true });
    const w = await list.createWritable();
    await w.write(names.length ? names.join('\n') + '\n' : '');
    await w.close();
  };
  input.addEventListener('change', () => finish(Array.from(input.files || [])));
  input.addEventListener('cancel', () => finish([]));
  input.click();
};
