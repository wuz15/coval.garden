/*********************************************************************
 *
 *  (c) 2022-      Astera Labs, Inc.
 *
 *  2022-May-04: CHL:
 *
 *  Generic defines for i2c (similar to libpmbus.c)
 *
 *  These are specific to the Aardvark API targeting the Aardvark device.
 *
 *  Using the _ext commands so we have more visibility on return
 *  codes.
 *
 *  Revision History
 *  2022-May-04: CHL: Initial version
 *
 ********************************************************************/

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
 * @file libi2c.h
 * @brief I2C functions used to access Aardvark
 * @details Generic defines for i2c access (similar to libpmbus.c) and are
 * specific to the Aardvark I2C APIs.
 */

#ifndef _LIBI2C_H_
#define _LIBI2C_H_

#include "../aardvark/aardvark.h"
#include <stdio.h>

/*********************************************************************
 *  Function prototype exports
 ********************************************************************/
extern int i2c_write_byte(Aardvark handle, u08 device, u08 addr, u08 data);

extern int i2c_write_word(Aardvark handle, u08 device, u08 addr, u16 data);

extern int i2c_write_quad(Aardvark handle, u08 device, u08 addr, u32 data);

extern int i2c_write_block(Aardvark handle, u08 device, u08 addr, u08 num_bytes,
                           u08 *data);

extern int i2c_write_block_32(Aardvark handle, u08 device, u32 addr,
                              u08 num_bytes, u08 *data);

extern int i2c_read_byte(Aardvark handle, u08 device, u08 addr, u08 *data);

extern int i2c_read_word(Aardvark handle, u08 device, u08 addr, u16 *data);

extern int i2c_read_byte_no_addr(Aardvark handle, u08 device, u08 *data);

extern int i2c_read_word_no_addr(Aardvark handle, u08 device, u16 *data);

extern int i2c_read_quad(Aardvark handle, u08 device, u08 addr, u32 *data);

extern int i2c_read_block(Aardvark handle, u08 device, u08 addr, u08 num_bytes,
                          u08 *data);

extern int i2c_read_block_32(Aardvark handle, u08 device, u32 addr,
                             u08 num_bytes, u08 *data);

extern int i2c_write_read(Aardvark handle, u08 device, u08 *w_buf, u08 w_len,
                          u08 *r_buf, u08 r_len);

#endif // _LIBI2C_H_
