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

status_t I2cRead(I2C_Type *baseAddress, uint8_t i2cAddress, uint8_t *data, uint8_t size)
{
	status_t result = i2c_preamble(baseAddress, i2cAddress, kI2C_Read);
	if (result){
		return result;
	}

	result = kStatus_Success;
	volatile uint8_t dummy = 0;

	/* Add this to avoid build warning. */
	dummy++;

	/* Clear the IICIF flag. */
	baseAddress->S = kI2C_IntPendingFlag;

	/* Setup the I2C peripheral to receive data. */
	baseAddress->C1 &= ~(I2C_C1_TX_MASK | I2C_C1_TXAK_MASK);

	/* If rxSize equals 1, configure to send NAK. */
	if (size == 1)
	{
		/* Issue NACK on read. */
		baseAddress->C1 |= I2C_C1_TXAK_MASK;
	}

	/* Do dummy read. */
	dummy = baseAddress->D;

	while ((size--))
	{
		/* Wait until data transfer complete. */
		result=pollForFlag(baseAddress, kI2C_IntPendingFlag);
		if (result){
			return result;
		}

		/* Clear the IICIF flag. */
		baseAddress->S = kI2C_IntPendingFlag;

		/* Single byte use case. */
		if (size == 0)
		{
			/* Read the final byte. */
			result = I2C_MasterStop(baseAddress);
		}

		if (size == 1)
		{
			/* Issue NACK on read. */
			baseAddress->C1 |= I2C_C1_TXAK_MASK;
		}

		/* Read from the data register. */
		*data++ = baseAddress->D;
	}

	return result;
}

status_t I2cWrite(I2C_Type *baseAddress, uint8_t i2cAddress, uint8_t *data, uint8_t size)
{
	status_t result = i2c_preamble(baseAddress, i2cAddress, kI2C_Read);
	if (result){
		return result;
	}

	uint8_t statusFlags = 0;

	/* Clear the IICIF flag. */
	baseAddress->S = kI2C_IntPendingFlag;

	/* Setup the I2C peripheral to transmit data. */
	baseAddress->C1 |= I2C_C1_TX_MASK;

	while (size--)
	{
		/* Send a byte of data. */
		baseAddress->D = *data++;

		/* Wait until data transfer complete. */
		result=pollForFlag(baseAddress, kI2C_IntPendingFlag);
		if (result){
			return result;
		}

		statusFlags = baseAddress->S;

		/* Clear the IICIF flag. */
		baseAddress->S = kI2C_IntPendingFlag;

		/* Check if arbitration lost or no acknowledgement (NAK), return failure status. */
		if (statusFlags & kI2C_ArbitrationLostFlag)
		{
			baseAddress->S = kI2C_ArbitrationLostFlag;
			result = kStatus_I2C_ArbitrationLost;
		}

		if (statusFlags & kI2C_ReceiveNakFlag)
		{
			baseAddress->S = kI2C_ReceiveNakFlag;
			result = kStatus_I2C_Nak;
		}

		if (result != kStatus_Success)
		{
			/* Breaking out of the send loop. */
			break;
		}
	}



	if ((result == kStatus_Success) || (result == kStatus_I2C_Nak))
	{
		/* Clear the IICIF flag. */
		baseAddress->S = kI2C_IntPendingFlag;

		/* Send stop. */
		result = I2C_MasterStop(baseAddress);
	}

	return result;
}

