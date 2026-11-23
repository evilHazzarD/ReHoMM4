// menu_button.h in the Mac app (H4_DESKTOP_APP): NSButtons over SDL's view.
// The ⚙︎ one (top right) opens the settings panel: a button per language (language.h; not an
// NSMenu: its modal tracking loop would stop the game's frames, and the hang watchdog with
// them), the one picked saved and the app started again with it; and the
// checkbox that shows the ☁️ button (left of ⚙︎, ShowSyncButton), which opens the iCloud saves
// panel (save_sync_apple.mm). The panels (ShowMacChoice) are views
// over the game, not sheets: a sheet takes the window's focus, and after it the system cursor stays
// on screen next to the game's own.
#import <AppKit/AppKit.h>
#import <objc/runtime.h>
#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "language.h"
#include "intro.h"
#include "window_mode.h"
#include "pad_config.h"
#include "runtime/hle/user32_window.h"
#include "menu_button.h"

@class H4MacChoiceTarget;
namespace h4 {
NSButton* MakeButton(NSString* title, SEL action);
void PresentPanel(NSView* view, const std::vector<NSView*>& rows, H4MacChoiceTarget* target, CGFloat width,
                  int esc, int enter, bool close_x = false);
}  // namespace h4

namespace {
std::string g_lang, g_saved, g_next;  // running, saved file, the one picked
NSButton* g_button = nil;  // ⚙︎: settings
NSButton* g_cloud = nil;   // saves in iCloud
NSButton* g_kofi = nil;    // ☕: the author's Ko-fi page
NSTextField* g_title = nil;  // "ReHoMM4 0.1", left of them
bool g_shown = false;
bool Uk() { return g_lang == "uk"; }
bool UkUi() { return Uk(); }

NSString* Str(const std::string& s) { return [NSString stringWithUTF8String:s.c_str()]; }

NSView* GameView() {
  int n = 0;
  SDL_Window** wins = SDL_GetWindows(&n);
  NSWindow* w = nil;
  if (wins && n > 0)
    w = (__bridge NSWindow*)SDL_GetPointerProperty(SDL_GetWindowProperties(wins[0]),
                                                   SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
  SDL_free(wins);
  return w.contentView;
}

void BackToSettings();

void Choose(const std::string& lang) {
  if (FILE* f = std::fopen(g_saved.c_str(), "w")) {
    std::fputs(lang.c_str(), f);
    std::fclose(f);
  }
  // The app starts again with it at once (the game reads its text and fonts at start). Should
  // that fail: it applies at the next start, as the language's note says.
  h4::RestartApp();
  const h4::LanguageInfo l = h4::FindLanguage(lang);
  h4::ShowMacChoice((__bridge void*)g_button, l.name, l.note, {"OK"}, [](int) { BackToSettings(); });
}

NSColor* Gold() { return [NSColor colorWithRed:0.84 green:0.67 blue:0.34 alpha:1]; }
NSColor* Cream() { return [NSColor colorWithRed:0.98 green:0.92 blue:0.78 alpha:1]; }
NSView* g_overlay = nil;  // the panel's backdrop while one is up
NSTextField* g_panel_text = nil;  // its text (changed while it is up: the download's progress)
id g_keys = nil;          // its key monitor
}  // namespace

// The backdrop: the whole view, dimmed; it takes every click (the game gets none while a panel is
// up) and shows the arrow over itself.
@interface H4MacBackdrop : NSView
@end
@implementation H4MacBackdrop
- (BOOL)acceptsFirstMouse:(NSEvent*)e { return YES; }
- (void)mouseDown:(NSEvent*)e {}
- (void)mouseUp:(NSEvent*)e {}
- (void)rightMouseDown:(NSEvent*)e {}
- (void)rightMouseUp:(NSEvent*)e {}
- (void)mouseDragged:(NSEvent*)e {}
- (void)scrollWheel:(NSEvent*)e {}
- (void)resetCursorRects { [self addCursorRect:self.bounds cursor:NSCursor.arrowCursor]; }
@end

// A panel's buttons call back with their tag.
@interface H4MacChoiceTarget : NSObject
@property(nonatomic) std::function<void(int)> done;
@property(nonatomic) std::vector<std::function<bool(bool)>> checked;  // by the checkbox's tag
@property(nonatomic) std::function<void(int)> popped;                 // a dropdown's pick
@property(nonatomic) std::function<void(int, float)> slid;            // a slider moved (its tag, value)
@property(nonatomic) std::function<void(int, int)> chose;             // a tagged dropdown's pick
@property(nonatomic) std::vector<std::string> paths;                  // the Show buttons' folders
@end
@implementation H4MacChoiceTarget
- (void)pick:(NSButton*)b { [self finish:int(b.tag)]; }
- (void)check:(NSButton*)b {
  const auto& c = self.checked;
  if (size_t(b.tag) < c.size() && c[size_t(b.tag)])  // what it is now (turning on may fail)
    b.state = c[size_t(b.tag)](b.state == NSControlStateValueOn) ? NSControlStateValueOn : NSControlStateValueOff;
}
- (void)popup:(NSPopUpButton*)p {
  if (self.popped) self.popped(int(p.indexOfSelectedItem));
}
- (void)slider:(NSSlider*)s {
  if (NSTextField* v = objc_getAssociatedObject(s, "value"))
    v.stringValue = [NSString stringWithFormat:@"%d%%", int(std::lround(s.doubleValue * 100))];
  if (self.slid) self.slid(int(s.tag), float(s.doubleValue));
}
- (void)choose:(NSPopUpButton*)p {
  if (self.chose) self.chose(int(p.tag), int(p.indexOfSelectedItem));
}
- (void)show:(NSButton*)b {
  if (size_t(b.tag) >= self.paths.size()) return;
  NSString* p = @(self.paths[size_t(b.tag)].c_str());
  [NSFileManager.defaultManager createDirectoryAtPath:p withIntermediateDirectories:YES attributes:nil error:nil];
  [NSWorkspace.sharedWorkspace openURL:[NSURL fileURLWithPath:p isDirectory:YES]];  // in Finder
}
- (void)finish:(int)i {
  NSView* o = g_overlay;
  if (!o) return;
  g_overlay = nil;
  if (g_keys) [NSEvent removeMonitor:g_keys], g_keys = nil;
  NSWindow* w = o.window;
  [o removeFromSuperview];
  // Back to SDL's cursor (the invisible one) where the pointer is, without waiting for it to move.
  if (w) {
    [w invalidateCursorRectsForView:w.contentView];
    if ([NSCursor respondsToSelector:@selector(invisibleCursor)] && !SDL_CursorVisible())
      [(NSCursor*)[NSCursor performSelector:@selector(invisibleCursor)] set];
  }
  auto done = self.done;  // the panel's target goes with it
  if (done) done(i);
}
@end

// Updates: the latest release on GitHub against the app's version; on a newer one, its DMG for this
// Mac downloaded into ~/Downloads and opened (the player drags the app into Applications).
namespace {
NSString* const kReleases = @"https://api.github.com/repos/evilHazzarD/ReHoMM4/releases/latest";
// A release's file: reHoMM4-v<version>-macos-<arch>.dmg (0.2: HeroesIV-<version>-…; 0.1: none).
#if defined(__arm64__)
NSString* const kDmg = @"macos-arm64.dmg";
#else
NSString* const kDmg = @"macos-x86_64.dmg";
#endif
bool IsOurDmg(NSString* name) {
  return ([name hasPrefix:@"reHoMM4-"] || [name hasPrefix:@"HeroesIV-"]) &&
         [name hasSuffix:[@"-" stringByAppendingString:kDmg]];
}
bool UkUi();
NSString* AppVersion() {
  return [NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"] ?: @"0";
}

// "0.10" > "0.9": by numbers ("v" in front ignored).
bool Newer(NSString* a, NSString* b) {
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

void ClosePanel() {
  if (!g_overlay) return;
  H4MacChoiceTarget* t = objc_getAssociatedObject(g_overlay, "target");
  [t finish:-1];
}

void ShowSettings();
// Back to the settings once a panel opened from them is closed (after it is gone).
void BackToSettings() {
  dispatch_async(dispatch_get_main_queue(), ^{ ShowSettings(); });
}

void Say(const std::string& title, const std::string& text) {
  ClosePanel();
  h4::ShowMacChoice((__bridge void*)g_button, title, text, {"OK"}, [](int) { BackToSettings(); });
}

void Download(NSURL* url, NSString* version) {
  const bool uk = UkUi();
  (void)version;
  NSString* dest_name = url.lastPathComponent;  // the release's own name for it
  auto cancelled = std::make_shared<bool>(false);  // Cancel pressed (or the panel closed)
  h4::ShowMacChoice((__bridge void*)g_button, uk ? "Завантаження" : "Downloading",
                    std::string(dest_name.UTF8String) + "…", {uk ? "Скасувати" : "Cancel"},
                    [cancelled](int i) {
                      *cancelled = true;
                      if (i >= 0) BackToSettings();
                    });
  __block NSURLSessionDownloadTask* task = nil;
  __block NSTimer* timer = nil;
  task = [NSURLSession.sharedSession
      downloadTaskWithURL:url
        completionHandler:^(NSURL* tmp, NSURLResponse* resp, NSError* err) {
          // The file at `tmp` is gone once this returns: moved here, the rest on the main thread.
          NSURL* downloads = [NSFileManager.defaultManager URLsForDirectory:NSDownloadsDirectory
                                                                  inDomains:NSUserDomainMask].firstObject;
          NSString* base = dest_name.stringByDeletingPathExtension;
          NSURL* dest = [downloads URLByAppendingPathComponent:[base stringByAppendingString:@".dmg"]];
          for (int i = 2; [NSFileManager.defaultManager fileExistsAtPath:dest.path]; i++)  // never over a file
            dest = [downloads URLByAppendingPathComponent:[NSString stringWithFormat:@"%@ %d.dmg", base, i]];
          NSError* moveErr = nil;
          const long status = [resp isKindOfClass:NSHTTPURLResponse.class] ? ((NSHTTPURLResponse*)resp).statusCode : 0;
          const bool ok = !err && tmp && status == 200 &&
                          [NSFileManager.defaultManager moveItemAtURL:tmp toURL:dest error:&moveErr];
          NSString* why = err ? err.localizedDescription
                        : moveErr ? moveErr.localizedDescription
                                  : [NSString stringWithFormat:@"HTTP %ld", status];
          dispatch_async(dispatch_get_main_queue(), ^{
            [timer invalidate];
            if (err.code == NSURLErrorCancelled) return;
            if (!ok) {
              Say(uk ? "Не вдалося завантажити" : "Download failed", why.UTF8String);
              return;
            }
            [NSWorkspace.sharedWorkspace openURL:dest];  // mounts it, Finder shows the app
            Say(uk ? "Завантажено" : "Downloaded",
                std::string(uk ? "ReHoMM4 " : "ReHoMM4 ") + version.UTF8String +
                    (uk ? " у «Завантаженнях» (" : " is in Downloads (") + dest.lastPathComponent.UTF8String +
                    (uk ? "). Закрийте гру й перетягніть Heroes IV у «Програми», замінивши стару."
                        : "). Quit the game and drag Heroes IV into Applications, replacing the old one."));
          });
        }];
  timer = [NSTimer scheduledTimerWithTimeInterval:0.25
                                          repeats:YES
                                            block:^(NSTimer*) {
    if (*cancelled) {
      [task cancel];
      [timer invalidate];
      return;
    }
    const int64_t got = task.countOfBytesReceived, all = task.countOfBytesExpectedToReceive;
    if (g_panel_text && all > 0)
      g_panel_text.stringValue = [NSString stringWithFormat:@"%@: %lld / %lld MB (%d%%)", dest_name,
                                  got >> 20, all >> 20, int(got * 100 / all)];
  }];
  [task resume];
}

void CheckForUpdates();
void ShowGamepad();

// The settings panel (the game's frames go on while it is up), top to bottom:
// "ReHoMM4 v<version>" with the update check, the language (a dropdown), fullscreen, the intro
// movies, save sync, the saves' folder (the synced one while the sync is on), the game's folder; ✕
// closes it.
void ShowSettings() {
  NSView* view = g_button.window.contentView;
  if (!view || g_overlay) return;
  const bool uk = UkUi();
  const CGFloat width = 440, pad = 18, inner = width - 2 * pad;
  H4MacChoiceTarget* target = [H4MacChoiceTarget new];
  enum { kUpdate, kClose, kPad };
  target.done = [uk](int i) {
    if (i == kUpdate) dispatch_async(dispatch_get_main_queue(), ^{ CheckForUpdates(); });
    if (i == kPad) dispatch_async(dispatch_get_main_queue(), ^{ ShowGamepad(); });
  };
  std::vector<NSView*> rows;
  auto text = [&](const std::string& t, NSFont* font, NSColor* color, NSTextAlignment align) {
    NSTextField* f = [NSTextField labelWithString:Str(t)];
    f.font = font;
    f.textColor = color;
    f.alignment = align;
    f.frame = NSMakeRect(0, 0, inner, f.fittingSize.height);
    return f;
  };
  std::vector<std::function<bool(bool)>> checked;
  // A checkbox in the panel's gold (the system's is the accent blue): a toggle button with a
  // square and a ticked square (SF Symbols, tinted).
  auto check = [&](const std::string& t, bool on, std::function<bool(bool)> changed) {
    NSButton* c = [NSButton buttonWithTitle:Str(t) target:target action:@selector(check:)];
    c.buttonType = NSButtonTypeToggle;
    c.bordered = NO;
    NSImageSymbolConfiguration* cfg = [NSImageSymbolConfiguration configurationWithPointSize:15
                                                                                     weight:NSFontWeightRegular];
    c.image = [[NSImage imageWithSystemSymbolName:@"square" accessibilityDescription:nil]
        imageWithSymbolConfiguration:cfg];
    c.alternateImage = [[NSImage imageWithSystemSymbolName:@"checkmark.square.fill" accessibilityDescription:nil]
        imageWithSymbolConfiguration:cfg];
    c.contentTintColor = Gold();
    c.imagePosition = NSImageLeft;
    c.imageHugsTitle = YES;
    c.alignment = NSTextAlignmentLeft;
    c.attributedTitle = [[NSAttributedString alloc]
        initWithString:[@" " stringByAppendingString:Str(t)]
            attributes:@{NSFontAttributeName : [NSFont systemFontOfSize:12], NSForegroundColorAttributeName : Cream()}];
    c.tag = NSInteger(checked.size());
    c.state = on ? NSControlStateValueOn : NSControlStateValueOff;
    c.frame = NSMakeRect(0, 0, inner, 26);
    checked.push_back(std::move(changed));
    rows.push_back(c);
  };
  std::vector<std::string> paths;
  // A row: its title at the left, `right` (a control) at the right, both centred on the row.
  auto titled = [&](const std::string& t, NSView* right, CGFloat h) {
    NSView* row = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, inner, h)];
    NSTextField* l = text(t, [NSFont boldSystemFontOfSize:12], Gold(), NSTextAlignmentLeft);
    const CGFloat rw = right.frame.size.width, rh = right.frame.size.height;
    l.frame = NSMakeRect(0, (h - l.frame.size.height) / 2, inner - rw - 8, l.frame.size.height);
    right.frame = NSMakeRect(inner - rw, (h - rh) / 2, rw, rh);
    [row addSubview:l];
    [row addSubview:right];
    rows.push_back(row);
  };
  // A folder: its title with Show at the right, the path (~ for home) under them.
  auto folder = [&](const std::string& t, const std::string& path) {
    NSButton* b = h4::MakeButton(uk ? @"Показати" : @"Show", @selector(show:));
    b.target = target;
    b.tag = NSInteger(paths.size());
    b.frame = NSMakeRect(0, 0, 80, 24);
    titled(t, b, 24);
    std::string shown = path;
    if (const char* home = std::getenv("HOME"); home && *home && shown.rfind(home, 0) == 0)
      shown = "~" + shown.substr(std::strlen(home));
    NSTextField* p = text(shown, [NSFont systemFontOfSize:11], Cream(), NSTextAlignmentLeft);
    p.selectable = YES;
    p.lineBreakMode = NSLineBreakByTruncatingMiddle;
    p.toolTip = Str(path);
    rows.push_back(p);
    paths.push_back(path);
  };

  rows.push_back(text(uk ? "Налаштування" : "Settings", [NSFont boldSystemFontOfSize:15], Gold(),
                      NSTextAlignmentCenter));
  {  // at the top: "ReHoMM4 <version>" and the update check right of it
    const CGFloat bh = 30, bw = 210;
    NSView* row = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, inner, bh)];
    NSTextField* v = text("ReHoMM4 v" + std::string(AppVersion().UTF8String), [NSFont boldSystemFontOfSize:12],
                          Gold(), NSTextAlignmentLeft);
    v.frame = NSMakeRect(0, (bh - v.frame.size.height) / 2, inner - bw - 8, v.frame.size.height);
    [row addSubview:v];
    NSButton* b = h4::MakeButton(Str(uk ? "Перевірити оновлення" : "Check for updates"), @selector(pick:));
    b.target = target;
    b.tag = kUpdate;
    b.frame = NSMakeRect(inner - bw, 0, bw, bh);
    [row addSubview:b];
    rows.push_back(row);
  }
  const std::vector<h4::LanguageInfo> all = g_saved.empty() ? std::vector<h4::LanguageInfo>{} : h4::Languages();
  if (!all.empty()) {
    NSPopUpButton* pop = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(0, 0, 180, 26) pullsDown:NO];
    for (const h4::LanguageInfo& l : all) [pop addItemWithTitle:Str(l.name)];
    for (size_t i = 0; i < all.size(); i++)
      if (all[i].code == g_next) [pop selectItemAtIndex:NSInteger(i)];
    pop.target = target;
    pop.action = @selector(popup:);
    titled(uk ? "Мова гри" : "Game language", pop, 28);
    target.popped = [all](int i) {
      if (i < 0 || size_t(i) >= all.size() || all[size_t(i)].code == g_next) return;
      g_next = all[size_t(i)].code;
      const std::string code = g_next;
      ClosePanel();  // its note (restart to apply), then the settings again
      dispatch_async(dispatch_get_main_queue(), ^{ Choose(code); });
    };
  } else if (const std::string gog = h4::GogLanguage(); !gog.empty()) {
    // Installed from GOG in that language: shown, nothing to choose.
    NSTextField* name = text(h4::GogLanguageName(gog) + " (GOG)", [NSFont systemFontOfSize:12], Cream(),
                             NSTextAlignmentRight);
    name.frame = NSMakeRect(0, 0, 180, name.frame.size.height);
    titled(uk ? "Мова гри" : "Game language", name, 28);
  }
  check(uk ? "Повноекранний режим" : "Fullscreen", h4::Fullscreen(), [](bool on) {
    h4::SetFullscreen(on);
    return on;
  });
  check(uk ? "Чорні бічні поля (з наступного запуску)" : "Black side panels (from the next start)",
        h4::ReadBlackSidePanels(h4::SidePanelsPath()), [](bool on) {
          return h4::SaveBlackSidePanels(on) ? on : h4::ReadBlackSidePanels(h4::SidePanelsPath());
        });
  check(uk ? "Пропускати вступні відео (з наступного запуску)" : "Skip the intro movies (from the next start)",
        h4::SkipIntro(), [](bool on) {
          h4::SetSkipIntro(on);
          return on;
        });
  check(uk ? "Синхронізація збережень з iCloud Drive (кнопка ☁️ у меню)"
           : "Sync saved games with iCloud Drive (the ☁️ button in the menu)",
        h4::ShowSyncButton(), [](bool on) {
          const bool now = h4::SetSaveSync(on, (__bridge void*)g_button);
          dispatch_async(dispatch_get_main_queue(), ^{  // again, with (or without) the synced folder
            ClosePanel();
            ShowSettings();
          });
          return now;
        });
  // One saves folder: the synced one while the sync is on (the saves are kept in step with it),
  // else the saves' own.
  if (const std::string synced = h4::SyncedFolder(); !synced.empty()) {
    folder(uk ? "Збереження (синхронізуються)" : "Saves (synced)", synced);
  } else if (const std::string saves = h4::SavesFolder(); !saves.empty()) {
    folder(uk ? "Збереження" : "Saves", saves);
  }
  if (!h4::GameFolder().empty()) {
    folder(uk ? "Тека гри" : "Game folder", h4::GameFolder());
  }
  {  // the gamepad's panel (ShowGamepad)
    NSButton* b = h4::MakeButton(uk ? @"Налаштувати" : @"Set up", @selector(pick:));
    b.target = target;
    b.tag = kPad;
    b.frame = NSMakeRect(0, 0, 110, 24);
    titled(uk ? "Геймпад" : "Gamepad", b, 28);
  }
  target.checked = checked;
  target.paths = paths;
  h4::PresentPanel(view, rows, target, width, kClose, kClose, true);  // ✕ (and Esc, Return): close
}

// The gamepad's panel: the sticks' speeds (sliders) and what each button does (a dropdown per
// button, two columns), the sticks' note under them (left-aligned); Reset, and ✕ back to the
// settings. Changes apply (and are saved) at once.
void ShowGamepad() {
  NSView* view = g_button.window.contentView;
  if (!view || g_overlay) return;
  const bool uk = UkUi();
  const CGFloat width = 780, pad = 18, inner = width - 2 * pad;
  H4MacChoiceTarget* target = [H4MacChoiceTarget new];
  enum { kClose, kReset };
  target.done = [](int i) {
    if (i == kReset) h4::ResetPadConfig();
    dispatch_async(dispatch_get_main_queue(), ^{ i == kClose ? ShowSettings() : ShowGamepad(); });
  };
  std::vector<NSView*> rows;
  auto text = [&](const std::string& t, NSFont* font, NSColor* color, CGFloat w) {
    NSTextField* f = [NSTextField wrappingLabelWithString:Str(t)];
    f.font = font;
    f.textColor = color;
    f.alignment = NSTextAlignmentLeft;
    f.preferredMaxLayoutWidth = w;
    f.frame = NSMakeRect(0, 0, w, f.fittingSize.height);
    return f;
  };
  NSTextField* title = text(std::string(uk ? "Геймпад — " : "Gamepad — ") + h4::PadStyleName(h4::PadStyle()),
                            [NSFont boldSystemFontOfSize:15], Gold(), inner);
  title.alignment = NSTextAlignmentCenter;
  rows.push_back(title);
  // A slider row: the title, the slider, its value (in %).
  enum { kPointer, kScroll };
  auto slider = [&](const std::string& t, int tag, float value, float lo, float hi) {
    const CGFloat h = 28, lw = 250, vw = 50;
    NSView* row = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, inner, h)];
    NSTextField* l = text(t, [NSFont boldSystemFontOfSize:12], Gold(), lw);
    l.frame = NSMakeRect(0, (h - l.frame.size.height) / 2, lw, l.frame.size.height);
    NSSlider* s = [NSSlider sliderWithValue:value minValue:lo maxValue:hi target:target action:@selector(slider:)];
    s.tag = tag;
    s.continuous = YES;
    s.frame = NSMakeRect(lw + 8, (h - 22) / 2, inner - lw - vw - 16, 22);
    NSTextField* v = text(std::to_string(int(std::lround(value * 100))) + "%", [NSFont systemFontOfSize:12], Cream(), vw);
    v.alignment = NSTextAlignmentRight;
    v.frame = NSMakeRect(inner - vw, (h - v.frame.size.height) / 2, vw, v.frame.size.height);
    objc_setAssociatedObject(s, "value", v, OBJC_ASSOCIATION_RETAIN);
    [row addSubview:l];
    [row addSubview:s];
    [row addSubview:v];
    rows.push_back(row);
  };
  slider(uk ? "Швидкість курсора (лівий стік)" : "Pointer speed (left stick)", kPointer, h4::PadPointerSpeed(),
         h4::kPointerMin, h4::kPointerMax);
  slider(uk ? "Швидкість прокрутки мапи (правий стік)" : "Map scrolling speed (right stick)", kScroll,
         h4::PadScrollSpeed(), h4::kScrollMin, h4::kScrollMax);
  target.slid = [](int tag, float value) {
    if (tag == kPointer) h4::SetPadPointerSpeed(value);
    if (tag == kScroll) h4::SetPadScrollSpeed(value);
  };
  rows.push_back(text(uk ? "Кнопки" : "Buttons", [NSFont boldSystemFontOfSize:13], Gold(), inner));
  // The buttons, two columns: the button's name, then its dropdown.
  const CGFloat col = (inner - 16) / 2, nw = 112, rh = 26;
  const int half = (h4::kPadButtons + 1) / 2;
  for (int r = 0; r < half; r++) {
    NSView* row = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, inner, rh)];
    for (int c = 0; c < 2; c++) {
      const int b = r + c * half;
      if (b >= h4::kPadButtons) continue;
      const CGFloat x = c * (col + 16);
      NSTextField* l = text(h4::PadButtonName(b, uk), [NSFont boldSystemFontOfSize:12], Cream(), nw);
      l.frame = NSMakeRect(x, (rh - l.frame.size.height) / 2, nw, l.frame.size.height);
      NSPopUpButton* p = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(x + nw, 0, col - nw, rh) pullsDown:NO];
      for (int a = 0; a < h4::PadActionCount(); a++) {
        [p addItemWithTitle:Str(h4::PadActionName(a, uk))];
        p.lastItem.tag = a;  // (two titles alike would be one item)
      }
      [p selectItemAtIndex:h4::PadBinding(b)];
      p.tag = b;
      p.target = target;
      p.action = @selector(choose:);
      [row addSubview:l];
      [row addSubview:p];
    }
    rows.push_back(row);
  }
  enum { kPresetTag = 1000 };  // the preset dropdown's tag (the buttons': 0...)
  target.chose = [](int b, int a) {
    if (b != kPresetTag) return h4::SetPadBinding(b, a);
    h4::ApplyPadPreset(a);
    dispatch_async(dispatch_get_main_queue(), ^{  // again, with its names
      ClosePanel();
      ShowGamepad();
    });
  };
  rows.push_back(text(uk ? "Геймпадом: клік — натиснути або обрати наступний пункт списку, правий клік — попередній, "
                           "Esc — закрити."
                         : "With a gamepad: click presses or takes a list's next item, right-click its previous one, "
                           "Esc closes.",
                      [NSFont systemFontOfSize:11], Cream(), inner));
  {  // at the bottom: the preset (a dropdown) at the left, Reset at the right
    const CGFloat bh = 30, bw = 230, lw = 70;
    NSView* row = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, inner, bh)];
    NSTextField* l = text(uk ? "Пресет" : "Preset", [NSFont boldSystemFontOfSize:12], Gold(), lw);
    l.frame = NSMakeRect(0, (bh - l.frame.size.height) / 2, lw, l.frame.size.height);
    NSPopUpButton* p = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(lw, 2, 180, 26) pullsDown:NO];
    for (int k = 0; k < h4::kPadStyles; k++) [p addItemWithTitle:Str(h4::PadStyleName(k))];
    [p selectItemAtIndex:h4::PadStyle()];
    p.tag = kPresetTag;
    p.target = target;
    p.action = @selector(choose:);
    NSButton* reset = h4::MakeButton(Str(uk ? "Скинути до стандартних" : "Reset to the defaults"),
                                     @selector(pick:));
    reset.target = target;
    reset.tag = kReset;
    reset.frame = NSMakeRect(inner - bw, 0, bw, bh);
    [row addSubview:l];
    [row addSubview:p];
    [row addSubview:reset];
    rows.push_back(row);
  }
  h4::PresentPanel(view, rows, target, width, kClose, kClose, true);
}

void CheckForUpdates() {
  const bool uk = UkUi();
  auto cancelled = std::make_shared<bool>(false);  // Cancel pressed (or the panel closed)
  h4::ShowMacChoice((__bridge void*)g_button, uk ? "Оновлення" : "Updates",
                    uk ? "Перевіряю GitHub…" : "Checking GitHub…", {uk ? "Скасувати" : "Cancel"},
                    [cancelled](int i) {
                      *cancelled = true;
                      if (i >= 0) BackToSettings();  // Cancel (not closed for the answer)
                    });
  NSMutableURLRequest* r = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:kReleases]];
  [r setValue:@"application/vnd.github+json" forHTTPHeaderField:@"Accept"];
  [r setValue:@"ReHoMM4" forHTTPHeaderField:@"User-Agent"];
  r.timeoutInterval = 20;
  [[NSURLSession.sharedSession
      dataTaskWithRequest:r
        completionHandler:^(NSData* data, NSURLResponse* resp, NSError* err) {
          NSDictionary* rel = data ? [NSJSONSerialization JSONObjectWithData:data options:0 error:nil] : nil;
          dispatch_async(dispatch_get_main_queue(), ^{
            if (*cancelled) return;
            NSString* tag = [rel isKindOfClass:NSDictionary.class] ? rel[@"tag_name"] : nil;
            if (![tag isKindOfClass:NSString.class]) {
              Say(uk ? "Не вдалося перевірити" : "Could not check",
                  err ? err.localizedDescription.UTF8String : (uk ? "GitHub не відповів як треба." : "GitHub gave no release."));
              return;
            }
            NSString* have = AppVersion();
            if (!Newer(tag, have)) {
              Say(uk ? "Оновлень немає" : "Up to date",
                  std::string(uk ? "У вас остання версія: " : "You have the latest version: ") + have.UTF8String + ".");
              return;
            }
            NSURL* dmg = nil;
            for (NSDictionary* a in rel[@"assets"])
              if ([a isKindOfClass:NSDictionary.class] && [a[@"name"] isKindOfClass:NSString.class] && IsOurDmg(a[@"name"]))
                dmg = [NSURL URLWithString:a[@"browser_download_url"]];
            NSURL* page = [NSURL URLWithString:rel[@"html_url"] ?: @"https://github.com/evilHazzarD/ReHoMM4/releases"];
            ClosePanel();
            std::vector<std::string> buttons;
            if (dmg) buttons.push_back(uk ? "Завантажити" : "Download");
            buttons.push_back(uk ? "Що нового" : "What's new");
            buttons.push_back(uk ? "Пізніше" : "Later");
            h4::ShowMacChoice((__bridge void*)g_button, uk ? "Є нова версія" : "A new version",
                              std::string("ReHoMM4 ") + tag.UTF8String + (uk ? " (у вас " : " (you have ") +
                                  have.UTF8String + ")",
                              buttons, [dmg, page, tag](int i) {
                                if (dmg && i == 0) {
                                  dispatch_async(dispatch_get_main_queue(), ^{ Download(dmg, tag); });
                                  return;
                                }
                                if (i == (dmg ? 1 : 0)) [NSWorkspace.sharedWorkspace openURL:page];
                                BackToSettings();
                              });
          });
        }] resume];
}
}  // namespace

@interface H4MacLangTarget : NSObject
@end
@implementation H4MacLangTarget
- (void)cloud:(NSButton*)b {
  h4::ShowSaveSyncSheet((__bridge void*)b, g_lang == "uk");
}
- (void)kofi:(NSButton*)b {
  [NSWorkspace.sharedWorkspace openURL:[NSURL URLWithString:@"https://ko-fi.com/hazzard"]];
}
- (void)settings:(NSButton*)b {
  if (b.window && !g_overlay) ShowSettings();
}
@end

namespace h4 {

void SetLanguageChoice(const std::string& lang, const std::string& saved_path) {
  g_lang = g_next = lang;
  g_saved = saved_path;
}

// The checkbox in the settings: the app's defaults, off at first.

NSButton* MakeButton(NSString* title, SEL action) {
  static H4MacLangTarget* target = [H4MacLangTarget new];
  NSButton* b = [NSButton buttonWithTitle:title target:target action:action];
  b.bordered = NO;
  b.wantsLayer = YES;
  b.layer.backgroundColor = [NSColor colorWithRed:0.10 green:0.07 blue:0.03 alpha:0.85].CGColor;
  b.layer.cornerRadius = 8;
  b.layer.borderWidth = 1.5;
  b.layer.borderColor = [NSColor colorWithRed:0.84 green:0.67 blue:0.34 alpha:1].CGColor;
  b.attributedTitle = [[NSAttributedString alloc]
      initWithString:title
          attributes:@{
            NSFontAttributeName : [NSFont boldSystemFontOfSize:12],
            NSForegroundColorAttributeName : [NSColor colorWithRed:0.98 green:0.92 blue:0.78 alpha:1]
          }];
  [b sizeToFit];
  NSRect f = b.frame;
  f.size.width += 18;
  f.size.height += 10;
  b.frame = f;
  return b;
}

// "ReHoMM4 <the app's version>" in cream with a dark shadow (readable on the menu's picture).
NSTextField* MakeTitle() {
  NSString* v = [NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
  NSShadow* sh = [NSShadow new];
  sh.shadowColor = [NSColor colorWithWhite:0 alpha:0.9];
  sh.shadowOffset = NSMakeSize(0, -1);
  sh.shadowBlurRadius = 3;
  NSTextField* t = [NSTextField labelWithAttributedString:[[NSAttributedString alloc]
      initWithString:[@"ReHoMM4 " stringByAppendingString:v ?: @""]
          attributes:@{
            NSFontAttributeName : [NSFont boldSystemFontOfSize:14],
            NSForegroundColorAttributeName : [NSColor colorWithRed:0.98 green:0.92 blue:0.78 alpha:1],
            NSShadowAttributeName : sh
          }]];
  [t sizeToFit];
  return t;
}

// The menu's icon buttons (⚙︎, ☁️): a square with the icon bigger.
constexpr CGFloat kIcon = 40;
NSButton* MakeIcon(NSString* icon, SEL action, CGFloat font = 22) {
  NSButton* b = MakeButton(icon, action);
  b.attributedTitle = [[NSAttributedString alloc]
      initWithString:icon
          attributes:@{
            NSFontAttributeName : [NSFont systemFontOfSize:font],
            NSForegroundColorAttributeName : [NSColor colorWithRed:0.98 green:0.92 blue:0.78 alpha:1]
          }];
  b.layer.cornerRadius = 10;
  b.frame = NSMakeRect(0, 0, kIcon, kIcon);
  return b;
}

// An icon button with an SF Symbol (centred by AppKit, unlike a font's glyph) in cream.
NSButton* MakeSymbolIcon(NSString* symbol, SEL action, CGFloat size) {
  NSButton* b = MakeIcon(@"", action);
  NSImageSymbolConfiguration* cfg =
      [[NSImageSymbolConfiguration configurationWithPointSize:size weight:NSFontWeightRegular]
          configurationByApplyingConfiguration:[NSImageSymbolConfiguration configurationWithPaletteColors:@[
            [NSColor colorWithRed:0.98 green:0.92 blue:0.78 alpha:1]
          ]]];
  b.image = [[NSImage imageWithSystemSymbolName:symbol accessibilityDescription:nil] imageWithSymbolConfiguration:cfg];
  b.imagePosition = NSImageOnly;
  b.imageScaling = NSImageScaleNone;
  return b;
}

void ShowMacChoice(void* any_view, const std::string& title, const std::string& text,
                   const std::vector<std::string>& buttons, std::function<void(int)> done,
                   const std::vector<MacCheck>& checks) {
  NSView* view = ((__bridge NSView*)any_view).window.contentView;
  if (!view || g_overlay) return;
  const CGFloat width = 340, pad = 18, bh = 30;
  std::vector<NSView*> rows;  // top to bottom
  auto label = [&](const std::string& t, NSFont* font, NSColor* color) {
    NSTextField* f = [NSTextField wrappingLabelWithString:Str(t)];
    f.font = font;
    f.textColor = color;
    f.alignment = NSTextAlignmentCenter;
    f.preferredMaxLayoutWidth = width - 2 * pad;
    f.frame = NSMakeRect(0, 0, width - 2 * pad, f.fittingSize.height);
    rows.push_back(f);
  };
  if (!title.empty()) label(title, [NSFont boldSystemFontOfSize:15], Gold());
  g_panel_text = nil;
  if (!text.empty()) {
    label(text, [NSFont systemFontOfSize:12], Cream());
    g_panel_text = (NSTextField*)rows.back();
  }
  H4MacChoiceTarget* target = [H4MacChoiceTarget new];
  target.done = std::move(done);
  for (size_t i = 0; i < buttons.size(); i++) {
    NSButton* b = MakeButton(Str(buttons[i]), @selector(pick:));
    b.target = target;
    b.tag = NSInteger(i);
    b.frame = NSMakeRect(0, 0, width - 2 * pad, bh);
    // The checkboxes go above their button (`before`; the last one, Close, by default).
    std::vector<std::function<bool(bool)>> calls = target.checked;
    for (size_t k = 0; k < checks.size(); k++) {
      const MacCheck& check = checks[k];
      if ((check.before < 0 ? long(buttons.size()) - 1 : long(check.before)) != long(i)) continue;
      calls.resize(std::max(calls.size(), k + 1));
      calls[k] = check.changed;
      NSButton* c = [NSButton checkboxWithTitle:Str(check.label) target:target action:@selector(check:)];
      c.tag = NSInteger(k);
      c.attributedTitle = [[NSAttributedString alloc]
          initWithString:Str(check.label)
              attributes:@{NSFontAttributeName : [NSFont systemFontOfSize:12], NSForegroundColorAttributeName : Cream()}];
      c.state = check.on ? NSControlStateValueOn : NSControlStateValueOff;
      [c sizeToFit];
      c.frame = NSMakeRect(0, 0, width - 2 * pad, c.frame.size.height + 8);
      rows.push_back(c);
    }
    target.checked = calls;
    rows.push_back(b);
  }
  const int last = int(buttons.size()) - 1;
  PresentPanel(view, rows, target, width, last, checks.empty() ? 0 : last);  // settings: Return closes
}

// Puts the rows (top to bottom, each its own width) on a panel over the game. Keys go to the
// panel, not the game: Esc picks button `esc` (Cancel, OK, Close), Return button `enter`.
void PresentPanel(NSView* view, const std::vector<NSView*>& rows, H4MacChoiceTarget* target, CGFloat width,
                  int esc, int enter, bool close_x) {
  const CGFloat pad = 18, gap = 8;
  H4MacBackdrop* o = [[H4MacBackdrop alloc] initWithFrame:view.bounds];
  o.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
  o.wantsLayer = YES;
  o.layer.backgroundColor = [NSColor colorWithWhite:0 alpha:0.45].CGColor;
  o.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];  // the dropdowns on the dark panel
  NSView* panel = [NSView new];
  panel.wantsLayer = YES;
  panel.layer.backgroundColor = [NSColor colorWithRed:0.10 green:0.07 blue:0.03 alpha:0.96].CGColor;
  panel.layer.cornerRadius = 12;
  panel.layer.borderWidth = 1.5;
  panel.layer.borderColor = Gold().CGColor;
  CGFloat h = pad;
  for (NSView* r : rows) h += r.frame.size.height + gap;
  h += pad - gap;
  panel.frame = NSMakeRect(0, 0, width, h);
  CGFloat y = h - pad;  // AppKit's y goes up
  for (NSView* r : rows) {
    y -= r.frame.size.height;
    r.frame = NSMakeRect(pad, y, r.frame.size.width, r.frame.size.height);
    [panel addSubview:r];
    y -= gap;
  }
  if (close_x) {  // ✕ in the top-right corner: button `esc`
    NSButton* x = MakeButton(@"✕", @selector(pick:));
    x.target = target;
    x.tag = esc;
    x.frame = NSMakeRect(width - 26 - 10, h - 26 - 10, 26, 26);
    [panel addSubview:x];
  }
  panel.frame = NSMakeRect(std::round((view.bounds.size.width - width) / 2),
                           std::round((view.bounds.size.height - h) / 2), width, h);
  panel.autoresizingMask = NSViewMinXMargin | NSViewMaxXMargin | NSViewMinYMargin | NSViewMaxYMargin;
  [o addSubview:panel];
  [view addSubview:o positioned:NSWindowAbove relativeTo:nil];
  g_overlay = o;
  objc_setAssociatedObject(o, "target", target, OBJC_ASSOCIATION_RETAIN);
  g_keys = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown | NSEventMaskKeyUp
                                                 handler:^NSEvent*(NSEvent* e) {
    if (e.type == NSEventTypeKeyDown && e.keyCode == 53) [target finish:esc];
    else if (e.type == NSEventTypeKeyDown && (e.keyCode == 36 || e.keyCode == 76) && esc >= 0) [target finish:enter];
    return nil;
  }];
  [view.window invalidateCursorRectsForView:o];
  [NSCursor.arrowCursor set];
}

namespace {
// A gamepad's pointer (recomp::SetPadEventHook) is the game's, not the system's: its clicks reach the
// game, never these buttons and panels. Here they do: a click (A, on release) on one of them is that
// control's click (a dropdown takes its next item: its menu would track the system mouse; a
// segmented control the segment under the pointer); over a panel the system arrow follows the
// stick, every click is the panel's, and ZL / ZR (Esc / Return) are its keys, the others nothing.
NSView* g_pad_down = nil;  // what A went down on (its release there clicks it)
// Where the pad's pointer is: its button events carry no place (PadTick moves it there first).
float g_pad_x = -1, g_pad_y = -1;

NSView* PadHit(NSView* view, float x, float y, NSPoint* in_window) {
  const NSPoint p = NSMakePoint(x, view.isFlipped ? y : view.bounds.size.height - y);
  *in_window = [view convertPoint:p toView:nil];
  NSView* hit = [view.superview ?: view hitTest:[(view.superview ?: view) convertPoint:*in_window fromView:nil]];
  return hit == view ? nil : hit;  // SDL's view: the game's
}

NSSlider* SliderOf(NSView* hit) {
  for (NSView* v = hit; v; v = v.superview)
    if ([v isKindOfClass:NSSlider.class]) return (NSSlider*)v;
  return nil;
}

// The slider's value where the pointer is.
void SlideTo(NSSlider* s, NSPoint in_window) {
  const NSRect track = [s.cell barRectFlipped:s.isFlipped];
  const CGFloat x = [s convertPoint:in_window fromView:nil].x;
  const double f = std::clamp((x - track.origin.x) / std::max(track.size.width, CGFloat(1)), CGFloat(0), CGFloat(1));
  s.doubleValue = s.minValue + f * (s.maxValue - s.minValue);
  [s sendAction:s.action to:s.target];
}

// `step`: +1 a click (A), -1 the right button (B: a dropdown's previous item).
void PadClick(NSView* hit, NSPoint in_window, int step = 1) {
  for (NSView* v = hit; v; v = v.superview) {
    if ([v isKindOfClass:NSPopUpButton.class]) {
      NSPopUpButton* p = (NSPopUpButton*)v;
      if (p.numberOfItems > 0) {
        [p selectItemAtIndex:(p.indexOfSelectedItem + step + p.numberOfItems) % p.numberOfItems];
        [p sendAction:p.action to:p.target];
      }
      return;
    }
    if (step < 0) continue;
    if ([v isKindOfClass:NSSlider.class]) {
      SlideTo((NSSlider*)v, in_window);
      return;
    }
    if ([v isKindOfClass:NSSegmentedControl.class]) {
      NSSegmentedControl* c = (NSSegmentedControl*)v;
      const CGFloat x = [c convertPoint:in_window fromView:nil].x;
      CGFloat left = (c.bounds.size.width - [&] {
        CGFloat w = 0;
        for (NSInteger i = 0; i < c.segmentCount; i++) w += [c widthForSegment:i];
        return w;
      }()) / 2;
      for (NSInteger i = 0; i < c.segmentCount; i++) {
        left += [c widthForSegment:i];
        if (x < left || i == c.segmentCount - 1) {
          c.selectedSegment = i;
          [c sendAction:c.action to:c.target];
          return;
        }
      }
      return;
    }
    if ([v isKindOfClass:NSButton.class]) {
      if (((NSButton*)v).enabled) [(NSButton*)v performClick:nil];
      return;
    }
  }
}

void PostKey(unsigned short code, NSString* chars, NSWindow* w) {
  for (NSEventType t : {NSEventTypeKeyDown, NSEventTypeKeyUp})
    [NSApp postEvent:[NSEvent keyEventWithType:t location:NSZeroPoint modifierFlags:0
                                     timestamp:NSProcessInfo.processInfo.systemUptime
                                  windowNumber:w.windowNumber context:nil characters:chars
                   charactersIgnoringModifiers:chars isARepeat:NO keyCode:code]
             atStart:NO];
}

bool PadEvent(const SDL_Event& e) {
  @autoreleasepool {
    NSView* view = GameView();
    if (!view) return false;
    switch (e.type) {
      case SDL_EVENT_KEY_DOWN:
      case SDL_EVENT_KEY_UP:
        if (!g_overlay) return false;
        if (e.type == SDL_EVENT_KEY_DOWN && e.key.scancode == SDL_SCANCODE_ESCAPE)
          PostKey(53, @"\x1b", view.window);
        else if (e.type == SDL_EVENT_KEY_DOWN && e.key.scancode == SDL_SCANCODE_RETURN)
          PostKey(36, @"\r", view.window);
        return true;
      case SDL_EVENT_MOUSE_MOTION:
        g_pad_x = e.motion.x;
        g_pad_y = e.motion.y;
        if (g_overlay) {  // the arrow over the panel follows the stick (and drags a slider A holds)
          NSPoint w;
          PadHit(view, e.motion.x, e.motion.y, &w);
          if (NSSlider* s = SliderOf(g_pad_down)) SlideTo(s, w);
          const NSPoint s = [view.window convertPointToScreen:w];
          CGWarpMouseCursorPosition(CGPointMake(s.x, NSScreen.screens.firstObject.frame.size.height - s.y));
          CGAssociateMouseAndMouseCursorPosition(true);
        }
        return false;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
      case SDL_EVENT_MOUSE_BUTTON_UP: {
        NSPoint w;
        if (g_pad_x < 0) return false;
        NSView* hit = PadHit(view, g_pad_x, g_pad_y, &w);
        if (!hit && !g_overlay) return false;
        if (e.button.button == SDL_BUTTON_RIGHT && e.type == SDL_EVENT_MOUSE_BUTTON_UP && hit)
          PadClick(hit, w, -1);
        if (e.button.button != SDL_BUTTON_LEFT) return true;
        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
          g_pad_down = hit;
        } else {
          if (hit && hit == g_pad_down) {
            std::fprintf(stderr, "h4: gamepad click on %s\n", NSStringFromClass(hit.class).UTF8String);
            PadClick(hit, w);
          }
          g_pad_down = nil;
        }
        return true;
      }
      default:
        return false;
    }
  }
}
}  // namespace

// The ⚙︎ button, its top-right corner at (x, y) in the frame; the ☁️ one (when the settings
// show it) just left of it, and "ReHoMM4 <version>" left of them.
void ShowLanguageButton(bool shown, int x, int y, int left) {
  static int last_x = -1, last_y = -1, last_left = -1;
  static bool last_sync = false;
  if (ShowSyncButton() != last_sync) {  // ☁️ shown or hidden: the title moves
    last_sync = !last_sync;
    g_shown = false;
  }
  static CGSize last_size;
  static bool hooked = false;
  if (!hooked) {
    hooked = true;
    recomp::SetPadEventHook(PadEvent);
  }
  @autoreleasepool {
    NSView* view = GameView();
    if (!view) return;
    const CGSize size = view.bounds.size;
    if (shown == g_shown && (!shown || (x == last_x && y == last_y && left == last_left && g_cloud &&
                                        CGSizeEqualToSize(size, last_size))))
      return;
    g_shown = shown;
    last_x = x;
    last_y = y;
    last_left = left;
    last_size = size;
    if (!g_cloud) {
      g_cloud = MakeIcon(@"☁️", @selector(cloud:));
      [view addSubview:g_cloud];
      g_button = MakeSymbolIcon(@"gearshape.fill", @selector(settings:), 22);
      [view addSubview:g_button];
      g_kofi = MakeSymbolIcon(@"cup.and.saucer.fill", @selector(kofi:), 20);
      g_kofi.toolTip = g_lang == "uk" ? @"Підтримати автора на Ko-fi" : @"Support the author on Ko-fi";
      [view addSubview:g_kofi];
      g_title = MakeTitle();
      [view addSubview:g_title];
    }
    if (shown) {
      float vx, vy;
      FrameToView(x, y, float(size.width), float(size.height), &vx, &vy);
      if (vx < 0) {
        g_shown = false;  // nothing shown yet: tried again next pass
        return;
      }
      auto top = [&](NSButton* b) {
        return view.isFlipped ? CGFloat(vy) : size.height - CGFloat(vy) - b.frame.size.height;
      };
      if (g_button) {
        NSRect f = g_button.frame;
        f.origin.x = std::min(std::max(CGFloat(vx) - f.size.width, CGFloat(4)), size.width - f.size.width - 4);
        f.origin.y = top(g_button);
        g_button.frame = f;
        [g_button removeFromSuperview];  // on top of SDL's Metal layer
        [view addSubview:g_button positioned:NSWindowAbove relativeTo:nil];
      }
      NSRect k = g_kofi.frame;  // ☕ left of ⚙︎
      k.origin.x = std::max(g_button.frame.origin.x - k.size.width - 8, CGFloat(4));
      k.origin.y = top(g_kofi);
      g_kofi.frame = k;
      [g_kofi removeFromSuperview];
      [view addSubview:g_kofi positioned:NSWindowAbove relativeTo:nil];
      NSRect c = g_cloud.frame;  // ☁️ left of ☕
      c.origin.x = std::max(k.origin.x - c.size.width - 8, CGFloat(4));
      c.origin.y = top(g_cloud);
      g_cloud.frame = c;
      [g_cloud removeFromSuperview];
      [view addSubview:g_cloud positioned:NSWindowAbove relativeTo:nil];
      // The title, left of the buttons shown, centred on their height.
      NSRect t = g_title.frame;
      const CGFloat edge = ShowSyncButton() ? c.origin.x : g_button.frame.origin.x;  // (hidden)
      t.origin.x = std::max(edge - t.size.width - 10, CGFloat(4));
      t.origin.y = g_button.frame.origin.y + (g_button.frame.size.height - t.size.height) / 2;
      g_title.frame = t;
      [g_title removeFromSuperview];
      [view addSubview:g_title positioned:NSWindowAbove relativeTo:nil];
      if (g_overlay) {  // a panel stays over them
        [g_overlay removeFromSuperview];
        [view addSubview:g_overlay positioned:NSWindowAbove relativeTo:nil];
      }
    }
    g_cloud.hidden = !shown || !ShowSyncButton();
    g_button.hidden = !shown;
    g_kofi.hidden = !shown;
    g_title.hidden = YES;  // "ReHoMM4 <version>" is in the settings now
    static bool tested = false;  // H4_TEST_SETTINGS=1: the settings open at the first main menu (=gamepad: its panel)
    if (shown && !tested && std::getenv("H4_TEST_SETTINGS")) {
      tested = true;
      const bool gamepad = !std::strcmp(std::getenv("H4_TEST_SETTINGS"), "gamepad");
      dispatch_async(dispatch_get_main_queue(), ^{ gamepad ? ShowGamepad() : ShowSettings(); });
    }
  }
}

}  // namespace h4
