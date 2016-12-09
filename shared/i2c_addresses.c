#include "fsl_i2c.h"
#include "i2c_addresses.h"

/**
 * This file has couple of hacks in it, because the blocking i2c api provided by kinetis does not handle
 * i2c errors properly. Because of this, there were some stalls when keyboards were disconnected in middle
 * of a transfer.
 *
 */

#define TO_CLEAR_FLAGS (kI2C_ArbitrationLostFlag | kI2C_IntPendingFlag | kI2C_StartDetectFlag | kI2C_StopDetectFlag)

static status_t I2C_CheckAndClearError(I2C_Type *base, uint32_t status)
{
    status_t result = kStatus_Success;

    /* Check arbitration lost. */
    if (status & kI2C_ArbitrationLostFlag)
    {
        /* Clear arbitration lost flag. */
        base->S = kI2C_ArbitrationLostFlag;
        result = kStatus_I2C_ArbitrationLost;
    }
    /* Check NAK */
    else if (status & kI2C_ReceiveNakFlag)
    {
        result = kStatus_I2C_Nak;
    }
    else
    {
    }

    return result;
}

status_t pollForFlag(I2C_Type *baseAddress, uint32_t flag){
	uint16_t timeout=-1;
	status_t result = kStatus_Success;

	while ((!(baseAddress->S & flag)) && (timeout!=0))
	{
		timeout--;
	}

	if (timeout==0){
		result=I2C_CheckAndClearError(baseAddress, baseAddress->S);
		I2C_MasterStop(baseAddress);

		if (result!=kStatus_Success){
			result=kStatus_I2C_Timeout;
		}
	}

	return result;
}

status_t i2c_preamble(I2C_Type *baseAddress, uint8_t i2cAddress, i2c_direction_t dir){

	I2C_MasterClearStatusFlags(baseAddress, TO_CLEAR_FLAGS);
	/* Wait until ready to complete. */

	status_t result=pollForFlag(baseAddress, kI2C_TransferCompleteFlag);
	if (result){
		return result;
	}

	result = I2C_MasterStart(baseAddress, i2cAddress, dir);
	if (result){
		return result;
	}
	/* Wait until address + command transfer complete. */
	result=pollForFlag(baseAddress, kI2C_IntPendingFlag);
	if (result){
		return result;
	}

	/* Check if there's transfer error. */
	result = I2C_CheckAndClearError(baseAddress, baseAddress->S);

	/* Return if error. */
	if (result == kStatus_I2C_Nak) {
		I2C_MasterStop(baseAddress);
	}

	if (result){
		return result;
	}

	/* Wait until the data register is ready for transmit. */
	result=pollForFlag(baseAddress, kI2C_TransferCompleteFlag);

	return result;
}

// status_t I2C_MasterReadBlocking(I2C_Type *base, uint8_t *rxBuff, size_t rxSize)
status_t I2cRead(I2C_Type *base, uint8_t i2cAddress, uint8_t *rxBuff, uint8_t rxSize)
{
	status_t result = i2c_preamble(base, i2cAddress, kI2C_Read); // Added by Robi
	if (result) {                                                // Added by Robi
		return result;                                           // Added by Robi
	}                                                            // Added by Robi

	result = kStatus_Success;
	volatile uint8_t dummy = 0;

	/* Add this to avoid build warning. */
	dummy++;

//    /* Wait until the data register is ready for transmit. */
//    while (!(base->S & kI2C_TransferCompleteFlag))
//    {
//    }

	/* Clear the IICIF flag. */
	base->S = kI2C_IntPendingFlag;

	/* Setup the I2C peripheral to receive data. */
	base->C1 &= ~(I2C_C1_TX_MASK | I2C_C1_TXAK_MASK);

	/* If rxSize equals 1, configure to send NAK. */
	if (rxSize == 1)
	{
		/* Issue NACK on read. */
		base->C1 |= I2C_C1_TXAK_MASK;
	}

	/* Do dummy read. */
	dummy = base->D;

	while ((rxSize--))
	{
		/* Wait until data transfer complete. */
		result = pollForFlag(base, kI2C_IntPendingFlag); // Added by Robi
		if (result) {                                    // Added by Robi
			return result;                               // Added by Robi
		}                                                // Added by Robi

//        while (!(base->S & kI2C_IntPendingFlag))
//        {
//        }

		/* Clear the IICIF flag. */
		base->S = kI2C_IntPendingFlag;

		/* Single byte use case. */
		if (rxSize == 0)
		{
			/* Read the final byte. */
			result = I2C_MasterStop(base);
		}

		if (rxSize == 1)
		{
			/* Issue NACK on read. */
			base->C1 |= I2C_C1_TXAK_MASK;
		}

		/* Read from the data register. */
		*rxBuff++ = base->D;
	}

	return result;
}

// status_t I2C_MasterWriteBlocking(I2C_Type *base, const uint8_t *txBuff, size_t txSize)
status_t I2cWrite(I2C_Type *base, uint8_t i2cAddress, uint8_t *txBuff, uint8_t txSize)
{
	status_t result = i2c_preamble(base, i2cAddress, kI2C_Read); // Added by Robi
	if (result) {                                                // Added by Robi
		return result;                                           // Added by Robi
	}                                                            // Added by Robi

//    result = kStatus_Success;
	uint8_t statusFlags = 0;

//    /* Wait until the data register is ready for transmit. */
//    while (!(base->S & kI2C_TransferCompleteFlag))
//    {
//    }

	/* Clear the IICIF flag. */
	base->S = kI2C_IntPendingFlag;

	/* Setup the I2C peripheral to transmit data. */
	base->C1 |= I2C_C1_TX_MASK;

	while (txSize--)
	{
		/* Send a byte of data. */
		base->D = *txBuff++;

		/* Wait until data transfer complete. */
		result = pollForFlag(base, kI2C_IntPendingFlag); // Added by Robi
		if (result) {                                    // Added by Robi
			return result;                               // Added by Robi
		}                                                // Added by Robi

//        while (!(base->S & kI2C_IntPendingFlag))
//        {
//        }

		statusFlags = base->S;

		/* Clear the IICIF flag. */
		base->S = kI2C_IntPendingFlag;

		/* Check if arbitration lost or no acknowledgement (NAK), return failure status. */
		if (statusFlags & kI2C_ArbitrationLostFlag)
		{
			base->S = kI2C_ArbitrationLostFlag;
			result = kStatus_I2C_ArbitrationLost;
		}

		if (statusFlags & kI2C_ReceiveNakFlag)
		{
			base->S = kI2C_ReceiveNakFlag;
			result = kStatus_I2C_Nak;
		}

		if (result != kStatus_Success)
		{
			/* Breaking out of the send loop. */
			break;
		}
	}

	if ((result == kStatus_Success) || (result == kStatus_I2C_Nak)) // Added by Robi
	{                                                               // Added by Robi
		/* Clear the IICIF flag. */                                 // Added by Robi
		base->S = kI2C_IntPendingFlag;                              // Added by Robi
                                                                    // Added by Robi
		/* Send stop. */                                            // Added by Robi
		result = I2C_MasterStop(base);                              // Added by Robi
	}                                                               // Added by Robi

	return result;
}

