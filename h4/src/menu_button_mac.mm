// app_ui/native.h in the Mac app: the browser and Finder (NSWorkspace). The settings themselves are
// drawn into the game's frame (menu_button_drawn.cpp); the save sync's parts are save_sync_apple.mm's.
#import <AppKit/AppKit.h>

#include <string>

#include "app_ui/native.h"

namespace h4::native {

// A web address in the browser; a local path opened as Finder would (a .dmg mounted).
bool OpenUrl(const std::string& url) {
  NSString* s = @(url.c_str());
  NSURL* u = [s hasPrefix:@"/"] ? [NSURL fileURLWithPath:s] : [NSURL URLWithString:s];
  if (!u) return false;
  dispatch_async(dispatch_get_main_queue(), ^{ [NSWorkspace.sharedWorkspace openURL:u]; });
  return true;
}

bool RevealFolder(const std::string& path) {
  NSString* p = @(path.c_str());
  [NSFileManager.defaultManager createDirectoryAtPath:p withIntermediateDirectories:YES attributes:nil error:nil];
  dispatch_async(dispatch_get_main_queue(), ^{ [NSWorkspace.sharedWorkspace openURL:[NSURL fileURLWithPath:p]]; });
  return true;
}

}  // namespace h4::native
