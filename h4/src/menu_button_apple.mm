// app_ui/native.h on iOS: the browser (UIApplication openURL). The settings themselves are drawn
// into the game's frame (menu_button_drawn.cpp); the save sync's parts are save_sync_apple.mm's.
#import <UIKit/UIKit.h>

#include <string>

#include "app_ui/native.h"

namespace h4::native {

bool OpenUrl(const std::string& url) {
  NSURL* u = [NSURL URLWithString:@(url.c_str())];
  if (!u) return false;
  dispatch_async(dispatch_get_main_queue(), ^{
    [UIApplication.sharedApplication openURL:u options:@{} completionHandler:nil];
  });
  return true;
}

// (the Files app shows the app's own folder by itself: nothing to open)
bool RevealFolder(const std::string&) { return false; }

}  // namespace h4::native
