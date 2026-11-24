import pathlib
import re

root = pathlib.Path(__file__).resolve().parents[2]
text = (root / "converter/h4/game.toml").read_text()
assert re.search(r'^id = "rehomm4"$', text, re.MULTILINE)
assert re.findall(r'^language = "([^"]+)"$', text, re.MULTILINE) == ["en"]
assert not re.search(r'^language = "pl"$', text, re.MULTILINE)
assert not re.search(r'^id = "[^"\n]*pol[^"\n]*"$', text, re.MULTILINE | re.IGNORECASE)
assert "b7c2f6caeeba640b1c5f1d812b172938bb4dd4ec62e34ec93ad827bd1ffaec87" in text
assert re.findall(r'^name = "([^"]+)"$', text, re.MULTILINE) == [
    "heroes4.exe", "binkw32.dll", "Mss32.dll", "Mp3dec.asi"
]
assert text.count('variants = ["gog-en-76223"]') == 4
print("converter game manifest: ok")
