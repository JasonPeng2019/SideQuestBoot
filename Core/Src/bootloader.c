#include "stm32l4xx_hal.h"
#include <string.h>
#include <stdbool.h>

// -------------------- Definitions --------------------
#define BLOCK_SIZE             8 // 64 bits = 8 bytes
#define MAX_RETRIES            3
#define EOF_MARKER             0xFFFFFFFFFFFFFFFF

#define APP_FLASH_BASE         0x08080000U  // App location in Flash2
#define FLASH_SIZE             0x10000      // Size of app region to erase
#define FLASH2_FLAGS_BASE      0x08090000U  // Flags parition in Flash2

// TO DO: Need to have a separate partition for the FLASH2_FLAGS_BASE
#define FLASH1_UPDATE_FW_BASE  0x08010000U  // Firmware update partition
#define FLASH1_STABLE_FW_BASE  0x08020000U  // Stable firmware partition
#define FLASH1_FLAGS_BASE      0x08030000U  // Flags partition

#define UART_HANDLE            huart2       // Adjust for your UART instance
#define CRC_HANDLE             hcrc         // HAL CRC handle

extern UART_HandleTypeDef UART_HANDLE;
extern CRC_HandleTypeDef CRC_HANDLE;

// -------------------- Flag Struct --------------------
typedef struct {
    bool crash_flag;
    bool update_ready;
    bool good_to_go;
    uint64_t device_id;
} BootFlags;

// -------------------- Globals --------------------
BootFlags flash1_flags;
BootFlags flash2_flags;

typedef enum {
    STATE_INIT,
    STATE_VERIFY_UPDATE,
    STATE_STARTING_READ_FROM_STABLE,
    STATE_ERASE_FLASH,
    STATE_READ_BLOCK,
    STATE_MOVING_BLOCK,
    STATE_FINAL_CHECKSUM,
    STATE_JUMP_TO_APP,
    STATE_ERROR
} BootState;

BootState current_state = STATE_INIT;

uint32_t start_address = 0;
uint32_t cursor = 0;
uint64_t buffer = 0;

// -------------------- Helper Functions --------------------
void UART_Print(const char* msg) {
    HAL_UART_Transmit(&UART_HANDLE, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
}

void delay_ms(uint32_t ms) {
    HAL_Delay(ms);
}

bool write_flash64(uint32_t addr, uint64_t data) {
    HAL_StatusTypeDef status;
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr, data);
    return (status == HAL_OK);
}

bool erase_flash_partition(uint32_t base_addr, uint32_t size) {
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t pageError = 0;

    eraseInit.TypeErase   = FLASH_TYPEERASE_PAGES;
    eraseInit.Page        = (base_addr - FLASH_BASE) / FLASH_PAGE_SIZE;
    eraseInit.NbPages     = size / FLASH_PAGE_SIZE;
    eraseInit.Banks       = FLASH_BANK_2;

    HAL_FLASH_Unlock();
    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&eraseInit, &pageError);
    HAL_FLASH_Lock();

    return (status == HAL_OK && pageError == 0xFFFFFFFFU);
}

uint32_t calculate_crc(uint32_t addr, uint32_t length) {
    HAL_CRC_DeInit(&CRC_HANDLE);
    HAL_CRC_Init(&CRC_HANDLE);
    return HAL_CRC_Calculate(&CRC_HANDLE, (uint32_t*)addr, length / 4);
}

void jump_to_app(uint32_t app_address) {
    typedef void (*pFunction)(void);
    uint32_t jump_address = *(__IO uint32_t*)(app_address + 4);
    pFunction app_entry = (pFunction)jump_address;
    __set_MSP(*(__IO uint32_t*)app_address);
    app_entry();
}

void load_flags() {
    // Cannot be set here, bootloader cannot write flags in its own partition while executing 
    // The main application will need to set the flags in flash 1
    // memcpy(&flash1_flags, (void*)FLASH1_FLAGS_BASE, sizeof(BootFlags));
    memcpy(&flash2_flags, (void*)FLASH2_FLAGS_BASE, sizeof(BootFlags));
    flash2_flags.good_to_go = true;
    flash2_flags.update_ready = false;
}

// -------------------- Main Bootloader Logic --------------------
void bootloader_main_loop() {
    load_flags();

    while (1) {
        switch (current_state) {

        case STATE_INIT:
            // This should be the flash2_flags, where do we set the crash flag?
            if (flash1_flags.crash_flag) {
                current_state = STATE_STARTING_READ_FROM_STABLE;
            } else {
                current_state = STATE_VERIFY_UPDATE;
            }
            break;

        case STATE_VERIFY_UPDATE: {
            if (flash1_flags.update_ready) {
                uint64_t fw_header;
                memcpy(&fw_header, (void*)FLASH1_UPDATE_FW_BASE, BLOCK_SIZE);
                if (fw_header == flash1_flags.device_id &&
                    fw_header == flash2_flags.device_id) {
                    start_address = FLASH1_UPDATE_FW_BASE;

                    //update flash2 flags
                    flash2_flags.good_to_go = false;
                    flash2_flags.update_ready = true;

                    current_state = STATE_ERASE_FLASH;
                } else {
                    current_state = STATE_JUMP_TO_APP;
                }
            } else {
                current_state = STATE_JUMP_TO_APP;
            }
            break;
        }

        case STATE_STARTING_READ_FROM_STABLE:
            start_address = FLASH1_STABLE_FW_BASE;
            current_state = STATE_ERASE_FLASH;
            break;

        case STATE_ERASE_FLASH:
            if (erase_flash_partition(APP_FLASH_BASE, FLASH_SIZE)) {
                cursor = 0;
                HAL_FLASH_Unlock();  // Unlock before writing
                current_state = STATE_READ_BLOCK;
            } else {
                current_state = STATE_ERROR;
            }
            break;

        case STATE_READ_BLOCK:
            memcpy(&buffer, (void*)(start_address + cursor), BLOCK_SIZE);
            if (buffer == EOF_MARKER) {
                current_state = STATE_FINAL_CHECKSUM;
            } else {
                current_state = STATE_MOVING_BLOCK;
            }
            break;

        case STATE_MOVING_BLOCK: {
            bool success = false;
            int retries = 0;
            while (retries < MAX_RETRIES && !success) {
                if (write_flash64(APP_FLASH_BASE + cursor, buffer)) {
                    uint32_t crc1 = calculate_crc(start_address, cursor + BLOCK_SIZE);
                    uint32_t crc2 = calculate_crc(APP_FLASH_BASE, cursor + BLOCK_SIZE);
                    if (crc1 == crc2) {
                        cursor += BLOCK_SIZE;
                        success = true;
                    }
                }
                retries++;
            }

            if (success) {
                current_state = STATE_READ_BLOCK;
            } else {
                current_state = STATE_ERROR;
            }
            break;
        }

        case STATE_FINAL_CHECKSUM: {
            uint32_t crc1 = calculate_crc(start_address, cursor);
            uint32_t crc2 = calculate_crc(APP_FLASH_BASE, cursor);
            HAL_FLASH_Lock();  // Done writing
            if (crc1 == crc2) {
                // Optionally update flag in flash2 that update completed
                flash2_flags.good_to_go = true;
                flash2_flags.update_ready = false;
                current_state = STATE_JUMP_TO_APP;
            } else {
                current_state = STATE_ERROR;
            }
            break;
        }

        case STATE_JUMP_TO_APP:
            jump_to_app(APP_FLASH_BASE);
            break;

        case STATE_ERROR:
            if (start_address == FLASH1_STABLE_FW_BASE) {
                while (1) {
                    UART_Print("FLASH FAILED\n");
                    delay_ms(5000);
                }
            } else {
                start_address = FLASH1_STABLE_FW_BASE;
                current_state = STATE_ERASE_FLASH;
            }
            break;
        }
    }
}
