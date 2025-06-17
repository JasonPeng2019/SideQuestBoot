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
static copyBuffer[64]

void SideQuestBootloader(void){

    switch (SideQuest_State){

    case STATE_INIT: {
        bool * id_flag_data;
        if (EEPROM_ReadByte(PAGE_ID, id_flag_data)){
            if (*id_flag_data == false){
                uint8_t random_bytes[30];
                if (generate_random_bytes(random_bytes)){
                    uint16_t start_address = PAGE_ID;
                    bool new_flag = true;
                    EEPROM_WriteByte(start_address, &new_flag, 1);
                    start_address += 1;
                    EEPROM_WriteByte(start_address, random_bytes, 30)
                } else {break;}
            }
        } else {break;}

        bool * crashed_flag;
        if (EEPROM_Read_Flag(crashed_flag, PAGE_CRASH_FLAG)){
            if (crashed_flag == CRASHED){
                SideQuest_State = STATE_STARTING_READ_FROM_STABLE
            } else {
                SideQuest_State = STATE_VERIFY_UPDATE;
            }
        } else {break;}
        
        break;
    }

    case STATE_VERIFY_UPDATE: {
        bool * update_ready_flag;
        if (EEPROM_Read_Flag(PAGE_UPDATE_FLAG, update_ready_flag)){
            if (update_ready_flag == UPDATE_NEEDED) {
                current_state = STATE_CHECK_FW;
            } else {
                current_state = STATE_JUMP_TO_APP;
            }
        } else {break;}

        break;
    }

    case STATE_CHECK_FW:{
        bool * update_flag;
        if (EEPROM_Read_Flag(update_flag, PAGE_UPDATE_FLAG)){
            if (update_flag == UPDATE_NEEDED){
                uint8_t fw_header[30];
                memcpy(fw_header, UPDATE_IMAGE_START, 30);
                uint8_t id_header[30];
                if (EEPROM_ReadByte(PAGE_ID + 1, id_header, 30)){
                    if (fw_header == id_header){
                        SideQuest_State = STATE_ERASE_FLASH
                    }
                } else {break;}
            }
        } else {break;}

        break;
    }
    case STATE_STARTING_READ_FROM_STABLE:
        firmware_address = UPDATE_IMAGE_START;
        current_state = STATE_ERASE_FLASH;
        break;

    case STATE_ERASE_FLASH:
        if (erase_flash_partition(FLASH2_START, BANK_SIZE)) {
            HAL_FLASH_Unlock();  // Unlock before writing
            // initiate variables here instead of on top;
            current_state = STATE_READ_BLOCK;
        } else {
            current_state = STATE_ERROR;
        }
        break;

    case STATE_READ_BLOCK:
        if (memcpy(copyBuffer, pfirmware_address, BLOCK_SIZE)){
            pfirmware_address += BLOCK_SIZE;
            if (buffer == EOF_MARKER) {
                current_state = STATE_FINAL_CHECKSUM;
            } else {
                current_state = STATE_MOVING_BLOCK;
            }
            break;
        } else {break;}

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




bool generate_random_bytes(uint8_t *buffer, uint32_t length) { //length should be 30
    if (buffer == NULL || length == 0) {
        return false;
    }

    uint32_t random32;
    for (uint32_t i = 0; i < length; i += 4) {
        if (HAL_RNG_GenerateRandomNumber(&hrng, &random32) != HAL_OK) {
            return false;
        }

        buffer[i] = random32 & 0xFF;
        if (i + 1 < length) buffer[i + 1] = (random32 >> 8) & 0xFF;
        if (i + 2 < length) buffer[i + 2] = (random32 >> 16) & 0xFF;
        if (i + 3 < length) buffer[i + 3] = (random32 >> 24) & 0xFF;
    }

    return true;
}