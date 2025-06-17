#include <stdio.h>
#include "stm32l4xx_hal.h"
#include "../../Drivers/EEPROM/EEPROM.h"
#include "../../Drivers/UART/UART.c"
#include "flags.h"
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>





#define BLOCKSIZE           64U
#define MAX_TRIES           8
#define HEADER_SIZE         60
#define BANK_SIZE           524288 // bank size in bytes

#define FLASH2_START         0x08080000




#define UPDATE_IMAGE_START      ((uint32_t)&__update_img_start__) // fill in here
#define STABLE_IMAGE_START       ((uint32_t)&__backup_app_start__)// fill in here


extern uint32_t __update_img_start__;
extern uint32_t __backup_app_start__;


#define EOF_MARKER                   0xDEADBEEF

typedef enum {
    STATE_INIT,
    STATE_VERIFY_UPDATE,
    STATE_CHECK_FW,
    STATE_STARTING_READ_FROM_STABLE,
    STATE_ERASE_FLASH,
    STATE_READ_BLOCK,
    STATE_MOVING_BLOCK,
    STATE_FINAL_CHECKSUM,
    STATE_JUMP_TO_APP,
    STATE_ERROR
} BootState;

typedef struct {
    UART_HandleTypeDef *  Boot_UART_Handle;
    I2C_HandleTypeDef *   Boot_I2C_Handle;
} tBootloader;

typedef void (*pFunction)(void);


void Bootloader_init(I2C_HandleTypeDef *i2c_handle, UART_HandleTypeDef *UART_Handle);
void SideQuestBootloader(void);
bool generate_random_bytes(uint8_t *buffer, uint32_t length);
void jump_to_app(uint32_t address_start);
bool Valid_FW_Check(void);



