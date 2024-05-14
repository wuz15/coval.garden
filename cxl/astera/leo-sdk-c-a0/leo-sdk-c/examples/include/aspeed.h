/*
 * Copyright 2020 Astera Labs, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may not
 * use this file except in compliance with the License. You may obtain a copy
 * of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/*
 * @file aspeed.h
 * @brief Implementation of helper functions used by A-Speed
 */

#ifndef ASTERA_ARIES_SDK_ASPEED_H_
#define ASTERA_ARIES_SDK_ASPEED_H_

#include <errno.h>
#include <fcntl.h>
#include <linux/i2c-dev.h> // linux library included on A-Speed
#include <linux/i2c.h>
#include <linux/types.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

/* Returns the number of read bytes */
extern __s32 i2c_smbus_read_i2c_block_data(int file, __u8 command, __u8 length,
                                           __u8 *values);
extern __s32 i2c_smbus_write_i2c_block_data(int file, __u8 command, __u8 length,
                                            const __u8 *values);

/**
 * @brief: Set I2C slave address
 *
 * @param[in] file      I2C handle
 * @param[in] address   Slave address
 * @param[in] force     Override user provied slave address with default
                         I2C_SLAVE address
 * @return int           Zero if success, else a negative value
 */
int setSlaveAddress(int file, int address, int force);

#endif /* ASTERA_ARIES_SDK_ASPEED_H_ */
