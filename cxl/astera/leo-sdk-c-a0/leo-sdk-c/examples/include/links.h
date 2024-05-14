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
 * @file links.h
 * @brief Helper functions used by link status examples
 */

#ifndef _LINKS_H_
#define _LINKS_H_

#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../../include/leo_error.h"

// Function to print a log entry to file
void writeLogToFile(FILE *fp, LeoLTSSMEntryType *entry);

// Function to iterate over logger and print entry
LeoErrorType leoPrintMicroLogs(LeoLinkType *link);

// Capture the detailed link state and print it to file
LeoErrorType leoPrintLinkDetailedState(LeoLinkType *link);

// Print the micro logger entries
LeoErrorType leoPrintLog(LeoLinkType *link, LeoLTSSMLoggerEnumType log,
                         FILE *fp);

#endif // _LINKS_H_
