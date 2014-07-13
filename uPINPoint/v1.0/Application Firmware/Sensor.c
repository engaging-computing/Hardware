/*
 * Sensor I/O functions.
 */

#include <math.h>

#include "I2C.h"
#include "ADC.h"
#include "Sensor.h"

/*
 * Globals
 */
//DataPoint CurrentDataPoint;

INT16 BMP085CalAC1  = 0;
INT16 BMP085CalAC2  = 0;
INT16 BMP085CalAC3  = 0;
UINT16 BMP085CalAC4 = 0;
UINT16 BMP085CalAC5 = 0;
UINT16 BMP085CalAC6 = 0;
INT16 BMP085CalB1   = 0;
INT16 BMP085CalB2   = 0;
INT16 BMP085CalMB   = 0;
INT16 BMP085CalMC   = 0;
INT16 BMP085CalMD   = 0;

UINT8 DataPointYear;
UINT8 DataPointMonth;
UINT8 DataPointDay;

UINT8 DataPointHours;
UINT8 DataPointMinutes;
UINT8 DataPointSeconds;

UINT32 DataPointElapsedSeconds;
UINT32 DataPointHundredths;

INT32 DataPointBatteryVoltage;
INT32 DataPointTemperature;
INT32 DataPointPressure;
INT32 DataPointAltitude;
INT32 DataPointAccelX;
INT32 DataPointAccelY;
INT32 DataPointAccelZ;
UINT32 DataPointAccelMag;
INT32 DataPointLight;

extern INT32 CalibrationTemperatureOffset;
extern INT32 CalibrationPressureOffset;

extern INT32 CalibrationAccelXOffset;
extern INT32 CalibrationAccelYOffset;
extern INT32 CalibrationAccelZOffset;

extern INT32 CalibrationLightOffset;

/*
 * Read the battery voltage.
 */
void BatteryReadVoltage(void)
{
    DataPointBatteryVoltage = ((((Int32)ADCRead()) * 330) / 1024);
}

/*
 * Read a value from the BMP085's given register address.
 */
UINT16 BMP085ReadRegister(UINT8 addr)
{
    UINT16 data;

    I2C3Start();

    I2C3WriteByte(0xEE, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + write bit
    I2C3WriteByte(addr, FALSE, I2C_STANDARD_TIMEOUT);

    I2C3Restart();

    I2C3WriteByte(0xEF, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + read bit
    data   = I2C3ReadByte(FALSE, I2C_STANDARD_TIMEOUT);
    data <<= 8;
    data  &= 0xFF00;
    data |= I2C3ReadByte(TRUE, I2C_STANDARD_TIMEOUT);

    I2C3Stop();

    return data;
}

void BMP085Init(void)
{
	I2C3Setup();

    BMP085CalAC1  = (INT16) BMP085ReadRegister(0xAA);
    BMP085CalAC2  = (INT16) BMP085ReadRegister(0xAC);
    BMP085CalAC3  = (INT16) BMP085ReadRegister(0xAE);
    BMP085CalAC4  = BMP085ReadRegister(0xB0);
    BMP085CalAC5  = BMP085ReadRegister(0xB2);
    BMP085CalAC6  = BMP085ReadRegister(0xB4);
    BMP085CalB1   = (INT16) BMP085ReadRegister(0xB6);
    BMP085CalB2   = (INT16) BMP085ReadRegister(0xB8);
    BMP085CalMB   = (INT16) BMP085ReadRegister(0xBA);
    BMP085CalMC   = (INT16) BMP085ReadRegister(0xBC);
    BMP085CalMD   = (INT16) BMP085ReadRegister(0xBE);
}

/*
 * Globals used for pressure compensation based on last temperature
 * reading.
 */
static INT32 u, x1, x2, x3, b3, b5, b6;
static UINT32 b4, b7;
static UINT8 oss = 0;

void BMP085StartTemperature(void)
{
    // read temperature
    I2C3Start();

    I2C3WriteByte(0xEE, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + write bit
    I2C3WriteByte(0xF4, FALSE, I2C_STANDARD_TIMEOUT);
    I2C3WriteByte(0x2E, FALSE, I2C_STANDARD_TIMEOUT);

    I2C3Stop();
}

void BMP085ReadTemperature(void)
{
    u = BMP085ReadRegister(0xF6);

    x1 = ((u - (INT32) BMP085CalAC6) * (INT32) BMP085CalAC5) >> 15;

    if ((x1 + BMP085CalMD) == 0)
        x2 = 0;
    else
        x2 = ((INT32) BMP085CalMC << 11) / (x1 + BMP085CalMD);

    b5 = x1 + x2;
    DataPointTemperature  = (b5 + 8) >> 4;
    DataPointTemperature += CalibrationTemperatureOffset;
}

void BMP085StartPressure(void)
{
    I2C3Start();

    I2C3WriteByte(0xEE, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + write bit
    I2C3WriteByte(0xF4, FALSE, I2C_STANDARD_TIMEOUT);
    I2C3WriteByte(0x34 + (oss << 6), FALSE, I2C_STANDARD_TIMEOUT);

    I2C3Stop();
}

void BMP085ReadPressure(void)
{
    u = BMP085ReadRegister(0xF6);
    u <<= 8;
    u += (BMP085ReadRegister(0xF8) & 0xFF);
    u >>= (8 - oss);

    b6 = b5 - 4000;

    x1 = (BMP085CalB2 * ((b6 * b6) >> 12)) >> 11;
    x2 = (BMP085CalAC2 * b6) >> 11;
    x3 = x1 + x2;
    b3 = ((((((INT32) BMP085CalAC1) << 2) + x3) << oss) + 2) >> 2;

    x1 = (BMP085CalAC3 * b6) >> 13;
    x2 = (BMP085CalB1 * ((b6 * b6) >> 12)) >> 16;
    x3 = ((x1 + x2) + 2) >> 2;
    b4 = (BMP085CalAC4 * (UINT32)(x3 + 32768)) >> 15;

    b7 = ((UINT32) u - b3) * (50000 >> oss);

    if (b7 < 0x80000000)
        DataPointPressure = (b7 << 1) / b4;
    else
        DataPointPressure = (b7 / b4) << 1;

    x1 = (DataPointPressure >> 8) * (DataPointPressure >> 8);
    x1 = (x1 * 3038) >> 16;
    x2 = (-7357 * DataPointPressure) >> 16;

    DataPointPressure += (x1 + x2 + 3791) >> 4;
    DataPointPressure += CalibrationPressureOffset;

    DataPointAltitude = (INT32) ((1.0 - pow((DataPointPressure / 101325.0), (1.0/5.255))) * 44330);
}

void ADXL345Init(void)
{
	I2C1Setup();

    // Tap config
    I2C1Start();
    I2C1WriteByte(0xA6, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + write bit
    I2C1WriteByte(0x2A, FALSE, I2C_STANDARD_TIMEOUT);
    I2C1WriteByte(0x00, FALSE, I2C_STANDARD_TIMEOUT);
    I2C1Stop();

    // Rate config
    I2C1Start();
    I2C1WriteByte(0xA6, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + write bit
    I2C1WriteByte(0x2C, FALSE, I2C_STANDARD_TIMEOUT);
    I2C1WriteByte(0x0E, FALSE, I2C_STANDARD_TIMEOUT);
    I2C1Stop();

    // Power control
    I2C1Start();
    I2C1WriteByte(0xA6, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + write bit
    I2C1WriteByte(0x2D, FALSE, I2C_STANDARD_TIMEOUT);
    I2C1WriteByte(0x04, FALSE, I2C_STANDARD_TIMEOUT);
    I2C1Stop();

    // Interrupt control
    I2C1Start();
    I2C1WriteByte(0xA6, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + write bit
    I2C1WriteByte(0x2E, FALSE, I2C_STANDARD_TIMEOUT);
    I2C1WriteByte(0x00, FALSE, I2C_STANDARD_TIMEOUT);
    I2C1Stop();

    // Data format
    I2C1Start();
    I2C1WriteByte(0xA6, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + write bit
    I2C1WriteByte(0x31, FALSE, I2C_STANDARD_TIMEOUT);
    I2C1WriteByte(0x02, FALSE, I2C_STANDARD_TIMEOUT);
    I2C1Stop();

    // FIFO control
    I2C1Start();
    I2C1WriteByte(0xA6, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + write bit
    I2C1WriteByte(0x38, FALSE, I2C_STANDARD_TIMEOUT);
    I2C1WriteByte(0x0F, FALSE, I2C_STANDARD_TIMEOUT);
    I2C1Stop();
/*
    // Y-axis offset
    I2C1Start();
    I2C1WriteByte(0xA6, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + write bit
    I2C1WriteByte(0x1F, FALSE, I2C_STANDARD_TIMEOUT);
    I2C1WriteByte(0x00, FALSE, I2C_STANDARD_TIMEOUT);
    I2C1WriteByte(0x00, FALSE, I2C_STANDARD_TIMEOUT);
    I2C1WriteByte(0x00, FALSE, I2C_STANDARD_TIMEOUT);
    I2C1Stop();
*/
}

void ADXL345GotoSleep(void)
{
    // Power control
    I2C1Start();
    I2C1WriteByte(0xA6, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + write bit
    I2C1WriteByte(0x2D, FALSE, I2C_STANDARD_TIMEOUT);
    I2C1WriteByte(0x04, FALSE, I2C_STANDARD_TIMEOUT);
    I2C1Stop();
}

void ADXL345GotoMeasurementMode(void)
{
    // Power control
    I2C1Start();
    I2C1WriteByte(0xA6, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + write bit
    I2C1WriteByte(0x2D, FALSE, I2C_STANDARD_TIMEOUT);
    I2C1WriteByte(0x08, FALSE, I2C_STANDARD_TIMEOUT);
    I2C1Stop();
}

UINT32 SquareRoot(UINT32 a)
{
    UINT32 op  = a;
    UINT32 res = 0;
    UINT32 one = 0x40000000;

    // "one" starts at the highest power of four <= than the argument.
    while (one > op)
        one >>= 2;

    while (one != 0)
    {
        if (op >= res + one)
        {
            op = op - (res + one);
            res = res +  2 * one;
        }

        res >>= 1;
        one >>= 2;
    }

    return res;
}

void ADXL345ReadData(void)
{
    INT32 data_a;
    double data_b;
    double mag;

    I2C1Start();

    I2C1WriteByte(0xA6, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + write bit
    I2C1WriteByte(0x32, FALSE, I2C_STANDARD_TIMEOUT);

    I2C1Restart();

    I2C1WriteByte(0xA7, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + read bit

    data_a  = I2C1ReadByte(FALSE, I2C_STANDARD_TIMEOUT);
    data_a |= (I2C1ReadByte(FALSE, I2C_STANDARD_TIMEOUT) << 8);
    data_b  = ((double) data_a / 64.0) * 9.81;
    DataPointAccelX  = (INT32) (data_b * 100);

    data_b *= data_b;
    mag     = data_b;

    data_a  = I2C1ReadByte(FALSE, I2C_STANDARD_TIMEOUT);
    data_a |= (I2C1ReadByte(FALSE, I2C_STANDARD_TIMEOUT) << 8);
    data_b  = ((double) data_a / 64.0) * 9.81;
    DataPointAccelY  = (INT32) (data_b * 100);

    data_b *= data_b;
    mag    += data_b;

    data_a  = I2C1ReadByte(FALSE, I2C_STANDARD_TIMEOUT);
    data_a |= (I2C1ReadByte(TRUE, I2C_STANDARD_TIMEOUT) << 8);
    data_b  = ((double) data_a / 64.0) * 9.81;
    DataPointAccelZ  = (INT32) (data_b * 100);

    data_b *= data_b;
    mag    += data_b;

    I2C1Stop();

    DataPointAccelMag  = SquareRoot((UINT32)mag * 10000);
}

void MAX44007Init(void)
{
    UINT8 data[2];

	I2C2Setup();

    data[0] = 0x01;
    data[1] = 0x00;
    I2C2Write(0xB4, data, 2, I2C_STANDARD_TIMEOUT);

    Delayms(80);

    data[0] = 0x02;
    data[1] = 0xC3;
    I2C2Write(0xB4, data, 2, I2C_STANDARD_TIMEOUT);

    I2C2Write(0xB4, data, 1, I2C_STANDARD_TIMEOUT);

    I2C2Start();
    I2C2WriteByte(0xB5, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + read bit
    I2C2ReadByte(FALSE, I2C_STANDARD_TIMEOUT);
    I2C2Stop();
}

void MAX44007ReadData(void)
{
    UINT8 data[2];

    data[0] = 0x03;
    I2C2Write(0xB4, data, 1, I2C_STANDARD_TIMEOUT);

    I2C2Start();
    I2C2WriteByte(0xB5, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + read bit
    data[0] = I2C2ReadByte(FALSE, I2C_STANDARD_TIMEOUT);
    data[1] = I2C2ReadByte(TRUE, I2C_STANDARD_TIMEOUT);
    I2C2Stop();

    DataPointLight  = 0x00000001 << ((data[0] >> 4) & 0x0F); // exponent
    DataPointLight *= (((data[0] << 4) & 0xF0) + (0x0F & data[1])); // mantissa
    DataPointLight <<= 2; // multiply by 4 (instead of 0.4 -- keep it as an int)

    DataPointLight += CalibrationLightOffset;
}

/*
 * End of sensor.c
 */
