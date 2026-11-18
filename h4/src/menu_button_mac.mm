// menu_button.h in the Mac app (H4_DESKTOP_APP): NSButtons over SDL's view.
// The language one (top right) opens a panel with a button per language (language.h; not an
// NSMenu: its modal tracking loop would stop the game's frames, and the hang watchdog with
// them); the one picked is saved, and a panel says it applies once the game is restarted. The ☁️ one
// (top left) opens the iCloud saves panel (save_sync_apple.mm). The panels (ShowMacChoice) are views
// over the game, not sheets: a sheet takes the window's focus, and after it the system cursor stays
// on screen next to the game's own.
#import <AppKit/AppKit.h>
#import <objc/runtime.h>
#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "language.h"
#include "menu_button.h"

namespace {
std::string g_lang, g_saved, g_next;  // running, saved file, the one picked
NSButton* g_button = nil;  // language
NSButton* g_cloud = nil;   // saves in iCloud
bool g_shown = false;

NSString* Str(const std::string& s) { return [NSString stringWithUTF8String:s.c_str()]; }
NSString* Name(const std::string& l) { return Str(h4::FindLanguage(l).name); }

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

void Choose(const std::string& lang) {
  if (FILE* f = std::fopen(g_saved.c_str(), "w")) {
    std::fputs(lang.c_str(), f);
    std::fclose(f);
  }
  const h4::LanguageInfo l = h4::FindLanguage(lang);
  h4::ShowMacChoice((__bridge void*)g_button, l.name, l.note, {"OK"}, [](int) {});
}

NSColor* Gold() { return [NSColor colorWithRed:0.84 green:0.67 blue:0.34 alpha:1]; }
NSColor* Cream() { return [NSColor colorWithRed:0.98 green:0.92 blue:0.78 alpha:1]; }
NSView* g_overlay = nil;  // the panel's backdrop while one is up
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
@end
@implementation H4MacChoiceTarget
- (void)pick:(NSButton*)b { [self finish:int(b.tag)]; }
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

@interface H4MacLangTarget : NSObject
@end
@implementation H4MacLangTarget
- (void)cloud:(NSButton*)b {
  h4::ShowSaveSyncSheet((__bridge void*)b, g_lang == "uk");
}
- (void)tap:(NSButton*)b {
  // A panel listing the languages (the game's frames go on while it is up).
  if (!b.window || g_overlay) return;
  const std::vector<h4::LanguageInfo> all = h4::Languages();
  std::vector<std::string> names;
  for (const h4::LanguageInfo& l : all) names.push_back((l.code == g_next ? "✓ " : "") + l.name);
  names.push_back(g_lang == "uk" ? "Скасувати" : "Cancel");
  h4::ShowMacChoice((__bridge void*)b, g_lang == "uk" ? "Мова гри" : "Game language", "", names,
            [all, b](int r) {
              const long i = r;
              if (i < 0 || i >= long(all.size()) || all[size_t(i)].code == g_next) return;
              g_next = all[size_t(i)].code;
              b.attributedTitle = [[NSAttributedString alloc]
                  initWithString:[@"🌐 " stringByAppendingString:Name(g_next)]
                      attributes:[b.attributedTitle attributesAtIndex:0 effectiveRange:nullptr]];
              const NSRect old = b.frame;
              [b sizeToFit];
              NSRect f = b.frame;
              f.size.width += 18;
              f.size.height = old.size.height;
              f.origin.x = old.origin.x + (old.size.width - f.size.width) / 2;
              f.origin.y = old.origin.y;
              b.frame = f;
              const std::string code = g_next;
              dispatch_async(dispatch_get_main_queue(), ^{ Choose(code); });  // after this panel
            });
}
@end

namespace h4 {

void SetLanguageChoice(const std::string& lang, const std::string& saved_path) {
  g_lang = g_next = lang;
  g_saved = saved_path;
}

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

void ShowMacChoice(void* any_view, const std::string& title, const std::string& text,
                   const std::vector<std::string>& buttons, std::function<void(int)> done) {
  NSView* view = ((__bridge NSView*)any_view).window.contentView;
  if (!view || g_overlay) return;
  const CGFloat width = 340, pad = 18, gap = 8, bh = 30;
  H4MacBackdrop* o = [[H4MacBackdrop alloc] initWithFrame:view.bounds];
  o.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
  o.wantsLayer = YES;
  o.layer.backgroundColor = [NSColor colorWithWhite:0 alpha:0.45].CGColor;

  NSView* panel = [NSView new];
  panel.wantsLayer = YES;
  panel.layer.backgroundColor = [NSColor colorWithRed:0.10 green:0.07 blue:0.03 alpha:0.96].CGColor;
  panel.layer.cornerRadius = 12;
  panel.layer.borderWidth = 1.5;
  panel.layer.borderColor = Gold().CGColor;
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
  if (!text.empty()) label(text, [NSFont systemFontOfSize:12], Cream());
  H4MacChoiceTarget* target = [H4MacChoiceTarget new];
  target.done = std::move(done);
  for (size_t i = 0; i < buttons.size(); i++) {
    NSButton* b = MakeButton(Str(buttons[i]), @selector(pick:));
    b.target = target;
    b.tag = NSInteger(i);
    b.frame = NSMakeRect(0, 0, width - 2 * pad, bh);
    rows.push_back(b);
  }
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
  panel.frame = NSMakeRect(std::round((view.bounds.size.width - width) / 2),
                           std::round((view.bounds.size.height - h) / 2), width, h);
  panel.autoresizingMask = NSViewMinXMargin | NSViewMaxXMargin | NSViewMinYMargin | NSViewMaxYMargin;
  [o addSubview:panel];
  [view addSubview:o positioned:NSWindowAbove relativeTo:nil];
  g_overlay = o;
  objc_setAssociatedObject(o, "target", target, OBJC_ASSOCIATION_RETAIN);
  // The keys go to the panel, not the game: Esc closes it (as its last button: Cancel, OK), Return
  // picks the first.
  const int last = int(buttons.size()) - 1;
  g_keys = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown | NSEventMaskKeyUp
                                                 handler:^NSEvent*(NSEvent* e) {
    if (e.type == NSEventTypeKeyDown && e.keyCode == 53) [target finish:last];
    else if (e.type == NSEventTypeKeyDown && (e.keyCode == 36 || e.keyCode == 76) && last >= 0) [target finish:0];
    return nil;
  }];
  [view.window invalidateCursorRectsForView:o];
  [NSCursor.arrowCursor set];
}

// The language button (when the app has the translation), its top centre at (x, y) in the
// frame; the ☁️ one in the menu's top-left corner, its top-left at (left, y).
void ShowLanguageButton(bool shown, int x, int y, int left) {
  static int last_x = -1, last_y = -1, last_left = -1;
  static CGSize last_size;
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
      g_cloud = MakeButton(@"☁️", @selector(cloud:));
      [view addSubview:g_cloud];
      if (!g_saved.empty()) {
        g_button = MakeButton([@"🌐 " stringByAppendingString:Name(g_lang)], @selector(tap:));
        [view addSubview:g_button];
      }
    }
    if (shown) {
      float vx, vy, lx, ly;
      FrameToView(x, y, float(size.width), float(size.height), &vx, &vy);
      FrameToView(left >= 0 ? left : 0, y, float(size.width), float(size.height), &lx, &ly);
      if (vx < 0) {
        g_shown = false;  // nothing shown yet: tried again next pass
        return;
      }
      auto top = [&](NSButton* b) {
        return view.isFlipped ? CGFloat(vy) : size.height - CGFloat(vy) - b.frame.size.height;
      };
      if (g_button) {
        NSRect f = g_button.frame;
        f.origin.x = std::min(std::max(CGFloat(vx) - f.size.width / 2, CGFloat(4)), size.width - f.size.width - 4);
        f.origin.y = top(g_button);
        g_button.frame = f;
        [g_button removeFromSuperview];  // on top of SDL's Metal layer
        [view addSubview:g_button positioned:NSWindowAbove relativeTo:nil];
      }
      NSRect c = g_cloud.frame;
      if (g_button) c.size.height = g_button.frame.size.height;
      c.origin.x = std::max(CGFloat(lx), CGFloat(4));
      c.origin.y = top(g_cloud);
      g_cloud.frame = c;
      [g_cloud removeFromSuperview];
      [view addSubview:g_cloud positioned:NSWindowAbove relativeTo:nil];
      if (g_overlay) {  // a panel stays over them
        [g_overlay removeFromSuperview];
        [view addSubview:g_overlay positioned:NSWindowAbove relativeTo:nil];
      }
    }
    g_cloud.hidden = !shown;
    g_button.hidden = !shown;
  }
}

}  // namespace h4
