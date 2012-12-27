
#ifndef __SENSOR_H__
#define __SENSOR_H__

#include <rtcc.h>

/*
typedef struct
{
//    rtccTimeDate time;
    UINT8 DataPointYear;
    UINT8 DataPointMonth;
    UINT8 DataPointDay;

    UINT8 DataPointHours;
    UINT8 DataPointMinutes;
    UINT8 DataPointSeconds;
    UINT16 DataPointHundredths;

    INT32 DataPointBatteryVoltage;
    INT32 DataPointTemperature;
    INT32 DataPointPressure;
    INT32 DataPointAltitude;
    INT32 DataPointAccelX;
    INT32 DataPointAccelY;
    INT32 DataPointAccelZ;
    UINT32 DataPointAccelMag;
    INT32 DataPointLight;
} DataPoint;
*/
/*
 * Globals
 */
//extern DataPoint CurrentDataPoint;

extern INT16 BMP085CalAC1;
extern INT16 BMP085CalAC2;
extern INT16 BMP085CalAC3;
extern UINT16 BMP085CalAC4;
extern UINT16 BMP085CalAC5;
extern UINT16 BMP085CalAC6;
extern INT16 BMP085CalB1;
extern INT16 BMP085CalB2;
extern INT16 BMP085CalMB;
extern INT16 BMP085CalMC;
extern INT16 BMP085CalMD;

extern UINT8 DataPointYear;
extern UINT8 DataPointMonth;
extern UINT8 DataPointDay;

extern UINT8 DataPointHours;
extern UINT8 DataPointMinutes;
extern UINT8 DataPointSeconds;

extern UINT32 DataPointElapsedSeconds;
extern UINT32 DataPointHundredths;

extern INT32 DataPointBatteryVoltage;
extern INT32 DataPointTemperature;
extern INT32 DataPointPressure;
extern INT32 DataPointAltitude;
extern INT32 DataPointAccelX;
extern INT32 DataPointAccelY;
extern INT32 DataPointAccelZ;
extern UINT32 DataPointAccelMag;
extern INT32 DataPointLight;

/*
 * Prototypes
 */
void BatteryReadVoltage(void);

void BMP085Init(void);
void BMP085StartTemperature(void);
void BMP085ReadTemperature(void);
void BMP085StartPressure(void);
void BMP085ReadPressure(void);

void ADXL345Init(void);
void ADXL345GotoSleep(void);
void ADXL345GotoMeasurementMode(void);
void ADXL345ReadData(void);

void MAX44007Init(void);
void MAX44007ReadData(void);

#endif // __SENSOR_H__

/*
 * End of Sensor.h
 */
