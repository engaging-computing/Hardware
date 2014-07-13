/*
 * I2C functions.
 */

#include <ports.h>
#include "I2C.h"

void I2C1Setup(void)
{
    // Note: enabling the I2C module overrides and I/O port
    // direction settings (so no need to consider these registers)

    // assure that interrupts are disabled during setup
    IEC1bits.MI2C1IE = 0;
    IFS1bits.MI2C1IF = 0;

    I2C1CON  = 0;
    I2C1STAT = 0;

    // calculate value to set with
    //   I2C1BRG = (Fcy/Fscl - Fcy/(10^7)) - 1

    // set baud rate based on Fosc = 32MHz (Fcy = Fosc/2 = 16MHz)
//    I2C1BRG = 0x39;
    I2C1BRG = 37;

    I2C1CONbits.I2CEN = 1; // enable the module
//    IEC1bits.MI2C1IE  = 1; // enable interrupts
}

void I2C1Start(void)
{
    I2C1CONbits.SEN = 1; // initiate start sequence
    while (I2C1CONbits.SEN == 1) {} // wait for start sequence to finish
}

void I2C1Restart(void)
{
    I2C1CONbits.RSEN = 1; // initiate start sequence
    while (I2C1CONbits.RSEN == 1) {} // wait for start sequence to finish
}

void I2C1Stop(void)
{
    I2C1CONbits.PEN = 1; // initiate start sequence
    while (I2C1CONbits.PEN == 1) {} // wait for start sequence to finish
}

UINT8 I2C1ReadByte(BOOL ack, UINT16 ackTimeout)
{
    UINT8 b;

    I2C1CONbits.RCEN = 1;            // start receive sequence

    while (I2C1CONbits.RCEN == 1 && ackTimeout > 0) // wait for receive sequence to finish
        ackTimeout--;

    b = I2C1RCV;

    I2C1CONbits.ACKDT = ack; // send ACK/NACK (0 -- ack, 1 -- nack)
    I2C1CONbits.ACKEN = 1;   // start ACK transmission

    while (I2C1CONbits.ACKEN == 1 && ackTimeout > 0)
        ackTimeout--;

    return b;
}

BOOL I2C1WriteByte(UINT8 b, BOOL ack, UINT16 ackTimeout)
{
    I2C1TRN = b;
    while (I2C1STATbits.TRSTAT == 1) {} // wait for transmission to finish

    while (I2C1STATbits.ACKSTAT == !ack && ackTimeout > 0) // wait for slave ACK/NACK
        ackTimeout--;

    if (ackTimeout == 0)
        return FALSE;

    return TRUE;
}

BOOL I2C1Write(UINT8 addr, UINT8* b, UINT8 count, UINT16 ackTimeout)
{
    UINT8 i;
    BOOL success = TRUE;

    I2C1Start();
    I2C1WriteByte(addr, FALSE, ackTimeout);

    for (i = 0; i < count; i++)
    {
        if (!I2C1WriteByte(b[i], FALSE, ackTimeout))
        {
            success = FALSE;
            break;
        }
    }

    I2C1Stop();
    return success;
}

void I2C2Setup(void)
{
    // Note: enabling the I2C module overrides and I/O port
    // direction settings (so no need to consider these registers)

    // assure that interrupts are disabled during setup
    IEC3bits.MI2C2IE = 0;
    IFS3bits.MI2C2IF = 0;

    I2C2CON  = 0;
    I2C2STAT = 0;

    // calculate value to set with
    //   I2C2BRG = (Fcy/Fscl - Fcy/(10^7)) - 1

    // set baud rate based on Fosc = 32MHz (Fcy = Fosc/2 = 16MHz)
//    I2C2BRG = 0x39;
    I2C2BRG = 37;

    I2C2CONbits.I2CEN = 1; // enable the module
//    IEC1bits.MI2C1IE  = 1; // enable interrupts
}

void I2C2Start(void)
{
    I2C2CONbits.SEN = 1; // initiate start sequence
    while (I2C2CONbits.SEN == 1) {} // wait for start sequence to finish
}

void I2C2Restart(void)
{
    I2C2CONbits.RSEN = 1; // initiate start sequence
    while (I2C2CONbits.RSEN == 1) {} // wait for start sequence to finish
}

void I2C2Stop(void)
{
    I2C2CONbits.PEN = 1; // initiate start sequence
    while (I2C2CONbits.PEN == 1) {} // wait for start sequence to finish
}

UINT8 I2C2ReadByte(BOOL ack, UINT16 ackTimeout)
{
    UINT8 b;

    I2C2CONbits.RCEN = 1;            // start receive sequence

    while (I2C2CONbits.RCEN == 1 && ackTimeout > 0) // wait for receive sequence to finish
        ackTimeout--;

    b = I2C2RCV;

    I2C2CONbits.ACKDT = ack; // send ACK/NACK (0 -- ack, 1 -- nack)
    I2C2CONbits.ACKEN = 1;   // start ACK transmission

    while (I2C2CONbits.ACKEN == 1 && ackTimeout > 0)
        ackTimeout--;

    return b;
}

BOOL I2C2WriteByte(UINT8 b, BOOL ack, UINT16 ackTimeout)
{
    I2C2TRN = b;
    while (I2C2STATbits.TRSTAT == 1) {} // wait for transmission to finish

    while (I2C2STATbits.ACKSTAT == !ack && ackTimeout > 0) // wait for slave ACK/NACK
        ackTimeout--;

    if (ackTimeout == 0)
        return FALSE;

    return TRUE;
}

BOOL I2C2Write(UINT8 addr, UINT8* b, UINT8 count, UINT16 ackTimeout)
{
    UINT8 i;
    BOOL success = TRUE;

    I2C2Start();
    I2C2WriteByte(addr, FALSE, ackTimeout);

    for (i = 0; i < count; i++)
    {
        if (!I2C2WriteByte(b[i], FALSE, ackTimeout))
        {
            success = FALSE;
            break;
        }
    }

    I2C2Stop();
    return success;
}

void I2C3Setup(void)
{
    // Note: enabling the I2C module overrides and I/O port
    // direction settings (so no need to consider these registers)

    // assure that interrupts are disabled during setup
    IEC5bits.MI2C3IE = 0;
    IFS5bits.MI2C3IF = 0;

    I2C3CON  = 0;
    I2C3STAT = 0;

    // calculate value to set with
    //   I2C3BRG = (Fcy/Fscl - Fcy/(10^7)) - 1

    // set baud rate based on Fosc = 32MHz (Fcy = Fosc/2 = 16MHz)
//    I2C3BRG = 0x39;
    I2C3BRG = 37;

    I2C3CONbits.I2CEN = 1; // enable the module
//    IEC1bits.MI2C1IE  = 1; // enable interrupts
}

void I2C3Start(void)
{
    I2C3CONbits.SEN = 1; // initiate start sequence
    while (I2C3CONbits.SEN == 1) {} // wait for start sequence to finish
}

void I2C3Restart(void)
{
    I2C3CONbits.RSEN = 1; // initiate start sequence
    while (I2C3CONbits.RSEN == 1) {} // wait for start sequence to finish
}

void I2C3Stop(void)
{
    I2C3CONbits.PEN = 1; // initiate start sequence
    while (I2C3CONbits.PEN == 1) {} // wait for start sequence to finish
}

UINT8 I2C3ReadByte(BOOL ack, UINT16 ackTimeout)
{
    UINT8 b;

    I2C3CONbits.RCEN = 1;            // start receive sequence

    while (I2C3CONbits.RCEN == 1 && ackTimeout > 0) // wait for receive sequence to finish
        ackTimeout--;

    b = I2C3RCV;

    I2C3CONbits.ACKDT = ack; // send ACK/NACK (0 -- ack, 1 -- nack)
    I2C3CONbits.ACKEN = 1;   // start ACK transmission

    while (I2C3CONbits.ACKEN == 1 && ackTimeout > 0)
        ackTimeout--;

    return b;
}

BOOL I2C3WriteByte(UINT8 b, BOOL ack, UINT16 ackTimeout)
{
    I2C3TRN = b;
    while (I2C3STATbits.TRSTAT == 1) {} // wait for transmission to finish

    while (I2C3STATbits.ACKSTAT == !ack && ackTimeout > 0) // wait for slave ACK/NACK
        ackTimeout--;

    if (ackTimeout == 0)
        return FALSE;

    return TRUE;
}

BOOL I2C3Write(UINT8 addr, UINT8* b, UINT8 count, UINT16 ackTimeout)
{
    UINT8 i;
    BOOL success = TRUE;

    I2C3Start();
    I2C3WriteByte(addr, FALSE, ackTimeout);

    for (i = 0; i < count; i++)
    {
        if (!I2C3WriteByte(b[i], FALSE, ackTimeout))
        {
            success = FALSE;
            break;
        }
    }

    I2C3Stop();
    return success;
}

/*
 * End of I2C.c
 */
