/*
 * PIC24F ADC functions.
 *
 * WARNING: I have determined that there seems to be an undocumented silicon error
 * on some PIC24F chips that once the ADC is enabled with AD1CON1bits.ADON = 1;
 * and later disabled with AD1CON1bits.ADON = 0; it still draws about 600uA.  Use
 * PMD1bits.ADC1MD = 1; to fully disable it and PMD1bits.ADC1MD = 0; to re-enable
 * (need to reinitialize all registers after re-enabling).
 */
// ADCON1bits.ADON: A/D Operating Mode bit (0 = disabled, 1 = operating)
// PMD1bits
#include "ADC.h"


void ADCSetup(void)
{
    // Configure A/D port
    TRISBbits.TRISB6 = 1;
    AD1PCFGL = 0xFFBF; // AN6 input pin is analog

    AD1CON1 = 0x00E0; // Internal counter triggers conversion
    AD1CHS = 0b0000011000000110; // Connect AN0 as positive input
    AD1CSSL = 0;
    AD1CON3 = 0x0F00; // Sample time = 15Tad, Tad = Tcy
    AD1CON2 = 0x003C; // Set AD1IF after every 16 samples

    AD1CON1bits.ADON = 1; // turn ADC ON
}

Int32 ADCRead(void)
{
    Int32 ADCValue, count;
    Int16* ADC16Ptr;

    // Configure A/D port
    PMD1bits.ADC1MD = 0; // power the module

    ADCSetup();

    ADCValue = 0; // clear value
    ADC16Ptr = &ADC1BUF0; // initialize ADC1BUF pointer
    IFS0bits.AD1IF = 0; // clear ADC interrupt flag
    AD1CON1bits.ASAM = 1; // auto start sampling for 31Tad

    // then go to conversion
    while (!IFS0bits.AD1IF){}; // conversion done?

    AD1CON1bits.ASAM = 0; // yes then stop sample/convert

    for (count = 0; count < 16; count++) // average the 16 ADC values
    {
        ADCValue = ADCValue + *ADC16Ptr++;
    }

    ADCValue = ADCValue >> 4;

    AD1CON1bits.ADON = 0; // turn ADC off (may be broken!)
    PMD1bits.ADC1MD = 1; // complete remove power

    return ADCValue;
}

/*
 * End of ADC.c
 */
