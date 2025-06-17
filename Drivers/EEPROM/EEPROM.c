/**
 * @Jason P
 * 6/16/2026
 * EEPROM.h for SideQuestBootloader v0.00
 * 
 */

#include "EEPROM.h"


/**
 * @brief Write a single byte to EEPROM
 * @params: memAddress: I2C memory address
 * @params: data: data buffer (1 byte) to copy byte from
 * @return: Status of Operation
 */
bool EEPROM_WriteByte(uint16_t memAddress, uint8_t data) {
    uint8_t buffer[3];
    buffer[0] = (uint8_t)(memAddress >> 8); // High byte
    buffer[1] = (uint8_t)(memAddress & 0xFF); // Low byte
    buffer[2] = data;

    return HAL_I2C_Master_Transmit(&hi2c1, M24_I2C_ADDR, buffer, 3, EEPROM_TIMEOUT) == HAL_OK;
}

/**
 * @brief Write a single byte to EEPROM
 * @params: memAddress: I2C memory address
 * @params: data: data buffer (1 byte) to copy byte to
 * @return: Status of Operation
 */
bool EEPROM_ReadByte(uint16_t memAddress, uint8_t *data) {
    uint8_t addr[2];
    addr[0] = (uint8_t)(memAddress >> 8);
    addr[1] = (uint8_t)(memAddress & 0xFF);

    if (HAL_I2C_Master_Transmit(&hi2c1, M24_I2C_ADDR, addr, 2, EEPROM_TIMEOUT) != HAL_OK)
        return false;

    return HAL_I2C_Master_Receive(&hi2c1, M24_I2C_ADDR, data, 1, EEPROM_TIMEOUT) == HAL_OK;
}

/**
 * @brief Write up to 64 bytes to a single EEPROM page
 * @params: memAddress: I2C memory address
 * @params: data: data buffer (1 byte) to copy byte from
 * @params: len: length of the data in the buffer
 * @return: Status of Operation
 */
bool EEPROM_WritePage(uint16_t memAddress, uint8_t *data, uint16_t len) {
    if (len > EEPROM_PAGE_SIZE || ((memAddress % EEPROM_PAGE_SIZE) + len) > EEPROM_PAGE_SIZE)
        return false; // prevent crossing page boundary

    uint8_t buffer[EEPROM_PAGE_SIZE + 2];
    buffer[0] = memAddress >> 8;
    buffer[1] = memAddress & 0xFF;
    memcpy(&buffer[2], data, len);

    return HAL_I2C_Master_Transmit(&hi2c1, M24_I2C_ADDR, buffer, len + 2, EEPROM_TIMEOUT) == HAL_OK;
}

/**
 * @brief Read arbitrary number of bytes
 * @params: memAddress: I2C memory address
 * @params: data: data buffer (1 byte) to copy byte from
 * @params: len: length of the data in the buffer
 * @return: Status of Operation
 */
bool EEPROM_Read(uint16_t memAddress, uint8_t *data, uint16_t len) {
    uint8_t addr[2];
    addr[0] = (uint8_t)(memAddress >> 8);
    addr[1] = (uint8_t)(memAddress & 0xFF);

    if (HAL_I2C_Master_Transmit(&hi2c1, M24_I2C_ADDR, addr, 2, EEPROM_TIMEOUT) != HAL_OK)
        return false;

    return HAL_I2C_Master_Receive(&hi2c1, M24_I2C_ADDR, data, len, EEPROM_TIMEOUT) == HAL_OK;
}

/**
 * @brief Polls EEPROM for ACK until it's ready or timeout occurs.
 * @return: if EEPROM is ready or not
 */
bool EEPROM_WaitReady(void) {
    uint32_t tickstart = HAL_GetTick();
    while (HAL_I2C_IsDeviceReady(&hi2c1, M24_I2C_ADDR, 1, EEPROM_TIMEOUT) != HAL_OK) {
        if ((HAL_GetTick() - tickstart) > 5)  // typical tWR is ~5ms
            return false;
    }
    return true;
}


uint64_t EEPROM_ReadUint64(uint16_t address) {
    uint64_t value = 0;
    uint8_t byte;
    for (int i = 0; i < 8; ++i) {
        if (!EEPROM_ReadByte(address + i, &byte)) return 0;
        value = (value << 8) | byte;
    }
    return value;
}

// Helper: Write uint64_t to EEPROM (big endian)
bool EEPROM_WriteUint64(uint16_t address, uint64_t value) {
    for (int i = 7; i >= 0; --i) {
        if (EEPROM_WriteByte(address + (7 - i), (uint8_t)(value >> (i * 8))) != true)
            return false;
    }
    return true;
}

bool EEPROM_Write_Flag(uint8_t *data, uint8_t page_start) {
    uint64_t highest_page_count = 0;
    int current_page_index = 0;
    // Step 1: Determine current page based on page write count
    for (int i = 0; i < 3; ++i) {
        uint16_t base_addr = page_start + (i * PAGE_SIZE);
        uint64_t page_count = EEPROM_ReadUint64(base_addr + 8);
        if (page_count > highest_page_count) {
            highest_page_count = page_count;
            current_page_index = i;
        }
    }
    uint16_t current_page_addr = page_start + (current_page_index * PAGE_SIZE);
    // Step 2: Read current page write count
    uint64_t page_count = EEPROM_ReadUint64(current_page_addr + 8);
    // Step 3: Switch page if needed
    if (page_count >= MAX_WRITES_PER_PAGE) {
        current_page_index = (current_page_index + 1) % 3;
        current_page_addr = page_start + (current_page_index * PAGE_SIZE);
        page_count = 0;
        // Reset page count
        if (!EEPROM_WriteUint64(current_page_addr + 8, 0)) return false;
    }
    // Step 4: Read total count
    uint64_t total_count = EEPROM_ReadUint64(current_page_addr);
    // Step 5: Write updated total + page counts
    if (!EEPROM_WriteUint64(current_page_addr, total_count + 1)) return false;
    if (!EEPROM_WriteUint64(current_page_addr + 8, page_count + 1)) return false;
    // Step 6: Write flag data (17th byte, index 16)
    if (EEPROM_WriteByte(current_page_addr + 16, data[16]) != true) return false;
    return true;
}

bool EEPROM_Read_Flag(uint8_t * flag, uint8_t page_start) {
    uint64_t highest_page_count = 0;
    int active_page_index = 0;
    // Step 1: Determine the page with the highest write count
    for (int i = 0; i < 3; ++i) {
        uint16_t base_addr = page_start + (i * 64);
        uint64_t page_count = EEPROM_ReadUint64(base_addr + 8);
        if (page_count > highest_page_count) {
            highest_page_count = page_count;
            active_page_index = i;
        }
    }
    // Step 2: Read the flag byte from byte 16 of the selected page
    uint16_t flag_address = page_start + (active_page_index * 64) + 16;
    return EEPROM_ReadByte(flag_address, flag);
}