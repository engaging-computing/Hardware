
#ifndef __I2C_H__
#define __I2C_H__

#include "GenericTypeDefs.h"

#define I2C_STANDARD_TIMEOUT	500

/*
 * Prototypes
 */
void I2C1Setup(void);
void I2C1Start(void);
void I2C1Restart(void);
void I2C1Stop(void);
UINT8 I2C1ReadByte(BOOL ack, UINT16 ackTimeout);
BOOL I2C1WriteByte(UINT8 b, BOOL ack, UINT16 ackTimeout);
BOOL I2C1Write(UINT8 addr, UINT8* b, UINT8 count, UINT16 ackTimeout);

void I2C2Setup(void);
void I2C2Start(void);
void I2C2Restart(void);
void I2C2Stop(void);
UINT8 I2C2ReadByte(BOOL ack, UINT16 ackTimeout);
BOOL I2C2WriteByte(UINT8 b, BOOL ack, UINT16 ackTimeout);
BOOL I2C2Write(UINT8 addr, UINT8* b, UINT8 count, UINT16 ackTimeout);

void I2C3Setup(void);
void I2C3Start(void);
void I2C3Restart(void);
void I2C3Stop(void);
UINT8 I2C3ReadByte(BOOL ack, UINT16 ackTimeout);
BOOL I2C3WriteByte(UINT8 b, BOOL ack, UINT16 ackTimeout);
BOOL I2C3Write(UINT8 addr, UINT8* b, UINT8 count, UINT16 ackTimeout);

/*
#define I2CSetup(a)				I2C##a##Setup()
#define I2CStart(a)				I2C##a##Start()
#define I2CRestart(a)			I2C##a##Restart()
#define I2CStop(a)				I2C##a##Stop()
#define I2CReadByte(a,b)		I2C##a##ReadByte(b)
#define I2CWriteByte(a,b,c,d)	I2C##a##WriteByte(b,c,d)
#define I2CWrite(a,b,c,d,e)		I2C##a##Write(b,c,d,e)
*/

#endif // __I2C_H__

/*
 * End of I2C.h
 */
