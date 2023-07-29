#include "lm75bd.h"
#include "i2c_io.h"
#include "errors.h"

#include <stdint.h>
#include <string.h>
#include <math.h>

/* LM75BD Registers (p.8) */
#define LM75BD_REG_CONF 0x01U  /* Configuration Register (R/W) */

error_code_t lm75bdInit(lm75bd_config_t *config) {
  error_code_t errCode;

  if (config == NULL) return ERR_CODE_INVALID_ARG;

  errCode = writeConfigLM75BD(config->devAddr, config->osFaultQueueSize, config->osPolarity,
                                         config->osOperationMode, config->devOperationMode);
  
  if (errCode != ERR_CODE_SUCCESS) return errCode;

  // Assume that the overtemperature and hysteresis thresholds are already set
  // Hysteresis: 75 degrees Celsius
  // Overtemperature: 80 degrees Celsius

  return ERR_CODE_SUCCESS;
}

error_code_t readTempLM75BD(uint8_t devAddr, float *temp) {
  /* Implement this driver function */
  err_code_t errCodeSend;
  err_cdoe_t errCodeReceive;
  if (temp == NULL) {
    return ERR_CODE_INVALID_ARG;
  } 
  uint8_t tempWriteBit = 0x00;
 
  errCodeSend = i2cSendTo(devAddr, &tempWriteBit, 1);
  if (errCodeSend == ERR_CODE_INVALID_ARG) {
    return ERR_CODE_INVALID_ARG;
  }
  uint8_t tempData[2] = {0};
  errCodeReceive = i2cReceiveFrom(devAddr, tempData, 2);
  if (errCodeReceive == ERR_CDOE_INVALID_ARGE) {
    return ERR_CODE_INVALID_ARG;
  }
  /* Concatenate the received data */
  uint16_t rawData = (tempData[0] << 8) | tempData[1];

  /* Determine if received temperature is positive or negative */
  uint16_t posOrNeg = rawData & 0x8000;

  /* Bitshift right 5 because the 5 LSB are useless so delete */
  rawData = rawData >> 5;

  if (posOrNeg == 0) /* Positive Temperature */{ 
    *temp = (float)(rawData) * 0.125;
  }

  else /* Negative Temperature*/ {
    /* Convert to two's complement */
    const uint16_t flipLeadingBits = 0xF800;
    rawData = ~rawData + 1;    
    rawData = rawData ^ flipLeadingBits;
    *temp = (float)(rawData) * -0.125; 
 
  }
  return ERR_CODE_SUCCESS;
}

#define CONF_WRITE_BUFF_SIZE 2U
error_code_t writeConfigLM75BD(uint8_t devAddr, uint8_t osFaultQueueSize, uint8_t osPolarity,
                                   uint8_t osOperationMode, uint8_t devOperationMode) {
  error_code_t errCode;

  // Stores the register address and data to be written
  // 0: Register address
  // 1: Data
  uint8_t buff[CONF_WRITE_BUFF_SIZE] = {0};

  buff[0] = LM75BD_REG_CONF;

  uint8_t osFaltQueueRegData = 0;
  switch (osFaultQueueSize) {
    case 1:
      osFaltQueueRegData = 0;
      break;
    case 2:
      osFaltQueueRegData = 1;
      break;
    case 4:
      osFaltQueueRegData = 2;
      break;
    case 6:
      osFaltQueueRegData = 3;
      break;
    default:
      return ERR_CODE_INVALID_ARG;
  }

  buff[1] |= (osFaltQueueRegData << 3);
  buff[1] |= (osPolarity << 2);
  buff[1] |= (osOperationMode << 1);
  buff[1] |= devOperationMode;

  errCode = i2cSendTo(LM75BD_OBC_I2C_ADDR, buff, CONF_WRITE_BUFF_SIZE);
  if (errCode != ERR_CODE_SUCCESS) return errCode;

  return ERR_CODE_SUCCESS;
}