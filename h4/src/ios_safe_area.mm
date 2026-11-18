// iOS: the screen's safe area (the notch, the rounded corners), so the game
// is shown, and touched, inside it (main.cpp, gfx::SetOutputInsets).

#import <UIKit/UIKit.h>

namespace h4 {

// The margin to keep clear on each side in landscape, as a fraction of the
// landscape width; the same on both sides (the phone may be turned either
// way). False on an iPad (it uses the whole screen) or if UIKit has no
// window yet.
bool IosSafeAreaMargin(float* fraction) {
  // iPads: the whole screen (no notch; their side insets are small or none).
  if (UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad) return false;
  UIWindow* w = nil;
  for (UIScene* s in UIApplication.sharedApplication.connectedScenes) {
    if (![s isKindOfClass:UIWindowScene.class]) continue;
    for (UIWindow* x in ((UIWindowScene*)s).windows) {
      w = x;
      break;
    }
    if (w) break;
  }
  if (!w) return false;
  const UIEdgeInsets in = w.safeAreaInsets;
  const CGSize sz = w.bounds.size;
  const bool landscape = sz.width >= sz.height;
  // In portrait (not laid out for landscape yet) the notch is at the top.
  const CGFloat side = landscape ? MAX(in.left, in.right) : MAX(in.top, in.bottom);
  const CGFloat width = MAX(sz.width, sz.height);
  if (width <= 0) return false;
  *fraction = float(side / width);
  return true;
}

}  // namespace h4
