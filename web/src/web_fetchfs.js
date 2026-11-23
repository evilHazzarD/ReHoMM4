// WasmFS's fetch backend, JS side (--js-library: replaces Emscripten's own of the same name).
// web_main.cpp mounts it for H4_WEB_STREAM, the game read from the server in place.
//
// Emscripten's version (src/lib/libwasmfs_fetch.js) fetches a file under twice the chunk size
// whole, then still finds pieces by the chunk size: past the first chunk it read the wrong bytes
// (heroes4.exe, 6.6 MB, was wrong from 4 MiB on with 4 MiB chunks). It also keeps everything it
// ever fetched. Here every file is read in chunks of the size given to
// wasmfs_create_fetch_backend, and at most kKeep bytes of them are kept (the least recently
// used go first).
addToLibrary({
  _wasmfs_create_fetch_backend_js__deps: [
    '_wasmfs_fetch_get_file_url',
    '_wasmfs_fetch_get_chunk_size',
    '$wasmFS$backends',
  ],
  _wasmfs_create_fetch_backend_js: async function(backend) {
    const kKeep = 256 * 1024 * 1024;
    const files = new Map();  // file pointer -> {url, size, chunk, chunks: Map(index -> bytes)}
    const lru = new Map();    // 'file:index' -> [file, index], least recently used first
    let kept = 0;

    async function info(file) {
      let f = files.get(file);
      if (f) return f;
      const url = new URL(UTF8ToString(__wasmfs_fetch_get_file_url(file)), self.location.href).toString();
      const head = await fetch(url, { method: 'HEAD' });
      if (!head.ok) throw { status: head.status };
      f = { url, size: parseInt(head.headers.get('Content-Length') || '0', 10),
            chunk: __wasmfs_fetch_get_chunk_size(file), chunks: new Map() };
      files.set(file, f);
      return f;
    }

    function use(file, i) {
      const key = file + ':' + i;
      lru.delete(key);
      lru.set(key, [file, i]);
    }

    function keep(file, f, i, bytes, busy) {
      f.chunks.set(i, bytes);
      kept += bytes.length;
      use(file, i);
      for (const [key, [of, oi]] of lru) {
        if (kept <= kKeep) break;
        if (of === file && busy(oi)) continue;
        const ff = files.get(of);
        const b = ff && ff.chunks.get(oi);
        if (b) { kept -= b.length; ff.chunks.delete(oi); }
        lru.delete(key);
      }
    }

    // The chunks [first, last] of the file, fetched (one request for the missing run) if needed.
    async function load(file, f, first, last) {
      let lo = first;
      while (lo <= last && f.chunks.has(lo)) use(file, lo++);
      let hi = last;
      while (hi >= lo && f.chunks.has(hi)) use(file, hi--);
      if (lo > hi) return;
      const start = lo * f.chunk, end = Math.min((hi + 1) * f.chunk, f.size);
      const r = await fetch(f.url, { headers: { Range: `bytes=${start}-${end - 1}` } });
      if (!r.ok) throw r;
      let bytes = new Uint8Array(await r.arrayBuffer());
      if (r.status === 200) bytes = bytes.subarray(start, end);  // a server without ranges
      const busy = (i) => i >= first && i <= last;
      for (let i = lo; i <= hi; i++) {
        keep(file, f, i, bytes.slice((i - lo) * f.chunk, (i - lo + 1) * f.chunk), busy);
      }
    }

    wasmFS$backends[backend] = {
      allocFile: async (file) => {},
      freeFile: async (file) => {
        const f = files.get(file);
        if (!f) return;
        for (const [i, b] of f.chunks) { kept -= b.length; lru.delete(file + ':' + i); }
        files.delete(file);
      },
      write: async (file, buffer, length, offset) => -{{{ cDefs.EROFS }}},
      read: async (file, buffer, length, offset) => {
        if (offset < 0 || length <= 0) return 0;
        let f;
        try {
          f = await info(file);
          length = Math.min(length, f.size - offset);
          if (length <= 0) return 0;
          await load(file, f, Math.floor(offset / f.chunk), Math.floor((offset + length - 1) / f.chunk));
        } catch (e) {
          return e && e.status === 404 ? -{{{ cDefs.ENOENT }}} : -{{{ cDefs.EIO }}};
        }
        for (let done = 0; done < length;) {
          const at = offset + done, i = Math.floor(at / f.chunk), b = f.chunks.get(i);
          const from = at - i * f.chunk, n = Math.min(b.length - from, length - done);
          HEAPU8.set(b.subarray(from, from + n), buffer + done);
          done += n;
        }
        return length;
      },
      getSize: async (file) => {
        try {
          return (await info(file)).size;
        } catch (e) {
          return 0;
        }
      },
    };
  },
});
