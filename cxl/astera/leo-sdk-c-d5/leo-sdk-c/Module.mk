# Copyright 2022 Astera Labs, Inc. or its affiliates. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License"). You may not
# use this file except in compliance with the License. You may obtain a copy
# of the License at:
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# or in the "license" file accompanying this file. This file is distributed
# on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
# express or implied. See the License for the specific language governing
# permissions and limitations under the License.

OS=$(shell if [ -x /bin/uname ]; then /bin/uname; fi)
-include .env

ifdef TARGET_PI
CC := gcc

else ifeq ($(OS),Linux)
CC := gcc -m64
ifeq (,$(wildcard /usr/include/linux/cxl_mem.h))
LEO_CXL_MAILBOX_TEST = 
else
LEO_CXL_MAILBOX_TEST = leo_cxl_mailbox_test
endif
endif

#INTERNAL_TEST = leo_internal

#SYSLIBS=-ldl -li2c
SYSLIBS=-ldl

LEO_DIR               := leo-sdk-c
LEO_SRC               := $(LEO_DIR)/source
LEO_EXAMPLES          := $(LEO_DIR)/examples
LEO_EXAMPLES_AARDVARK := $(LEO_EXAMPLES)/aardvark
LEO_EXAMPLES_SRC      := $(LEO_DIR)/examples/source

# C Flags
# Set warnings and gdb
# add include directories
LEO_CFLAGS := -Wstrict-prototypes -Wpointer-arith -Wcast-qual \
		-Wcast-align -Wwrite-strings -Wnested-externs -Winline -W -Wundef \
		-Wmissing-prototypes \
		-I../include -I../include -I../include/aardvark \
		-I $(LEO_DIR)/include \
		-I $(LEO_DIR)/examples/aardvark \
		-I $(LEO_DIR)/include/aardvark \
		-I $(LEO_DIR)/examples/include \
		-I /include

CFLAGS=-I. -DES1 
ifdef TARGET_PI
CFLAGS += -DTARGET_PI
endif

#Compile for Leo-D5
CFLAGS += -DCHIP_D5


#CFLAGS += -DLEO_CSDK_DEBUG

# By default, create executables for these directories
# LEO_TARGETS := link_example spi_util_rpi aa_test aa_read_fw_version
ifdef TARGET_PI
LEO_TARGETS := leo_get_ddr_margins_example
else
#LEO_TARGETS	:= leo_fw_update_example link_example read_flash_jedec_example read_fru_example leo_read_spare_reg_example
LEO_TARGETS	:= leo_fw_update_example leo_api_test leo_memscrb_test leo_inject_err_test leo_tgc_test leo_read_fruprom_example leo_read_tsod_example leo_sample_cxl_bw  leo_event_records leo_poison_list leo_get_ddr_margins_example leo_read_eeprom_example leo_get_recent_uart_rx_example leo_telemetry $(LEO_CXL_MAILBOX_TEST) 
endif


# Libraries to include
#    -lpigpio pigpio.h library for RPI
#    -lrt pigpio.h library for RPI
#RPI_LDFLAGS := -lpigpio -lrt

# Libraries to include
#    -lm math.h library

LEO_OBJ_COMMON := $(LEO_EXAMPLES_AARDVARK)/aardvark.o \
	$(LEO_EXAMPLES_AARDVARK)/aardvark_setup.o \
	$(LEO_EXAMPLES_SRC)/aa.o \
	$(LEO_EXAMPLES_SRC)/libi2c.o \
	$(LEO_EXAMPLES_SRC)/board.o \
	$(LEO_EXAMPLES_SRC)/leo_common_global.o \
	$(LEO_SRC)/leo_common.o \
	$(LEO_SRC)/leo_pcie.o \
	$(LEO_SRC)/leo_interface.o \
	$(LEO_SRC)/leo_spi.o \
	$(LEO_SRC)/leo_scrb.o \
	$(LEO_SRC)/leo_api.o \
	$(LEO_SRC)/astera_log.o \
	$(LEO_SRC)/leo_mailbox.o \
	$(LEO_SRC)/leo_tgc.o \
	$(LEO_SRC)/leo_err_inject.o


################################
########### Programs ###########
################################

$(LEO_EXAMPLES)/leo_get_recent_uart_rx_example: $(LEO_EXAMPLES)/leo_get_recent_uart_rx_example.o \
	$(LEO_OBJ_COMMON)
	$(CC) -o $@ $^ $(LEO_CFLAGS) $(CFLAGS) $(SYSLIBS)

$(LEO_EXAMPLES)/leo_read_eeprom_example: $(LEO_EXAMPLES)/leo_read_eeprom_example.o \
	$(LEO_OBJ_COMMON)
	$(CC) -o $@ $^ $(LEO_CFLAGS) $(CFLAGS) $(SYSLIBS)

$(LEO_EXAMPLES)/read_fw_version_example: $(LEO_EXAMPLES)/read_fw_version_example.o \
	$(LEO_OBJ_COMMON)
	$(CC) -o $@ $^ $(LEO_CFLAGS) $(CFLAGS) $(SYSLIBS)

$(LEO_EXAMPLES)/leo_api_test: $(LEO_EXAMPLES)/leo_api_test.o \
	$(LEO_OBJ_COMMON)
	$(CC) $(LDFLAGS) $(LEO_LDFLAGS) -o $@ $^ $(SYSLIBS)

$(LEO_EXAMPLES)/leo_fw_update_example: $(LEO_EXAMPLES)/leo_fw_update_example.o \
	$(LEO_OBJ_COMMON)
	$(CC) $(LDFLAGS) $(LEO_LDFLAGS) -o $@ $^ $(SYSLIBS)

$(LEO_EXAMPLES)/leo_inject_err_test: $(LEO_EXAMPLES)/leo_inject_err_test.o \
	$(LEO_OBJ_COMMON)
	$(CC) $(LDFLAGS) $(LEO_LDFLAGS) -o $@ $^ $(SYSLIBS)

$(LEO_EXAMPLES)/$(LEO_CXL_MAILBOX_TEST): $(LEO_EXAMPLES)/$(LEO_CXL_MAILBOX_TEST).o \
	$(LEO_OBJ_COMMON)
	$(CC) $(LDFLAGS) $(LEO_LDFLAGS) -o $@ $^ $(SYSLIBS)

$(LEO_EXAMPLES)/leo_tgc_test: $(LEO_EXAMPLES)/leo_tgc_test.o \
	$(LEO_OBJ_COMMON)
	$(CC) $(LDFLAGS) $(LEO_LDFLAGS) -o $@ $^ $(SYSLIBS)

$(LEO_EXAMPLES)/link_example: $(LEO_EXAMPLES)/link_example.o \
	$(LEO_OBJ_COMMON)
	$(CC) $(LDFLAGS) $(LEO_LDFLAGS) -o $@ $^ $(SYSLIBS)

$(LEO_EXAMPLES)/read_flash_jedec_example: $(LEO_EXAMPLES)/read_flash_jedec_example.o \
	$(LEO_OBJ_COMMON)
	$(CC) $(LDFLAGS) $(LEO_LDFLAGS) -o $@ $^ $(SYSLIBS)

$(LEO_EXAMPLES)/leo_read_fruprom_example: $(LEO_EXAMPLES)/leo_read_fruprom_example.o \
	$(LEO_OBJ_COMMON)
	$(CC) -o $@ $^ $(LEO_CFLAGS) $(CFLAGS) $(SYSLIBS)

$(LEO_EXAMPLES)/leo_memscrb_test: $(LEO_EXAMPLES)/leo_memscrb_test.o \
	$(LEO_OBJ_COMMON)
	$(CC) $(LDFLAGS) $(LEO_LDFLAGS) -o $@ $^ $(SYSLIBS)	

$(LEO_EXAMPLES)/leo_read_tsod_example: $(LEO_EXAMPLES)/leo_read_tsod_example.o \
	$(LEO_OBJ_COMMON)
	$(CC) -o $@ $^ $(LEO_CFLAGS) $(CFLAGS) $(SYSLIBS)

$(LEO_EXAMPLES)/leo_sample_cxl_bw: $(LEO_EXAMPLES)/leo_sample_cxl_bw.o \
	$(LEO_OBJ_COMMON)
	$(CC) -o $@ $^ $(LEO_CFLAGS) $(CFLAGS) $(SYSLIBS)

$(LEO_EXAMPLES)/leo_telemetry: $(LEO_EXAMPLES)/leo_telemetry.o \
	$(LEO_OBJ_COMMON)
	$(CC) -o $@ $^ $(LEO_CFLAGS) $(CFLAGS) $(SYSLIBS)

$(LEO_EXAMPLES)/leo_event_records: $(LEO_EXAMPLES)/leo_event_records.o \
	$(LEO_OBJ_COMMON)
	$(CC) -o $@ $^ $(LEO_CFLAGS) $(CFLAGS) $(SYSLIBS)

$(LEO_EXAMPLES)/leo_poison_list: $(LEO_EXAMPLES)/leo_poison_list.o \
	$(LEO_OBJ_COMMON)
	$(CC) -o $@ $^ $(LEO_CFLAGS) $(CFLAGS) $(SYSLIBS)

$(LEO_EXAMPLES)/leo_get_ddr_margins_example: $(LEO_EXAMPLES)/leo_get_ddr_margins_example.o \
	$(LEO_OBJ_COMMON)
	$(CC) -o $@ $^ $(LEO_CFLAGS) $(CFLAGS) $(SYSLIBS)

###############################
########### Objects ###########
###############################

$(LEO_EXAMPLES)/read_flash_jedec_example.o: $(LEO_EXAMPLES)/read_flash_jedec_example.c
	$(CC) $(CFLAGS) $(LEO_CFLAGS) -c $< -o $@

$(LEO_SRC)/leo_pcie.o: $(LEO_SRC)/leo_pcie.c
	$(CC) $(CFLAGS) $(LEO_CFLAGS) -c $< -o $@

$(LEO_SRC)/leo_interface.o: $(LEO_SRC)/leo_interface.c
	$(CC) $(CFLAGS) $(LEO_CFLAGS) -c $< -o $@

$(LEO_SRC)/astera_log.o: $(LEO_SRC)/astera_log.c
	$(CC) $(CFLAGS) $(LEO_CFLAGS) -c $< -o $@

$(LEO_SRC)/leo_logger.o: $(LEO_SRC)/leo_logger.c
	$(CC) $(CFLAGS) $(LEO_CFLAGS) -c $< -o $@

$(LEO_SRC)/leo_mailbox.o: $(LEO_SRC)/leo_mailbox.c
	$(CC) $(CFLAGS) $(LEO_CFLAGS) -c $< -o $@

$(LEO_EXAMPLES_SRC)/board.o: $(LEO_EXAMPLES_SRC)/board.c
	$(CC) $(CFLAGS) $(LEO_CFLAGS) -c $< -o $@

$(LEO_EXAMPLES_SRC)/aa.o: $(LEO_EXAMPLES_SRC)/aa.c
	$(CC) $(CFLAGS) $(LEO_CFLAGS) -c $< -o $@

$(LEO_EXAMPLES_AARDVARK)/aardvark.o: $(LEO_EXAMPLES_AARDVARK)/aardvark.c
	$(CC) $(CFLAGS) $(LEO_CFLAGS) -c $< -o $@

$(LEO_EXAMPLES_AARDVARK)/aardvark_setup.o: $(LEO_EXAMPLES_AARDVARK)/aardvark_setup.c
	$(CC) $(CFLAGS) $(LEO_CFLAGS) -c $< -o $@

################################
########### Commands ###########
################################

all-tools: $(addprefix $(LEO_EXAMPLES)/,$(LEO_TARGETS))

strip-tools: $(addprefix $(LEO_DIR)/,$(LEO_TARGETS))
	strip $(addprefix $(LEO_DIR)/,$(LEO_TARGETS))

clean-tools:
	$(RM) $(addprefix $(LEO_DIR)/,*.o) \
		$(addprefix $(LEO_EXAMPLES)/,$(LEO_TARGETS)) \
		$(addprefix $(LEO_EXAMPLES)/,*.o) \
		$(addprefix $(LEO_EXAMPLES_SRC)/,*.o) \
		$(addprefix $(LEO_EXAMPLES_AARDVARK)/,*.o) \
		$(addprefix $(LEO_SRC)/,*.o)

install-tools: $(addprefix $(LEO_DIR)/,$(LEO_TARGETS))
	$(INSTALL_DIR) $(DESTDIR)$(sbindir) $(DESTDIR)$(man8dir)
	for program in $(LEO_TARGETS) ; do \
	$(INSTALL_PROGRAM) $(LEO_DIR)/$$program $(DESTDIR)$(sbindir) ; \
	$(INSTALL_DATA) $(LEO_DIR)/$$program.8 $(DESTDIR)$(man8dir) ; done

uninstall-tools:
	for program in $(LEO_TARGETS) ; do \
	$(RM) $(DESTDIR)$(sbindir)/$$program ; \
	$(RM) $(DESTDIR)$(man8dir)/$$program.8 ; done



all: all-tools

strip: strip-tools

clean: clean-tools

install: install-tools

uninstall: uninstall-tools
