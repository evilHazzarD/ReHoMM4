// The language button's pictures for the Linux and Switch apps (menu_button_linux.cpp draws them into
// the frame; the app cannot draw text). Generic, the same for every language:
//   letters.argb   the characters of the button "🌐 UK" (LETTERS below, in that order, each in a
//                  cell `cell` pixels wide: the app trims each to its ink), cream on transparent
//   note.argb      the "restart the game to apply the new language" note (English)
// .argb: width, height (little-endian 32-bit), then ARGB pixels (premultiplied).
//   swift render_ui.swift <out dir>   (platforms/linux/package.sh, switch/package.sh)
import AppKit

let out = CommandLine.arguments[1]
try? FileManager.default.createDirectory(atPath: out, withIntermediateDirectories: true)
let cream = NSColor(red: 0.98, green: 0.92, blue: 0.78, alpha: 1)
let gold = NSColor(red: 0.84, green: 0.67, blue: 0.34, alpha: 1)
let brown = NSColor(red: 0.10, green: 0.07, blue: 0.03, alpha: 0.92)

func render(_ name: String, _ text: String, size: CGFloat, maxWidth: CGFloat) {
  let para = NSMutableParagraphStyle(); para.alignment = .center
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
