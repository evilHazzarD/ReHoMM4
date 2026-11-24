// iOS: the screen's safe area (the notch, the rounded corners), so the game
// is shown, and touched, inside it (main.cpp, gfx::SetOutputInsets).

#include <sys/sysctl.h>

#include <string>

#import <UIKit/UIKit.h>

namespace h4 {

// The margin to keep clear on each side in landscape, as a fraction of the
// landscape width; the same on both sides (the phone may be turned either
// way). False on an iPad (it uses the whole screen) or if UIKit has no
// window yet.
bool IosPhone() { return UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPhone; }

// A big iPhone (the Plus and Pro Max ones): a long side of 926 points and more (12 Pro Max on), or
// 896 at 3x (XS Max, 11 Pro Max; the XR and the 11 have 896 at 2x, on a smaller screen).
bool IosBigPhone() {
  const CGSize s = UIScreen.mainScreen.bounds.size;
  const CGFloat l = MAX(s.width, s.height);
  return l >= 926 || (l >= 896 && UIScreen.mainScreen.scale >= 3);
}

// An iPad mini (by its model: the 5th has the 9.7-inch iPads' size in points): iPad11,1-2 (5th),
// iPad14,1-2 (6th), iPad16,1-2 (A17 Pro); a later one by its short side under 768 points.
bool IosIpadMini() {
  char model[64] = {};
  size_t n = sizeof model - 1;
  if (sysctlbyname("hw.machine", model, &n, nullptr, 0) == 0) {
    const std::string m(model);
    for (const char* mini : {"iPad11,1", "iPad11,2", "iPad14,1", "iPad14,2", "iPad16,1", "iPad16,2"})
      if (m == mini) return true;
  }
  const CGSize s = UIScreen.mainScreen.bounds.size;
  return MIN(s.width, s.height) < 768;
}

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
