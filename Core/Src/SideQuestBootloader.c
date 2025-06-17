/*
6/15/2026 Jason.P
Collaborators: Olir.E

SideQuestBootloader.c v0.00
*/

#include "../Inc/SideQuestBootloader.h"
#include "main.c"

/* Define start and end symbols for UPDATE_IMG */
// __update_img_start__ = ORIGIN(UPDATE_IMG);
// __update_img_end__   = ORIGIN(UPDATE_IMG) + LENGTH(UPDATE_IMG);

// ########################### -- FLAGS -- ##################################
// /*
// Memory Map of Flash 2 Flags:
// Bytes 0-60 of Page 1: Header Set in Main App; Device ID
// Page 2: Header Set in Main App
// Page 3: Crash flag
// Crash 62: Good-to-go flag (Update needed) - see .h file for definitions
// */

// // structure of flags:
// // When changing flags: ALWAYS do *flag = NEW_STATUS

// PROVIDE(_flash_flags = ORIGIN(B1_FLAGS));
// extern const uint8_t _flash_flags[];

// //read flags
// flashread_crash_flag = _flash_flags[61];
// flashread_update_needed = _flash_flags[62];

// //write flags
// # define FLASH2FLAG_ADDRESS      0x08078000; // 32 KB from the end of Flash Bank 2 - double check w Ben on linker
// #define EEPROM_PAGE_SIZE      0x800       // 2 KB
// #define EEPROM_NUM_PAGES      8
// uint8_t * flashwrite_crash_flag = (volatile uint8_t*)(FLASH2FLAG_ADDRESS + 61);
// uint8_t * flashwrite_update_needed = (volatile uint8_t*)(FLASH2FLAG_ADDRESS  + 62);

// ########################### ###############  ##################################


// these need to be externs as defined in linker, not defined here
// #define APP_FLASH_BASE         0x08080000U  // App location in Flash2
// #define FLASH_SIZE             0x10000      // Size of app region to erase
// #define FLASH2_FLAGS_BASE      0x08090000U  // Flags parition in Flash2
// // TO DO: Need to have a separate partition for the FLASH2_FLAGS_BASE
// #define FLASH1_UPDATE_FW_BASE  0x08010000U  // Firmware update partition
// #define FLASH1_STABLE_FW_BASE  0x08020000U  // Stable firmware partition
// #define FLASH1_FLAGS_BASE      0x08030000U  // Flags partition



Bootstate SideQuest_State;

void SideQuestBootloader(void){

    switch (SideQuest_State){

    case STATE_INIT:
        // This should be the flash2_flags, where do we set the crash flag?
        HAL_FLASH_Unlock();
        uint8_t random_bytes[60];
        if (generate_random_bytes(random_bytes)){

        }


        if (flashread_crash_flag == CRASHED) {
            SideQuest_State = STATE_STARTING_READ_FROM_STABLE;
        } else {
            SideQuest_State = STATE_VERIFY_UPDATE;
        }
        break;

    case STATE_VERIFY_UPDATE: {
        if (flashread_update_needed == UPDATE_NEEDED) {
            *flashwrite_update_needed = UPDATE_NEEDED;

            uint64_t fw_header;
            memcpy(&fw_header, (void*)UPDATE_PARTITION, HEADER_SIZE);
            if (fw_header == deviceID &&
                fw_header == flash2_flags.device_id) {
                start_address = FLASH1_UPDATE_FW_BASE;
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
                uint32_t crc1 = HAL_CRC_Calculate(&CRC_HANDLE, start_address, cursor + BLOCKSIZE);
                uint32_t crc2 = HAL_CRC_Calculate(&CRC_HANDLE, start_address, cursor + BLOCKSIZE);
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
        uint32_t crc1 = HAL_CRC_Calculate(&CRC_HANDLE, start_address, cursor);
        uint32_t crc2 = HAL_CRC_Calculate(&CRC_HANDLE, APP_FLASH_BASE, cursor);
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




HAL_StatusTypeDef generate_random_bytes(uint8_t *buffer, uint32_t length) { //length should be 30
    if (buffer == NULL || length == 0) {
        return HAL_ERROR;
    }

    uint32_t random32;
    for (uint32_t i = 0; i < length; i += 4) {
        if (HAL_RNG_GenerateRandomNumber(&hrng, &random32) != HAL_OK) {
            return HAL_ERROR;
        }

        buffer[i] = random32 & 0xFF;
        if (i + 1 < length) buffer[i + 1] = (random32 >> 8) & 0xFF;
        if (i + 2 < length) buffer[i + 2] = (random32 >> 16) & 0xFF;
        if (i + 3 < length) buffer[i + 3] = (random32 >> 24) & 0xFF;
    }

    return HAL_OK;
}