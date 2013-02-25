//
//  AppDelegate.m
//  uPINPoint Live
//
//  Created by Nicholas Ver Voort on 1/10/13.
//  Copyright (c) 2013 Engaging Computing Group. All rights reserved.
//

#import "AppDelegate.h"
#import <IOKit/Hid/IOHIDManager.h>
#import <CoreFoundation/CFSet.h>

@implementation AppDelegate

//Variables for data read from the READ ALL command
@synthesize year, month, day, hour, minute, second, battVolt, temperature, pressure, altitude;

id selfRef;
NSColor *colorRed, *colorGreen, *colorWhite;
IOHIDDeviceRef uPPT;

//Quit the application when there are no open windows
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication {
    return YES;
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    selfRef = self;
    colorRed =  [NSColor colorWithCalibratedRed:0.7f green:0.0f blue:0.0f alpha:1.0f];
    colorGreen = [NSColor colorWithCalibratedRed:0.0f green:0.7f blue:0.0f alpha:1.0f];
    colorWhite = [NSColor colorWithCalibratedRed:1.0f green:1.0f blue:1.0f alpha:1.0f];
        
    [self.resConsole setFont:[NSFont fontWithName:@"Courier" size:12]];
        
    int vendorID = 0x04D8;
    int productID = 0x0054;
    //Create a HID Manager
    IOHIDManagerRef hidManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    
    //Create a dictionary and limit it to the uPPT
    CFMutableDictionaryRef dict = CFDictionaryCreateMutable (kCFAllocatorDefault, 2, &kCFTypeDictionaryKeyCallBacks,
                                                             &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(dict, CFSTR(kIOHIDVendorIDKey), CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &vendorID));
    CFDictionarySetValue(dict, CFSTR(kIOHIDProductIDKey), CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &productID));
    IOHIDManagerSetDeviceMatching(hidManager, dict);
    
    //Register a callback for USB detection
    IOHIDManagerRegisterDeviceMatchingCallback(hidManager, &Handle_DeviceMatchingCallback, NULL);
    //Register a callback for USB detection
    IOHIDManagerRegisterDeviceRemovalCallback(hidManager, &Handle_DeviceRemovalCallback, NULL);
    //Register the HID Manager on our app's run loop
    IOHIDManagerScheduleWithRunLoop(hidManager, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
    
    //Open the HID Manager
    IOReturn IOReturn = IOHIDManagerOpen(hidManager, kIOHIDOptionsTypeNone);
    if(IOReturn) NSLog(@"IOHIDManagerOpen failed."); //Couldn't open the HID Manager!
    
}

//Changes the connection status field in the UI
- (void)changeConnectionStatusView:(Boolean)status {
    if(status) {
        [self.cStatus setStringValue:@"Connected"];
        [self.cStatus setBackgroundColor:(colorGreen)];
        [self.cStatus setTextColor:(colorWhite)];
    } else {
        [self.cStatus setStringValue:@"Disconnected"];
        [self.cStatus setBackgroundColor:(colorRed)];
        [self.cStatus setTextColor:(colorWhite)];
    }
}

//Called when the Set Date/Time button is pressed. Sends the new date/time to the uPPT
- (IBAction)setTime:(id)sender {
    CFIndex reportSize = 64;
    uint8_t *report = malloc(reportSize * sizeof(uint8_t));
    
    //unitFlags to tell the NSDateComponents which components we're interested in
    unsigned unitFlags = NSYearCalendarUnit | NSMonthCalendarUnit |  NSDayCalendarUnit |
                         NSHourCalendarUnit | NSMinuteCalendarUnit | NSSecondCalendarUnit | NSWeekdayCalendarUnit;
    NSDate *now = [NSDate date];
    NSCalendar *calendar = [NSCalendar currentCalendar];
    NSDateComponents *components = [calendar components:unitFlags fromDate:now];
    
    report[0] = CMD_SET_DATE_TIME;
    //Convert to BCD for the device
    report[1] = (uint8) (((([components year] - 2000)/10) << 4) | (([components year] - 2000) % 10));
    report[2] = (uint8) ((([components month] / 10) << 4) | ([components month] % 10));
    report[3] = (uint8) ((([components day] / 10) << 4) | ([components day] % 10));
    report[4] = (uint8) [components weekday];
    
    report[5] = (uint8) ((([components hour] / 10) << 4) | ([components hour] % 10));
    report[6] = (uint8) ((([components minute] / 10) << 4) | ([components minute] % 10));
    report[7] = (uint8) ((([components second] / 10) << 4) | ([components second] % 10));
    
    for(int i = 8; i < 64; i++) { //Initialize unused bytes of report to 0xFF
        report[i] = 0xFF;         //For lower power consumption on USB bus
    }
    
    //Send the built report to the uPPT
    IOHIDDeviceSetReport(uPPT, kIOHIDReportTypeOutput, 0, report, reportSize);
}
//Called when the Read Battery Voltage button is pressed
- (IBAction)sendCmdBattVolt:(id)sender {
    [self sendGenericCommand:CMD_READ_BATTERY_VOLTAGE];
}
//Called when the Read Button button is pressed
- (IBAction)sendCmdReadButton:(id)sender {
    [self sendGenericCommand:CMD_READ_BUTTON];
}
//Called when the Test LEDs button is pressed
- (IBAction)sendCmdTestLEDs:(id)sender {
    [self sendGenericCommand:CMD_TEST_LEDS];
}
//Called when the Check RTCC button is pressed
- (IBAction)sendCmdCheckRTCC:(id)sender {
    [self sendGenericCommand:CMD_CHECK_RTCC];
}

//Sends one of the generic single-byte command to the uPPT
- (void)sendGenericCommand:(uint8_t)cmd {
    CFIndex reportSize = 64;
    uint8_t *report = malloc(reportSize * sizeof(uint8_t));

    report[0] = cmd;              //Sets the first byte to the command to be sent
    for(int i = 1; i < 64; i++) { //Initialize unused bytes of report to 0xFF
        report[i] = 0xFF;         //For lower power consumption on USB bus
    }
    
    //Send the built report to the uPPT
    IOHIDDeviceSetReport(uPPT, kIOHIDReportTypeOutput, 0, report, reportSize);
}

//Writes a message to the console under the Diagnostics tab
- (void)writeTextToConsole:(NSString*)message {
    NSRange mRange;
    mRange = NSMakeRange([[self.resConsole string] length], 0);
    [self.resConsole replaceCharactersInRange:mRange withString:message];
    [self.resConsole scrollRangeToVisible:mRange];
    [self.resConsole display];
}

//Displays data from the PPT's "CMD_READ_ALL" in the appropriate fields in the UI
- (void)showData {
    [self.dayField setStringValue:[NSString stringWithFormat:@"%d", day]];
    [self.monthField setStringValue:[NSString stringWithFormat:@"%d", month]];
    [self.yearField setStringValue:[NSString stringWithFormat:@"%d", year]];
    [self.hourField setStringValue:[NSString stringWithFormat:@"%d", hour]];
    [self.minuteField setStringValue:[NSString stringWithFormat:@"%02d", minute]];
    [self.secondField setStringValue:[NSString stringWithFormat:@"%02d", second]];
    
    [self.battField setStringValue:[NSString stringWithFormat:@"%.2f", ((double)battVolt/100.0)]];
    [self.tempField setStringValue:[NSString stringWithFormat:@"%.1f", ((double)temperature/10.0)]];
}

//Called when a new uPPT is plugged in
static void Handle_DeviceMatchingCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef){
    if(USBDeviceCount(inSender) == 1) {
        CFIndex reportSize = 64;
        uint8_t report = (uint8_t) malloc(reportSize);
        uPPT = inIOHIDDeviceRef;
        [selfRef changeConnectionStatusView:true];
        
        //Register a callback for Input HID Reports
        IOHIDDeviceRegisterInputReportCallback(uPPT, &report, reportSize, Handle_IOHIDDeviceIOHIDReportCallback, NULL);
    }
}

//Called when a uPPT is removed
static void Handle_DeviceRemovalCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef){
    if(USBDeviceCount(inSender) == 0) {
        [selfRef changeConnectionStatusView:false];
        uPPT = NULL;
        //Deregister the callback for Input HID Reports
        IOHIDDeviceRegisterInputReportCallback(NULL, NULL, NULL, Handle_IOHIDDeviceIOHIDReportCallback, NULL);
    }
}

static void Handle_IOHIDDeviceIOHIDReportCallback(void *          inContext,          //context from IOHIDDeviceRegisterInputReportCallback
                                                  IOReturn        inResult,           //completion result for the input report operation
                                                  void *          inSender,           //IOHIDDeviceRef of the device this report is from
                                                  IOHIDReportType inType,             //the report type
                                                  uint32_t        inReportID,         //the report ID
                                                  uint8_t *       inReport,           //pointer to the report data
                                                  CFIndex         inReportLength)     //the actual size of the input report
{
    if(inResult == kIOReturnSuccess) {
        [selfRef readData:inReport];
    }
}


static long USBDeviceCount(IOHIDManagerRef HIDManager) {
    CFSetRef devSet = IOHIDManagerCopyDevices (HIDManager);
    if( devSet ) {
        return CFSetGetCount(devSet);
    }
    return 0;
}

- (void) readData:(uint8_t *)inReport {
    int temp; //temporary storage for values
    
    //inReport[0] is the command, or response code
    //inReport[1-63] vary by type
    switch( inReport[0] ) {
        case CMD_READ_ALL: { //NOTE TO SELF, THIS IS TEMPORARILY BROKEN BECAUSE OF THE OTHER SWITCH CASES. FIGURE IT OUT
            year = [[NSString stringWithFormat:@"%02x",inReport[1]] intValue] + 2000;
            month = [[NSString stringWithFormat:@"%02x",inReport[2]] intValue];
            day = [[NSString stringWithFormat:@"%02x",inReport[3]] intValue];
            hour = [[NSString stringWithFormat:@"%02x",inReport[5]] intValue];
            minute = [[NSString stringWithFormat:@"%02x",inReport[6]] intValue];
            second = [[NSString stringWithFormat:@"%02x",inReport[7]] intValue];
            
            battVolt = ((uint32)inReport[8] << 24) + ((uint32)inReport[9] << 16) + ((uint32)inReport[10] << 8) + (uint32)inReport[11];
            
            temperature = ((uint32)inReport[12] << 24) + ((uint32)inReport[13] << 16) + ((uint32)inReport[14] << 8) + (uint32)inReport[15];
            
            pressure = ((uint32)inReport[16] << 24) + ((uint32)inReport[17] << 16) + ((uint32)inReport[18] << 8) + (uint32)inReport[19];
            
            altitude = ((uint32)inReport[20] << 24) + ((uint32)inReport[21] << 16) + ((uint32)inReport[22] << 8) + (uint32)inReport[23];
            
            [selfRef showData];
            
            break;
        }
        case CMD_READ_BATTERY_VOLTAGE: {
            NSMutableString *message = [NSMutableString stringWithString:@"Read battery voltage: "];
            temp = ((uint32)inReport[1] << 24) + ((uint32)inReport[2] << 16) + ((uint32)inReport[3] << 8) + (uint32)inReport[4];
            [message appendString:[NSString stringWithFormat:@"%.2f V\r\n", ((double)temp/100.0)]];
            [selfRef writeTextToConsole:message];
            break;
        }
        case CMD_TEST_LEDS:{
            [selfRef writeTextToConsole:@"Test LEDs: Did they both come on?\r\n"];
            break;
        }
        case CMD_READ_BUTTON:{
            NSMutableString *message = [NSMutableString stringWithString:@"Read button: "];
            
            if (inReport[1] == 0) {
                [message appendString:@"Button is DOWN\r\n"];
            } else {
                [message appendString:@"Button is UP\r\n"];
            }
            
            [selfRef writeTextToConsole:message];
            
            break;
        }
        case CMD_CHECK_RTCC: {
            NSMutableString *message = [NSMutableString stringWithString:@"Check RTCC: "];
            
            if (inReport[1] == 0) {
                [message appendString:@"PASS, RTCC is running\r\n"];
            } else {
                [message appendString:@"FAIL, RTCC does not seem to be running\r\n"];
            }
            
            [selfRef writeTextToConsole:message];
            
            break;
        }
    }
}
@end
