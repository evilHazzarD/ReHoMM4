// menu_button.h on iOS: a UIButton over SDL's view; a tap opens the list of
// languages (language.h: the app's and the latest translations release's); the
// one picked is saved, and an alert says it applies once the game is restarted.
#import <UIKit/UIKit.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "language.h"
#include "menu_button.h"

namespace {
std::string g_lang, g_saved, g_next;  // running, saved file, the one picked
UIButton* g_button = nil;  // language
UIButton* g_cloud = nil;   // saves in iCloud (save_sync_apple.mm)
bool g_shown = false;

NSString* Str(const std::string& s) { return [NSString stringWithUTF8String:s.c_str()]; }
NSString* Name(const std::string& l) { return Str(h4::FindLanguage(l).name); }

UIView* GameView() {
  for (UIScene* s in UIApplication.sharedApplication.connectedScenes) {
    if (![s isKindOfClass:UIWindowScene.class]) continue;
    for (UIWindow* w in ((UIWindowScene*)s).windows)
      if (w.isKeyWindow && w.rootViewController) return w.rootViewController.view;
  }
  return nil;
}

UIViewController* Top() {
  UIViewController* c = GameView().window.rootViewController;
  while (c.presentedViewController) c = c.presentedViewController;
  return c;
}

void Choose(const std::string& lang) {
  if (FILE* f = std::fopen(g_saved.c_str(), "w")) {
    std::fputs(lang.c_str(), f);
    std::fclose(f);
  }
  const h4::LanguageInfo l = h4::FindLanguage(lang);
  UIAlertController* a = [UIAlertController alertControllerWithTitle:Str(l.name)
                                                             message:Str(l.note)
                                                      preferredStyle:UIAlertControllerStyleAlert];
  [a addAction:[UIAlertAction actionWithTitle:@"OK"
                                        style:UIAlertActionStyleDefault
                                      handler:nil]];
  [Top() presentViewController:a animated:YES completion:nil];
}
}  // namespace

@interface H4LangTarget : NSObject
@end
@implementation H4LangTarget
- (void)cloud:(UIButton*)b {
  h4::ShowSaveSyncSheet((__bridge void*)b, g_lang == "uk");
}
- (void)tap:(UIButton*)b {
  // The list of languages; the one picked is saved, with the restart message.
  UIAlertController* sheet = [UIAlertController alertControllerWithTitle:nil
                                                                 message:nil
                                                          preferredStyle:UIAlertControllerStyleActionSheet];
  for (const h4::LanguageInfo& l : h4::Languages()) {
    const std::string code = l.code;
    NSString* title = Str((code == g_next ? "✓ " : "") + l.name);
    [sheet addAction:[UIAlertAction actionWithTitle:title
                                              style:UIAlertActionStyleDefault
                                            handler:^(UIAlertAction*) {
                                              if (code == g_next) return;
                                              g_next = code;
                                              Choose(code);
                                              [b setTitle:[@"🌐 " stringByAppendingString:Name(code)]
                                                 forState:UIControlStateNormal];
                                              [b sizeToFit];
                                              g_shown = false;  // placed again (its width changed)
                                            }]];
  }
  [sheet addAction:[UIAlertAction actionWithTitle:g_lang == "uk" ? @"Скасувати" : @"Cancel"
                                            style:UIAlertActionStyleCancel
                                          handler:nil]];
  sheet.popoverPresentationController.sourceView = b;  // iPad: a popover from the button
  sheet.popoverPresentationController.sourceRect = b.bounds;
  [Top() presentViewController:sheet animated:YES completion:nil];
}
@end

namespace h4 {

void SetLanguageChoice(const std::string& lang, const std::string& saved_path) {
  g_lang = g_next = lang;
  g_saved = saved_path;
}

UIButton* MakeButton(NSString* title, SEL action) {
  static H4LangTarget* target = [H4LangTarget new];
  UIButton* b = [UIButton buttonWithType:UIButtonTypeSystem];
  [b setTitle:title forState:UIControlStateNormal];
  [b setTitleColor:[UIColor colorWithRed:0.98 green:0.92 blue:0.78 alpha:1] forState:UIControlStateNormal];
  b.titleLabel.font = [UIFont boldSystemFontOfSize:12];
  b.backgroundColor = [UIColor colorWithRed:0.10 green:0.07 blue:0.03 alpha:0.85];
  b.layer.cornerRadius = 10;
  b.layer.borderWidth = 1.5;
  b.layer.borderColor = [UIColor colorWithRed:0.84 green:0.67 blue:0.34 alpha:1].CGColor;
  b.contentEdgeInsets = UIEdgeInsetsMake(6, 12, 6, 12);
  [b addTarget:target action:action forControlEvents:UIControlEventTouchUpInside];
  [b sizeToFit];
  return b;
}

// The language button (when the app has the translation), its top centre at (x, y) in the
// frame; the ☁️ one in the menu's top-left corner, its top-left at (left, y).
void ShowLanguageButton(bool shown, int x, int y, int left) {
  static int last_x = -1, last_y = -1, last_left = -1;
  if (shown == g_shown && (!shown || (x == last_x && y == last_y && left == last_left && g_cloud))) return;
  g_shown = shown;
  last_x = x;
  last_y = y;
  last_left = left;
  @autoreleasepool {
    UIView* view = GameView();
    if (!view) {
      g_shown = false;  // tried again next pass
      return;
    }
    if (!g_cloud) {
      g_cloud = MakeButton(@"☁️", @selector(cloud:));
      [view addSubview:g_cloud];
      if (!g_saved.empty()) {
        g_button = MakeButton([@"🌐 " stringByAppendingString:Name(g_lang)], @selector(tap:));
        [view addSubview:g_button];
      }
    }
    if (shown) {
      // The view's size changes with the zoom, not with this: placed again on every change.
      float vx, vy;
      FrameToView(x, y, float(view.bounds.size.width), float(view.bounds.size.height), &vx, &vy);
      if (vx < 0) {
        g_shown = false;  // nothing shown yet: tried again next pass
        return;
      }
      const CGFloat W = view.bounds.size.width;
      CGRect c = g_cloud.frame, l = g_button ? g_button.frame : CGRectZero;
      const CGFloat top = std::max(CGFloat(vy), view.safeAreaInsets.top + 4);
      if (g_button) {
        l.origin.x = std::min(std::max(CGFloat(vx) - l.size.width / 2, CGFloat(4)), W - l.size.width - 4);
        l.origin.y = top;
        g_button.frame = l;
        c.size.height = l.size.height;
      }
      float lx = 0, ly;
      FrameToView(left >= 0 ? left : 0, y, float(W), float(view.bounds.size.height), &lx, &ly);
      c.origin.x = std::max(CGFloat(lx), view.safeAreaInsets.left + 4);
      c.origin.y = top;
      g_cloud.frame = c;
      [view bringSubviewToFront:g_cloud];
      if (g_button) [view bringSubviewToFront:g_button];
    }
    g_cloud.hidden = !shown;
    g_button.hidden = !shown;
  }
}

}  // namespace h4
