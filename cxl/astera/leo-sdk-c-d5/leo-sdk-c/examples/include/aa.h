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
 * @file aa.h
 * @brief Aardvark related implementation of i2c
 */
#ifndef _AA_H
#define _AA_H

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>

#include "../../include/astera_log.h"
#include "../../include/leo_error.h"
#include "../../include/leo_i2c.h"
#include "../aardvark/aardvark.h"
#include "../aardvark/aardvark_setup.h"
#include "../include/libi2c.h"

void setSlaveAddress(uint8_t address);
void setHandle(Aardvark handle);
void signalHandler(int signum);
void enableSignalHandler(int enable);
int setGpioSpiMux(Aardvark handle, int outputVal);

/**
 *  * @brief Low-level I2C method to implement a lock around I2C transactions such
 *  * that a set of transactions can be atomic.
 *  *
 *  * @warning THIS FUNCTION MUST BE IMPLEMENTED IN THE USER'S APPLICATION.
 *  *
 *  * @param[in]  handle Handle to I2C driver
 *  * @return     int - Error code
 *  */
int asteraI2CBlock(
        int handle);

/**
 *  * @brief Low-level I2C method to unlock a previous lock around I2C
 *  * transactions such that a set of transactions can be atomic.
 *  *
 *  * @warning THIS FUNCTION MUST BE IMPLEMENTED IN THE USER'S APPLICATION.
 *  *
 *  * @param[in]  handle Handle to I2C driver
 *  * @return     int - Error code
 *  */
int asteraI2CUnblock(
        int handle);


#endif //_AA_H
