// menu_button.h on iOS: UIButtons over SDL's view. A tap on ⚙︎ opens the settings, a menu: the
// languages (language.h: the app's and the latest translations release's; the one picked is
// saved, and an alert says it applies once the game is restarted) and the checkbox (a checkmark)
// that shows the ☁️ button, the save sync one (save_sync_apple.mm).
#import <UIKit/UIKit.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "language.h"
#include "gamepad_help.h"
#include "intro.h"
#include "menu_button.h"

namespace {
std::string g_lang, g_saved, g_next;  // running, saved file, the one picked
UIButton* g_button = nil;  // ⚙︎: settings
UIButton* g_cloud = nil;   // saves in iCloud (save_sync_apple.mm)
UIButton* g_kofi = nil;    // ☕: the author's Ko-fi page
UILabel* g_title = nil;    // "ReHoMM4 0.1", left of them
bool g_shown = false;

NSString* Str(const std::string& s) { return [NSString stringWithUTF8String:s.c_str()]; }
bool Uk() { return g_lang == "uk"; }

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
- (void)kofi:(UIButton*)b {
  [UIApplication.sharedApplication openURL:[NSURL URLWithString:@"https://ko-fi.com/hazzard"]
                                   options:@{}
                         completionHandler:nil];
}
- (void)cloud:(UIButton*)b {
  h4::ShowSaveSyncSheet((__bridge void*)b, Uk());
}
@end

namespace {
// Updates: the latest release on GitHub against the app's version. An app cannot install an .ipa
// itself (AltStore or Sideloadly does): a newer one opens the release's page.
void Alert(NSString* title, NSString* text, NSArray<UIAlertAction*>* more) {
  UIAlertController* a = [UIAlertController alertControllerWithTitle:title
                                                             message:text
                                                      preferredStyle:UIAlertControllerStyleAlert];
  for (UIAlertAction* x in more) [a addAction:x];
  [a addAction:[UIAlertAction actionWithTitle:more.count ? (Uk() ? @"Пізніше" : @"Later") : @"OK"
                                        style:UIAlertActionStyleCancel
                                      handler:nil]];
  [Top() presentViewController:a animated:YES completion:nil];
}

bool Newer(NSString* a, NSString* b) {  // "0.10" > "0.9" ("v" in front ignored)
  auto nums = [](NSString* s) {
    std::vector<long> v;
    if ([s hasPrefix:@"v"]) s = [s substringFromIndex:1];
    for (NSString* p in [s componentsSeparatedByString:@"."]) v.push_back(p.integerValue);
    return v;
  };
  std::vector<long> x = nums(a), y = nums(b);
  const size_t n = std::max(x.size(), y.size());
  x.resize(n), y.resize(n);
  return x > y;
}

void CheckForUpdates() {
  const bool uk = Uk();
  NSMutableURLRequest* r = [NSMutableURLRequest
      requestWithURL:[NSURL URLWithString:@"https://api.github.com/repos/evilHazzarD/ReHoMM4/releases/latest"]];
  [r setValue:@"application/vnd.github+json" forHTTPHeaderField:@"Accept"];
  r.timeoutInterval = 20;
  [[NSURLSession.sharedSession
      dataTaskWithRequest:r
        completionHandler:^(NSData* data, NSURLResponse*, NSError* err) {
          NSDictionary* rel = data ? [NSJSONSerialization JSONObjectWithData:data options:0 error:nil] : nil;
          dispatch_async(dispatch_get_main_queue(), ^{
            NSString* tag = [rel isKindOfClass:NSDictionary.class] ? rel[@"tag_name"] : nil;
            NSString* have = [NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"] ?: @"0";
            if (![tag isKindOfClass:NSString.class]) {
              Alert(uk ? @"Не вдалося перевірити" : @"Could not check",
                    err ? err.localizedDescription : (uk ? @"GitHub не відповів як треба." : @"GitHub gave no release."), @[]);
            } else if (!Newer(tag, have)) {
              Alert(uk ? @"Оновлень немає" : @"Up to date",
                    [NSString stringWithFormat:uk ? @"У вас остання версія: %@." : @"You have the latest version: %@.", have], @[]);
            } else {
              NSURL* page = [NSURL URLWithString:rel[@"html_url"] ?: @"https://github.com/evilHazzarD/ReHoMM4/releases"];
              Alert(uk ? @"Є нова версія" : @"A new version",
                    [NSString stringWithFormat:uk ? @"ReHoMM4 %@ (у вас %@). Завантажте HeroesIV-ios.ipa і встановіть "
                                                    @"через AltStore чи Sideloadly, як і першу."
                                                  : @"ReHoMM4 %@ (you have %@). Download HeroesIV-ios.ipa and install "
                                                    @"it with AltStore or Sideloadly, as the first one.",
                                               tag, have],
                    @[ [UIAlertAction actionWithTitle:uk ? @"Відкрити реліз" : @"Open the release"
                                                style:UIAlertActionStyleDefault
                                              handler:^(UIAlertAction*) {
                                                [UIApplication.sharedApplication openURL:page options:@{}
                                                                       completionHandler:nil];
                                              }] ]);
            }
          });
        }] resume];
}

// The settings menu of ⚙︎, made again after every change (the checkmarks).
void SetMenu(UIButton* b) {
  NSMutableArray<UIMenuElement*>* langs = [NSMutableArray array];
  if (!g_saved.empty())
    for (const h4::LanguageInfo& l : h4::Languages()) {
      const std::string code = l.code;
      UIAction* a = [UIAction actionWithTitle:Str(l.name) image:nil identifier:nil handler:^(UIAction*) {
        if (code == g_next) return;
        g_next = code;
        Choose(code);
        SetMenu(b);
      }];
      a.state = code == g_next ? UIMenuElementStateOn : UIMenuElementStateOff;
      [langs addObject:a];
    }
  UIAction* sync = [UIAction actionWithTitle:Uk() ? @"Синхронізація збережень (кнопка ☁️ у меню)"
                                                  : @"Sync saved games (the ☁️ button in the menu)"
                                       image:nil
                                  identifier:nil
                                     handler:^(UIAction*) {
                                       h4::SetSaveSync(!h4::ShowSyncButton(), (__bridge void*)b);
                                       SetMenu(b);
                                     }];
  sync.state = h4::ShowSyncButton() ? UIMenuElementStateOn : UIMenuElementStateOff;
  UIAction* intro = [UIAction actionWithTitle:Uk() ? @"Пропускати вступні відео (з наступного запуску)"
                                                   : @"Skip the intro movies (from the next start)"
                                        image:nil
                                   identifier:nil
                                      handler:^(UIAction*) {
                                        h4::SetSkipIntro(!h4::SkipIntro());
                                        SetMenu(b);
                                      }];
  intro.state = h4::SkipIntro() ? UIMenuElementStateOn : UIMenuElementStateOff;
  UIAction* pad = [UIAction actionWithTitle:Str(h4::GamepadHelpTitle(Uk()))
                                      image:[UIImage systemImageNamed:@"gamecontroller"]
                                 identifier:nil
                                    handler:^(UIAction*) {
                                      Alert(Str(h4::GamepadHelpTitle(Uk())), Str(h4::GamepadHelp(Uk())), @[]);
                                    }];
  NSMutableArray<UIMenuElement*>* items = [NSMutableArray array];
  if (langs.count)
    [items addObject:[UIMenu menuWithTitle:Uk() ? @"Мова гри" : @"Game language"
                                     image:nil
                                identifier:nil
                                   options:UIMenuOptionsDisplayInline
                                  children:langs]];
  [items addObject:[UIMenu menuWithTitle:@"" image:nil identifier:nil options:UIMenuOptionsDisplayInline
                                children:@[ sync, intro, pad ]]];
  UIAction* upd = [UIAction actionWithTitle:Uk() ? @"Перевірити оновлення" : @"Check for updates"
                                      image:[UIImage systemImageNamed:@"arrow.down.circle"]
                                 identifier:nil
                                    handler:^(UIAction*) { CheckForUpdates(); }];
  // The version: the update check's section's title (not in the main menu).
  NSString* v = [NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
  [items addObject:[UIMenu menuWithTitle:[@"ReHoMM4 v" stringByAppendingString:v ?: @""]
                                   image:nil
                              identifier:nil
                                 options:UIMenuOptionsDisplayInline
                                children:@[ upd ]]];
  b.menu = [UIMenu menuWithTitle:Uk() ? @"Налаштування" : @"Settings" children:items];
  b.showsMenuAsPrimaryAction = YES;
}
}  // namespace


namespace h4 {

void SetLanguageChoice(const std::string& lang, const std::string& saved_path) {
  g_lang = g_next = lang;
  g_saved = saved_path;
}

// The checkbox in the settings: the app's defaults, off at first.

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
  if (action) [b addTarget:target action:action forControlEvents:UIControlEventTouchUpInside];
  [b sizeToFit];
  return b;
}

// The menu's icon buttons (⚙︎, ☁️): a square with the icon bigger.
UIButton* MakeIcon(NSString* icon, SEL action, CGFloat font = 22) {
  UIButton* b = MakeButton(icon, action);
  b.titleLabel.font = [UIFont systemFontOfSize:font];
  b.contentEdgeInsets = UIEdgeInsetsZero;
  b.layer.cornerRadius = 10;
  b.frame = CGRectMake(0, 0, 44, 44);
  return b;
}

// An icon button with an SF Symbol (centred by UIKit, unlike a font's glyph) in cream.
UIButton* MakeSymbolIcon(NSString* symbol, SEL action, CGFloat size) {
  UIButton* b = MakeIcon(@"", action);
  UIImage* img = [UIImage systemImageNamed:symbol
                         withConfiguration:[UIImageSymbolConfiguration configurationWithPointSize:size]];
  [b setImage:img forState:UIControlStateNormal];
  b.tintColor = [UIColor colorWithRed:0.98 green:0.92 blue:0.78 alpha:1];
  return b;
}

// The ⚙︎ button, its top-right corner at (x, y) in the frame; the ☁️ one (when the settings
// show it) just left of it, and "ReHoMM4 <version>" left of them.
void ShowLanguageButton(bool shown, int x, int y, int left) {
  static int last_x = -1, last_y = -1, last_left = -1;
  static bool last_sync = false;
  if (ShowSyncButton() != last_sync) {  // ☁️ shown or hidden: the title moves
    last_sync = !last_sync;
    g_shown = false;
  }
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
      g_cloud = MakeIcon(@"☁️", @selector(cloud:));
      [view addSubview:g_cloud];
      g_button = MakeSymbolIcon(@"gearshape.fill", nil, 22);
      g_title = [UILabel new];
      NSString* v = [NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
      g_title.text = [@"ReHoMM4 " stringByAppendingString:v ?: @""];
      g_title.font = [UIFont boldSystemFontOfSize:14];
      g_title.textColor = [UIColor colorWithRed:0.98 green:0.92 blue:0.78 alpha:1];
      g_title.shadowColor = [UIColor colorWithWhite:0 alpha:0.9];
      g_title.shadowOffset = CGSizeMake(0, 1);
      [g_title sizeToFit];
      [view addSubview:g_title];
      SetMenu(g_button);
      [view addSubview:g_button];
      g_kofi = MakeSymbolIcon(@"cup.and.saucer.fill", @selector(kofi:), 20);
      g_kofi.accessibilityLabel = Uk() ? @"Підтримати автора на Ko-fi" : @"Support the author on Ko-fi";
      [view addSubview:g_kofi];
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
        l.origin.x = std::min(std::max(CGFloat(vx) - l.size.width, CGFloat(4)),
                              W - l.size.width - std::max(view.safeAreaInsets.right, CGFloat(4)));
        l.origin.y = top;
        g_button.frame = l;
      }
      CGRect k = g_kofi.frame;  // ☕ left of ⚙︎, ☁️ left of it
      k.origin.x = std::max(l.origin.x - k.size.width - 8, view.safeAreaInsets.left + 4);
      k.origin.y = top;
      g_kofi.frame = k;
      c.origin.x = std::max(k.origin.x - c.size.width - 8, view.safeAreaInsets.left + 4);
      c.origin.y = top;
      g_cloud.frame = c;
      CGRect t = g_title.frame;  // the title, left of the buttons shown
      t.origin.x = std::max((ShowSyncButton() ? c.origin.x : k.origin.x) - t.size.width - 10, CGFloat(4));
      t.origin.y = top + (l.size.height - t.size.height) / 2;
      g_title.frame = t;
      [view bringSubviewToFront:g_title];
      [view bringSubviewToFront:g_cloud];
      [view bringSubviewToFront:g_kofi];
      if (g_button) [view bringSubviewToFront:g_button];
    }
    if (shown && g_button && AskSaveSyncFolder()) {  // sync on by default: the folder, once
      UIButton* source = g_button;
      dispatch_async(dispatch_get_main_queue(), ^{ ShowSaveSyncSheet((__bridge void*)source, Uk()); });
    }
    g_cloud.hidden = !shown || !ShowSyncButton();
    g_button.hidden = !shown;
    g_kofi.hidden = !shown;
    g_title.hidden = YES;  // "ReHoMM4 v<version>" is in the settings
  }
}

}  // namespace h4
