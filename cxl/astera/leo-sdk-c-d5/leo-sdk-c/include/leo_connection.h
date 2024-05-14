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
 * @file leo_connection.h
 * @brief Definition of CONNECTION types for the SDK.
 */

#ifndef ASTERA_LEO_SDK_CONNECTION_H_
#define ASTERA_LEO_SDK_CONNECTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PATH 4096
#define SERIAL_STR_SIZE 32
#define SERIAL_VALID_SIZE 11 // expected format xxxx-xxxxxx

/*
 * Structure to store any connectivity details related to the target.
 * Using this structure as a function argument may help to scale
 * the function in the future to include details like inband/oob,
 * serial number of CONNECTION etc all referenced with a struct pointer.
 */
typedef struct conn_s {
  char serialnum[SERIAL_STR_SIZE];
  char bdf[MAX_PATH];
} conn_t;

#ifdef __cplusplus
}
#endif

#endif /* ASTERA_LEO_SDK_CONNECTION_H_ */
