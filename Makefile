######################################
# target
######################################
TARGET = Auto_Switch_Pro

######################################
# building variables
######################################
DEBUG = 1
OPT = -Og

#######################################
# paths
#######################################
BUILD_DIR = build

######################################
# source
######################################
# C sources
C_SOURCES =  \
Core/Src/main.c \
Core/Src/gpio.c \
Core/Src/usb.c \
Core/Src/stm32h5xx_it.c \
Core/Src/stm32h5xx_hal_msp.c \
Core/Src/system_stm32h5xx.c \
Core/Src/sysmem.c \
Core/Src/syscalls.c \
Core/Src/usb_descriptors.c \
Core/Src/usb_callbacks.c \
Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal.c \
Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_cortex.c \
Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_dma.c \
Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_dma_ex.c \
Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_exti.c \
Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_flash.c \
Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_flash_ex.c \
Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_gpio.c \
Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_pcd.c \
Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_pcd_ex.c \
Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_pwr.c \
Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_pwr_ex.c \
Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_rcc.c \
Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_rcc_ex.c \
Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_usb.c \
lib/tinyusb/src/tusb.c \
lib/tinyusb/src/common/tusb_fifo.c \
lib/tinyusb/src/device/usbd.c \
lib/tinyusb/src/device/usbd_control.c \
lib/tinyusb/src/class/hid/hid_device.c \
lib/tinyusb/src/portable/st/stm32_fsdev/dcd_stm32_fsdev.c

# ASM sources
ASM_SOURCES =  \
Core/Startup/startup_stm32h562rgtx.s

#######################################
# binaries
#######################################
PREFIX = arm-none-eabi-
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

#######################################
# CFLAGS
#######################################
# cpu
CPU = -mcpu=cortex-m33

# fpu
FPU = -mfpu=fpv5-sp-d16

# float-abi
FLOAT-ABI = -mfloat-abi=hard

# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# macros for gcc
AS_DEFS =

# C defines
C_DEFS =  \
-DUSE_HAL_DRIVER \
-DSTM32H562xx \
-DCFG_TUSB_MCU=OPT_MCU_STM32H5

# AS includes
AS_INCLUDES =

# C includes
C_INCLUDES =  \
-ICore/Inc \
-IDrivers/STM32H5xx_HAL_Driver/Inc \
-IDrivers/STM32H5xx_HAL_Driver/Inc/Legacy \
-IDrivers/CMSIS/Device/ST/STM32H5xx/Include \
-IDrivers/CMSIS/Include \
-Ilib/tinyusb/src

# compile gcc flags
ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

CFLAGS += $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif

# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

#######################################
# LDFLAGS
#######################################
# link script
LDSCRIPT = STM32H562RGTX_FLASH.ld

# libraries
LIBS = -lc -lm -lnosys
LIBDIR =
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

$(BUILD_DIR):
	mkdir $@

#######################################
# clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR)

#######################################
# flash
#######################################
flash: $(BUILD_DIR)/$(TARGET).bin
	st-flash write $< 0x08000000

#######################################
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)
