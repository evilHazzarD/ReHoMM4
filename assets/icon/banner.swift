// banner.swift <emblem.png> <out.png>: the Android TV banner (640x360 px: 320x180 dp at xhdpi) — the
// icon's background colour, its emblem (icon.swift fg) at the left and "Heroes IV" in gold.
import AppKit
import CoreText
import ImageIO
import UniformTypeIdentifiers

let a = CommandLine.arguments
let W = 640, H = 360
let cs = CGColorSpaceCreateDeviceRGB()
let ctx = CGContext(data: nil, width: W, height: H, bitsPerComponent: 8, bytesPerRow: W * 4, space: cs,
                    bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue)!
ctx.setFillColor(CGColor(red: 26 / 255, green: 18 / 255, blue: 9 / 255, alpha: 1))
ctx.fill(CGRect(x: 0, y: 0, width: W, height: H))
if let src = CGImageSourceCreateWithURL(URL(fileURLWithPath: a[1]) as CFURL, nil),
   let emblem = CGImageSourceCreateImageAtIndex(src, 0, nil) {
  ctx.draw(emblem, in: CGRect(x: -20, y: 0, width: 360, height: 360))
}
let font = NSFont(name: "Georgia-Bold", size: 72) ?? NSFont.boldSystemFont(ofSize: 72)
func text(_ s: String, _ baseline: CGFloat) {
  for (dx, dy, color) in [(3.0, -4.0, CGColor(gray: 0, alpha: 1)),
                          (0.0, 0.0, CGColor(red: 236 / 255, green: 190 / 255, blue: 80 / 255, alpha: 1))] {
    let line = CTLineCreateWithAttributedString(NSAttributedString(string: s, attributes: [
      .font: font, NSAttributedString.Key(kCTForegroundColorAttributeName as String): color]))
    let w = CTLineGetTypographicBounds(line, nil, nil, nil)
    ctx.textPosition = CGPoint(x: 320 + (300 - w) / 2 + dx, y: baseline + dy)
    CTLineDraw(line, ctx)
  }
}
text("Heroes", 200)
text("IV", 110)
let out = CGImageDestinationCreateWithURL(URL(fileURLWithPath: a[2]) as CFURL, UTType.png.identifier as CFString, 1, nil)!
CGImageDestinationAddImage(out, ctx.makeImage()!, nil)
guard CGImageDestinationFinalize(out) else { exit(1) }
