/*
 * Copyright 2022 Astera Labs, Inc. or its affiliates. All Rights Reserved.
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

/**
 * @file libi2c.c
 * @brief I2C low level implementation
 */

/*********************************************************************
 *
 *  (c) 2022-      Astera Labs, Inc.
 *
 *  2022-May-04: CHL:
 *
 *  Generic defines for i2c (similar to libpmbus.c)
 *
 *  These are specific to the Aardvark API.  I should probably add a
 *  generic layer to decouple the underlying I2C methods so it can
 *  work under any i2c driver.
 *
 *  Using the _ext commands so we have more visibility on return
 *  codes.
 *
 *  Revision History
 *  2022-May-04: CHL: Initial version
 *
 ********************************************************************/

#include "../include/libi2c.h"
#include "../../include/astera_log.h"
#include <stdint.h>

#define MAX_READ_SIZE 16

/*********************************************************************
 *  Creating a specific {read,write}_{byte,word}, since those are
 *  the most widely used.  Yes, these are duplicates, but I don't
 *  want to incur a malloc/free penalty for the 4 functions that
 *  will be used the most.
 *
 *  Using the _ext commands to get the return codes from the calls.
 ********************************************************************/

/*********************************************************************
 *  Generic I2C write-byte command
 *  -S- {devad,0} -A- cmd[8] -A- data[7:0] -A- -P-
 ********************************************************************/
int i2c_write_byte(Aardvark handle, u08 device, u08 addr, u08 data) {
  int rc;
  u16 count;
  u08 data_out[2];
  data_out[0] = addr;
  data_out[1] = data;

  rc = aa_i2c_write_ext(handle, device, AA_I2C_NO_FLAGS, 2, data_out, &count);
  if (0 != rc) {
    ASTERA_ERROR("error: %s\n", aa_status_string(rc));
  } else if (2 != count) {
    rc = 1;
    ASTERA_WARN("WARNING: Wrote %d, expected %d\n", count, 2);
  }

  aa_sleep_ms(10);
  return (rc);
} // int i2c_write_byte()

/*********************************************************************
 *  Generic I2C write-word command
 *  -S- {devad,0} -A- cmd[8] -A- data[7:0] -A- data[15:8] -A- -P-
 ********************************************************************/
int i2c_write_word(Aardvark handle, u08 device, u08 addr, u16 data) {
  int rc;
  u16 count;
  u08 data_out[3];
  data_out[0] = addr;
  data_out[1] = ((data & 0x00ff));
  data_out[2] = ((data & 0xff00) >> 8);

  rc = aa_i2c_write_ext(handle, device, AA_I2C_NO_FLAGS, 3, data_out, &count);
  if (0 != rc) {
    ASTERA_ERROR("error: %s\n", aa_status_string(rc));
  } else if (3 != count) {
    rc = 1;
    ASTERA_WARN("WARNING: Wrote %d, expected %d\n", count, 3);
  }

  aa_sleep_ms(10);
  return (rc);
} // int i2c_write_word()

int i2c_write_block(Aardvark handle, u08 device, u08 addr, u08 num_bytes,
                    u08 *data) {
  int rc;
  u16 count;
  u08 data_out[num_bytes + 2];
  int i;
  data_out[0] = addr;
  for (i = 0; i < num_bytes; i++) {
    data_out[i + 2] = data[i];
  }
  rc = aa_i2c_write_ext(handle, device, AA_I2C_NO_FLAGS, num_bytes + 1,
                        data_out, &count);
  if (0 != rc) {
    ASTERA_ERROR("error (%d): %s\n", rc, aa_status_string(rc));
  } else if (num_bytes + 1 != count) {
    rc = 1;
    ASTERA_WARN("WARNING: Write %d, expected %d\n", count, num_bytes + 1);
  }
  aa_sleep_ms(10);
  return rc;
} // int i2c_write_block()

int i2c_write_block_32(Aardvark handle, u08 device, u32 addr, u08 num_bytes,
                       u08 *data) {
  int rc;
  int reg_addr_num_bytes = 3;
  u16 count;
  u08 data_out[num_bytes + 6];
  int i;
  data_out[0] = 0x4f;
  data_out[1] = 0x8;
  /* data_out[1] = 1 + reg_addr_num_bytes + num_bytes; */
  data_out[2] = 0x4e;
  /* data_out[2] = ((reg_addr_num_bytes + num_bytes) << 1) | 0x40; */
  data_out[3] = (addr >> 16) & 0xff;
  data_out[4] = (addr >> 8) & 0xff;
  data_out[5] = (addr)&0xff;

  for (i = 0; i < num_bytes; i++) {
    data_out[i + 6] = data[num_bytes - i - 1];
  }
  rc = aa_i2c_write_ext(handle, device, AA_I2C_NO_FLAGS, num_bytes + 6,
                        data_out, &count);
  if (0 != rc) {
    ASTERA_ERROR("error (%d): %s\n", rc, aa_status_string(rc));
  } else if (num_bytes + 6 != count) {
    rc = 1;
    ASTERA_WARN("WARNING: Write %d, expected %d\n", count, num_bytes + 6);
  }
  /* aa_sleep_ms(10); */
  return rc;
} // int i2c_write_block_32()

/*********************************************************************
 *  Generic I2C write-quad command
 *  -S- {devad,0} -A- cmd[8] -A- d0 d1 d2 d3 -A- -P-
 ********************************************************************/
int i2c_write_quad(Aardvark handle, u08 device, u08 addr, u32 data) {
  int rc;
  u16 count;
  u08 data_out[5];
  data_out[0] = addr;
  data_out[1] = ((data & 0x000000ff));
  data_out[2] = ((data & 0x0000ff00) >> 8);
  data_out[3] = ((data & 0x00ff0000) >> 16);
  data_out[4] = ((data & 0xff000000) >> 24);

  rc = aa_i2c_write_ext(handle, device, AA_I2C_NO_FLAGS, 5, data_out, &count);
  if (0 != rc) {
    ASTERA_ERROR("error: %s\n", aa_status_string(rc));
  } else if (5 != count) {
    rc = 1;
    ASTERA_WARN("WARNING: Wrote %d, expected %d\n", count, 5);
  }

  aa_sleep_ms(10);
  return (rc);
} // int i2c_write_quad()

/*********************************************************************
 *  Generic I2C read-byte command
 *  -S- {devad,0} -A- cmd[8] -A- -RS- {devad,1} -A- data[7:0] -A- -P-
 ********************************************************************/
int i2c_read_byte(Aardvark handle, u08 device, u08 addr, u08 *data) {
  int rc = 0;
  u16 count;
  u08 rval = 0xff;
  u08 buffer[1];

  buffer[0] = addr;

  rc = aa_i2c_write_ext(handle, device, AA_I2C_NO_STOP, 1, buffer, &count);
  if (0 != rc) {
    ASTERA_ERROR("error: %s\n", aa_status_string(rc));
    return (rc);
  }

  rc = aa_i2c_read_ext(handle, device, AA_I2C_NO_FLAGS, 1, buffer, &count);

  if (0 != rc) {
    ASTERA_ERROR("error: %s\n", aa_status_string(rc));
  } else if (count == 0) {
    rc = 1;
    ASTERA_ERROR("error: no bytes read\n");
    ASTERA_ERROR("  are you sure you have the right slave address?\n");
  } else if (count != 1) {
    rc = 1;
    ASTERA_ERROR("error: read %d bytes (expected %d)\n", count, 1);
  } else {
    rval = buffer[0];
  }
  *data = rval;
  return (rc);
} // int i2c_read_byte()

/*********************************************************************
 *  Generic I2C read-word command
 *  -S- {devad,0} -A- cmd[8] -A- -RS- {devad,1} -A- data[7:0] -A-
 *                                                  data[15:8] -A- -P-
 ********************************************************************/
int i2c_read_word(Aardvark handle, u08 device, u08 addr, u16 *data) {
  int rc = 0;
  u16 count;
  u16 rval = 0xffff;
  u08 buffer[2];

  buffer[0] = addr;

  rc = aa_i2c_write_ext(handle, device, AA_I2C_NO_STOP, 1, buffer, &count);
  if (0 != rc) {
    ASTERA_ERROR("error: %s\n", aa_status_string(rc));
    return (rc);
  }
  rc = aa_i2c_read_ext(handle, device, AA_I2C_NO_FLAGS, 2, buffer, &count);

  if (0 != rc) {
    ASTERA_ERROR("error: %s\n", aa_status_string(rc));
  } else if (count == 0) {
    rc = 1;
    ASTERA_ERROR("error: no bytes read\n");
    ASTERA_ERROR("  are you sure you have the right slave address?\n");
  } else if (count != 2) {
    rc = 1;
    ASTERA_ERROR("error: read %d bytes (expected %d)\n", count, 2);
  } else {
    rval = (buffer[1] << 8) | (buffer[0]);
  }
  *data = rval;
  return (rc);
} // int i2c_read_word()

/*********************************************************************
 *  -S- {devad,1} -A- data[7:0] -N- -P-
 ********************************************************************/
int i2c_read_byte_no_addr(Aardvark handle, u08 device, u08 *data) {
  int rc = 0;
  u16 count;
  u08 rval = 0xff;
  u08 buffer[1];

  rc = aa_i2c_read_ext(handle, device, AA_I2C_NO_FLAGS, 1, buffer, &count);

  if (0 != rc) {
    ASTERA_ERROR("error: %s\n", aa_status_string(rc));
  } else if (count == 0) {
    rc = 1;
    ASTERA_ERROR("error: no bytes read\n");
    ASTERA_ERROR("  are you sure you have the right slave address?\n");
  } else if (count != 1) {
    rc = 1;
    ASTERA_ERROR("error: read %d bytes (expected %d)\n", count, 1);
  } else {
    rval = buffer[0];
  }
  *data = rval;
  return (rc);
} // int i2c_read_byte_no_addr()

/*********************************************************************
 *  -S- {devad,1} -A- data[7:0] -A- data[15:8] -N- -P-
 ********************************************************************/
int i2c_read_word_no_addr(Aardvark handle, u08 device, u16 *data) {
  int rc = 0;
  u16 count;
  u16 rval = 0xffff;
  u08 buffer[2];

  rc = aa_i2c_read_ext(handle, device, AA_I2C_NO_FLAGS, 2, buffer, &count);

  if (0 != rc) {
    ASTERA_ERROR("error: %s\n", aa_status_string(rc));
  } else if (count == 0) {
    rc = 1;
    ASTERA_ERROR("error: no bytes read\n");
    ASTERA_ERROR("  are you sure you have the right slave address?\n");
  } else if (count != 2) {
    rc = 1;
    ASTERA_ERROR("error: read %d bytes (expected %d)\n", count, 2);
  } else {
    rval = (buffer[1] << 8) | (buffer[0]);
  }
  *data = rval;
  return (rc);
} // int i2c_read_word_no_addr()

/*********************************************************************
 *  Generic I2C read-word command
 *  -S- {devad,0} -A- cmd[8] -A- -RS- {devad,1} -A- d0 d1 d2 d3 -A- -P-
 ********************************************************************/
int i2c_read_quad(Aardvark handle, u08 device, u08 addr, u32 *data) {
  int rc = 0;
  u16 count;
  u32 rval = 0xffffffff;
  u08 buffer[4];

  buffer[0] = addr;

  rc = aa_i2c_write_ext(handle, device, AA_I2C_NO_STOP, 1, buffer, &count);
  if (0 != rc) {
    ASTERA_ERROR("error: %s\n", aa_status_string(rc));
    return (rc);
  }
  rc = aa_i2c_read_ext(handle, device, AA_I2C_NO_FLAGS, 4, buffer, &count);

  if (0 != rc) {
    ASTERA_ERROR("error: %s\n", aa_status_string(rc));
  } else if (count == 0) {
    rc = 1;
    ASTERA_ERROR("error: no bytes read\n");
    ASTERA_ERROR("  are you sure you have the right slave address?\n");
  } else if (count != 4) {
    rc = 1;
    ASTERA_ERROR("error: read %d bytes (expected %d)\n", count, 4);
  } else {
    rval =
        (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | (buffer[0]);
  }
  *data = rval;
  return (rc);
} // int i2c_read_quad()

int i2c_read_block(Aardvark handle, u08 device, u08 addr, u08 num_bytes,
                   u08 *data) {
  int rc = 0;
  u16 count;
  u08 rval[MAX_READ_SIZE];
  int ii;
  u08 buffer[4];

  if (num_bytes > MAX_READ_SIZE) {
    ASTERA_ERROR("error: exceeded max read size (%d)", MAX_READ_SIZE);
  }

  for (ii = 0; ii < MAX_READ_SIZE; ii++) {
    rval[ii] = 0xff;
  }

  buffer[0] = addr;
  rc = aa_i2c_write_ext(handle, device, AA_I2C_NO_STOP, 1, buffer, &count);
  if (0 != rc) {
    ASTERA_ERROR("error (%d): %s\n", rc, aa_status_string(rc));
    ASTERA_ERROR("addr[0x%02x%02x]\n", buffer[0], buffer[1]);
    return (rc);
  }

  rc =
      aa_i2c_read_ext(handle, device, AA_I2C_NO_FLAGS, num_bytes, rval, &count);

  if (0 != rc) {
    ASTERA_ERROR("error: %s\n", aa_status_string(rc));
  } else if (count == 0) {
    rc = 1;
    ASTERA_ERROR("error: no bytes read\n");
    ASTERA_ERROR("  are you sure you have the right slave address?\n");
  } else if (count != num_bytes) {
    rc = 1;
    ASTERA_ERROR("error: read %d bytes (expected %d)\n", count, num_bytes);
  } else {
    for (ii = 0; ii < count; ii++) {
      data[ii] = rval[ii];
    }
  }
  return 0;
} // int i2c_read_block()

int i2c_read_block_32(Aardvark handle, u08 device, u32 addr, u08 num_bytes,
                      u08 *data) {
  int rc = 0;
  u16 count;
  u08 rval[MAX_READ_SIZE];
  int ii;
  u08 buffer[6];

  if (num_bytes > MAX_READ_SIZE) {
    ASTERA_ERROR("error: exceeded max read size (%d)", MAX_READ_SIZE);
  }

  for (ii = 0; ii < MAX_READ_SIZE; ii++) {
    rval[ii] = 0xff;
  }

  buffer[0] = 0x4A;
  buffer[1] = 0x4;
  buffer[2] = 0x6;
  buffer[3] = (addr >> 16) & 0xff;
  buffer[4] = (addr >> 8) & 0xff;
  buffer[5] = (addr)&0xff;

  rc = aa_i2c_write_ext(handle, device, AA_I2C_NO_FLAGS, 6, buffer, &count);
  if (0 != rc) {
    ASTERA_ERROR("error (%d): %s\n", rc, aa_status_string(rc));
    return (rc);
  }

  buffer[0] = 0x9;
  rc = aa_i2c_write_ext(handle, device, AA_I2C_NO_STOP, 1, buffer, &count);
  if (0 != rc) {
    ASTERA_ERROR("error (%d): %s\n", rc, aa_status_string(rc));
    return (rc);
  }

  rc = aa_i2c_read_ext(handle, device, AA_I2C_NO_FLAGS, num_bytes + 1, rval,
                       &count);
  if (0 != rc) {
    ASTERA_ERROR("error (%d): %s\n", rc, aa_status_string(rc));
    return (rc);
  } else if (count == 0) {
    rc = 1;
    ASTERA_ERROR("error: no bytes read\n");
    ASTERA_ERROR("  are you sure you have the right slave address?\n");
  } else if (count != num_bytes + 1) {
    rc = 1;
    ASTERA_ERROR("error: read %d bytes (expected %d)\n", count, num_bytes);
  } else {
    for (ii = 0; ii < count - 1; ii++) {
      data[ii] = rval[ii + 1];
    }
  }
  return 0;
} // int i2c_read_block_32()
