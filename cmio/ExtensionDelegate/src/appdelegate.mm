/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
 *
 * akvirtualcamera is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * akvirtualcamera is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with akvirtualcamera. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#import "appdelegate.h"
#include "VCamUtils/src/logger.h"

static NSString * const kExtensionBundleID     = @EXTENSION_BUNDLE_ID;
static NSString * const kExtensionInstalledKey = @"extensionInstalled";

// Represents the known installation state of the system extension.
typedef NS_ENUM(NSInteger, ExtensionStatus) {
    ExtensionStatusUnknown,       // Indeterminate state (operation in progress or pending reboot)
    ExtensionStatusInstalled,     // Extension is active and running
    ExtensionStatusNotInstalled   // Extension is not installed or has been deactivated
};

@interface AppDelegate ()
@property (nonatomic, strong) NSButton *installButton;
@property (nonatomic, strong) NSButton *uninstallButton;
@property (nonatomic, strong) NSTextField *statusLabel;
@property (nonatomic, assign) ExtensionStatus extensionStatus;
@property (nonatomic, assign) BOOL pendingInstall; // YES = install pending, NO = uninstall pending
@end

@implementation AppDelegate

/* applicationDidFinishLaunching:
 *
 * Builds the main window and its controls, then queries the last known
 * extension state from persistent storage to configure the UI accordingly.
 */
- (void) applicationDidFinishLaunching: (NSNotification *) notification
{
    AkLogFunction();

    self.window = [[NSWindow alloc]
                   initWithContentRect: NSMakeRect(0, 0, 340, 170)
                   styleMask: NSWindowStyleMaskTitled
                            | NSWindowStyleMaskClosable
                            | NSWindowStyleMaskMiniaturizable
                   backing: NSBackingStoreBuffered
                   defer: NO];
    self.window.title = @COMMONS_APPNAME;
    [self.window center];

    self.statusLabel = [NSTextField labelWithString:@"Checking status..."];
    self.statusLabel.frame = NSMakeRect(20, 128, 300, 20);
    self.statusLabel.alignment = NSTextAlignmentCenter;
    self.statusLabel.font = [NSFont systemFontOfSize:12];

    self.installButton = [NSButton
                          buttonWithTitle: @"Install extension"
                          target: self
                          action: @selector(installExtension:)];
    self.installButton.frame = NSMakeRect(20, 88, 300, 32);

    self.uninstallButton = [NSButton
                            buttonWithTitle: @"Uninstall extension"
                            target: self
                            action: @selector(uninstallExtension:)];
    self.uninstallButton.frame = NSMakeRect(20, 48, 300, 32);

    [self.window.contentView addSubview: self.statusLabel];
    [self.window.contentView addSubview: self.installButton];
    [self.window.contentView addSubview: self.uninstallButton];
    [self.window makeKeyAndOrderFront: nil];

    [self refreshStatus];
}

/* refreshStatus
 *
 * Reads the last known extension state from NSUserDefaults and updates the
 * UI accordingly. If no value has ever been stored (fresh install), the
 * extension is assumed to be not installed, which is the safer default.
 */
- (void) refreshStatus
{
    AkLogFunction();

    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

    /* boolForKey returns NO when the key is absent, which correctly maps to
     * "not installed" on a clean system with no prior state recorded.
     */
    BOOL installed = [defaults boolForKey:kExtensionInstalledKey];

    if (installed) {
        [self updateUIForStatus: ExtensionStatusInstalled
              message: @"Extension installed"];
    } else {
        [self updateUIForStatus: ExtensionStatusNotInstalled
              message: @"Extension not installed"];
    }
}

/* updateUIForStatus:message:
 *
 * Enables or disables the install/uninstall buttons based on the current
 * extension status. ExtensionStatusUnknown (operation in progress or pending
 * reboot) leaves both buttons enabled so the user can retry if needed.
 */
- (void) updateUIForStatus: (ExtensionStatus) status
                   message: (NSString *) message
{
    AkLogFunction();

    self.statusLabel.stringValue = message;

    switch (status) {
        case ExtensionStatusInstalled:
            self.installButton.enabled   = NO;
            self.uninstallButton.enabled = YES;
            break;
        case ExtensionStatusNotInstalled:
            self.installButton.enabled   = YES;
            self.uninstallButton.enabled = NO;
            break;
        case ExtensionStatusUnknown:
            // Both buttons remain active to allow the user to retry the operation.
            self.installButton.enabled   = YES;
            self.uninstallButton.enabled = YES;
            break;
    }
}

/* installExtension:
 *
 * Submits an activation request for the system extension. The UI is set to
 * the unknown/in-progress state while the operation is pending.
 */
- (IBAction) installExtension: (id) sender
{
    AkLogFunction();

    self.pendingInstall = YES;
    self.installButton.enabled   = NO;
    self.uninstallButton.enabled = NO;
    [self updateUIForStatus: ExtensionStatusUnknown
          message: @"Installing..."];

    OSSystemExtensionRequest *request =
        [OSSystemExtensionRequest
         activationRequestForExtension: kExtensionBundleID
         queue: dispatch_get_main_queue()];
    request.delegate = self;
    [[OSSystemExtensionManager sharedManager] submitRequest: request];
}

/* uninstallExtension:
 *
 * Submits a deactivation request for the system extension. The UI is set to
 * the unknown/in-progress state while the operation is pending.
 */
- (IBAction) uninstallExtension: (id) sender
{
    AkLogFunction();

    self.pendingInstall = NO;
    self.installButton.enabled   = NO;
    self.uninstallButton.enabled = NO;
    [self updateUIForStatus: ExtensionStatusUnknown message: @"Uninstalling..."];

    OSSystemExtensionRequest *request =
        [OSSystemExtensionRequest
         deactivationRequestForExtension: kExtensionBundleID
         queue: dispatch_get_main_queue()];
    request.delegate = self;
    [[OSSystemExtensionManager sharedManager] submitRequest: request];
}

/* request:actionForReplacingExtension:withExtension:
 *
 * Called when a newer version of the extension is being installed over an
 * existing one. Always instructs the system to replace the old version.
 */
- (OSSystemExtensionReplacementAction) request: (OSSystemExtensionRequest *) request
                   actionForReplacingExtension: (OSSystemExtensionProperties *) existing
                                 withExtension: (OSSystemExtensionProperties *) ext
{
    AkLogFunction();

    const char *existingVersion = existing.bundleVersion? [existing.bundleVersion UTF8String]: "unknown";
    const char *newVersion = ext.bundleVersion? [ext.bundleVersion UTF8String]: "unknown";

    AkPrintOut("Updating extension %s -> %s\n", existingVersion, newVersion);

    return OSSystemExtensionReplacementActionReplace;
}


/* request:didFinishWithResult:
 *
 * Called when the activation or deactivation request completes. The new state
 * is persisted to NSUserDefaults before updating the UI, regardless of whether
 * the change takes effect immediately or requires a session restart. On error,
 * NSUserDefaults is not modified; the UI is restored from the last valid state.
 */
- (void)        request: (OSSystemExtensionRequest *) request
    didFinishWithResult: (OSSystemExtensionRequestResult) result
{
    AkLogFunction();

    /* Persist the new state before updating the UI. The change will happen
     * regardless of whether it requires a reboot, so storing it now prevents
     * the buttons from showing a stale state after the system restarts.
     */
    [[NSUserDefaults standardUserDefaults]
      setBool: self.pendingInstall
      forKey: kExtensionInstalledKey];

    if (result == OSSystemExtensionRequestCompleted) {
        [self refreshStatus];
    } else {
        // OS system extension request will complete after reboot
        [self updateUIForStatus: ExtensionStatusUnknown
              message: @"Pending session restart"];

        dispatch_async(dispatch_get_main_queue(), ^{
            NSAlert *alert = [[NSAlert alloc] init];
            alert.alertStyle      = NSAlertStyleInformational;
            alert.messageText     = @"Session restart required";
            alert.informativeText =
                @"The change will take effect after you log out and "
                @"log back in to macOS.";
            [alert addButtonWithTitle: @"OK"];
            [alert runModal];
        });
    }
}

/* request:didFailWithError:
 *
 * Called when the request fails. NSUserDefaults is left untouched so the UI
 * can be restored to the last valid known state, allowing the user to retry.
 */
- (void)     request: (OSSystemExtensionRequest *) request
    didFailWithError: (NSError *) error
{
    AkLogFunction();

    auto errorStr = [[error description] UTF8String];
    AkPrintErr("Error: %s", errorStr);

    [self refreshStatus];

    dispatch_async(dispatch_get_main_queue(), ^{
        NSAlert *alert = [NSAlert alertWithError: error];
        [alert runModal];
    });
}

/* requestNeedsUserApproval:
 *
 * Called when the system requires explicit user approval before the extension
 * can be loaded. The user must grant permission in System Preferences.
 */
- (void) requestNeedsUserApproval: (OSSystemExtensionRequest *) request
{
    AkLogFunction();
    AkPrintOut("System requires user approval to load the extension.");
    [self updateUIForStatus: ExtensionStatusUnknown
          message: @"Approval required in System Preferences"];
}

@end
