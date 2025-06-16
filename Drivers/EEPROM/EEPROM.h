#include "string.h"
#include "main.c"
#include "stdbool.h"


#define M24_I2C_ADDR            (0x50 << 1) // Shifted left for STM32 HAL
#define PAGE_SIZE               64
#define MAX_WRITES_PER_PAGE     25000

//for writing flags
#define EEPROM_PAGE_SIZE      64
#define NUM_FLAG_PAGES        3
#define MAX_WRITES_PER_PAGE   25000
#define FLAG_RECORD_SIZE      3

extern I2C_HandleTypeDef hi2c1; // or use your instance (from CubeMX)

bool EEPROM_WriteByte(uint16_t memAddress, uint8_t data);
bool EEPROM_ReadByte(uint16_t memAddress, uint8_t *data);
bool EEPROM_WritePage(uint16_t memAddress, uint8_t *data, uint16_t len);
bool EEPROM_Read(uint16_t memAddress, uint8_t *data, uint16_t len);
bool EEPROM_WaitReady(void);
uint64_t EEPROM_ReadUint64(uint16_t address);
bool EEPROM_WriteUint64(uint16_t address, uint64_t value);
bool EEPROM_Write_Flag(uint8_t *data, uint8_t page_start);
bool EEPROM_Read_Flag(uint8_t *flag, uint8_t page_start);