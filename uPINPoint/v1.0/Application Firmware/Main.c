

#include <libpic30.h>
#include <rtcc.h>
#include <PwrMgnt.h>
#include <ports.h>
#include <timer.h>
#include <math.h>

#include "PlatformPIC.h"

#include "USB/USB.h"
#include "USB/usb_function_msd.h"
#include "USB/usb_function_hid.h"

#include "MDD File System/SD-SPI.h"
#include "MDD File System/FSIO.h"

#include "IOConfig.h"

#include "I2C.h"
#include "Sensor.h"
#include "ADC.h"

/*
 * Processor configuration bits
 * This are set in the bootloader, so changing these here will have no effect.
 */
_CONFIG1( JTAGEN_OFF & GCP_OFF & GWRP_OFF & BKBUG_ON & ICS_PGx2 & FWDTEN_OFF & WINDIS_OFF )
// Two-Speed startup is disabled
// PLL divide by 1 (4MHz input)
// PLL is enabled
// Primary oscillator block
// Clock switching / fail safe clock monitor are disabled
// OSCO/CLKO/RC15 functions as port I/O
// IOLOCK bit can be set and cleared as needed
// Internal USB regulator is disabled
// Primary oscillator is enabled and set to external (EC
_CONFIG2( IESO_OFF & PLLDIV_NODIV & FNOSC_PRIPLL & FCKSM_CSDCMD & OSCIOFNC_OFF & IOL1WAY_OFF & DISUVREG_OFF & POSCMOD_HS )
// Last page and Flash Configuration words are not protected
// Segmented code protection is disabled
_CONFIG3( WPCFG_WPCFGDIS & WPDIS_WPDIS )

/*
 * Firmware version information
 */
#define FIRMWARE_MAJOR_VERSION		1
#define FIRMWARE_MINOR_VERSION		5

/*
 * Globals
 */
#pragma udata USB_VARS
unsigned char ReceivedDataBuffer[64];
unsigned char ToSendDataBuffer[64];
#pragma udata

USB_HANDLE USBOutHandle = 0;
USB_HANDLE USBInHandle = 0;

//The LUN variable definition is critical to the MSD function driver.  This
//  array is a structure of function pointers that are the functions that 
//  will take care of each of the physical media.  For each additional LUN
//  that is added to the system, an entry into this array needs to be added
//  so that the stack can know where to find the physical layer functions.
//  In this example the media initialization function is named 
//  "MediaInitialize", the read capacity function is named "ReadCapacity",
//  etc.  
LUN_FUNCTIONS LUN[MAX_LUN + 1] = 
{
    {
        &MDD_SDSPI_MediaInitialize,
        &MDD_SDSPI_ReadCapacity,
        &MDD_SDSPI_ReadSectorSize,
        &MDD_SDSPI_MediaDetect,
        &MDD_SDSPI_SectorRead,
        &MDD_SDSPI_WriteProtectState,
        &MDD_SDSPI_SectorWrite
    }
};

/* Standard Response to INQUIRY command stored in ROM 	*/
const ROM InquiryResponse inq_resp = {
	0x00,		// peripheral device is connected, direct access block device
	0x80,           // removable
	0x04,	 	// version = 00=> does not conform to any standard, 4=> SPC-2
	0x02,		// response is in format specified by SPC-2
	0x20,		// n-4 = 36-4=32= 0x20
	0x00,		// sccs etc.
	0x00,		// bque=1 and cmdque=0,indicates simple queueing 00 is obsolete,
			// but as in case of other device, we are just using 00
	0x00,		// 00 obsolete, 0x80 for basic task queueing
	{'U','M','L',' ','E','C','G',' '
    },
	// this is the T10 assigned Vendor ID
	{'M','a','s','s',' ','S','t','o','r','a','g','e',' ',' ',' ',' '
    },
	{'0','0','0','1'
    }
};

/*
 * Calibration variables (read from/stored in program flash)
 */
// Last full page of flash on the PIC24FJ128GB110 (which does not contain the flash configuration words)
#define CALIBRATION_PAGE	84

UINT32 SerialNumber = 0;

INT32 CalibrationTemperatureOffset = 0;
INT32 CalibrationPressureOffset = 0;

INT32 CalibrationAccelXOffset = 0;
INT32 CalibrationAccelYOffset = 0;
INT32 CalibrationAccelZOffset = 0;

INT32 CalibrationLightOffset  = 0;

/*
 * Logging state variables
 */
typedef enum
{
    LOGGING_STATE_STANDBY = 0,
    LOGGING_STATE_FAST, // fast is sample interval < 500ms
    LOGGING_STATE_SLOW, // slow is sample interval >= 500ms
} LoggingState;

LoggingState CurrentLoggingState = LOGGING_STATE_STANDBY;

#define SAMPLE_FREQ_64HZ		6400
#define SAMPLE_FREQ_32HZ		3200
#define SAMPLE_FREQ_16HZ		1600
#define SAMPLE_FREQ_8HZ			800
#define SAMPLE_FREQ_4HZ			400
#define SAMPLE_FREQ_2HZ			200
#define SAMPLE_FREQ_1HZ			100
#define SAMPLE_FREQ_1_OVER_10HZ	10

// Sampling frequency in 0.01 Hz (ie. 6400 = 64 Hz sampling frequency)
UINT32 SamplingFrequency = SAMPLE_FREQ_32HZ;

// smallest unit of time in .01 milliseconds
#define ELAPSED_TIME_QUANTUM	15625
UINT32 quantum = (ELAPSED_TIME_QUANTUM * 2); // *****temp

// Timer1 is used for fast logging.  This is the reload value.
UINT16 Timer1Interval = 0x0100;

BOOL FastTimerSync = FALSE;

// NOTE: The slow logging interval is set using the RTCC alarm register

UINT32 RelativeTimeSeconds = 0;
UINT32 RelativeTimeHundredths = 0;

BOOL PoweredUp = FALSE;

// flags set in interrupt handlers for main loop operations (things that take time)
UINT8 TakeOrRecordSample = 0;

//BOOL TakeSample = FALSE;
//BOOL RecordSample = FALSE;

// Used to synchronize to RTC seconds
BOOL StartRecording = FALSE;

typedef enum
{
    BMP085_STATE_IDLE = 0,
    BMP085_STATE_CONVERTING_TEMPERATURE,
    BMP085_STATE_DONE_TEMPERATURE,
    BMP085_STATE_CONVERTING_PRESSURE,
    BMP085_STATE_DONE_PRESSURE,
} BMP085State;

BMP085State BMP085CurrentState = BMP085_STATE_IDLE;

BOOL ADXL345DataReady = FALSE;
BOOL MAX44007DataReady = FALSE;

FSFILE* LogFile = NULL;

typedef enum
{
    UI_STATE_IDLE = 0,
    UI_STATE_BUTTON_HOLD,
    UI_STATE_FLASH_GREEN,
    UI_STATE_FLASH_RED,
    UI_STATE_PAST_EVENT_HOLD,
} UIState;

UIState CurrentUIState = UI_STATE_IDLE;
INT8 UITickCount = 0;
BOOL ButtonContinuousHold = FALSE; // temp - hack to require release of button

/** PRIVATE PROTOTYPES *********************************************/
void Timer1Setup(UINT16 interval);
static void InitializeSystem(void);
void USBDeviceTasks(void);
void ProcessUSBIO(void);
//void YourHighPriorityISRCode(void);
//void YourLowPriorityISRCode(void);
void USBCBSendResume(void);
void UserInit(void);

void CalibrationWrite(void);
void CalibrationRead(void);

/** DECLARATIONS ***************************************************/
void __attribute__ ((interrupt, auto_psv)) _T1Interrupt (void)
{
//    UINT8 temp;

    PR1 = Timer1Interval; // reload for next event

//    TakeSample = TRUE;

    if (CurrentLoggingState == LOGGING_STATE_FAST)
    {
        TakeOrRecordSample = 2;
//        RecordSample = TRUE;
/*
        switch (SamplingFrequency)
        {
            case SAMPLE_FREQ_64HZ: temp = 1; break;
            case SAMPLE_FREQ_32HZ: temp = 2; break;
            case SAMPLE_FREQ_16HZ: temp = 4; break;
            case SAMPLE_FREQ_8HZ : temp = 8; break;
            case SAMPLE_FREQ_4HZ : temp = 16; break;
            case SAMPLE_FREQ_2HZ : temp = 32; break;
        }

        RelativeTimeHundredths += (ELAPSED_TIME_QUANTUM * temp);
*/

        RelativeTimeHundredths += quantum;
//		RelativeTimeHundredths += ELAPSED_TIME_QUANTUM*2;
/*
        switch (SamplingFrequency)
        {
            case SAMPLE_FREQ_64HZ: RelativeTimeHundredths += ((UINT32) ELAPSED_TIME_QUANTUM * 1); break;
            case SAMPLE_FREQ_32HZ: RelativeTimeHundredths += ((UINT32) ELAPSED_TIME_QUANTUM * 2); break;
            case SAMPLE_FREQ_16HZ: RelativeTimeHundredths += ((UINT32) ELAPSED_TIME_QUANTUM * 4); break;
            case SAMPLE_FREQ_8HZ : RelativeTimeHundredths += ((UINT32) ELAPSED_TIME_QUANTUM * 8); break;
            case SAMPLE_FREQ_4HZ : RelativeTimeHundredths += ((UINT32) ELAPSED_TIME_QUANTUM * 16); break;
            case SAMPLE_FREQ_2HZ : RelativeTimeHundredths += ((UINT32) ELAPSED_TIME_QUANTUM * 32); break;
        }
*/
        if (RelativeTimeHundredths >= 1000000)
            RelativeTimeHundredths = 0;
    }
    else
        TakeOrRecordSample = 1;

    IFS0bits.T1IF   = 0;
}

void __attribute__((interrupt, auto_psv)) _RTCCInterrupt(void)
{
//    UINT16 temp;

    mRtcc_Clear_Intr_Status_Bit; // clear interrupt flag

    if (CurrentLoggingState == LOGGING_STATE_SLOW)
    {
        TakeOrRecordSample = 2;

//        TakeSample = TRUE;
//        RecordSample = TRUE;
    }
    else if (CurrentLoggingState == LOGGING_STATE_FAST)
    {
        IEC0bits.T1IE = 0;
        IFS0bits.T1IF = 0;
        T1CON = 0;

        switch (SamplingFrequency)
        {
            case SAMPLE_FREQ_64HZ: Timer1Interval = 0x0200; break;
            case SAMPLE_FREQ_32HZ: Timer1Interval = 0x0400; break;
            case SAMPLE_FREQ_16HZ: Timer1Interval = 0x0800; break;
            case SAMPLE_FREQ_8HZ : Timer1Interval = 0x1000; break;
            case SAMPLE_FREQ_4HZ : Timer1Interval = 0x2000; break;
            case SAMPLE_FREQ_2HZ : Timer1Interval = 0x4000; break;
        }

        PR1 = Timer1Interval;
        T1CONbits.TCS   = 1;
        T1CONbits.TSYNC = 1;
        T1CONbits.TON   = 1;

        RelativeTimeHundredths = 0;

        TakeOrRecordSample = 2;

        if (StartRecording == TRUE)
        {
            RelativeTimeSeconds = 0;
//            RelativeTimeHundredths = 0;
/*
            switch (SamplingFrequency)
            {
                case SAMPLE_FREQ_64HZ: temp = 0x0200; break;
                case SAMPLE_FREQ_32HZ: temp = 0x0400; break;
                case SAMPLE_FREQ_16HZ: temp = 0x0800; break;
                case SAMPLE_FREQ_8HZ : temp = 0x1000; break;
                case SAMPLE_FREQ_4HZ : temp = 0x2000; break;
                case SAMPLE_FREQ_2HZ : temp = 0x4000; break;
            }

            Timer1Setup(temp);
*/

            TakeOrRecordSample = 3;
/*
            switch (SamplingFrequency)
            {
                case SAMPLE_FREQ_64HZ: Timer1Setup(0x0200); break;
                case SAMPLE_FREQ_32HZ: Timer1Setup(0x0400); break;
                case SAMPLE_FREQ_16HZ: Timer1Setup(0x0800); break;
                case SAMPLE_FREQ_8HZ : Timer1Setup(0x1000); break;
                case SAMPLE_FREQ_4HZ : Timer1Setup(0x2000); break;
                case SAMPLE_FREQ_2HZ : Timer1Setup(0x4000); break;
            }
*/
            StartRecording = FALSE;

//            TakeOrRecordSample = 2; // take first sample
/*
            IEC0bits.T1IE = 0;
            IFS0bits.T1IF = 0;
            T1CON = 0;

            switch (SamplingFrequency)
            {
                case SAMPLE_FREQ_64HZ: Timer1Interval = 0x0200; break;
                case SAMPLE_FREQ_32HZ: Timer1Interval = 0x0400; break;
                case SAMPLE_FREQ_16HZ: Timer1Interval = 0x0800; break;
                case SAMPLE_FREQ_8HZ : Timer1Interval = 0x1000; break;
                case SAMPLE_FREQ_4HZ : Timer1Interval = 0x2000; break;
                case SAMPLE_FREQ_2HZ : Timer1Interval = 0x4000; break;
            }

            PR1 = Timer1Interval;
            T1CONbits.TCS   = 1;
            T1CONbits.TSYNC = 1;
            T1CONbits.TON   = 1;
*/
//            IFS0bits.T1IF = 0;
//            IEC0bits.T1IE = 1;
        }
        else
        {
            IFS0bits.T1IF = 0;
            IEC0bits.T1IE = 1;

            if (++RelativeTimeSeconds > 999999) // max relative time we can store...
                RelativeTimeSeconds = 0;
        }
    }
}

/*
void __attribute__((interrupt,no_auto_psv)) _MI2C1Interrupt(void)
{
    MI2C1_Clear_Intr_Status_Bit; // Clear interrupt flag
}
*/

/*
 * Change notification interrupts are used to trigger different events (UI/sensor read complete/etc)
 */
void __attribute__((interrupt, auto_psv)) _CNInterrupt(void)
{
    IFS1bits.CNIF = 0;

    // determine which pin changed
/*
    if (USB_BUS_SENSE)
    {

    }

    if (BUTTON == 0)
    {
        if (CurrentUIState == UI_STATE_IDLE)
        {
            CurrentUIState = UI_STATE_BUTTON_HOLD;
            UITickCount = 0;
        }
    }
*/
    if (BMP085_EOC == 1)
    {
        if (BMP085CurrentState == BMP085_STATE_CONVERTING_TEMPERATURE
          || BMP085CurrentState == BMP085_STATE_CONVERTING_PRESSURE)
            BMP085CurrentState++;
    }

    if (ADXL_INTR == 0)
        ADXL345DataReady = TRUE;

    if (LIGHT_INTR == 0)
        MAX44007DataReady = TRUE;
}

/*
 * Power-up on-chip and on-board devices
 */
void PowerUp(void)
{
    if (PoweredUp)
        return;

    SD_PWR = 1; // power the SD card

    // pull-up SD_DO since some cards are out-drain output
    CNPU1bits.CN11PUE = 1;

    ADXL345GotoMeasurementMode();
    PoweredUp = TRUE;
}

/*
 * Power down on-chip and on-board devices
 */
void PowerDown(void)
{
    if (!PoweredUp)
        return;

    // take port control of SPI pins
    SPICON1 = 0x1800;

    SD_CS   = 0; // de-select sd card
    SPISTAT = 0x0000; // disable SPI module

    // force MOSI pin low (to avoid parasitic power comsumption)
    PORTGbits.RG7 = 0;

    // disable pull-up on SD_DO (to avoid parasitic power comsumption)
    CNPU1bits.CN11PUE = 0;

    SD_PWR = 0; // finally remove power to the SD card

    ADXL345GotoSleep();
    PoweredUp = FALSE;
}

/*
 * Open a file and begin data logging.
 */
BOOL DataLoggerStart(void)
{
    rtccTimeDate time;
    char fileName[20];

    if (LogFile == NULL)
    {
        // initialize the file system if needed
        if (MDD_SDSPI_MediaDetect() == TRUE)
        {
            if (!FSInit())
                return FALSE;
        }
    }
    else // otherwise just close the previous file before opening new one
        FSfclose(LogFile);

    RtccReadTimeDate(&time);

    sprintf(fileName, "%02x-%02x-%02x", time.f.year, time.f.mon, time.f.mday);
    FSmkdir(fileName); // create the date directory if need be
    FSchdir(fileName); // change to the date working directory

    sprintf(fileName, "%02x_%02x_%02x.csv", time.f.hour, time.f.min, time.f.sec);
    LogFile = FSfopen(fileName, "w");
    FSchdir(".."); // return to root after opening (for next time)

    // write column headings
    FSfwrite("Time, ElapsedTime, ", 19, 1, LogFile);
    FSfwrite("Temperature, Pressure, Altitude, ", 33, 1, LogFile);
    FSfwrite("AccelX, AccelY, AccelZ, AccelMag, Light\r\n", 41, 1, LogFile);

    /*
     * Setup interrupts for fast or slow logging based on set interval
     */
    IEC3bits.RTCIE = 0;
    IFS3bits.RTCIF = 0;

    if (SamplingFrequency <= SAMPLE_FREQ_1HZ)
    {
        CurrentLoggingState = LOGGING_STATE_SLOW;

        RtccWrOn(); // enable RTCC register writing

        switch (SamplingFrequency)
        {
            case SAMPLE_FREQ_1HZ: RtccSetAlarmRpt(RTCC_RPT_SEC, FALSE); break;
            case SAMPLE_FREQ_1_OVER_10HZ: RtccSetAlarmRpt(RTCC_RPT_TEN_SEC, FALSE); break;
        }

        RtccSetChimeEnable(TRUE, FALSE); // allow repeating alarm
        mRtccAlrmEnable();
        mRtccWrOff(); // disable RTCC register writing

        IFS3bits.RTCIF = 0;
        IEC3bits.RTCIE = 1; // enable RTCC interupts
    }
    else // if (SamplingFrequency > SAMPLE_FREQ_1HZ)
    {
        CurrentLoggingState = LOGGING_STATE_FAST;

        switch (SamplingFrequency)
        {
            case SAMPLE_FREQ_64HZ: quantum = ELAPSED_TIME_QUANTUM; break;
            case SAMPLE_FREQ_32HZ: quantum = (UINT32)ELAPSED_TIME_QUANTUM * 2; break;
            case SAMPLE_FREQ_16HZ: quantum = (UINT32)ELAPSED_TIME_QUANTUM * 4; break;
            case SAMPLE_FREQ_8HZ : quantum = (UINT32)ELAPSED_TIME_QUANTUM * 8; break;
            case SAMPLE_FREQ_4HZ : quantum = (UINT32)ELAPSED_TIME_QUANTUM * 16; break;
            case SAMPLE_FREQ_2HZ : quantum = (UINT32)ELAPSED_TIME_QUANTUM * 32; break;
        }

        StartRecording = TRUE;

        RtccWrOn(); // enable RTCC register writing
        mRtccOn();
        RtccSetAlarmRpt(RTCC_RPT_SEC, FALSE);
        RtccSetChimeEnable(TRUE, FALSE); // allow repeating alarm
        mRtccAlrmEnable();
        mRtccWrOff(); // disable RTCC register writing

        IFS3bits.RTCIF = 0;
        IEC3bits.RTCIE = 1; // enable RTCC interupts
    }

    return TRUE;
}

void DataLoggerStop(void)
{
    CurrentLoggingState = LOGGING_STATE_STANDBY;

    if (LogFile == NULL)
        return;

    FSfclose(LogFile);
    LogFile = NULL;
}

/*
 * Record the current data in our open file.
 */
char CSVBuffer[160];
char* pCSVBuffer;

void ConvertToASCII(INT32 data, UINT8 length, UINT8 places)
{
    char* p;
    UINT8 neg = 0;

    if (data < 0)
    {
        neg = 1;
        data = -data;
    }

    p = (pCSVBuffer + (length - 1));

    if (places > 0)
    {
        while (places > 0)
        {
            *p-- = (data % 10) + '0';
            data /= 10;
            places--;
        }

        *p-- = '.';
    }

    while (data >= 10 && p > pCSVBuffer)
    {
        *p-- = (data % 10) + '0';
        data /= 10;
    }

    *p-- = (data % 10) + '0';

    if (neg)
        *p-- = '-';

    while (p >= pCSVBuffer)
        *p-- = ' ';

    pCSVBuffer += length;
}

/*
 * Format elapsed time field.
 */
void ElapsedTimeToASCII(INT32 seconds, UINT32 hundredths)
{
    char* p;
    UINT8 i;

    p = (pCSVBuffer + 12);

    for (i = 0; i < 6; i++)
    {
        *p-- = (hundredths % 10) + '0';
        hundredths /= 10;
    }

    *p-- = '.';

    while (seconds >= 10 && p >= pCSVBuffer)
    {
        *p-- = (seconds % 10) + '0';
        seconds /= 10;
    }

    *p-- = (seconds % 10) + '0';

    while (p >= pCSVBuffer)
        *p-- = ' ';

    pCSVBuffer += 13;
}

/*
 * Format elapsed time field.
 */
void ElapsedTimeHundredthsToASCII(UINT32 hundredths)
{
    char* p;
    UINT8 i;

    p = (pCSVBuffer + 5);

    for (i = 0; i < 6; i++)
    {
        *p-- = (hundredths % 10) + '0';
        hundredths /= 10;
    }

    pCSVBuffer += 6;
}

void DataLoggerRecord(void)
{
    if (LogFile == NULL)
        return;

    pCSVBuffer = &CSVBuffer[0];
/*
    if (DataPointHundredths == 0)
    {
        *pCSVBuffer++ = '#';
        *pCSVBuffer++ = '\r';
        *pCSVBuffer++ = '\n';
    }
*/
    // timestamp - time is in BCD format
    *pCSVBuffer++ = (DataPointDay >> 4) + '0';
    *pCSVBuffer++ = (DataPointDay & 0x0F) + '0';
    *pCSVBuffer++ = '/';
    *pCSVBuffer++ = (DataPointMonth >> 4) + '0';
    *pCSVBuffer++ = (DataPointMonth & 0x0F) + '0';
    *pCSVBuffer++ = '/';
    *pCSVBuffer++ = '2';
    *pCSVBuffer++ = '0';
    *pCSVBuffer++ = (DataPointYear >> 4) + '0';
    *pCSVBuffer++ = (DataPointYear & 0x0F) + '0';

    *pCSVBuffer++ = ' ';

    *pCSVBuffer++ = (DataPointHours >> 4) + '0';
    *pCSVBuffer++ = (DataPointHours & 0x0F) + '0';
    *pCSVBuffer++ = ':';
    *pCSVBuffer++ = (DataPointMinutes >> 4) + '0';
    *pCSVBuffer++ = (DataPointMinutes & 0x0F) + '0';
    *pCSVBuffer++ = ':';
    *pCSVBuffer++ = (DataPointSeconds >> 4) + '0';
    *pCSVBuffer++ = (DataPointSeconds & 0x0F) + '0';

    *pCSVBuffer++ = '.';
    ElapsedTimeHundredthsToASCII(DataPointHundredths);
    *pCSVBuffer++ = ',';
    *pCSVBuffer++ = ' ';

    // Relative (Elapsed) Time
    ElapsedTimeToASCII(DataPointElapsedSeconds, DataPointHundredths);
    *pCSVBuffer++ = ',';
    *pCSVBuffer++ = ' ';

    // Temperature
    ConvertToASCII(DataPointTemperature, 5, 1);
    *pCSVBuffer++ = ',';
    *pCSVBuffer++ = ' ';

    // Pressure
    ConvertToASCII(DataPointPressure, 7, 3);
    *pCSVBuffer++ = ',';
    *pCSVBuffer++ = ' ';

    // Altitude
    ConvertToASCII(DataPointAltitude, 5, 0);
    *pCSVBuffer++ = ',';
    *pCSVBuffer++ = ' ';

    // Acceleration
    ConvertToASCII(DataPointAccelX, 6, 2);
    *pCSVBuffer++ = ',';
    *pCSVBuffer++ = ' ';

    ConvertToASCII(DataPointAccelY, 6, 2);
    *pCSVBuffer++ = ',';
    *pCSVBuffer++ = ' ';

    ConvertToASCII(DataPointAccelZ, 6, 2);
    *pCSVBuffer++ = ',';
    *pCSVBuffer++ = ' ';

    ConvertToASCII(DataPointAccelMag, 6, 2);
    *pCSVBuffer++ = ',';
    *pCSVBuffer++ = ' ';

    // Light
    ConvertToASCII(DataPointLight, 8, 1);
    *pCSVBuffer++ = '\r';
    *pCSVBuffer++ = '\n';

    FSfwrite(CSVBuffer, (pCSVBuffer - &CSVBuffer[0]), 1, LogFile);
}

void LoadConfigFile(void)
{
    FSFILE* configFile;
    char c;
    char buf[32];
    UINT8 i;
    UINT32 newSampleFreq;

    // initialize the file system if needed
    if (MDD_SDSPI_MediaDetect() == TRUE)
    {
        if (!FSInit())
            return;
    }

    if (!(configFile = FSfopen("config.txt", "r")))
        return;

    for (i = 0; i < (32 - 1); i++)
    {
        if (FSfread(&c, 1, 1, configFile) <= 0)
            break;

        if (c == '\n' || c == '\r')
            break;

        buf[i] = c;
    }

    buf[i] = '\0';

    FSfclose(configFile);

    newSampleFreq = atol(buf);

    // goto the closest match for sampling frequency
    if (newSampleFreq >= 6400)
        SamplingFrequency = SAMPLE_FREQ_64HZ;
    else if (newSampleFreq >= 3200)
    {
        newSampleFreq -= 3200;

        if (newSampleFreq >= 1600)
            SamplingFrequency = SAMPLE_FREQ_64HZ;
        else
            SamplingFrequency = SAMPLE_FREQ_32HZ;
    }
    else if (newSampleFreq >= 1600)
    {
        newSampleFreq -= 1600;

        if (newSampleFreq >= 800)
            SamplingFrequency = SAMPLE_FREQ_32HZ;
        else
            SamplingFrequency = SAMPLE_FREQ_16HZ;
    }
    else if (newSampleFreq >= 800)
    {
        newSampleFreq -= 800;

        if (newSampleFreq >= 400)
            SamplingFrequency = SAMPLE_FREQ_16HZ;
        else
            SamplingFrequency = SAMPLE_FREQ_8HZ;
    }
    else if (newSampleFreq >= 400)
    {
        newSampleFreq -= 400;

        if (newSampleFreq >= 200)
            SamplingFrequency = SAMPLE_FREQ_8HZ;
        else
            SamplingFrequency = SAMPLE_FREQ_4HZ;
    }
    else if (newSampleFreq >= 200)
    {
        newSampleFreq -= 200;

        if (newSampleFreq >= 100)
            SamplingFrequency = SAMPLE_FREQ_4HZ;
        else
            SamplingFrequency = SAMPLE_FREQ_2HZ;
    }
    else if (newSampleFreq >= 100)
    {
        newSampleFreq -= 100;

        if (newSampleFreq >= 50)
            SamplingFrequency = SAMPLE_FREQ_2HZ;
        else
            SamplingFrequency = SAMPLE_FREQ_1HZ;
    }
    else if (newSampleFreq >= 10)
    {
        newSampleFreq -= 10;

        if (newSampleFreq >= 5)
            SamplingFrequency = SAMPLE_FREQ_1HZ;
        else
            SamplingFrequency = SAMPLE_FREQ_1_OVER_10HZ;
    }
    else
        SamplingFrequency = SAMPLE_FREQ_64HZ;
}

void Timer1Setup(UINT16 interval)
{
    T1CON = 0;

    Timer1Interval = interval;
    PR1 = Timer1Interval;
    T1CONbits.TCS   = 1;
    T1CONbits.TSYNC = 1;
    T1CONbits.TON   = 1; // turn on timer 1

    IFS0bits.T1IF   = 0;
    IEC0bits.T1IE   = 1;
}

void Timer1Disable(void)
{
    IEC0bits.T1IE   = 0;
    IFS0bits.T1IF   = 0;
    T1CON = 0;
}

/*
 * Main program entry point.
 */
int main(void)
{
    rtccTimeDate time;
    rtccTimeDate lastTime;
    UINT8 intrSaveRTC;
    UINT8 intrSaveTimer1;
    UINT8 intrSave;
    UINT8 tempTakeOrRecordSample;
    UINT32 tempDataPointElapsedSeconds;
    UINT32 tempDataPointHundredths;
    BMP085State tempBMP085CurrentState;

    InitializeSystem();

    while (1)
    {
        // handle reads from the BMP085 (slow device, so we use multiple software states)
        intrSave = IEC1bits.CNIE;
        IEC1bits.CNIE = 0;
        tempBMP085CurrentState = BMP085CurrentState;
        IEC1bits.CNIE = intrSave;

        switch (tempBMP085CurrentState)
        {
            case BMP085_STATE_IDLE:
                break;

            case BMP085_STATE_CONVERTING_TEMPERATURE:
                break;

            case BMP085_STATE_DONE_TEMPERATURE:
                BMP085ReadTemperature();
                BMP085StartPressure();
                BMP085CurrentState++;
                break;

            case BMP085_STATE_CONVERTING_PRESSURE:
                break;

            case BMP085_STATE_DONE_PRESSURE:
                BMP085ReadPressure();
                BMP085CurrentState = BMP085_STATE_IDLE;
                break;
        }

        intrSaveTimer1 = IEC0bits.T1IE;
        intrSaveRTC    = IEC3bits.RTCIE;
        IEC0bits.T1IE = 0;

        mRtccSetInt(FALSE);
        IEC3bits.RTCIE = 0;

        RtccReadTimeDate(&time);

        tempDataPointElapsedSeconds = RelativeTimeSeconds;
        tempDataPointHundredths = RelativeTimeHundredths;

        tempTakeOrRecordSample = TakeOrRecordSample;
        TakeOrRecordSample = 0;

        mRtccSetInt(intrSaveRTC);
        IEC3bits.RTCIE = intrSaveRTC;
        
        IEC0bits.T1IE  = intrSaveTimer1;

        // take a sample when needed
        if (tempTakeOrRecordSample >= 1)
        {
            LED_GREEN = 0; // light green LED while sampling
            LED_RED = 0; // light green LED while sampling

            DataPointYear = time.f.year;
            DataPointMonth = time.f.mon;
            DataPointDay = time.f.mday;

            DataPointHours = time.f.hour;
            DataPointMinutes = time.f.min;
            DataPointSeconds = time.f.sec;

            BatteryReadVoltage();

            if (BMP085CurrentState == BMP085_STATE_IDLE)
            {
                BMP085StartTemperature();
                BMP085CurrentState++;
            }

            // when in slow logging mode just wait here for BMP085 to complete
            if (CurrentLoggingState == LOGGING_STATE_SLOW)
            {
                while (BMP085_EOC == 0)
                    mPWRMGNT_GotoSleepMode();

                BMP085ReadTemperature();
                BMP085StartPressure();

                while (BMP085_EOC == 0)
                    mPWRMGNT_GotoSleepMode();

                BMP085ReadPressure();
                BMP085CurrentState = BMP085_STATE_IDLE;
            }

            ADXL345ReadData();
            MAX44007ReadData();

            DataPointElapsedSeconds = tempDataPointElapsedSeconds;
            DataPointHundredths = tempDataPointHundredths;

            // record a sample when needed
            if (tempTakeOrRecordSample >= 2)
            {
                USBMaskInterrupts();

                if (USB_BUS_SENSE == 0)
                    DataLoggerRecord();

                if (tempTakeOrRecordSample >= 3)
                {
/*
                   switch (SamplingFrequency)
                   {
                        case SAMPLE_FREQ_64HZ: Timer1Setup(0x0200); break;
                        case SAMPLE_FREQ_32HZ: Timer1Setup(0x0400); break;
                        case SAMPLE_FREQ_16HZ: Timer1Setup(0x0800); break;
                        case SAMPLE_FREQ_8HZ : Timer1Setup(0x1000); break;
                        case SAMPLE_FREQ_4HZ : Timer1Setup(0x2000); break;
                        case SAMPLE_FREQ_2HZ : Timer1Setup(0x4000); break;
                    }
*/
                    IFS0bits.T1IF = 0;
                    IEC0bits.T1IE = 1;
                }
                else
                {
//                    if (USB_BUS_SENSE == 0)
//                        DataLoggerRecord();
                }

                USBUnmaskInterrupts();
            }

            LED_GREEN = 1;
            LED_RED = 1;
        }

        // handle USB transfers when attached
        if (USB_BUS_SENSE == 1)
        {
            USBMaskInterrupts();

            if (CurrentLoggingState != LOGGING_STATE_STANDBY)
            {
                IEC0bits.T1IE  = 0;
                IFS0bits.T1IF  = 0;

                IEC3bits.RTCIE = 0;
                IFS3bits.RTCIF = 0;

                DataLoggerStop();
            }

            // Set timer1 for our purposes
            if (!T1CONbits.TON || Timer1Interval != 0x0F00)
                Timer1Setup(0x0F00);

            // make sure we are powered up
            if (!PoweredUp)
                PowerUp();

            // handle attachment when needed
            if (USBDeviceState == DETACHED_STATE)
                USBDeviceAttach();

            // when we are attached to a USB port don't process UI or logging events
            ProcessUSBIO();

            USBUnmaskInterrupts();
            continue;
        }

        if (USBDeviceState > DETACHED_STATE) // handle USB detachment when needed
        {
            USBDeviceDetach();
            Timer1Disable();
            LoadConfigFile();
            PowerDown();
        }

        switch (CurrentUIState)
        {
            case UI_STATE_IDLE:
                UITickCount = 0;

                if (BUTTON == 0)
                {
                    CurrentUIState = UI_STATE_BUTTON_HOLD;
                    RtccReadTimeDate(&lastTime);
                }

                break;

            case UI_STATE_BUTTON_HOLD:
                if (BUTTON != 0)
                {
                    LED_GREEN = 1; // off
                    CurrentUIState = UI_STATE_IDLE;
                    LED_RED   = 1; // off
                    break;
                }

                if (CurrentLoggingState == LOGGING_STATE_STANDBY)
                    LED_GREEN = 0; // on
                else
                    LED_RED   = 0; // on

                RtccReadTimeDate(&time);

                if (lastTime.f.sec == time.f.sec)
                    break;

                lastTime.f.sec = time.f.sec;

                if (++UITickCount >= 2) // held for at least 2 seconds?
                {
                    UITickCount = 0;

                    if (CurrentLoggingState != LOGGING_STATE_STANDBY)
                        CurrentUIState = UI_STATE_FLASH_RED;
                    else
                        CurrentUIState = UI_STATE_FLASH_GREEN;

                    break;
                }

                break;

            case UI_STATE_FLASH_GREEN:
                RtccReadTimeDate(&time);

                if (lastTime.f.sec == time.f.sec)
                    break;

                lastTime.f.sec = time.f.sec;
                UITickCount++;

                if (UITickCount >= 5)
                {
                    LED_GREEN = 1; // off
                    CurrentUIState = UI_STATE_PAST_EVENT_HOLD;

                    IEC0bits.T1IE = 0;
                    IFS0bits.T1IF  = 0;

                    IEC3bits.RTCIE = 0;
                    IFS3bits.RTCIF = 0;

                    if (!PoweredUp)
                        PowerUp();

//                    Delayms(100);

                    TakeOrRecordSample = 0;

//                    LED_RED = 0;

                    if (DataLoggerStart() == TRUE)
                        LED_RED = 1;
                }
                else if (UITickCount >= 4)
                    LED_GREEN = 0; // on
                else if (UITickCount >= 3)
                    LED_GREEN = 1; // off
                else if (UITickCount >= 2)
                    LED_GREEN = 0; // on
                else if (UITickCount >= 1)
                    LED_GREEN = 1; // off

                break;

            case UI_STATE_FLASH_RED:
                RtccReadTimeDate(&time);

                if (lastTime.f.sec == time.f.sec)
                    break;

                lastTime.f.sec = time.f.sec;
                UITickCount++;

                if (UITickCount >= 5)
                {
                    LED_RED = 1; // off
                    CurrentUIState = UI_STATE_PAST_EVENT_HOLD;

                    CurrentLoggingState = LOGGING_STATE_STANDBY;

                    IEC0bits.T1IE = 0;
                    IFS0bits.T1IF  = 0;

                    IEC3bits.RTCIE = 0;
                    IFS3bits.RTCIF = 0;

                    Timer1Disable();

                    DataLoggerStop();
                    TakeOrRecordSample = 0;

                    PowerDown();
                }
                else if (UITickCount >= 4)
                    LED_RED = 0; // on
                else if (UITickCount >= 3)
                    LED_RED = 1; // off
                else if (UITickCount >= 2)
                    LED_RED = 0; // on
                else if (UITickCount >= 1)
                    LED_RED = 1; // off

                break;

            case UI_STATE_PAST_EVENT_HOLD:
                if (BUTTON != 0)
                    CurrentUIState = UI_STATE_IDLE;

                break;
        }

        // go to sleep until woken by interrupt when not in fast logging mode
        if (CurrentLoggingState != LOGGING_STATE_FAST && CurrentUIState == UI_STATE_IDLE)
        {
            U1PWRCbits.USBPWR = 0;

            mPWRMGNT_GotoSleepMode();

            // small delay here because when we wake from sleep it puts a huge load
            // on the power supply, which was problably in PFM mode to save power -- we
            // need to give it time to switch to PWM mode.
            Delayms(10);
            U1PWRCbits.USBPWR = 1;

            Delayms(10);
			USBDeviceInit();
        }

//        else
//            mPWRMGNT_GotoIdleMode();
    }
}

/*
 * Called immediately after reset to setup everything.
 */
static void InitializeSystem(void)
{
    rtccTimeDate time;

    // disable primary oscillator when in sleep mode
    OSCCONbits.POSCEN = 0;

//    RCON &= ~(0x0100);

    AD1PCFGL = 0xFFFF; // All digital I/Os initially

//	The USB specifications require that USB peripheral devices must never source
//	current onto the Vbus pin.  Additionally, USB peripherals should not source
//	current on D+ or D- when the host/hub is not actively powering the Vbus line.
//	When designing a self powered (as opposed to bus powered) USB peripheral
//	device, the firmware should make sure not to turn on the USB module and D+
//	or D- pull up resistor unless Vbus is actively powered.  Therefore, the
//	firmware needs some means to detect when Vbus is being powered by the host.
//	A 5V tolerant I/O pin can be connected to Vbus (through a resistor), and
// 	can be used to detect when Vbus is high (host actively powering), or low
//	(host is shut down or otherwise not supplying power).  The USB firmware
// 	can then periodically poll this I/O pin to know when it is okay to turn on
//	the USB module/D+/D- pull up resistor.  When designing a purely bus powered
//	peripheral device, it is not possible to source current on D+ or D- when the
//	host is not actively providing power on Vbus. Therefore, implementing this
//	bus sense feature is optional.  This firmware can be made to use this bus
//	sense feature by making sure "USE_USB_BUS_SENSE_IO" has been defined in the
//	HardwareProfile.h file.    
#if defined(USE_USB_BUS_SENSE_IO)
    tris_usb_bus_sense = INPUT_PIN; // See IOConfig.h
#endif
    
//	If the host PC sends a GetStatus (device) request, the firmware must respond
//	and let the host know if the USB peripheral device is currently bus powered
//	or self powered.  See chapter 9 in the official USB specifications for details
//	regarding this request.  If the peripheral device is capable of being both
//	self and bus powered, it should not return a hard coded value for this request.
//	Instead, firmware should check if it is currently self or bus powered, and
//	respond accordingly.  If the hardware has been configured like demonstrated
//	on the PICDEM FS USB Demo Board, an I/O pin can be polled to determine the
//	currently selected power source.  On the PICDEM FS USB Demo Board, "RA2" 
//	is used for	this purpose.  If using this feature, make sure "USE_SELF_POWER_SENSE_IO"
//	has been defined in HardwareProfile.h, and that an appropriate I/O pin has been mapped
//	to it in HardwareProfile.h.
#if defined(USE_SELF_POWER_SENSE_IO)
    tris_self_power = INPUT_PIN;	// See HardwareProfile.h
#endif

    // Read the serial number and software calibration values from flash program memory
    CalibrationRead();

    // Initialize Peripheral Pin Select (PPS)

    // Initialize the SPI port for SD card access
    RPINR22bits.SDI2R = 27; // MISO
    RPOR9bits.RP19R   = 11; // SCK
    RPOR13bits.RP26R  = 10; // MOSI

//    CNPU1bits.CN11PUE = 1; // enable pull-up on MISO

    SD_PWR         = 0;
//    SD_PWR_TRIS    = OUTPUT_PIN;

    SD_CS          = 0;
    SD_CS_TRIS     = OUTPUT_PIN;

    LED_RED        = 1; // Off
    LED_GREEN      = 1; // Off
/*
    LED_RED_TRIS   = OUTPUT_PIN; // Set LED pin as output
    LED_GREEN_TRIS = OUTPUT_PIN; // Set LED pin as output
    BUTTON_TRIS    = INPUT_PIN;  // Set Button pin as input
*/
    TRISB = 0xCFF7; // 1100_1111_1111_0111

    BMP085_XCLR      = 1;

    TRISD = 0xF7FF; // 1111_0111_1111_1111

/*
    BMP085_XCLR_TRIS = OUTPUT_PIN;
    LIGHT_INTR_TRIS  = INPUT_PIN;
    BMP085_EOC_TRIS  = INPUT_PIN;
    ADXL_INTR_TRIS   = INPUT_PIN;
*/
    LED_RED = 0; // On

    // initialize the three on-board sensors
    BMP085Init();
    MAX44007Init();
    ADXL345Init();

    // setup input change interrupts
    CNEN3bits.CN32IE  = 1; // Enable CN32 pin for interrupt detection (button signal)
    CNEN4bits.CN52IE  = 1; // Enable CN52 pin for interrupt detection (usb_attach signal) 
    CNEN4bits.CN49IE  = 1; // Enable CN49 pin for interrupt detection (BMP085 end-of-conversion signal)
//    CNEN4bits.CN53IE  = 1; // Enable CN53 pin for interrupt detection (ADXL345 interrupt signal)
//    CNEN1bits.CN12IE  = 1; // Enable CN12 pin for interrupt detection (MAX44007 interrupt signal)

    IFS1bits.CNIF     = 0; // Reset CN interrupt 
    IEC1bits.CNIE     = 1; // Enable CN interrupts 

    CNPU3bits.CN32PUE = 1; // enable pull-up on button input

    // Set up the real-time clock module (with initial time)
    time.f.year = 0x01;
    time.f.mon  = 0x01;
    time.f.mday = 0x01;
    time.f.wday = 0x00;

    time.f.hour = 0x00;
    time.f.min  = 0x00;
    time.f.sec  = 0x00;

    RtccInitClock(); // initialize RTCC variables

    RtccWrOn();      // enable RTCC register writing
    mRtccOn();       // actually turn it on
    RtccWriteTimeDate(&time, TRUE);
    mRtccWrOff(); // disable RTCC register writing

    // Initialize the variables holding the handle for the last USB transmission
    USBOutHandle = 0;
    USBInHandle = 0;

    USBDeviceInit();	// usb_device.c.  Initializes USB module SFRs and firmware
    					// variables to known states.

    LED_RED = 1; // Off
}

/*
 * Pack a 32-bit integer into a buffer (used for HID transfers)
 */
void BufferPackINT32(BYTE* buf, INT32 i)
{
    *buf++ = (i >> 24) & 0xFF;
    *buf++ = (i >> 16) & 0xFF;
    *buf++ = (i >> 8) & 0xFF;
    *buf++ = i & 0xFF;
}

/*
 * Unpack a 32-bit integer from a buffer (used for HID transfers)
 */
INT32 BufferUnpackINT32(BYTE* buf)
{
    UINT8 i;
    INT32 res;

    res = 0;

    for (i = 0; i < 4; i++)
    {
        res  = (res << 8) & 0xFFFFFF00;
        res |= *buf++;
    }

    return res;
}

/*
 * Top level routine to handle USB I/O.
 */
void ProcessUSBIO(void)
{
    static rtccTimeDate lastTime;
    rtccTimeDate time;
    UInt8 temp;
    Bool error;

    error = FALSE;

    // Do nothing if we are USB suspended or not yet configured
    if ((USBDeviceState < CONFIGURED_STATE) || (USBSuspendControl == 1))
        return;

    if (!HIDRxHandleBusy(USBOutHandle))
    {
        // Handle HID messages
        switch (ReceivedDataBuffer[0])
        {
            case 0x81: // Set current device date/time
                ToSendDataBuffer[0] = 0x81; // Echo the command back to the host

                time.f.year = ReceivedDataBuffer[1];
                time.f.mon  = ReceivedDataBuffer[2];
                time.f.mday = ReceivedDataBuffer[3];
                time.f.wday = ReceivedDataBuffer[4];
                time.f.hour = ReceivedDataBuffer[5];
                time.f.min  = ReceivedDataBuffer[6];
                time.f.sec  = ReceivedDataBuffer[7];

                RtccWriteTimeDate(&time, TRUE);

                if (!HIDTxHandleBusy(USBInHandle))
                    USBInHandle = HIDTxPacket(HID_EP, (BYTE*)&ToSendDataBuffer, 64);

                break;

            case 0x82: // Set calibration constants
                SerialNumber = BufferUnpackINT32(&ReceivedDataBuffer[1]);

                CalibrationTemperatureOffset = BufferUnpackINT32(&ReceivedDataBuffer[5]);
                CalibrationPressureOffset    = BufferUnpackINT32(&ReceivedDataBuffer[9]);

                CalibrationAccelXOffset = BufferUnpackINT32(&ReceivedDataBuffer[13]);
                CalibrationAccelYOffset = BufferUnpackINT32(&ReceivedDataBuffer[17]);
                CalibrationAccelZOffset = BufferUnpackINT32(&ReceivedDataBuffer[21]);

                CalibrationLightOffset = BufferUnpackINT32(&ReceivedDataBuffer[25]);
                break;

            case 0x83: // Flash write calibration constants
                LED_RED = 0;
                CalibrationWrite();
                LED_RED = 1;
                break;

            case 0x84: // Flash write calibration constants
                LED_RED = 0;
                CalibrationRead();
                LED_RED = 1;
                break;

            case 0x70: // Read System Info
                ToSendDataBuffer[0] = 0x70; // Echo the command back to the host

                ToSendDataBuffer[1] = FIRMWARE_MAJOR_VERSION;
                ToSendDataBuffer[2] = FIRMWARE_MINOR_VERSION;

                BufferPackINT32(&ToSendDataBuffer[3], SerialNumber);

                BufferPackINT32(&ToSendDataBuffer[7], CalibrationTemperatureOffset);
                BufferPackINT32(&ToSendDataBuffer[11], CalibrationPressureOffset);

                BufferPackINT32(&ToSendDataBuffer[15], CalibrationAccelXOffset);
                BufferPackINT32(&ToSendDataBuffer[19], CalibrationAccelYOffset);
                BufferPackINT32(&ToSendDataBuffer[23], CalibrationAccelZOffset);

                BufferPackINT32(&ToSendDataBuffer[27], CalibrationLightOffset);

                if (!HIDTxHandleBusy(USBInHandle))
                    USBInHandle = HIDTxPacket(HID_EP, (BYTE*)&ToSendDataBuffer, 64);

                break;

            case 0x71: // Read Battery Voltage
                ToSendDataBuffer[0] = 0x71; // Echo the command back to the host

                BufferPackINT32(&ToSendDataBuffer[1], ((((Int32)ADCRead()) * 330) / 1024));

                if (!HIDTxHandleBusy(USBInHandle))
                    USBInHandle = HIDTxPacket(HID_EP, (BYTE*)&ToSendDataBuffer, 64);

                break;

            case 0x72: // Read Button
                ToSendDataBuffer[0] = 0x72; // Echo the command back to the host

                ToSendDataBuffer[1] = BUTTON;

                if (!HIDTxHandleBusy(USBInHandle))
                    USBInHandle = HIDTxPacket(HID_EP, (BYTE*)&ToSendDataBuffer, 64);

                break;

            case 0x73: // Test LEDs
                ToSendDataBuffer[0] = 0x73; // Echo the command back to the host

                LED_RED = 0;
                Delayms(1);
                LED_GREEN = 0;

                Delayms(1000);
                LED_RED = 1;
                Delayms(1);
                LED_GREEN = 1;

                if (!HIDTxHandleBusy(USBInHandle))
                    USBInHandle = HIDTxPacket(HID_EP, (BYTE*)&ToSendDataBuffer, 64);

                break;

            case 0x74: // Check RTCC
                ToSendDataBuffer[0] = 0x74; // Echo the command back to the host

                RtccReadTimeDate(&time);
                temp = time.f.sec;
                Delayms(1300); // wait more than a second
                Delayms(1000);
                Delayms(1000);
                RtccReadTimeDate(&time);

                if (temp == time.f.sec) // no change? error!
                    ToSendDataBuffer[1] = 1;
                else
                    ToSendDataBuffer[1] = 0;

                if (!HIDTxHandleBusy(USBInHandle))
                    USBInHandle = HIDTxPacket(HID_EP, (BYTE*)&ToSendDataBuffer, 64);

                break;

            case 0x75: // Scan BMP085
                I2C3Start();
                error  = !I2C3WriteByte(0xEE, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + write bit
                error |= !I2C3WriteByte(0xAA, FALSE, I2C_STANDARD_TIMEOUT);
                I2C3Restart();
                I2C3WriteByte(0xEF, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + read bit
                ToSendDataBuffer[3] = I2C3ReadByte(FALSE, I2C_STANDARD_TIMEOUT);
                ToSendDataBuffer[2] = I2C3ReadByte(TRUE, I2C_STANDARD_TIMEOUT);
                I2C3Stop();

                ToSendDataBuffer[0] = 0x75; // Echo the command back to the host

                // also check for lack of pullups or stuck low
                ToSendDataBuffer[1] = error | !PORTEbits.RE6 | !PORTEbits.RE7;

                if (!HIDTxHandleBusy(USBInHandle))
                    USBInHandle = HIDTxPacket(HID_EP, (BYTE*)&ToSendDataBuffer, 64);

                break;

            case 0x76: // Scan MAX44007
                I2C2Start();
                error  = !I2C2WriteByte(0xB4, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + write bit
                error |= !I2C2WriteByte(0x03, FALSE, I2C_STANDARD_TIMEOUT);
                I2C2Restart();
                error |= !I2C2WriteByte(0xB5, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + read bit
                ToSendDataBuffer[3] = I2C2ReadByte(FALSE, I2C_STANDARD_TIMEOUT);
                ToSendDataBuffer[2] = I2C2ReadByte(TRUE, I2C_STANDARD_TIMEOUT);
                I2C2Stop();

                ToSendDataBuffer[0] = 0x76; // Echo the command back to the host

                // also check for lack of pullups or stuck low
                ToSendDataBuffer[1] = error | !PORTFbits.RF4 | !PORTFbits.RF5;

                if (!HIDTxHandleBusy(USBInHandle))
                    USBInHandle = HIDTxPacket(HID_EP, (BYTE*)&ToSendDataBuffer, 64);

                break;

            case 0x77: // Scan ADXL345
                I2C1Start();
                error  = !I2C1WriteByte(0xA6, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + write bit
                error |= !I2C1WriteByte(0x32, FALSE, I2C_STANDARD_TIMEOUT);
                I2C1Restart();
                error |= !I2C1WriteByte(0xA7, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + read bit
                ToSendDataBuffer[3] = I2C1ReadByte(FALSE, I2C_STANDARD_TIMEOUT);
                ToSendDataBuffer[2] = I2C1ReadByte(TRUE, I2C_STANDARD_TIMEOUT);
                I2C1Stop();

                ToSendDataBuffer[0] = 0x77; // Echo the command back to the host

                // also check for lack of pullups or stuck low
                ToSendDataBuffer[1] = error | !PORTDbits.RD9 | !PORTDbits.RD10;

                if (!HIDTxHandleBusy(USBInHandle))
                    USBInHandle = HIDTxPacket(HID_EP, (BYTE*)&ToSendDataBuffer, 64);

                break;

            case 0x78: // Check Pins for Shorts - not yet implemented
                ToSendDataBuffer[0] = 0x78; // Echo the command back to the host

                if (!HIDTxHandleBusy(USBInHandle))
                    USBInHandle = HIDTxPacket(HID_EP, (BYTE*)&ToSendDataBuffer, 64);

                break;

            case 0x79: // Run full diagnostics
                ToSendDataBuffer[0] = 0x79; // Echo the command back to the host

                // firmware version
                ToSendDataBuffer[1] = FIRMWARE_MAJOR_VERSION;
                ToSendDataBuffer[2] = FIRMWARE_MINOR_VERSION;

                // read battery voltage
                BufferPackINT32(&ToSendDataBuffer[3], ((((Int32)ADCRead()) * 330) / 1024));

                // read button
                ToSendDataBuffer[7] = BUTTON;

                // test LEDs
                LED_RED = 0;
                Delayms(1);
                LED_GREEN = 0;
                Delayms(1000);
                LED_RED = 1;
                Delayms(1);
                LED_GREEN = 1;

                // check RTCC
                RtccReadTimeDate(&time);
                temp = time.f.sec;
                Delayms(1300); // wait more than a second
                Delayms(1000);
                Delayms(1000);
                RtccReadTimeDate(&time);

                if (temp == time.f.sec) // no change? error!
                    ToSendDataBuffer[8] = 1;
                else
                    ToSendDataBuffer[8] = 0;

                // Scan BMP085
                I2C3Start();
                error  = !I2C3WriteByte(0xEE, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + write bit
                error |= !I2C3WriteByte(0xAA, FALSE, I2C_STANDARD_TIMEOUT);
                I2C3Restart();
                I2C3WriteByte(0xEF, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + read bit
                ToSendDataBuffer[11] = I2C3ReadByte(FALSE, I2C_STANDARD_TIMEOUT);
                ToSendDataBuffer[10]  = I2C3ReadByte(TRUE, I2C_STANDARD_TIMEOUT);
                I2C3Stop();
                ToSendDataBuffer[9] = error | !PORTEbits.RE6 | !PORTEbits.RE7;

                // Scan MAX440007
                I2C2Start();
                error  = !I2C2WriteByte(0xB4, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + write bit
                error |= !I2C2WriteByte(0x03, FALSE, I2C_STANDARD_TIMEOUT);
                I2C2Restart();
                error |= !I2C2WriteByte(0xB5, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + read bit
                ToSendDataBuffer[14] = I2C2ReadByte(FALSE, I2C_STANDARD_TIMEOUT);
                ToSendDataBuffer[13] = I2C2ReadByte(TRUE, I2C_STANDARD_TIMEOUT);
                I2C2Stop();
                ToSendDataBuffer[12] = error | !PORTFbits.RF4 | !PORTFbits.RF5;

                // Scan ADXL345
                I2C1Start();
                error  = !I2C1WriteByte(0xA6, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + write bit
                error |= !I2C1WriteByte(0x32, FALSE, I2C_STANDARD_TIMEOUT);
                I2C1Restart();
                error |= !I2C1WriteByte(0xA7, FALSE, I2C_STANDARD_TIMEOUT); // I2C 7-bit address + read bit
                ToSendDataBuffer[17] = I2C1ReadByte(FALSE, I2C_STANDARD_TIMEOUT);
                ToSendDataBuffer[16] = I2C1ReadByte(TRUE, I2C_STANDARD_TIMEOUT);
                I2C1Stop();
                ToSendDataBuffer[15] = error | !PORTDbits.RD9 | !PORTDbits.RD10;

                // Check Pins for Shorts - not yet implemented

                if (!HIDTxHandleBusy(USBInHandle))
                    USBInHandle = HIDTxPacket(HID_EP, (BYTE*)&ToSendDataBuffer, 64);

                break;

            default:
                break;
        }

        // Re-arm the OUT endpoint for the next packet
        USBOutHandle = HIDRxPacket(HID_EP, (BYTE*)&ReceivedDataBuffer, 64);
    }

    // Send updated date/time if no other outgoing traffic
    if (!HIDTxHandleBusy(USBInHandle))
    {
        RtccReadTimeDate(&time);

        if (time.f.sec != lastTime.f.sec)
        {
//            DataPoint* dp;

            lastTime.f.sec = time.f.sec;

            ToSendDataBuffer[0] = 0x80; // Echo the command back to the host

            // date and time
            ToSendDataBuffer[1] = time.f.year;
            ToSendDataBuffer[2] = time.f.mon;
            ToSendDataBuffer[3] = time.f.mday;
            ToSendDataBuffer[4] = time.f.wday;
            ToSendDataBuffer[5] = time.f.hour;
            ToSendDataBuffer[6] = time.f.min;
            ToSendDataBuffer[7] = time.f.sec;

//            dp = &DataPointBuffer[DataPointBufferIndex];

            // battery voltage
            BufferPackINT32(&ToSendDataBuffer[8], DataPointBatteryVoltage);

            // temperature and pressure
            BufferPackINT32(&ToSendDataBuffer[12], DataPointTemperature);
            BufferPackINT32(&ToSendDataBuffer[16], DataPointPressure);

            // altitude
            BufferPackINT32(&ToSendDataBuffer[20], DataPointAltitude);

            // acceleration
            BufferPackINT32(&ToSendDataBuffer[24], DataPointAccelX);
            BufferPackINT32(&ToSendDataBuffer[28], DataPointAccelY);
            BufferPackINT32(&ToSendDataBuffer[32], DataPointAccelZ);
            BufferPackINT32(&ToSendDataBuffer[36], DataPointAccelMag);

            // light
            BufferPackINT32(&ToSendDataBuffer[40], DataPointLight);

            USBInHandle = HIDTxPacket(HID_EP, (BYTE*)&ToSendDataBuffer, 64);
        }
    }

    MSDTasks(); // Handle the MSD USB tasks
}

// ******************************************************************************************************
// ************** USB Callback Functions ****************************************************************
// ******************************************************************************************************
// The USB firmware stack will call the callback functions USBCBxxx() in response to certain USB related
// events.  For example, if the host PC is powering down, it will stop sending out Start of Frame (SOF)
// packets to your device.  In response to this, all USB devices are supposed to decrease their power
// consumption from the USB Vbus to <2.5mA each.  The USB module detects this condition (which according
// to the USB specifications is 3+ms of no bus activity/SOF packets) and then calls the USBCBSuspend()
// function.  You should modify these callback functions to take appropriate actions for each of these
// conditions.  For example, in the USBCBSuspend(), you may wish to add code that will decrease power
// consumption from Vbus to <2.5mA (such as by clock switching, turning off LEDs, putting the
// microcontroller to sleep, etc.).  Then, in the USBCBWakeFromSuspend() function, you may then wish to
// add code that undoes the power saving things done in the USBCBSuspend() function.

// The USBCBSendResume() function is special, in that the USB stack will not automatically call this
// function.  This function is meant to be called from the application firmware instead.  See the
// additional comments near the function.

/******************************************************************************
 * Function:        void USBCBSuspend(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Call back that is invoked when a USB suspend is detected
 *
 * Note:            None
 *****************************************************************************/
void USBCBSuspend(void)
{
	//Example power saving code.  Insert appropriate code here for the desired
	//application behavior.  If the microcontroller will be put to sleep, a
	//process similar to that shown below may be used:
	
	//ConfigureIOPinsForLowPower();
	//SaveStateOfAllInterruptEnableBits();
	//DisableAllInterruptEnableBits();
	//EnableOnlyTheInterruptsWhichWillBeUsedToWakeTheMicro();	//should enable at least USBActivityIF as a wake source
	//Sleep();
	//RestoreStateOfAllPreviouslySavedInterruptEnableBits();	//Preferrably, this should be done in the USBCBWakeFromSuspend() function instead.
	//RestoreIOPinsToNormal();									//Preferrably, this should be done in the USBCBWakeFromSuspend() function instead.

	//IMPORTANT NOTE: Do not clear the USBActivityIF (ACTVIF) bit here.  This bit is 
	//cleared inside the usb_device.c file.  Clearing USBActivityIF here will cause 
	//things to not work as intended.	

/*
    U1EIR = 0xFFFF;
    U1IR = 0xFFFF;
    U1OTGIR = 0xFFFF;
    IFS5bits.USB1IF = 0;
    IEC5bits.USB1IE = 1;
    U1OTGIEbits.ACTVIE = 1;
    U1OTGIRbits.ACTVIF = 1;
    Sleep();
*/
}

/******************************************************************************
 * Function:        void USBCBWakeFromSuspend(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The host may put USB peripheral devices in low power
 *					suspend mode (by "sending" 3+ms of idle).  Once in suspend
 *					mode, the host may wake the device back up by sending non-
 *					idle state signalling.
 *					
 *					This call back is invoked when a wakeup from USB suspend 
 *					is detected.
 *
 * Note:            None
 *****************************************************************************/
void USBCBWakeFromSuspend(void)
{
	// If clock switching or other power savings measures were taken when
	// executing the USBCBSuspend() function, now would be a good time to
	// switch back to normal full power run mode conditions.  The host allows
	// a few milliseconds of wakeup time, after which the device must be 
	// fully back to normal, and capable of receiving and processing USB
	// packets.  In order to do this, the USB module must receive proper
	// clocking (IE: 48MHz clock must be available to SIE for full speed USB
	// operation).
}

/********************************************************************
 Function:        void USBCB_SOF_Handler(void)

 Description:     The USB host sends out a SOF packet to full-speed
                  devices every 1 ms. This interrupt may be useful
                  for isochronous pipes. End designers should
                  implement callback routine as necessary.

 PreCondition:    None
 
 Parameters:      None
 
 Return Values:   None
 
 Remarks:         None
 *******************************************************************/
void USBCB_SOF_Handler(void)
{
    // No need to clear UIRbits.SOFIF to 0 here.
    // Callback caller is already doing that.
}

/*******************************************************************
 * Function:        void USBCBErrorHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The purpose of this callback is mainly for
 *                  debugging during development. Check UEIR to see
 *                  which error causes the interrupt.
 *
 * Note:            None
 *******************************************************************/
void USBCBErrorHandler(void)
{
    // No need to clear UEIR to 0 here.
    // Callback caller is already doing that.

	// Typically, user firmware does not need to do anything special
	// if a USB error occurs.  For example, if the host sends an OUT
	// packet to your device, but the packet gets corrupted (ex:
	// because of a bad connection, or the user unplugs the
	// USB cable during the transmission) this will typically set
	// one or more USB error interrupt flags.  Nothing specific
	// needs to be done however, since the SIE will automatically
	// send a "NAK" packet to the host.  In response to this, the
	// host will normally retry to send the packet again, and no
	// data loss occurs.  The system will typically recover
	// automatically, without the need for application firmware
	// intervention.
	
	// Nevertheless, this callback function is provided, such as
	// for debugging purposes.
}

/*******************************************************************
 * Function:        void USBCBCheckOtherReq(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        When SETUP packets arrive from the host, some
 * 					firmware must process the request and respond
 *					appropriately to fulfill the request.  Some of
 *					the SETUP packets will be for standard
 *					USB "chapter 9" (as in, fulfilling chapter 9 of
 *					the official USB specifications) requests, while
 *					others may be specific to the USB device class
 *					that is being implemented.  For example, a HID
 *					class device needs to be able to respond to
 *					"GET REPORT" type of requests.  This
 *					is not a standard USB chapter 9 request, and 
 *					therefore not handled by usb_device.c.  Instead
 *					this request should be handled by class specific 
 *					firmware, such as that contained in usb_function_hid.c.
 *
 * Note:            None
 *******************************************************************/
void USBCBCheckOtherReq(void)
{
    USBCheckHIDRequest();
    USBCheckMSDRequest();
}

/*******************************************************************
 * Function:        void USBCBStdSetDscHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The USBCBStdSetDscHandler() callback function is
 *					called when a SETUP, bRequest: SET_DESCRIPTOR request
 *					arrives.  Typically SET_DESCRIPTOR requests are
 *					not used in most applications, and it is
 *					optional to support this type of request.
 *
 * Note:            None
 *******************************************************************/
void USBCBStdSetDscHandler(void)
{
    // Must claim session ownership if supporting this request
}

/*******************************************************************
 * Function:        void USBCBInitEP(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is called when the device becomes
 *                  initialized, which occurs after the host sends a
 * 					SET_CONFIGURATION (wValue not = 0) request.  This 
 *					callback function should initialize the endpoints 
 *					for the device's usage according to the current 
 *					configuration.
 *
 * Note:            None
 *******************************************************************/
void USBCBInitEP(void)
{
    #if (MSD_DATA_IN_EP == MSD_DATA_OUT_EP)
        USBEnableEndpoint(MSD_DATA_IN_EP,USB_IN_ENABLED|USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
    #else
        USBEnableEndpoint(MSD_DATA_IN_EP,USB_IN_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
        USBEnableEndpoint(MSD_DATA_OUT_EP,USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
    #endif

    // Enable the HID endpoint
    USBEnableEndpoint(HID_EP,USB_IN_ENABLED|USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
    // Re-arm the OUT endpoint for the next packet
    USBOutHandle = HIDRxPacket(HID_EP,(BYTE*)&ReceivedDataBuffer,64);

    USBMSDInit();
}

/********************************************************************
 * Function:        void USBCBSendResume(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The USB specifications allow some types of USB
 * 					peripheral devices to wake up a host PC (such
 *					as if it is in a low power suspend to RAM state).
 *					This can be a very useful feature in some
 *					USB applications, such as an Infrared remote
 *					control	receiver.  If a user presses the "power"
 *					button on a remote control, it is nice that the
 *					IR receiver can detect this signalling, and then
 *					send a USB "command" to the PC to wake up.
 *					
 *					The USBCBSendResume() "callback" function is used
 *					to send this special USB signalling which wakes 
 *					up the PC.  This function may be called by
 *					application firmware to wake up the PC.  This
 *					function will only be able to wake up the host if
 *                  all of the below are true:
 *					
 *					1.  The USB driver used on the host PC supports
 *						the remote wakeup capability.
 *					2.  The USB configuration descriptor indicates
 *						the device is remote wakeup capable in the
 *						bmAttributes field.
 *					3.  The USB host PC is currently sleeping,
 *						and has previously sent your device a SET 
 *						FEATURE setup packet which "armed" the
 *						remote wakeup capability.   
 *
 *                  If the host has not armed the device to perform remote wakeup,
 *                  then this function will return without actually performing a
 *                  remote wakeup sequence.  This is the required behavior, 
 *                  as a USB device that has not been armed to perform remote 
 *                  wakeup must not drive remote wakeup signalling onto the bus;
 *                  doing so will cause USB compliance testing failure.
 *                  
 *					This callback should send a RESUME signal that
 *                  has the period of 1-15ms.
 *
 * Note:            This function does nothing and returns quickly, if the USB
 *                  bus and host are not in a suspended condition, or are 
 *                  otherwise not in a remote wakeup ready state.  Therefore, it
 *                  is safe to optionally call this function regularly, ex: 
 *                  anytime application stimulus occurs, as the function will
 *                  have no effect, until the bus really is in a state ready
 *                  to accept remote wakeup. 
 *
 *                  When this function executes, it may perform clock switching,
 *                  depending upon the application specific code in 
 *                  USBCBWakeFromSuspend().  This is needed, since the USB
 *                  bus will no longer be suspended by the time this function
 *                  returns.  Therefore, the USB module will need to be ready
 *                  to receive traffic from the host.
 *
 *                  The modifiable section in this routine may be changed
 *                  to meet the application needs. Current implementation
 *                  temporary blocks other functions from executing for a
 *                  period of ~3-15 ms depending on the core frequency.
 *
 *                  According to USB 2.0 specification section 7.1.7.7,
 *                  "The remote wakeup device must hold the resume signaling
 *                  for at least 1 ms but for no more than 15 ms."
 *                  The idea here is to use a delay counter loop, using a
 *                  common value that would work over a wide range of core
 *                  frequencies.
 *                  That value selected is 1800. See table below:
 *                  ==========================================================
 *                  Core Freq(MHz)      MIP         RESUME Signal Period (ms)
 *                  ==========================================================
 *                      48              12          1.05
 *                       4              1           12.6
 *                  ==========================================================
 *                  * These timing could be incorrect when using code
 *                    optimization or extended instruction mode,
 *                    or when having other interrupts enabled.
 *                    Make sure to verify using the MPLAB SIM's Stopwatch
 *                    and verify the actual signal on an oscilloscope.
 *******************************************************************/
void USBCBSendResume(void)
{
    static WORD delay_count;
    
    //First verify that the host has armed us to perform remote wakeup.
    //It does this by sending a SET_FEATURE request to enable remote wakeup,
    //usually just before the host goes to standby mode (note: it will only
    //send this SET_FEATURE request if the configuration descriptor declares
    //the device as remote wakeup capable, AND, if the feature is enabled
    //on the host (ex: on Windows based hosts, in the device manager 
    //properties page for the USB device, power management tab, the 
    //"Allow this device to bring the computer out of standby." checkbox 
    //should be checked).
    if (USBGetRemoteWakeupStatus() == TRUE) 
    {
        // Verify that the USB bus is in fact suspended, before we send
        // remote wakeup signalling.
        if (USBIsBusSuspended() == TRUE)
        {
            USBMaskInterrupts();
            
            //Clock switch to settings consistent with normal USB operation.
            USBCBWakeFromSuspend();
            USBSuspendControl = 0; 
            USBBusIsSuspended = FALSE;  //So we don't execute this code again, 
                                        //until a new suspend condition is detected.

            //Section 7.1.7.7 of the USB 2.0 specifications indicates a USB
            //device must continuously see 5ms+ of idle on the bus, before it sends
            //remote wakeup signalling.  One way to be certain that this parameter
            //gets met, is to add a 2ms+ blocking delay here (2ms plus at 
            //least 3ms from bus idle to USBIsBusSuspended() == TRUE, yeilds
            //5ms+ total delay since start of idle).
            delay_count = 3600U;

            do
            {
                delay_count--;
            }
            while(delay_count);
            
            // Now drive the resume K-state signalling onto the USB bus.
            USBResumeControl = 1;       // Start RESUME signaling
            delay_count = 1800U;        // Set RESUME line for 1-13 ms

            do
            {
                delay_count--;
            }
            while(delay_count);

            USBResumeControl = 0;       //Finished driving resume signalling

            USBUnmaskInterrupts();
        }
    }
}

/*******************************************************************
 * Function:        BOOL USER_USB_CALLBACK_EVENT_HANDLER(
 *                        USB_EVENT event, void *pdata, WORD size)
 *
 * PreCondition:    None
 *
 * Input:           USB_EVENT event - the type of event
 *                  void *pdata - pointer to the event data
 *                  WORD size - size of the event data
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is called from the USB stack to
 *                  notify a user application that a USB event
 *                  occured.  This callback is in interrupt context
 *                  when the USB_INTERRUPT option is selected.
 *
 * Note:            None
 *******************************************************************/
BOOL USER_USB_CALLBACK_EVENT_HANDLER(USB_EVENT event, void *pdata, WORD size)
{
    switch((INT)event)
    {
        case EVENT_TRANSFER:
            //Add application specific callback task or callback function here if desired.
            break;

        case EVENT_SOF:
            USBCB_SOF_Handler();
            break;

        case EVENT_SUSPEND:
            USBCBSuspend();
            break;

        case EVENT_RESUME:
            USBCBWakeFromSuspend();
            break;

        case EVENT_CONFIGURED: 
            USBCBInitEP();
            break;

        case EVENT_SET_DESCRIPTOR:
            USBCBStdSetDscHandler();
            break;

        case EVENT_EP0_REQUEST:
            USBCBCheckOtherReq();
            break;

        case EVENT_BUS_ERROR:
            USBCBErrorHandler();
            break;

        case EVENT_TRANSFER_TERMINATED:
            //Add application specific callback task or callback function here if desired.
            //The EVENT_TRANSFER_TERMINATED event occurs when the host performs a CLEAR
            //FEATURE (endpoint halt) request on an application endpoint which was 
            //previously armed (UOWN was = 1).  Here would be a good place to:
            //1.  Determine which endpoint the transaction that just got terminated was 
            //      on, by checking the handle value in the *pdata.
            //2.  Re-arm the endpoint if desired (typically would be the case for OUT 
            //      endpoints).
            
            //Check if the host recently did a clear endpoint halt on the MSD OUT endpoint.
            //In this case, we want to re-arm the MSD OUT endpoint, so we are prepared
            //to receive the next CBW that the host might want to send.
            //Note: If however the STALL was due to a CBW not valid condition, 
            //then we are required to have a persistent STALL, where it cannot 
            //be cleared (until MSD reset recovery takes place).  See MSD BOT 
            //specs v1.0, section 6.6.1.
            if(MSDWasLastCBWValid() == FALSE)
            {
                //Need to re-stall the endpoints, for persistent STALL behavior.
    			USBStallEndpoint(MSD_DATA_IN_EP, IN_TO_HOST);
      			USBStallEndpoint(MSD_DATA_OUT_EP, OUT_FROM_HOST);                 
            }
            else
            {   
                //Check if the host cleared halt on the bulk out endpoint.  In this
                //case, we should re-arm the endpoint, so we can receive the next CBW.
                if((USB_HANDLE)pdata == USBGetNextHandle(MSD_DATA_OUT_EP, OUT_FROM_HOST))
                {
                    USBMSDOutHandle = USBRxOnePacket(MSD_DATA_OUT_EP, (BYTE*)&msd_cbw, MSD_OUT_EP_SIZE);
                }    
            }    
            break;

        default:
            break;
    }

    return TRUE; 
}

/*
 * Flash read/write routines (for serial number and calibration values)
 * Since we can only erase a full page at a time we need a full page for this, even though we only use a few bytes...
 */

/*
 * NOTE: Modifies and restores TBLPAG.  Make sure that if using interrupts and the PSV feature of the CPU 
 * in an ISR that the TBLPAG register is preloaded with the correct value (rather than assuming TBLPAG
 * is always pointing to the .const section.
 */
void CalibrationWrite(void)
{
    UINT16 buf[32];
    UINT8 i;
    DWORD_VAL addr = {0x00000000};
    WORD wTBLPAGSave;

    /*
     * Fill in the values we are going to write out to flash.
     */
    buf[0]  = ((SerialNumber >> 16) & 0xFFFF);
    buf[1]  = (SerialNumber & 0xFFFF);

    buf[2]  = ((CalibrationTemperatureOffset >> 16) & 0xFFFF);
    buf[3]  = (CalibrationTemperatureOffset & 0xFFFF);

    buf[4]  = ((CalibrationPressureOffset >> 16) & 0xFFFF);
    buf[5]  = (CalibrationPressureOffset & 0xFFFF);

    buf[6]  = ((CalibrationAccelXOffset >> 16) & 0xFFFF);
    buf[7]  = (CalibrationAccelXOffset & 0xFFFF);

    buf[8]  = ((CalibrationAccelYOffset >> 16) & 0xFFFF);
    buf[9]  = (CalibrationAccelYOffset & 0xFFFF);

    buf[10] = ((CalibrationAccelZOffset >> 16) & 0xFFFF);
    buf[11] = (CalibrationAccelZOffset & 0xFFFF);

    buf[12] = ((CalibrationLightOffset >> 16) & 0xFFFF);
    buf[13] = (CalibrationLightOffset & 0xFFFF);

    wTBLPAGSave = TBLPAG; // save TBLPAG register

    /*
     * First erase the page
     */
    addr.Val = ((DWORD)CALIBRATION_PAGE << 10);
    NVMCON = 0x4042; // erase page on next WR
    TBLPAG = addr.byte.UB;
    __builtin_tblwtl(addr.word.LW, 0xFFFF);

    asm("DISI #16"); // disable interrupts for next few instructions for unlock sequence
    __builtin_write_NVM();

    while(NVMCONbits.WR == 1) {} // wait until completion

    /*
     * Now write the new data to page.
     */
    addr.Val = ((DWORD)CALIBRATION_PAGE << 10);
    NVMCON = 0x4003; // perform WORD write next time WR gets set = 1.
    TBLPAG = addr.byte.UB;

    for (i = 0; i < 32; i++ )
    {
        addr.Val += i*2;
		__builtin_tblwtl(addr.word.LW, buf[i]); // write the low word to the latch
		__builtin_tblwth(addr.word.LW, 0xFFFF); // write the hight word to the latch (not used)

        asm("DISI #16"); // disable interrupts for next few instructions for unlock sequence
        __builtin_write_NVM();
        while (NVMCONbits.WR == 1) {} // wait until completion
    }

	NVMCONbits.WREN = 0; // clear WREN bit to reduce probability of accidental activation.

    TBLPAG = wTBLPAGSave; // restore TBLPAG register
}

void CalibrationRead(void)
{  
    UINT16 buf[32];
    UINT8 i;
    DWORD_VAL addr = {0x00000000};
    WORD wTBLPAGSave;
 
    wTBLPAGSave = TBLPAG; // save TBLPAG register

    addr.Val = ((DWORD)CALIBRATION_PAGE << 10);
    TBLPAG = addr.byte.UB;

    for (i = 0; i < 32; i++ )
    {
        addr.Val += i*2;
        buf[i] = __builtin_tblrdl(addr.word.LW);
    }

    TBLPAG = wTBLPAGSave; // restore TBLPAG register

    /*
     * Fill in the global variables.
     */
    SerialNumber = buf[0];
    SerialNumber = (((SerialNumber << 16) & 0xFFFF0000) | buf[1]);

    CalibrationTemperatureOffset = buf[2];
    CalibrationTemperatureOffset = (((CalibrationTemperatureOffset << 16) & 0xFFFF0000) | buf[3]);

    CalibrationPressureOffset    = buf[4];
    CalibrationPressureOffset    = (((CalibrationPressureOffset << 16) & 0xFFFF0000) | buf[5]);

    CalibrationAccelXOffset      = buf[6];
    CalibrationAccelXOffset      = (((CalibrationAccelXOffset << 16) & 0xFFFF0000) | buf[7]);

    CalibrationAccelYOffset      = buf[8];
    CalibrationAccelYOffset      = (((CalibrationAccelYOffset << 16) & 0xFFFF0000) | buf[9]);

    CalibrationAccelZOffset      = buf[10];
    CalibrationAccelZOffset      = (((CalibrationAccelZOffset << 16) & 0xFFFF0000) | buf[11]);

    CalibrationLightOffset       = buf[12];
    CalibrationLightOffset       = (((CalibrationLightOffset << 16) & 0xFFFF0000) | buf[13]);
}

/*
 * End of main.c
 */
