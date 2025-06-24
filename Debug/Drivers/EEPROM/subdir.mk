################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/EEPROM/EEPROM.c 

OBJS += \
./Drivers/EEPROM/EEPROM.o 

C_DEPS += \
./Drivers/EEPROM/EEPROM.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/EEPROM/%.o Drivers/EEPROM/%.su Drivers/EEPROM/%.cyclo: ../Drivers/EEPROM/%.c Drivers/EEPROM/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L476xx -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-EEPROM

clean-Drivers-2f-EEPROM:
	-$(RM) ./Drivers/EEPROM/EEPROM.cyclo ./Drivers/EEPROM/EEPROM.d ./Drivers/EEPROM/EEPROM.o ./Drivers/EEPROM/EEPROM.su

.PHONY: clean-Drivers-2f-EEPROM

