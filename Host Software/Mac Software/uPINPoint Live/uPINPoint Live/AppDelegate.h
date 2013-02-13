//
//  AppDelegate.h
//  uPINPoint Live
//
//  Created by Nicholas Ver Voort on 1/10/13.
//  Copyright (c) 2013 Engaging Computing Group. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <IOKit/hid/IOHIDManager.h>

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (unsafe_unretained) IBOutlet NSTextField *cStatus;

//Time and date fields
@property (unsafe_unretained) IBOutlet NSTextField *monthField;
@property (unsafe_unretained) IBOutlet NSTextField *dayField;
@property (unsafe_unretained) IBOutlet NSTextField *yearField;
@property (unsafe_unretained) IBOutlet NSTextField *hourField;
@property (unsafe_unretained) IBOutlet NSTextField *minuteField;
@property (unsafe_unretained) IBOutlet NSTextField *secondField;
@property (unsafe_unretained) IBOutlet NSTextField *battField;
@property (unsafe_unretained) IBOutlet NSTextField *tempField;

@property (assign) IBOutlet NSWindow *window;

- (IBAction)setTime:(id)sender;

- (void)showData;

//HID Device Callbacks taken from tutorial at http://ontrak.net/xcode.htm
// USB device added callback function
static void Handle_DeviceMatchingCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef);
// USB device removed callback function
static void Handle_DeviceRemovalCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef);
// Counts the number of devices in the device set (includes all USB devices that match our dictionary)
static long USBDeviceCount(IOHIDManagerRef HIDManager);

@end
