#include "string.h"
#include "stm32l4xx_hal.h"
#include "stdbool.h"


#define M24_I2C_ADDR            (0x50 << 1) // Shifted left for STM32 HAL
#define PAGE_SIZE               64
#define MAX_WRITES_PER_PAGE     25000

//for writing flags
#define EEPROM_PAGE_SIZE      64
#define NUM_FLAG_PAGES        3
#define MAX_WRITES_PER_PAGE   25000
#define FLAG_RECORD_SIZE      3
#define EEPROM_TIMEOUT 100

bool EEPROM_WriteByte(uint16_t memAddress, uint8_t data, I2C_HandleTypeDef *i2c_handle);
bool EEPROM_ReadByte(uint16_t memAddress, uint8_t *data, I2C_HandleTypeDef *i2c_handle);
bool EEPROM_WritePage(uint16_t memAddress, uint8_t *data, uint16_t len, I2C_HandleTypeDef *i2c_handle);
bool EEPROM_Read(uint16_t memAddress, uint8_t *data, uint16_t len, I2C_HandleTypeDef *i2c_handle);
bool EEPROM_WaitReady(I2C_HandleTypeDef *i2c_handle);
uint64_t EEPROM_ReadUint64(uint16_t address, I2C_HandleTypeDef *i2c_handle);
bool EEPROM_WriteUint64(uint16_t address, uint64_t value, I2C_HandleTypeDef *i2c_handle);
bool EEPROM_Write_Flag(uint8_t *data, uint8_t page_start, I2C_HandleTypeDef *i2c_handle);
bool EEPROM_Read_Flag(uint8_t *flag, uint8_t page_start, I2C_HandleTypeDef *i2c_handle);
