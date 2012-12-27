
#ifndef __IOCONFIG_H__
#define __IOCONFIG_H__

    #define INPUT_PIN 1
    #define OUTPUT_PIN 0

    /** I/O pin definitions ********************************************/
    #define BUTTON_TRIS			TRISBbits.TRISB14
    #define LED_RED_TRIS		TRISBbits.TRISB12
    #define LED_GREEN_TRIS		TRISBbits.TRISB13
    #define BUTTON				PORTBbits.RB14
    #define LED_RED				PORTBbits.RB12
    #define LED_GREEN			PORTBbits.RB13

	#define SD_PWR_TRIS			TRISBbits.TRISB3
	#define SD_PWR				PORTBbits.RB3

	#define BMP085_EOC_TRIS		TRISDbits.TRISD0
	#define BMP085_XCLR_TRIS	TRISDbits.TRISD11
	#define BMP085_EOC			PORTDbits.RD0
	#define BMP085_XCLR			PORTDbits.RD11

	#define ADXL_INTR_TRIS		TRISDbits.TRISD8
	#define ADXL_INTR			PORTDbits.RD8

	#define LIGHT_INTR_TRIS		TRISBbits.TRISB15
	#define LIGHT_INTR			PORTBbits.RB15

    /*******************************************************************/
    /******** USB stack hardware selection options *********************/
    /*******************************************************************/
    //This section is the set of definitions required by the MCHPFSUSB
    //  framework.  These definitions tell the firmware what mode it is
    //  running in, and where it can find the results to some information
    //  that the stack needs.
    //These definitions are required by every application developed with
    //  this revision of the MCHPFSUSB framework.  Please review each
    //  option carefully and determine which options are desired/required
    //  for your application.

    //#define USE_SELF_POWER_SENSE_IO
    //#define tris_self_power     TRISAbits.TRISA2    // Input
    #define self_power          1

    #define USE_USB_BUS_SENSE_IO
    #define tris_usb_bus_sense  TRISDbits.TRISD3    // Input
    #define USB_BUS_SENSE       PORTDbits.RD3 

    //Uncomment this to make the output HEX of this project 
    //   to be able to be bootloaded using the HID bootloader
    #define PROGRAMMABLE_WITH_USB_HID_BOOTLOADER	

    //If the application is going to be used with the HID bootloader
    //  then this will provide a function for the application to 
    //  enter the bootloader from the application (optional)
    #if defined(PROGRAMMABLE_WITH_USB_HID_BOOTLOADER)
        #define EnterBootloader() __asm__("goto 0x400")
    #endif   

    /*******************************************************************/
    /******** MDD File System selection options ************************/
    /*******************************************************************/
    #define USE_SD_INTERFACE_WITH_SPI

    // SD Card connections
    #define SD_CS				PORTGbits.RG6
    #define SD_CS_TRIS			TRISGbits.TRISG6
    
    //#define SD_CD				PORTCbits.RC15
    //#define SD_CD_TRIS		TRISCbits.TRISC15
    #define SD_CD               0
   
    //#define SD_WE				PORTFbits.RF1
    //#define SD_WE_TRIS		TRISFbits.TRISF1
    #define SD_WE				0

    // Registers for the SPI module we are using
    #define SPICON1				SPI2CON1
    #define SPISTAT				SPI2STAT
    #define SPIBUF				SPI2BUF
    #define SPISTAT_RBF			SPI2STATbits.SPIRBF
    #define SPICON1bits			SPI2CON1bits
    #define SPISTATbits			SPI2STATbits
    #define SPIENABLE           SPISTATbits.SPIEN

    // Tristate pins for SCK/SDI/SDO lines
    #define SPICLOCK			TRISGbits.TRISG8
    #define SPIIN				TRISGbits.TRISG9
    #define SPIOUT			    TRISGbits.TRISG7

	// Clock frequency
    #define CLOCK_FREQ 32000000
//    #define CLOCK_FREQ 16000000
    #define GetSystemClock() CLOCK_FREQ
    #define GetInstructionClock() GetSystemClock()

    /** MDD File System error checking *********************************/
    // Will generate an error if the clock speed is too low to interface to the card
    #if (GetSystemClock() < 100000)
        #error Clock speed must exceed 100 kHz
    #endif 

#endif  // __IOCONFIG_H__
