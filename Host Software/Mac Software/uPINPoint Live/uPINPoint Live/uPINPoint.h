//
//  uPINPoint.h
//  uPINPoint Live
//
//  Created by Nicholas Ver Voort on 1/28/13.
//  Copyright (c) 2013 Engaging Computing Group. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <IOKit/Hid/IOHIDManager.h>

//define uPPT Command Bytes
const static int CMD_READ_ALL = 0x80;
const static int CMD_SET_DATE_TIME = 0x81;
const static int CMD_SET_CALIBRATION = 0x82;
const static int CMD_WRITE_CALIBRATION = 0x83;
const static int MD_READ_CALIBRATION = 0x84;
const static int CMD_READ_SYSTEM_INFO = 0x70;
const static int CMD_READ_BATTERY_VOLTAGE = 0x71;
const static int CMD_READ_BUTTON = 0x72;
const static int CMD_TEST_LEDS = 0x73;
const static int CMD_CHECK_RTCC = 0x74;
const static int CMD_SCAN_BMP085 = 0x75;
const static int CMD_SCAN_MAX44007 = 0x76;
const static int CMD_SCAN_ADXL345 = 0x77;
const static int CMD_CHECK_SHORTS = 0x78;
const static int CMD_FULL_DIAGNOSTICS = 0x79;

@interface uPINPoint : NSObject

- (void) init:(IOHIDDeviceRef)ppt;
- (void) deinit;
- (void) readData:(uint8_t *)inReport;

@property (nonatomic, assign) int month;
@property (nonatomic, assign) int day;
@property (nonatomic, assign) int year;
@property (nonatomic, assign) int hour;
@property (nonatomic, assign) int minute;
@property (nonatomic, assign) int second;

@end
