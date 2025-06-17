#include <stdio.h>
#include "stm32l4xx_hal.h"
#include "../Drivers/UART/UART.c"
#include <string.h>
#include <stdbool.h>

uint32_t cursor;
uint32_t success_read_marker;
uint32_t firmware_address;


#define BLOCKSIZE           64U
#define MAX_TRIES           8
#define HEADER_SIZE         60

#define FLASH2_START         0x08080000

uint32_t *psuccess_read_marker;
uint32_t *pfirmware_address;

@define GOOD_TO_GO      1
#define UPDATE_NEEDED   0


#define CRASHED         1
#define NO_CRASH        0


#define UPDATE_IMAGE_START      ((uint32_t)&__update_img_start__) // fill in here
#define STABLE_IMAGE_START       ((uint32_t)&__backup_app_start__)// fill in here

#define PAGE_ID                      0
#define PAGE_CRASH_FLAG              1
#define UPDATE_NEEDED_FLAG           4  

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


void SideQuestBoot_delay_ms(uint32_t ms);
// for flags: flash 1 is ReadOnly, flash2 is WriteOnly
bool SideQuestBoot_read_flag(uint32_t flag_addr); // read the flags in flash 1 
bool SideQuestBoot_write_flag(uint32_t flag_addr, bool value); // to write to the flags in flash 2

void SideQuestBootloader(void) // need to fill in params

bool SideQuestBoot_write_block(uint32_t address, uint8_t * data, uint32_t size);
bool SideQuestBoot_erase_flash_partition(uint32_t base_addr, uint32_t size);
uint32_t SideQuestBoot_calculate_crc(uint32_t addr, uint32_t length);
uint32_t SideQuestBoot_accumulate_crc(uint32_t addr, uint32_t prev_accum_crc); //prev_crc is the crc of the previous accumulated blocks
void SideQuestBoot_init_crc();


