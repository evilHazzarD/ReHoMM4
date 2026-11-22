// The settings and language buttons' pictures for the Linux and Switch apps (menu_button_linux.cpp draws them into
// the frame; the app cannot draw text). Generic, the same for every language:
//   letters.argb   the characters of the button "🌐 UK" (LETTERS below, in that order, each in a
//                  cell `cell` pixels wide: the app trims each to its ink), cream on transparent
//   note.argb      the "restart the game to apply the new language" note (English)
//   gear.argb      the settings button: ⚙ on its plate
//   kofi_gear.argb  the Linux app's: ☕ (Ko-fi) and ⚙, 8 pixels apart
//   upd_*.argb     the update check's button and messages (Linux)
//   sync_button.argb, sync_help.argb  the save sync's row and its help (Linux)
//   pad_button.argb  the settings' "Gamepad" row; gp_*.argb its panel's labels (menu_button_linux.cpp)
// .argb: width, height (little-endian 32-bit), then ARGB pixels (premultiplied).
//   swift render_ui.swift <out dir>   (platforms/linux/package.sh, switch/package.sh)
import AppKit

let out = CommandLine.arguments[1]
try? FileManager.default.createDirectory(atPath: out, withIntermediateDirectories: true)
let cream = NSColor(red: 0.98, green: 0.92, blue: 0.78, alpha: 1)
let gold = NSColor(red: 0.84, green: 0.67, blue: 0.34, alpha: 1)
let brown = NSColor(red: 0.10, green: 0.07, blue: 0.03, alpha: 0.92)

func render(_ name: String, _ text: String, size: CGFloat, maxWidth: CGFloat, left: Bool = false) {
  let para = NSMutableParagraphStyle(); para.alignment = left ? .left : .center
  let attrs: [NSAttributedString.Key: Any] = [.font: NSFont.boldSystemFont(ofSize: size),
                                             .foregroundColor: cream, .paragraphStyle: para]
  let s = NSAttributedString(string: text, attributes: attrs)
  let tb = s.boundingRect(with: NSSize(width: maxWidth, height: 1000),
                          options: [.usesLineFragmentOrigin, .usesFontLeading])
  let w = Int(ceil(tb.width)) + 28, h = Int(ceil(tb.height)) + 14
  let rep = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: w, pixelsHigh: h, bitsPerSample: 8,
                             samplesPerPixel: 4, hasAlpha: true, isPlanar: false,
                             colorSpaceName: .deviceRGB, bytesPerRow: w * 4, bitsPerPixel: 32)!
  NSGraphicsContext.saveGraphicsState()
  NSGraphicsContext.current = NSGraphicsContext(bitmapImageRep: rep)
  let r = NSRect(x: 1, y: 1, width: w - 2, height: h - 2)
  let path = NSBezierPath(roundedRect: r, xRadius: 9, yRadius: 9)
  brown.setFill(); path.fill()
  gold.setStroke(); path.lineWidth = 1.5; path.stroke()
  s.draw(with: NSRect(x: 14, y: 7, width: CGFloat(w - 28), height: tb.height),
         options: [.usesLineFragmentOrigin, .usesFontLeading])
  NSGraphicsContext.restoreGraphicsState()
  var data = Data()
  var dims = [UInt32(w).littleEndian, UInt32(h).littleEndian]
  data.append(Data(bytes: &dims, count: 8))
  for y in 0..<h {
    for x in 0..<w {
      var p = [Int](repeating: 0, count: 4)
      rep.getPixel(&p, atX: x, y: y)
      // Premultiplied in the rep: the colour as is over a dark frame; alpha cut at 0x80 when drawn.
      var v = UInt32(p[3]) << 24 | UInt32(p[0]) << 16 | UInt32(p[1]) << 8 | UInt32(p[2])
      v = v.littleEndian
      data.append(Data(bytes: &v, count: 4))
    }
  }
  try! data.write(to: URL(fileURLWithPath: "\(out)/\(name).argb"))
}

// Must match kLetters in h4/src/menu_button_linux.cpp.
let letters = ["🌐", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q",
               "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "-"]
let cell = 20, rows = 16
let font = NSFont.boldSystemFont(ofSize: 12)
let rep = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: cell * letters.count, pixelsHigh: rows,
                           bitsPerSample: 8, samplesPerPixel: 4, hasAlpha: true, isPlanar: false,
                           colorSpaceName: .deviceRGB, bytesPerRow: cell * letters.count * 4, bitsPerPixel: 32)!
NSGraphicsContext.saveGraphicsState()
NSGraphicsContext.current = NSGraphicsContext(bitmapImageRep: rep)
for (i, l) in letters.enumerated() {
  let s = NSAttributedString(string: l, attributes: [.font: font, .foregroundColor: cream])
  let w = s.size().width
  s.draw(at: NSPoint(x: CGFloat(i * cell) + (CGFloat(cell) - w) / 2, y: 1))
}
NSGraphicsContext.restoreGraphicsState()
var data = Data()
var dims = [UInt32(rep.pixelsWide).littleEndian, UInt32(rows).littleEndian]
data.append(Data(bytes: &dims, count: 8))
for y in 0..<rows {
  for x in 0..<rep.pixelsWide {
    var p = [Int](repeating: 0, count: 4)
    rep.getPixel(&p, atX: x, y: y)
    var v = (UInt32(p[3]) << 24 | UInt32(p[0]) << 16 | UInt32(p[1]) << 8 | UInt32(p[2])).littleEndian
    data.append(Data(bytes: &v, count: 4))
  }
}
try! data.write(to: URL(fileURLWithPath: "\(out)/letters.argb"))
render("note", "Restart the game to apply the new language.", size: 15, maxWidth: 300)
// The Linux app's note: the same question, with OK (the app restarts) and Cancel side by side
// in its bottom 40 pixel rows, each half of its width (menu_button_linux.cpp: kRestartBand).
do {
  let para = NSMutableParagraphStyle(); para.alignment = .center
  let s = NSAttributedString(string: "Restart the game now to apply the new language?",
                             attributes: [.font: NSFont.boldSystemFont(ofSize: 15), .foregroundColor: cream,
                                          .paragraphStyle: para])
  let tb = s.boundingRect(with: NSSize(width: 300, height: 1000), options: [.usesLineFragmentOrigin, .usesFontLeading])
  let band = 40
  let w = max(Int(ceil(tb.width)) + 28, 240), h = Int(ceil(tb.height)) + 14 + band
  let rep = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: w, pixelsHigh: h, bitsPerSample: 8,
                             samplesPerPixel: 4, hasAlpha: true, isPlanar: false,
                             colorSpaceName: .deviceRGB, bytesPerRow: w * 4, bitsPerPixel: 32)!
  NSGraphicsContext.saveGraphicsState()
  NSGraphicsContext.current = NSGraphicsContext(bitmapImageRep: rep)
  let plate = NSBezierPath(roundedRect: NSRect(x: 1, y: 1, width: w - 2, height: h - 2), xRadius: 9, yRadius: 9)
  brown.setFill(); plate.fill()
  gold.setStroke(); plate.lineWidth = 1.5; plate.stroke()
  s.draw(with: NSRect(x: 14, y: CGFloat(band) + 4, width: CGFloat(w - 28), height: tb.height),
         options: [.usesLineFragmentOrigin, .usesFontLeading])
  let bw = (w - 3 * 12) / 2
  for (i, label) in ["OK", "Cancel"].enumerated() {
    let br = NSRect(x: 12 + i * (bw + 12), y: 8, width: bw, height: 26)
    let b = NSBezierPath(roundedRect: br, xRadius: 6, yRadius: 6)
    NSColor(red: 0.22, green: 0.15, blue: 0.07, alpha: 1).setFill(); b.fill()
    gold.setStroke(); b.lineWidth = 1.2; b.stroke()
    let t = NSAttributedString(string: label, attributes: [.font: NSFont.boldSystemFont(ofSize: 13),
                                                           .foregroundColor: cream, .paragraphStyle: para])
    let th = t.size().height
    t.draw(with: NSRect(x: br.minX, y: br.minY + (br.height - th) / 2, width: br.width, height: th),
           options: [.usesLineFragmentOrigin])
  }
  NSGraphicsContext.restoreGraphicsState()
  writeArgb(rep, "restart")
}
// The fullscreen setting's row, off and on (Linux; menu_button_linux.cpp).
render("full_off", "☐  Fullscreen", size: 12, maxWidth: 400)
render("full_on", "☑  Fullscreen", size: 12, maxWidth: 400)
// "ReHoMM4 v<version>" at the bottom of the settings (the version: the second argument).
if CommandLine.arguments.count > 2 && !CommandLine.arguments[2].isEmpty {
  render("version", "ReHoMM4 v" + CommandLine.arguments[2], size: 12, maxWidth: 300)
}
// The intro setting's row, off and on (menu_button_linux.cpp).
render("skip_off", "☐  Skip the intro movies (from the next start)", size: 12, maxWidth: 400)
render("skip_on", "☑  Skip the intro movies (from the next start)", size: 12, maxWidth: 400)
// The gamepad's settings: the settings' row, and the labels its panel is put together from at run
// time (menu_button_linux.cpp, the gamepad panel: the boxes, sliders and plates are drawn there).
// The actions and the buttons' names: keep in step with h4/src/pad_config.cpp (its order and ids).
render("pad_button", "Gamepad", size: 12, maxWidth: 300)
// A label: text on transparent (premultiplied), `maxWidth` wide at most (wrapped).
func label(_ name: String, _ text: String, size: CGFloat = 12, color: NSColor = cream, maxWidth: CGFloat = 600) {
  let attrs: [NSAttributedString.Key: Any] = [.font: NSFont.boldSystemFont(ofSize: size), .foregroundColor: color]
  let s = NSAttributedString(string: text, attributes: attrs)
  let tb = s.boundingRect(with: NSSize(width: maxWidth, height: 1000), options: [.usesLineFragmentOrigin, .usesFontLeading])
  let w = max(1, Int(ceil(tb.width)) + 2), h = max(1, Int(ceil(tb.height)) + 2)
  let rep = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: w, pixelsHigh: h, bitsPerSample: 8,
                             samplesPerPixel: 4, hasAlpha: true, isPlanar: false,
                             colorSpaceName: .deviceRGB, bytesPerRow: w * 4, bitsPerPixel: 32)!
  NSGraphicsContext.saveGraphicsState()
  NSGraphicsContext.current = NSGraphicsContext(bitmapImageRep: rep)
  s.draw(with: NSRect(x: 1, y: 1, width: CGFloat(w - 2), height: tb.height), options: [.usesLineFragmentOrigin, .usesFontLeading])
  NSGraphicsContext.restoreGraphicsState()
  writeArgb(rep, name)
}
label("gp_title", "Gamepad", size: 15, color: gold)
label("gp_pointer", "Pointer speed (left stick)", color: gold)
label("gp_scroll", "Map scrolling speed (right stick)", color: gold)
label("gp_buttons", "Buttons", size: 13, color: gold)
label("gp_preset", "Preset", color: gold)
label("gp_reset", "Reset to the defaults")
label("gp_hint", "With a gamepad: click presses or takes a list's next item, right-click its previous one, Esc closes.",
      size: 11, maxWidth: 570)
label("gp_pct", "%")
for (id, text) in [("ps", "PlayStation"), ("xbox", "Xbox"), ("switch", "Switch")] { label("gp_style_\(id)", text) }
for (id, text) in [("none", "Nothing"), ("click", "Click (left button)"), ("rclick", "Right-click (information)"),
                   ("esc", "Cancel, close (Esc)"), ("enter", "Move the army / OK (Enter)"), ("spell", "Cast a spell (C)"),
                   ("wait", "Wait / wake the hero (W)"), ("defend", "Defend / dig (D)"), ("hero", "Next hero (H)"),
                   ("town", "Next town (T)"), ("sleep", "The hero sleeps (Z)"), ("pgup", "Page Up (lists, spell tabs)"),
                   ("pgdn", "Page Down (lists, spell tabs)"), ("endturn", "End the turn (E)"),
                   ("save", "Save the game (Alt+S)"), ("home", "Centre on the hero (Home)"), ("shift", "Shift (held)"),
                   ("ctrl", "Ctrl (held)")] {
  label("gp_act_\(id)", text)
}
// id, then its name on a PlayStation, an Xbox and a Nintendo pad.
for (id, ps, xbox, sw) in [("A", "✕", "A", "A"), ("B", "○", "B", "B"), ("X", "□", "X", "X"), ("Y", "△", "Y", "Y"),
                           ("L", "L1", "LB", "L"), ("R", "R1", "RB", "R"), ("ZL", "L2", "LT", "ZL"), ("ZR", "R2", "RT", "ZR"),
                           ("Up", "D-pad ↑", "D-pad ↑", "D-pad ↑"), ("Down", "D-pad ↓", "D-pad ↓", "D-pad ↓"),
                           ("Left", "D-pad ←", "D-pad ←", "D-pad ←"), ("Right", "D-pad →", "D-pad →", "D-pad →"),
                           ("Plus", "Options", "Menu", "+"), ("Minus", "Share", "View", "−"),
                           ("L3", "L3", "LS", "L3 (left stick)"), ("R3", "R3", "RS", "R3 (right stick)")] {
  label("gp_b_ps_\(id)", ps)
  label("gp_b_xbox_\(id)", xbox)
  label("gp_b_switch_\(id)", sw)
}
// The update check's (Linux; menu_button_linux.cpp).
render("upd_button", "Check for updates", size: 12, maxWidth: 300)
render("upd_checking", "Checking GitHub for a new version…", size: 15, maxWidth: 320)
render("upd_latest", "You have the latest version.", size: 15, maxWidth: 320)
render("upd_newer", "A new version is out.\nClick here to download it into Downloads.", size: 15, maxWidth: 320)
render("upd_downloading", "Downloading the new version…", size: 15, maxWidth: 320)
render("upd_done", "Downloaded into Downloads (reHoMM4-v<version>-linux-x86_64.tar.gz).\nQuit the game and unpack it over the old one.", size: 15, maxWidth: 360)
render("upd_failed", "Could not reach GitHub. Try again later.", size: 15, maxWidth: 320)
// The save sync's row and its help (Linux: save_sync.h; menu_button_linux.cpp).
render("sync_button", "Saved games sync   ( ? )", size: 12, maxWidth: 300)
render("sync_help", """
Saved games sync

Your saves are kept in step with the folder ~/ReHOMM4 (made at the first start): file by file, \
the newer copy wins, nothing is deleted. Carry that folder to your other devices with a sync tool:

• Syncthing: share ~/ReHOMM4 with your other devices. On Android or an iPhone, turn the sync \
on in ReHoMM4's settings and pick the shared folder; the Mac app syncs iCloud Drive/ReHOMM4.

• A cloud (Google Drive, Dropbox, OneDrive…) through rclone: set the remote up once with \
`rclone config`, then put the line  rclone=<remote>:ReHOMM4  into \
~/.local/share/ReHoMM4/sync.conf. The game brings newer saves down and sends its own up.

In sync.conf, folder=<path> syncs another folder instead; a line  off  stops the sync.

Click to close.
""", size: 12, maxWidth: 520, left: true)
// The settings button: a 44-pixel square plate, ⚙ centred on its ink; and the Linux app's
// "☕ ⚙" (the Ko-fi button left of it, 8 pixels apart: kofi_gear.argb).
// Draws `glyph` on a plate at x in `rep` (side x side), the glyph's ink centred.
func drawPlate(_ glyph: NSAttributedString, side: Int, x: Int, into rep: NSBitmapImageRep) {
  // Its ink, measured on pixels (the font's metrics put it off centre): drawn at (8, 8) alone first.
  let probe = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: 100, pixelsHigh: 100, bitsPerSample: 8,
                               samplesPerPixel: 4, hasAlpha: true, isPlanar: false,
                               colorSpaceName: .deviceRGB, bytesPerRow: 400, bitsPerPixel: 32)!
  NSGraphicsContext.saveGraphicsState()
  NSGraphicsContext.current = NSGraphicsContext(bitmapImageRep: probe)
  glyph.draw(at: NSPoint(x: 8, y: 8))
  NSGraphicsContext.restoreGraphicsState()
  var x0 = 100, x1 = -1, y0 = 100, y1 = -1  // pixel rows: 0 at the top
  for y in 0..<100 { for x in 0..<100 {
    var p = [Int](repeating: 0, count: 4); probe.getPixel(&p, atX: x, y: y)
    if p[3] > 0 { x0 = min(x0, x); x1 = max(x1, x); y0 = min(y0, y); y1 = max(y1, y) }
  } }
  // Where to draw it so the ink's centre is the plate's (drawing's y goes up from the bottom).
  let dx = CGFloat(side) / 2 - (CGFloat(x0 + x1 + 1) / 2 - 8)
  let dy = CGFloat(side) / 2 - ((100 - CGFloat(y0 + y1 + 1) / 2) - 8)
  NSGraphicsContext.saveGraphicsState()
  NSGraphicsContext.current = NSGraphicsContext(bitmapImageRep: rep)
  let path = NSBezierPath(roundedRect: NSRect(x: CGFloat(x) + 1, y: 1, width: CGFloat(side - 2), height: CGFloat(side - 2)),
                          xRadius: 9, yRadius: 9)
  brown.setFill(); path.fill()
  gold.setStroke(); path.lineWidth = 1.5; path.stroke()
  glyph.draw(at: NSPoint(x: CGFloat(x) + dx, y: dy))
  NSGraphicsContext.restoreGraphicsState()
}
func writeArgb(_ rep: NSBitmapImageRep, _ name: String) {
  var data = Data()
  var dims = [UInt32(rep.pixelsWide).littleEndian, UInt32(rep.pixelsHigh).littleEndian]
  data.append(Data(bytes: &dims, count: 8))
  for y in 0..<rep.pixelsHigh {
    for x in 0..<rep.pixelsWide {
      var p = [Int](repeating: 0, count: 4)
      rep.getPixel(&p, atX: x, y: y)
      var v = (UInt32(p[3]) << 24 | UInt32(p[0]) << 16 | UInt32(p[1]) << 8 | UInt32(p[2])).littleEndian
      data.append(Data(bytes: &v, count: 4))
    }
  }
  try! data.write(to: URL(fileURLWithPath: "\(out)/\(name).argb"))
}
do {
  let side = 44, gap = 8
  let gearGlyph = NSAttributedString(string: "⚙\u{FE0E}",
                                     attributes: [.font: NSFont.systemFont(ofSize: 38), .foregroundColor: cream])
  let cupGlyph = NSAttributedString(string: "☕\u{FE0F}", attributes: [.font: NSFont.systemFont(ofSize: 26)])
  func newRep(_ w: Int) -> NSBitmapImageRep {
    NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: w, pixelsHigh: side, bitsPerSample: 8,
                     samplesPerPixel: 4, hasAlpha: true, isPlanar: false,
                     colorSpaceName: .deviceRGB, bytesPerRow: w * 4, bitsPerPixel: 32)!
  }
  let gear = newRep(side)
  drawPlate(gearGlyph, side: side, x: 0, into: gear)
  writeArgb(gear, "gear")
  let both = newRep(2 * side + gap)
  drawPlate(cupGlyph, side: side, x: 0, into: both)
  drawPlate(gearGlyph, side: side, x: side + gap, into: both)
  writeArgb(both, "kofi_gear")
}
