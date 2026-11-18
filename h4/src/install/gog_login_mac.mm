// GogLoginCode/CanSignInToGog (install/platform.h) in the Mac app: GOG's
// login page in a WKWebView laid over the game's window (as on iOS: it
// works fullscreen too), with a Cancel button; the code is taken from the
// address GOG sends it to once signed in.
#import <AppKit/AppKit.h>
#import <WebKit/WebKit.h>
#include <SDL3/SDL.h>

#include <string>

#include "install/gog.h"
#include "install/platform.h"

// Google (a way to sign in to GOG) refuses web views whose user agent says so; this is Safari's.
static NSString* const kSafariAgent =
    @"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 "
    @"(KHTML, like Gecko) Version/18.0 Safari/605.1.15";

@interface H4MacGogLogin : NSObject <WKNavigationDelegate, WKUIDelegate>
@property(nonatomic) NSView* pane;
@property(nonatomic) NSView* pages;  // the web views, popups over the page
@property(nonatomic) NSString* code;
@property(nonatomic) BOOL finished;
@property(nonatomic) NSMutableArray<WKWebView*>* popups;  // GOG opens Google/Steam/... sign-in in one
@end

@implementation H4MacGogLogin
- (void)showIn:(NSView*)host url:(NSString*)url {
  self.popups = [NSMutableArray new];
  NSView* pane = [[NSView alloc] initWithFrame:host.bounds];
  pane.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
  pane.wantsLayer = YES;
  pane.layer.backgroundColor = NSColor.windowBackgroundColor.CGColor;
  const CGFloat bar = 44;
  NSView* top = [[NSView alloc] initWithFrame:NSMakeRect(0, pane.bounds.size.height - bar, pane.bounds.size.width, bar)];
  top.autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;
  NSButton* cancel = [NSButton buttonWithTitle:@"Cancel" target:self action:@selector(cancel)];
  cancel.frame = NSMakeRect(10, 8, 90, 28);
  cancel.keyEquivalent = @"\e";
  [top addSubview:cancel];
  NSTextField* title = [NSTextField labelWithString:@"Sign in to GOG.com"];
  title.font = [NSFont boldSystemFontOfSize:15];
  [title sizeToFit];
  title.frame = NSMakeRect((top.bounds.size.width - title.frame.size.width) / 2, 12,
                           title.frame.size.width, title.frame.size.height);
  title.autoresizingMask = NSViewMinXMargin | NSViewMaxXMargin;
  [top addSubview:title];
  [pane addSubview:top];
  self.pages = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, pane.bounds.size.width, pane.bounds.size.height - bar)];
  self.pages.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
  [pane addSubview:self.pages];
  [host addSubview:pane positioned:NSWindowAbove relativeTo:nil];
  self.pane = pane;
  WKWebViewConfiguration* c = [WKWebViewConfiguration new];
  c.websiteDataStore = WKWebsiteDataStore.nonPersistentDataStore;  // nothing kept after
  c.preferences.javaScriptCanOpenWindowsAutomatically = YES;
  WKWebView* web = [self addWebView:c];
  [web loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:url]]];
  [host.window makeFirstResponder:web];
}
- (WKWebView*)addWebView:(WKWebViewConfiguration*)c {
  WKWebView* web = [[WKWebView alloc] initWithFrame:self.pages.bounds configuration:c];
  web.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
  web.customUserAgent = kSafariAgent;
  web.navigationDelegate = self;
  web.UIDelegate = self;
  [self.pages addSubview:web];
  return web;
}
// window.open: the popup over the page (it keeps window.opener, so it can report back).
- (WKWebView*)webView:(WKWebView*)web
    createWebViewWithConfiguration:(WKWebViewConfiguration*)c
               forNavigationAction:(WKNavigationAction*)action
                    windowFeatures:(WKWindowFeatures*)features {
  WKWebView* pop = [self addWebView:c];
  [self.popups addObject:pop];
  return pop;
}
- (void)webViewDidClose:(WKWebView*)web {
  if (![self.popups containsObject:web]) return;
  [web removeFromSuperview];
  [self.popups removeObject:web];
}
- (void)cancel {
  if (self.popups.count) {  // Cancel closes a popup first
    [self webViewDidClose:self.popups.lastObject];
    return;
  }
  self.finished = YES;
}
- (void)webView:(WKWebView*)web
    decidePolicyForNavigationAction:(WKNavigationAction*)action
                    decisionHandler:(void (^)(WKNavigationActionPolicy))decide {
  NSURL* u = action.request.URL;
  if (h4::install::gog::IsLoginSuccessUrl(u.absoluteString.UTF8String ?: "")) {
    NSURLComponents* comps = [NSURLComponents componentsWithURL:u resolvingAgainstBaseURL:NO];
    for (NSURLQueryItem* q in comps.queryItems)
      if ([q.name isEqualToString:@"code"]) self.code = q.value;
    self.finished = YES;
    decide(WKNavigationActionPolicyCancel);
    return;
  }
  decide(WKNavigationActionPolicyAllow);
}
@end

namespace h4::install {

bool CanSignInToGog() { return true; }

std::string GogLoginCode(const std::string& url) {
  @autoreleasepool {
    int n = 0;
    SDL_Window** wins = SDL_GetWindows(&n);
    NSWindow* w = nil;
    if (wins && n > 0)
      w = (__bridge NSWindow*)SDL_GetPointerProperty(SDL_GetWindowProperties(wins[0]),
                                                     SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
    SDL_free(wins);
    if (!w.contentView) return "";
    H4MacGogLogin* login = [H4MacGogLogin new];
    [login showIn:w.contentView url:@(url.c_str())];
    // SDL's text input would take the keys meant for the page.
    SDL_Window* sw = SDL_GetKeyboardFocus();
    const bool text = sw && SDL_TextInputActive(sw);
    if (text) SDL_StopTextInput(sw);
    while (!login.finished) {
      SDL_PumpEvents();
      [NSRunLoop.mainRunLoop runMode:NSDefaultRunLoopMode beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.02]];
    }
    [login.pane removeFromSuperview];
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);  // the clicks and keys on the page
    if (text) SDL_StartTextInput(sw);
    return login.code ? std::string(login.code.UTF8String) : std::string();
  }
}

}  // namespace h4::install
