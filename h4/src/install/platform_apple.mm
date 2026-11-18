// install/platform.h on Apple platforms: HTTPS (NSURLSession) everywhere;
// on iOS also GOG's login page (WKWebView), the file picker
// (UIDocumentPickerViewController) and the Keychain for the token. The Mac
// build of the installer's test harness (platforms/tools/install_test) uses the
// HTTPS part only.

#import <Foundation/Foundation.h>
#import <Security/Security.h>
#include <TargetConditionals.h>

#include <SDL3/SDL.h>

#include <string>
#include <vector>

#include "install/gog.h"
#include "install/platform.h"

#if TARGET_OS_IOS
#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <WebKit/WebKit.h>
#endif

namespace h4::install {

void PlatformInit() {}

std::string ScratchDir() {
  @autoreleasepool {
    return std::string(NSTemporaryDirectory().UTF8String) + "h4-scratch";
  }
}

bool HttpGet(const std::string& url, const std::string& bearer, int* status, std::string* body,
             std::string* error, const std::string& range) {
  @autoreleasepool {
    static NSURLSession* session = [] {
      NSURLSessionConfiguration* c = NSURLSessionConfiguration.ephemeralSessionConfiguration;
      c.timeoutIntervalForRequest = 60;
      c.HTTPMaximumConnectionsPerHost = 8;
      return [NSURLSession sessionWithConfiguration:c];
    }();
    NSURL* u = [NSURL URLWithString:@(url.c_str())];
    if (!u) {
      *error = "bad address";
      return false;
    }
    NSMutableURLRequest* req = [NSMutableURLRequest requestWithURL:u];
    if (!bearer.empty())
      [req setValue:[@"Bearer " stringByAppendingString:@(bearer.c_str())]
          forHTTPHeaderField:@"Authorization"];
    if (!range.empty())
      [req setValue:[@"bytes=" stringByAppendingString:@(range.c_str())] forHTTPHeaderField:@"Range"];
    __block NSData* data = nil;
    __block NSInteger code = 0;
    __block NSError* err = nil;
    dispatch_semaphore_t done = dispatch_semaphore_create(0);
    [[session dataTaskWithRequest:req
                completionHandler:^(NSData* d, NSURLResponse* r, NSError* e) {
                  data = d;
                  err = e;
                  if ([r isKindOfClass:NSHTTPURLResponse.class]) code = ((NSHTTPURLResponse*)r).statusCode;
                  dispatch_semaphore_signal(done);
                }] resume];
    dispatch_semaphore_wait(done, DISPATCH_TIME_FOREVER);
    if (err) {
      // English text (the debug font is ASCII; the system's may be localised).
      *error = err.code == NSURLErrorNotConnectedToInternet ? "no internet connection"
               : err.code == NSURLErrorTimedOut             ? "the connection timed out"
                                                            : "network error " + std::to_string(err.code);
      return false;
    }
    *status = int(code);
    body->assign(static_cast<const char*>(data.bytes), data.length);
    return true;
  }
}

#if TARGET_OS_IOS

namespace {

UIViewController* TopController() {
  UIWindow* w = nil;
  for (UIScene* s in UIApplication.sharedApplication.connectedScenes)
    if ([s isKindOfClass:UIWindowScene.class])
      for (UIWindow* x in ((UIWindowScene*)s).windows)
        if (x.isKeyWindow || !w) w = x;
  UIViewController* vc = w.rootViewController;
  while (vc.presentedViewController) vc = vc.presentedViewController;
  return vc;
}

// Runs the main run loop (and SDL's events) until done() is true.
void WaitUntil(bool (^done)(void)) {
  while (!done()) {
    SDL_PumpEvents();
    [NSRunLoop.mainRunLoop runMode:NSDefaultRunLoopMode beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.02]];
  }
}

}  // namespace

}  // namespace h4::install

// Google (a way to sign in to GOG) refuses web views whose user agent says so; this is Safari's.
static NSString* const kSafariAgent =
    @"Mozilla/5.0 (iPhone; CPU iPhone OS 18_0 like Mac OS X) AppleWebKit/605.1.15 "
    @"(KHTML, like Gecko) Version/18.0 Mobile/15E148 Safari/604.1";

@interface H4GogLogin : UIViewController <WKNavigationDelegate, WKUIDelegate>
@property(nonatomic) NSString* start;
@property(nonatomic) NSString* code;
@property(nonatomic) BOOL finished;
@property(nonatomic) NSMutableArray<WKWebView*>* popups;  // GOG opens Google/Steam/... sign-in in one
@end

@implementation H4GogLogin
- (void)viewDidLoad {
  [super viewDidLoad];
  WKWebViewConfiguration* c = [WKWebViewConfiguration new];
  c.websiteDataStore = WKWebsiteDataStore.nonPersistentDataStore;  // nothing kept after
  c.preferences.javaScriptCanOpenWindowsAutomatically = YES;
  self.popups = [NSMutableArray new];
  WKWebView* web = [self addWebView:c];
  self.title = @"Sign in to GOG.com";
  self.navigationItem.leftBarButtonItem =
      [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel
                                                    target:self
                                                    action:@selector(cancel)];
  [web loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:self.start]]];
}
- (WKWebView*)addWebView:(WKWebViewConfiguration*)c {
  WKWebView* web = [[WKWebView alloc] initWithFrame:self.view.bounds configuration:c];
  web.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  web.customUserAgent = kSafariAgent;
  web.navigationDelegate = self;
  web.UIDelegate = self;
  [self.view addSubview:web];
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

@interface H4Picker : NSObject <UIDocumentPickerDelegate>
@property(nonatomic) NSArray<NSURL*>* urls;
@property(nonatomic) BOOL finished;
@end

@implementation H4Picker
- (void)documentPicker:(UIDocumentPickerViewController*)c didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls {
  self.urls = urls;
  self.finished = YES;
}
- (void)documentPickerWasCancelled:(UIDocumentPickerViewController*)c {
  self.finished = YES;
}
@end

namespace h4::install {

namespace {
NSMutableArray<NSURL*>* g_accessed;
NSString* const kKeychainService = @"games.hazzard.heroes4.gog";
std::string g_token_memory;  // when the Keychain is not available
}  // namespace

std::string GogLoginCode(const std::string& url) {
  @autoreleasepool {
    H4GogLogin* login = [H4GogLogin new];
    login.start = @(url.c_str());
    UINavigationController* nav = [[UINavigationController alloc] initWithRootViewController:login];
    nav.modalPresentationStyle = UIModalPresentationFullScreen;
    [TopController() presentViewController:nav animated:YES completion:nil];
    WaitUntil(^{ return (bool)login.finished; });
    __block bool gone = false;
    [nav dismissViewControllerAnimated:YES completion:^{ gone = true; }];
    WaitUntil(^{ return gone; });
    return login.code ? std::string(login.code.UTF8String) : std::string();
  }
}

bool CanSignInToGog() { return true; }

static H4Picker* g_picker;  // the picker being shown (BeginPickFiles)

bool BeginPickFiles() {
  @autoreleasepool {
    EndPickedFiles();
    H4Picker* del = [H4Picker new];
    UIDocumentPickerViewController* pick = [[UIDocumentPickerViewController alloc]
        initForOpeningContentTypes:@[ UTTypeItem, UTTypeFolder ]
                            asCopy:NO];
    pick.allowsMultipleSelection = YES;
    pick.delegate = del;
    UIViewController* top = TopController();
    if (!top) return false;
    g_picker = del;
    [top presentViewController:pick animated:YES completion:nil];
    return true;
  }
}

bool PickedFiles(std::vector<PickedFile>* result) {
  @autoreleasepool {
    if (g_picker && !g_picker.finished) return false;
    H4Picker* del = g_picker;
    g_picker = nil;
    std::vector<PickedFile>& out = *result;
    out.clear();
    g_accessed = [NSMutableArray new];
    for (NSURL* u in del.urls) {
      if ([u startAccessingSecurityScopedResource]) [g_accessed addObject:u];
      NSNumber* isDir = nil;
      [u getResourceValue:&isDir forKey:NSURLIsDirectoryKey error:nil];
      if (isDir.boolValue) {  // a folder: the files in it
        for (NSURL* f in [NSFileManager.defaultManager contentsOfDirectoryAtURL:u
                                                     includingPropertiesForKeys:nil
                                                                        options:0
                                                                          error:nil])
          out.push_back({f.path.UTF8String, f.lastPathComponent.UTF8String});
      } else {
        out.push_back({u.path.UTF8String, u.lastPathComponent.UTF8String});
      }
    }
    return true;
  }
}

void EndPickedFiles() {
  for (NSURL* u in g_accessed) [u stopAccessingSecurityScopedResource];
  g_accessed = nil;
}

std::string LoadToken() {
  NSDictionary* q = @{
    (__bridge id)kSecClass : (__bridge id)kSecClassGenericPassword,
    (__bridge id)kSecAttrService : kKeychainService,
    (__bridge id)kSecReturnData : @YES,
  };
  CFTypeRef out = nullptr;
  if (SecItemCopyMatching((__bridge CFDictionaryRef)q, &out) != errSecSuccess || !out)
    return g_token_memory;
  NSData* d = (__bridge_transfer NSData*)out;
  return std::string(static_cast<const char*>(d.bytes), d.length);
}

void SaveToken(const std::string& token) {
  g_token_memory = token;
  NSDictionary* q = @{
    (__bridge id)kSecClass : (__bridge id)kSecClassGenericPassword,
    (__bridge id)kSecAttrService : kKeychainService,
  };
  SecItemDelete((__bridge CFDictionaryRef)q);
  if (token.empty()) return;
  NSMutableDictionary* add = [q mutableCopy];
  add[(__bridge id)kSecValueData] = [NSData dataWithBytes:token.data() length:token.size()];
  add[(__bridge id)kSecAttrAccessible] = (__bridge id)kSecAttrAccessibleAfterFirstUnlockThisDeviceOnly;
  OSStatus s = SecItemAdd((__bridge CFDictionaryRef)add, nullptr);
  if (s != errSecSuccess) NSLog(@"h4: no keychain (%d): the GOG sign-in lasts until the app closes", (int)s);
}

#endif  // TARGET_OS_IOS

}  // namespace h4::install
