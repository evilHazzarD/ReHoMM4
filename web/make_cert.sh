#!/usr/bin/env bash
# A certificate for serving the browser build over HTTPS on the local network (serve.py --tls), so
# that phones get a secure page (shared memory and the origin private file system need one).
#
#   bash web/make_cert.sh [ADDRESS...]   (default: this Mac's address on en0, and its .local name)
#
# Writes .cache/web-cert/: ca.crt (the authority: install it on the phone and trust it), server.crt
# and server.key (for serve.py). The authority is made once and kept; the server certificate is
# made again for the addresses given.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
D="$ROOT/.cache/web-cert"
mkdir -p "$D"
cd "$D"
ADDRS=("$@")
if [[ ${#ADDRS[@]} -eq 0 ]]; then
  ip="$(ipconfig getifaddr en0 2>/dev/null || true)"
  [[ -n "$ip" ]] && ADDRS+=("$ip")
  ADDRS+=("$(scutil --get LocalHostName 2>/dev/null || hostname -s).local")
fi
SAN="DNS:localhost,IP:127.0.0.1"
for a in "${ADDRS[@]}"; do
  if [[ "$a" =~ ^[0-9.]+$ ]]; then SAN+=",IP:$a"; else SAN+=",DNS:$a"; fi
done
if [[ ! -f ca.key ]]; then
  # Its extensions from a config of its own: -addext on top of openssl's default config writes
  # basicConstraints twice, and a certificate with an extension twice is invalid.
  openssl req -x509 -new -newkey rsa:2048 -nodes -keyout ca.key -out ca.crt -days 3650 \
    -config <(printf '[req]\ndistinguished_name=dn\nx509_extensions=ext\n[dn]\n[ext]\nbasicConstraints=critical,CA:TRUE\nkeyUsage=critical,keyCertSign,cRLSign\nsubjectKeyIdentifier=hash\n') \
    -subj "/CN=ReHoMM4 local test CA" 2>/dev/null
fi
openssl req -new -newkey rsa:2048 -nodes -keyout server.key -out server.csr -subj "/CN=ReHoMM4 local" 2>/dev/null
# 397 days at most: Apple refuses server certificates valid longer.
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 397 \
  -extfile <(printf 'subjectAltName=%s\nextendedKeyUsage=serverAuth\nbasicConstraints=CA:FALSE\nkeyUsage=critical,digitalSignature,keyEncipherment\nsubjectKeyIdentifier=hash\nauthorityKeyIdentifier=keyid,issuer\n' "$SAN") 2>/dev/null
rm -f server.csr
echo "certificate for $SAN in $D"
