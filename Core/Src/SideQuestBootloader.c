/*
6/15/2026 Jason.P
Collaborators: Olir.E

SideQuestBootloader.c v0.00
*/

#define DEBUG_MODE

#include "../Inc/SideQuestBootloader.h"
#include "main.h"
#include <stm32l476xx.h>

extern CRC_HandleTypeDef hcrc;
extern RNG_HandleTypeDef hrng;

//to do: set IWDG script, link IWDG
// need to prepad the block with 0's at the end when saving from BLE

static uint32_t firmware_address;
//pointer at the address in src we have successfully copied`
static uint32_t * pSuccess_read_marker;
//pointer at the address of dest we have successfully copied
static uint32_t * pSuccess_write_marker;
BootState SideQuest_State;
FLASH_EraseInitTypeDef * pEraseInit;

tBootloader *SideQuest;

void SideQuestBootloader(void){    
    
    switch (SideQuest_State){

    case STATE_INIT: {
    	uint8_t IWDG_Flag = __HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST);
        if (IWDG_Flag){
            uint8_t crash_Flag = CRASHED;
            EEPROM_Write_Flag(&crash_Flag, PAGE_CRASH_FLAG, &SideQuest->Boot_I2C_Handle);
        } else {break;}

        uint8_t LP_flag;
        if (EEPROM_Read_Flag(&LP_flag, PAGE_LP_FLAG, &SideQuest->Boot_I2C_Handle)){
            if (LP_flag == LOW_POWER){
                printf("Low Power. Jumping to app");
                jump_to_app(FLASH2_START);
            }
        } else {break;}

        uint8_t id_flag_data;
        if (EEPROM_ReadByte(PAGE_ID, &id_flag_data, &SideQuest->Boot_I2C_Handle)){
            if (id_flag_data == 0){
                uint8_t random_bytes[30];
                if (generate_random_bytes(random_bytes, sizeof(random_bytes))){
                    uint16_t start_address = PAGE_ID;
                    uint8_t new_flag = 1;
                    EEPROM_WriteByte(start_address, new_flag, &SideQuest->Boot_I2C_Handle);
                    start_address += 1;
                    EEPROM_WritePage(start_address, random_bytes, 30, &SideQuest->Boot_I2C_Handle);
                } else {break;}
            }
        } else {break;}

        uint8_t crashed_flag;
        if (EEPROM_Read_Flag(&crashed_flag, PAGE_CRASH_FLAG, &SideQuest->Boot_I2C_Handle)){
            if (crashed_flag == CRASHED){
                SideQuest_State = STATE_STARTING_READ_FROM_STABLE;
            } else {
                SideQuest_State = STATE_VERIFY_UPDATE;
            }
        } else {break;}
        
        break;
    }

    case STATE_VERIFY_UPDATE: {
        uint8_t update_ready_flag;
        if (EEPROM_Read_Flag(&update_ready_flag, PAGE_UPDATE_FLAG, &SideQuest->Boot_I2C_Handle)){
        	update_ready_flag = UPDATE_NEEDED;//comment out later; for debugging
            if (update_ready_flag == UPDATE_NEEDED) {
                SideQuest_State = STATE_CHECK_FW;
            } else {
                SideQuest_State = STATE_JUMP_TO_APP;
            }
        } else {break;}

        break;
    }

    case STATE_CHECK_FW:{
        uint8_t update_flag;
        if (EEPROM_Read_Flag(&update_flag, PAGE_UPDATE_FLAG, &SideQuest->Boot_I2C_Handle)){
            if (update_flag == UPDATE_NEEDED){
                uint8_t fw_header[30];
                memcpy(fw_header, UPDATE_IMAGE_START, 30);
                uint8_t id_header[30];
                if (EEPROM_Read(PAGE_ID + 1, id_header, 30, &SideQuest->Boot_I2C_Handle)){
                    if (memcmp(fw_header, id_header, sizeof(fw_header)) == 0){
                        firmware_address = UPDATE_IMAGE_START;
                        SideQuest_State = STATE_ERASE_FLASH;
                    } else {
                        SideQuest_State = STATE_JUMP_TO_APP;
                    }
                } else {break;}
            }
        } else {break;}

        break;
    }

    case STATE_STARTING_READ_FROM_STABLE:{
        firmware_address = STABLE_IMAGE_START;
        SideQuest_State = STATE_ERASE_FLASH;
        break;
    }

    case STATE_ERASE_FLASH:{
        pEraseInit->TypeErase = FLASH_TYPEERASE_PAGES;
        pEraseInit->Banks = FLASH_BANK_2;
        pEraseInit->Page = 0;
        pEraseInit->NbPages = 120;
        uint32_t * Error_Var;
        if (HAL_FLASHEx_Erase(pEraseInit, Error_Var)) {
            HAL_FLASH_Unlock(); 
            uint32_t copy_firmware_addr = firmware_address;
            pSuccess_read_marker = (uint32_t *)copy_firmware_addr;
            pSuccess_write_marker = (uint32_t *)FLASH2_START;

            SideQuest_State = STATE_MOVING_BLOCK;
        } else {
            SideQuest_State = STATE_ERROR;
        }
        break;
    }

    case STATE_READ_BLOCK:{
        if (pSuccess_read_marker[0] == EOF_MARKER) {  
            SideQuest_State = STATE_FINAL_CHECKSUM;
        } else {
            SideQuest_State = STATE_MOVING_BLOCK;
        }
        break;
    }

    case STATE_MOVING_BLOCK: {
        bool success = false;
        int retries = 0;
        while (retries < MAX_TRIES && !success) {
            //make a copy of pfirmware_success_marker and use it to get the next 64 bits of data
            uint32_t * copy_psuccess_marker = pSuccess_read_marker;
            uint64_t data64 = ((uint64_t)copy_psuccess_marker[1] << 32) | copy_psuccess_marker[0];
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, (uint32_t)pSuccess_write_marker, data64) == HAL_OK) {
                uint32_t crc1 = HAL_CRC_Accumulate(&hcrc, pSuccess_read_marker, (BLOCKSIZE / 8) / (sizeof(uint32_t)));
                uint32_t crc2 = HAL_CRC_Accumulate(&hcrc, pSuccess_write_marker, (BLOCKSIZE / 8) / (sizeof(uint32_t)));
                #ifdef DEBUG_MODE
                    crc1 = HAL_CRC_Calculate(&hcrc, pSuccess_read_marker, (BLOCKSIZE / 8) / (sizeof(uint32_t)));
                    crc2 = HAL_CRC_Calculate(&hcrc, pSuccess_write_marker, (BLOCKSIZE / 8) / (sizeof(uint32_t)));
                #endif
                if (crc1 == crc2) {
                    // if crc check passes increment pointers in each flash
                    pSuccess_read_marker += (BLOCKSIZE / (sizeof(uint32_t)));
                    pSuccess_write_marker += (BLOCKSIZE / (sizeof(uint32_t)));
                    success = true;
                }
            }
            retries++;
        }

        if (success) {
            SideQuest_State = STATE_READ_BLOCK;
        } else {
            SideQuest_State = STATE_ERROR;
        }
        break;
    }

    case STATE_FINAL_CHECKSUM: {
        HAL_FLASH_Lock();  // Done writing
        uint32_t crc1 = HAL_CRC_Calculate(&hcrc, pSuccess_read_marker, (BLOCKSIZE / 8) / (sizeof(uint32_t)));
        uint32_t crc2 = HAL_CRC_Calculate(&hcrc, pSuccess_write_marker, (BLOCKSIZE / 8) / (sizeof(uint32_t)));
        if (crc1 == crc2) {
            uint8_t good_to_go = GOOD_TO_GO;
            EEPROM_Write_Flag(&good_to_go, PAGE_UPDATE_FLAG, &SideQuest->Boot_I2C_Handle);
        } else {
            SideQuest_State = STATE_ERROR;
        }
        break;
    }

    case STATE_JUMP_TO_APP: {
        jump_to_app(FLASH2_START);
        break;
    }

    case STATE_ERROR: {
        if (firmware_address == STABLE_IMAGE_START) {
        #ifdef DEBUG_MODE:
            while (1) { // in deploy, make it jump to app instead;
                printf("FLASH FAILED\n");
                HAL_Delay(5000);
                }
        #endif
            jump_to_app(FLASH2_START);
        } else {
        firmware_address = STABLE_IMAGE_START;
        SideQuest_State = STATE_ERASE_FLASH;
        }
    break;
    }
}}




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

void Bootloader_init(I2C_HandleTypeDef i2c_handle, UART_HandleTypeDef UART_Handle){
    SideQuest = (tBootloader *)malloc(sizeof(tBootloader));
    SideQuest->Boot_I2C_Handle = i2c_handle;
    SideQuest->Boot_UART_Handle = UART_Handle;
    UART_SetHandle(&SideQuest->Boot_UART_Handle);
    SideQuest_State = STATE_INIT;
}

void jump_to_app(uint32_t address_start){
    if (Valid_FW_Check()){
        HAL_I2C_DeInit(&SideQuest->Boot_I2C_Handle);
        HAL_UART_DeInit(&SideQuest->Boot_UART_Handle);
        HAL_CRC_DeInit(&hcrc);
        HAL_RCC_DeInit();
        HAL_DeInit();
        SysTick->CTRL = 0;
        SysTick->LOAD = 0;
        SysTick->VAL  = 0;
        uint32_t appStack = *(volatile uint32_t*)address_start;
        uint32_t appResetHandler = *(volatile uint32_t*)(address_start + 4);

        __set_MSP(appStack);

        // Jump to application
        pFunction appEntry = (pFunction)appResetHandler;
        appEntry();
    } else {
        printf("BOOT Failed - invalid App FW");
        SideQuest_State = STATE_ERROR;
        return;
    }

}

bool Valid_FW_Check(){
    return (((*(uint32_t*)FLASH2_START) - SRAM1_BASE) <= SRAM1_SIZE_MAX);
}
