// icon.swift <out.png> <style> [size]: the apps' icon, the project's own art drawn here (no game
// file): a gold-rimmed crimson heater shield with a serif "IV" on it. assets/icon/icon.sh makes
// every platform's icon from it.
//   full   opaque, the dark background to the edges (iOS rounds the corners itself; Switch)
//   fg     transparent, the shield inside Android's adaptive-icon safe zone (66/108 of the side)
//   mac    the macOS grid: an 824/1024 rounded square with a shadow, transparent around it
//   plain  transparent, the shield alone (Linux)
import CoreGraphics
import Foundation
import ImageIO
import UniformTypeIdentifiers

let a = CommandLine.arguments
guard a.count >= 3, ["full", "fg", "mac", "plain"].contains(a[2]) else {
  FileHandle.standardError.write("usage: icon.swift out.png full|fg|mac|plain [size]\n".data(using: .utf8)!)
  exit(2)
}
let style = a[2]
let n = a.count > 3 ? Int(a[3]) ?? 1024 : 1024
let N = CGFloat(n)
let cs = CGColorSpace(name: CGColorSpace.sRGB)!
let ctx = CGContext(data: nil, width: n, height: n, bitsPerComponent: 8, bytesPerRow: n * 4, space: cs,
                    bitmapInfo: style == "full" ? CGImageAlphaInfo.noneSkipLast.rawValue
                                                : CGImageAlphaInfo.premultipliedLast.rawValue)!
// Top-left origin, y down.
ctx.translateBy(x: 0, y: N)
ctx.scaleBy(x: 1, y: -1)

func rgb(_ r: Int, _ g: Int, _ b: Int, _ alpha: CGFloat = 1) -> CGColor {
  CGColor(srgbRed: CGFloat(r) / 255, green: CGFloat(g) / 255, blue: CGFloat(b) / 255, alpha: alpha)
}
func gradient(_ colors: [CGColor], _ stops: [CGFloat]) -> CGGradient {
  CGGradient(colorsSpace: cs, colors: colors as CFArray, locations: stops)!
}

// The dark background: warm brown, lighter in the middle.
func background(_ rect: CGRect) {
  ctx.saveGState()
  ctx.clip(to: rect)
  ctx.setFillColor(rgb(18, 11, 5))
  ctx.fill(rect)
  let c = CGPoint(x: rect.midX, y: rect.midY - rect.height * 0.06)
  ctx.drawRadialGradient(gradient([rgb(70, 44, 20), rgb(18, 11, 5)], [0, 1]),
                         startCenter: c, startRadius: 0, endCenter: c, endRadius: rect.width * 0.72,
                         options: [.drawsAfterEndLocation])
  ctx.restoreGState()
}

// A heater shield in a w x 1.2w box at (x, y).
func shield(_ x: CGFloat, _ y: CGFloat, _ w: CGFloat) -> CGPath {
  let p = CGMutablePath()
  let h = w * 1.2
  p.move(to: CGPoint(x: x, y: y + w * 0.04))
  p.addQuadCurve(to: CGPoint(x: x + w / 2, y: y), control: CGPoint(x: x + w * 0.25, y: y + w * 0.06))
  p.addQuadCurve(to: CGPoint(x: x + w, y: y + w * 0.04), control: CGPoint(x: x + w * 0.75, y: y + w * 0.06))
  p.addLine(to: CGPoint(x: x + w, y: y + h * 0.42))
  p.addCurve(to: CGPoint(x: x + w / 2, y: y + h), control1: CGPoint(x: x + w, y: y + h * 0.78),
             control2: CGPoint(x: x + w * 0.72, y: y + h * 0.92))
  p.addCurve(to: CGPoint(x: x, y: y + h * 0.42), control1: CGPoint(x: x + w * 0.28, y: y + h * 0.92),
             control2: CGPoint(x: x, y: y + h * 0.78))
  p.closeSubpath()
  return p
}

// "IV" as serif capitals, H high, its top-left at (x, y); returns the path and its width.
func numeral(_ x: CGFloat, _ y: CGFloat, _ H: CGFloat) -> (CGPath, CGFloat) {
  let p = CGMutablePath()
  let serifH = H * 0.07, stem = H * 0.15
  // I
  let iw = H * 0.40
  p.addRect(CGRect(x: x, y: y, width: iw, height: serifH))
  p.addRect(CGRect(x: x, y: y + H - serifH, width: iw, height: serifH))
  p.addRect(CGRect(x: x + (iw - stem) / 2, y: y, width: stem, height: H))
  // V: a thick left stroke, a thin right one, meeting at the foot.
  let vx = x + iw + H * 0.10, vw = H * 0.86
  let thick = H * 0.17, thin = H * 0.07
  let foot = CGPoint(x: vx + vw / 2, y: y + H)
  let v = CGMutablePath()
  v.move(to: CGPoint(x: vx + H * 0.06, y: y))
  v.addLine(to: CGPoint(x: vx + H * 0.06 + thick, y: y))
  v.addLine(to: CGPoint(x: foot.x + H * 0.05, y: y + H * 0.70))
  v.addLine(to: CGPoint(x: vx + vw - H * 0.06 - thin, y: y))
  v.addLine(to: CGPoint(x: vx + vw - H * 0.06, y: y))
  v.addLine(to: CGPoint(x: foot.x + H * 0.04, y: foot.y))
  v.addLine(to: CGPoint(x: foot.x - H * 0.06, y: foot.y))
  v.closeSubpath()
  p.addPath(v)
  p.addRect(CGRect(x: vx, y: y, width: H * 0.06 + thick + H * 0.10, height: serifH))
  p.addRect(CGRect(x: vx + vw - H * 0.06 - thin - H * 0.10, y: y, width: thin + H * 0.16, height: serifH))
  return (p, vx + vw - x)
}

// The shield, w wide, centred on (cx, cy).
func emblem(cx: CGFloat, cy: CGFloat, w: CGFloat) {
  let h = w * 1.2, x = cx - w / 2, y = cy - h / 2
  let outer = shield(x, y, w)
  // Shadow under it.
  ctx.saveGState()
  ctx.setShadow(offset: CGSize(width: 0, height: w * 0.025), blur: w * 0.06, color: rgb(0, 0, 0, 0.6))
  ctx.addPath(outer); ctx.setFillColor(rgb(40, 25, 8)); ctx.fillPath()
  ctx.restoreGState()
  // Gold rim.
  ctx.saveGState()
  ctx.addPath(outer); ctx.clip()
  ctx.drawLinearGradient(gradient([rgb(255, 229, 150), rgb(214, 160, 52), rgb(140, 92, 22), rgb(226, 180, 80)],
                                  [0, 0.35, 0.75, 1]),
                         start: CGPoint(x: x, y: y), end: CGPoint(x: x + w, y: y + h), options: [])
  ctx.restoreGState()
  // Crimson field, a dark line between.
  let r = w * 0.075
  let line = shield(x + r * 0.8, y + r * 0.8, w - r * 1.6)
  ctx.addPath(line); ctx.setFillColor(rgb(45, 22, 6)); ctx.fillPath()
  let fw = w - r * 2, field = shield(x + r, y + r, fw)
  ctx.saveGState()
  ctx.addPath(field); ctx.clip()
  let fc = CGPoint(x: cx - w * 0.1, y: y + h * 0.3)
  ctx.drawRadialGradient(gradient([rgb(190, 38, 38), rgb(120, 16, 20), rgb(58, 6, 10)], [0, 0.55, 1]),
                         startCenter: fc, startRadius: 0, endCenter: fc, endRadius: w * 0.95,
                         options: [.drawsAfterEndLocation])
  // A soft highlight across the top.
  ctx.drawLinearGradient(gradient([rgb(255, 255, 255, 0.16), rgb(255, 255, 255, 0)], [0, 1]),
                         start: CGPoint(x: cx, y: y), end: CGPoint(x: cx, y: y + h * 0.45), options: [])
  ctx.restoreGState()
  // IV, gold, with a dark edge and shadow.
  let H = h * 0.40
  let (_, iv) = numeral(0, 0, H)
  let (letters, _) = numeral(cx - iv / 2, y + h * 0.43 - H / 2, H)
  ctx.saveGState()
  ctx.setShadow(offset: CGSize(width: 0, height: H * 0.04), blur: H * 0.08, color: rgb(0, 0, 0, 0.7))
  ctx.addPath(letters); ctx.setFillColor(rgb(50, 26, 4)); ctx.fillPath()
  ctx.restoreGState()
  ctx.saveGState()
  ctx.addPath(letters); ctx.setLineWidth(H * 0.035); ctx.setLineJoin(.miter)
  ctx.setStrokeColor(rgb(60, 30, 4)); ctx.strokePath()
  ctx.addPath(letters); ctx.clip()
  let ly = y + h * 0.43 - H / 2
  ctx.drawLinearGradient(gradient([rgb(255, 240, 180), rgb(236, 190, 80), rgb(170, 112, 28)], [0, 0.5, 1]),
                         start: CGPoint(x: cx, y: ly), end: CGPoint(x: cx, y: ly + H), options: [])
  ctx.restoreGState()
}

switch style {
case "full":
  background(CGRect(x: 0, y: 0, width: N, height: N))
  emblem(cx: N / 2, cy: N / 2, w: N * 0.62)
case "fg":
  emblem(cx: N / 2, cy: N / 2, w: N * 0.46)
case "mac":
  // Apple's grid: an 824 px rounded square (radius 185) in 1024, a little above centre.
  let s = N * 824 / 1024, o = (N - s) / 2
  let rr = CGPath(roundedRect: CGRect(x: o, y: o - N * 0.01, width: s, height: s),
                  cornerWidth: s * 0.225, cornerHeight: s * 0.225, transform: nil)
  ctx.saveGState()
  ctx.setShadow(offset: CGSize(width: 0, height: N * 0.012), blur: N * 0.03, color: rgb(0, 0, 0, 0.45))
  ctx.addPath(rr); ctx.setFillColor(rgb(18, 11, 5)); ctx.fillPath()
  ctx.restoreGState()
  ctx.saveGState()
  ctx.addPath(rr); ctx.clip()
  background(CGRect(x: o, y: o - N * 0.01, width: s, height: s))
  ctx.restoreGState()
  emblem(cx: N / 2, cy: N / 2 - N * 0.01, w: s * 0.6)
default:
  emblem(cx: N / 2, cy: N / 2, w: N * 0.78)
}

let out = CGImageDestinationCreateWithURL(URL(fileURLWithPath: a[1]) as CFURL, UTType.png.identifier as CFString, 1, nil)!
CGImageDestinationAddImage(out, ctx.makeImage()!, nil)
guard CGImageDestinationFinalize(out) else { exit(1) }
