#!/usr/bin/env python3
"""Serves the browser build (.cache/outputs/web/dist, web/build.sh) on http://localhost:8080.

    python3 web/serve.py [--port N] [--dir DIR] [--bind ADDR] [--game GAME_DIR]
                            [--tls CERT KEY]

--tls: HTTPS with that certificate (PEM) and key, for phones and other computers on the network
(web/make_cert.sh makes a pair, with a certificate authority to install on them).

With the headers a page needs for shared memory (the game's threads): Cross-Origin-Opener-Policy
and Cross-Origin-Embedder-Policy. Browsers treat localhost as a secure context; anything else must
be HTTPS (the origin private file system and shared memory are only given to secure pages)."""
import argparse
import functools
import http.server
import json
import os
import urllib.parse

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


GAME_PREFIX = '/game-files/'


class Handler(http.server.SimpleHTTPRequestHandler):
    extensions_map = {**http.server.SimpleHTTPRequestHandler.extensions_map,
                      '.wasm': 'application/wasm', '.js': 'text/javascript', '.data': 'application/octet-stream'}
    game_dir = None

    # --game DIR: an installed game served at /game-files/ (index.json lists its files), which
    # index.html?game=game-files copies into the browser's storage instead of the installer.
    # For testing, and for a server of your own; never put a game where others can download it.
    def translate_path(self, path):
        p = path.split('?', 1)[0].split('#', 1)[0]
        if self.game_dir and p.startswith(GAME_PREFIX):
            rel = os.path.normpath(urllib.parse.unquote(p[len(GAME_PREFIX):])).lstrip('/')
            if rel.startswith('..'):
                return os.path.join(self.game_dir, '\0')
            return os.path.join(self.game_dir, rel)
        return super().translate_path(path)

    def do_GET(self):
        if self.game_dir and self.path.split('?', 1)[0] == GAME_PREFIX + 'index.json':
            files = []
            for d, _, names in os.walk(self.game_dir):
                for n in names:
                    full = os.path.join(d, n)
                    files.append({'path': os.path.relpath(full, self.game_dir).replace(os.sep, '/'),
                                  'size': os.path.getsize(full)})
            body = json.dumps({'files': files}).encode()
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.send_header('Content-Length', str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return
        if self.send_range(head=False):
            return
        super().do_GET()

    def do_HEAD(self):
        if self.send_range(head=True):
            return
        super().do_HEAD()

    # Byte ranges: of the game's files (index.html?env=H4_WEB_STREAM=game-files reads them in
    # place, a piece at a time: web_main.cpp) and of the site's (downloads resumed, as web
    # servers do). False: not a ranged request, answered as usual.
    def send_range(self, head):
        rng = self.headers.get('Range', '')
        if not rng.startswith('bytes='):
            return False
        path = self.translate_path(self.path)
        if not os.path.isfile(path):
            return False
        size = os.path.getsize(path)
        a, _, b = rng[6:].split(',')[0].strip().partition('-')
        try:
            start = int(a) if a else max(0, size - int(b))
            end = min(int(b), size - 1) if a and b else size - 1
        except ValueError:
            return False
        if start >= size or end < start:
            self.send_response(416)
            self.send_header('Content-Range', f'bytes */{size}')
            self.end_headers()
            return True
        self.send_response(206)
        self.send_header('Content-Type', 'application/octet-stream')
        self.send_header('Content-Range', f'bytes {start}-{end}/{size}')
        self.send_header('Content-Length', str(end - start + 1))
        self.end_headers()
        if not head:
            with open(path, 'rb') as f:
                f.seek(start)
                left = end - start + 1
                while left > 0:
                    chunk = f.read(min(left, 1 << 20))
                    if not chunk:
                        break
                    self.wfile.write(chunk)
                    left -= len(chunk)
        return True

    def end_headers(self):
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        self.send_header('Cross-Origin-Resource-Policy', 'same-origin')
        # index.html anew each time; the big files from the cache when unchanged.
        self.send_header('Cache-Control', 'no-cache')
        self.send_header('Accept-Ranges', 'bytes')
        super().end_headers()


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--port', type=int, default=8080)
    ap.add_argument('--bind', default='127.0.0.1')
    # The release's web zip has this script beside the site: that folder, else the build's.
    here = os.path.dirname(os.path.abspath(__file__))
    site = here if os.path.isfile(os.path.join(here, 'index.html')) else os.path.join(ROOT, '.cache', 'outputs', 'web', 'dist')
    ap.add_argument('--dir', default=site)
    ap.add_argument('--tls', nargs=2, metavar=('CERT', 'KEY'), help='HTTPS with this certificate and key')
    ap.add_argument('--game', help='an installed game folder, for index.html?game=game-files')
    a = ap.parse_args()
    if a.game:
        if not os.path.isfile(os.path.join(a.game, 'heroes4.exe')):
            raise SystemExit(f'no heroes4.exe in {a.game}')
        Handler.game_dir = os.path.abspath(a.game)
    if not os.path.isfile(os.path.join(a.dir, 'index.html')):
        raise SystemExit(f'no site in {a.dir}: run web/build.sh first')
    handler = functools.partial(Handler, directory=a.dir)
    with http.server.ThreadingHTTPServer((a.bind, a.port), handler) as httpd:
        scheme = 'http'
        if a.tls:
            import ssl
            ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
            ctx.load_cert_chain(*a.tls)
            httpd.socket = ctx.wrap_socket(httpd.socket, server_side=True)
            scheme = 'https'
        if a.game:
            print(f'game files from {Handler.game_dir}: open /?game=game-files to copy them in')
        print(f'serving {a.dir} on {scheme}://{"localhost" if a.bind == "127.0.0.1" else a.bind}:{a.port}')
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            pass


if __name__ == '__main__':
    main()
