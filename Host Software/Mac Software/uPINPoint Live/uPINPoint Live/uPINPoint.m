//
//  uPINPoint.m
//  uPINPoint Live
//
//  Created by Nicholas Ver Voort on 1/28/13.
//  Copyright (c) 2013 Engaging Computing Group. All rights reserved.
//

#import "uPINPoint.h"
#import "AppDelegate.h"

@implementation uPINPoint

AppDelegate* myAppDelegate;
id selfRef;
//Variables for data read from the READ ALL command
@synthesize year, month, day, hour, minute, second, battVolt, temperature;

- (void) init:(IOHIDDeviceRef)ppt {
    selfRef = self;
    myAppDelegate = (AppDelegate *)[[NSApplication sharedApplication] delegate];
    
    CFIndex reportSize = 64;
    uint8_t report = (uint8_t) malloc(reportSize);
    
    IOHIDDeviceRegisterInputReportCallback(ppt, &report, reportSize, Handle_IOHIDDeviceIOHIDReportCallback, NULL);
    
}

- (void) deinit {
    
    IOHIDDeviceRegisterInputReportCallback(NULL, NULL, NULL, NULL, NULL);
    
}

static void Handle_IOHIDDeviceIOHIDReportCallback(void *          inContext,          // context from IOHIDDeviceRegisterInputReportCallback
                                                  IOReturn        inResult,           // completion result for the input report operation
                                                  void *          inSender,           // IOHIDDeviceRef of the device this report is from
                                                  IOHIDReportType inType,             // the report type
                                                  uint32_t        inReportID,         // the report ID
                                                  uint8_t *       inReport,           // pointer to the report data
                                                  CFIndex         inReportLength)     // the actual size of the input report
{
    if(inResult == kIOReturnSuccess) {
        [selfRef readData:inReport];
    }
}
- (void) readData:(uint8_t *)inReport {
    //inReport[0] is the command, or response code
    //inReport[1-63] vary by type
    switch( inReport[0] ) {
        case CMD_READ_ALL:{
            year = [[NSString stringWithFormat:@"%02x",inReport[1]] intValue] + 2000;
            month = [[NSString stringWithFormat:@"%02x",inReport[2]] intValue];
            day = [[NSString stringWithFormat:@"%02x",inReport[3]] intValue];
            hour = [[NSString stringWithFormat:@"%02x",inReport[5]] intValue];
            minute = [[NSString stringWithFormat:@"%02x",inReport[6]] intValue];
            second = [[NSString stringWithFormat:@"%02x",inReport[7]] intValue];
            
            battVolt = ((uint32)inReport[8] << 24) + ((uint32)inReport[9] << 16) + ((uint32)inReport[10] << 8) + (uint32)inReport[11];
            
            //temperature = ((uint32)inReport[12] << 24) + ((uint32)inReport[13] << 16) + ((uint32)inReport[14] << 8) + (uint32)inReport[15];
            
            [myAppDelegate showData];
                        
            break;
        }
        case CMD_READ_BUTTON:{
            NSMutableString *message = [NSMutableString stringWithString:@"Read button: "];
            
            if (inReport[1] == 0) {
                [message appendString:@"Button is DOWN\r\n"];
            } else {
                [message appendString:@"Button is UP\r\n"];
            }
            
            [myAppDelegate writeTextToConsole:message];
            
            break;
        }
    }
}

@end
