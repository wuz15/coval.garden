/*=========================================================================
| (c) 2003-2007  Total Phase, Inc.
|--------------------------------------------------------------------------
| Project : Aardvark Sample Code
| File    : aadetect.c

| Auto-detection test routine
| modified only to setup aardvark and close it
|--------------------------------------------------------------------------
| Redistribution and use of this file in source and binary forms, with
| or without modification, are permitted.
|
| THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
| "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
| LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
| FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
| COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
| INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
| BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
| LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
| CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
| LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
| ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
| POSSIBILITY OF SUCH DAMAGE.
 ========================================================================*/

//=========================================================================
// INCLUDES
//=========================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aardvark.h"
#include "aardvark_setup.h"

#define OPEN_RETRY 3
#define MAX_ERRORS 10
#define SPI_PAGE_SIZE 256
#define ONE_KB 1024
#define BUS_TIMEOUT 150 /* ms */

/*********************************************************************
 *  Aardvark dongle setup
 *
 ********************************************************************/
int aardvark_setup(int port, int i2c_bitrate, int spi_bitrate,
                   Aardvark *aardvark) {
  int rc;
  int ii;
  int nelem = 16;
  u16 ports[16];
  u32 unique_ids[16];
  Aardvark handle;
  static int print_once = 0;

  // Find all the attached devices
  int num_devices;
  int port_found = 0;

  num_devices = aa_find_devices_ext(nelem, ports, nelem, unique_ids);
  if (num_devices < 1) {
    printf("ERROR: No Aardvark devices found (%d)\n", num_devices);
    return (1);
  }

  // Print the information on each device
  if (num_devices > nelem)
    num_devices = nelem;
  for (ii = 0; ii < num_devices; ii++) {
    // Determine if the device is in-use
    const char *status = "(avail) ";
    if (ports[ii] & AA_PORT_NOT_FREE) {
      ports[ii] &= ~AA_PORT_NOT_FREE;
      status = "(in-use)";
    }
    if (ports[ii] == port) {
      port_found = 1;
    }

    // Display device port number, in-use status, and serial number
    if (!print_once) {
      printf("## Aardvark Info: port=%-3d %s (%04d-%06d) (%04x)\n", ports[ii],
             status, unique_ids[ii] / 1000000, unique_ids[ii] % 1000000,
             ports[ii]);
    }
  } // for (ii=0; ii<num_devices; ii++)
  if (0 == port_found) {
    printf("ERROR: Requested port %d not found\n", port);
    return (1);
  }

  if (!print_once) {
    printf("\nport %d found and available, using it\n", port);
  }

  handle = aa_open(port);
  if (handle <= 0) {
    printf("Unable to open Aardvark device on port %d\n", port);
    printf("Error code = %d %s\n", handle, aa_status_string(handle));
    return (1);
  }

  aa_configure(handle, AA_CONFIG_SPI_I2C);
  aa_i2c_pullup(handle, AA_I2C_PULLUP_BOTH);
  aa_target_power(handle, AA_TARGET_POWER_BOTH);

  rc = aa_i2c_bitrate(handle, i2c_bitrate);
  if (rc != i2c_bitrate) {
    printf("WARNING: Requested I2C bitrate %d, got %d\n", i2c_bitrate, rc);
  }
  rc = aa_i2c_bus_timeout(handle, BUS_TIMEOUT);

  //    aa_spi_configure(handle, mode >> 1, mode & 1, AA_SPI_BITORDER_MSB);
  aa_spi_configure(handle, 0, 0, AA_SPI_BITORDER_MSB);
  rc = aa_spi_bitrate(handle, spi_bitrate);
  if (rc != spi_bitrate) {
    printf("WARNING: Requested SPI bitrate %d, got %d\n", spi_bitrate, rc);
  }
  *aardvark = handle;
  print_once++;
  return (0);

} // int aardvark_setup()

/*********************************************************************
 *  Aardvark dongle setup
 ********************************************************************/
int aardvark_setup_ext(int i2c_bitrate, int spi_bitrate, Aardvark *aardvark,
                       conn_t conn) {
  int rc;
  int ii;
  int nelem = 16;
  u16 ports[16];
  u32 unique_ids[16];
  Aardvark handle;
  char systemSerial[32];
  int port;
  static int print_once = 0;

  // Find all the attached devices
  int num_devices;
  int port_found = 0;

  num_devices = aa_find_devices_ext(nelem, ports, nelem, unique_ids);
  if (num_devices < 1) {
    printf("ERROR: No Aardvark devices found (%d)\n", num_devices);
    return (1);
  }

  // Print the information on each device
  if (num_devices > nelem)
    num_devices = nelem;
  for (ii = 0; ii < num_devices; ii++) {
    // Determine if the device is in-use
    const char *status = "(avail) ";
    if (ports[ii] & AA_PORT_NOT_FREE) {
      ports[ii] &= ~AA_PORT_NOT_FREE;
      status = "(in-use)";
    }

    sprintf(systemSerial, "%04d-%06d", unique_ids[ii] / 1000000,
            unique_ids[ii] % 1000000);
    if (strcmp(systemSerial, conn.serialnum) == 0) {
      port_found = 1;
      if (strcmp(status, "(in-use)") == 0) {
        printf("\nport %d found for serial number %s, \
                   but it is %s, marking as not found\n",
               port, conn.serialnum, status);
        port_found = 0;
      }
      port = ports[ii];
      if (!print_once) {
        printf("\nport %d found for serial number %s, using it\n", port,
               conn.serialnum);
      }
    }

    // Display device port number, in-use status, and serial number
    if (!print_once) {
      printf("## Aardvark Info: port=%-3d %s (%04d-%06d) (%04x)\n", ports[ii],
             status, unique_ids[ii] / 1000000, unique_ids[ii] % 1000000,
             ports[ii]);
    }
  } // for (ii=0; ii<num_devices; ii++)
  if (0 == port_found) {
    printf("ERROR: Requested serial %s not found or in-use \n", conn.serialnum);
    return (1);
  }

  handle = aa_open(port);
  if (handle <= 0) {
    printf("Unable to open Aardvark device on port %d\n", port);
    printf("Error code = %d %s\n", handle, aa_status_string(handle));
    return (1);
  }

  aa_configure(handle, AA_CONFIG_SPI_I2C);
  aa_i2c_pullup(handle, AA_I2C_PULLUP_BOTH);
  aa_target_power(handle, AA_TARGET_POWER_BOTH);

  rc = aa_i2c_bitrate(handle, i2c_bitrate);
  if (rc != i2c_bitrate) {
    printf("WARNING: Requested I2C bitrate %d, got %d\n", i2c_bitrate, rc);
  }
  rc = aa_i2c_bus_timeout(handle, BUS_TIMEOUT);

  //    aa_spi_configure(handle, mode >> 1, mode & 1, AA_SPI_BITORDER_MSB);
  aa_spi_configure(handle, 0, 0, AA_SPI_BITORDER_MSB);
  rc = aa_spi_bitrate(handle, spi_bitrate);
  if (rc != spi_bitrate) {
    printf("WARNING: Requested SPI bitrate %d, got %d\n", spi_bitrate, rc);
  }
  *aardvark = handle;
  print_once++;
  return (0);

} // int aardvark_setup_ext()

/*********************************************************************
 *  Aardvark dongle close
 ********************************************************************/
int aardvark_close(Aardvark aardvark) {
  int rc = 0;
  aa_target_power(aardvark, AA_TARGET_POWER_NONE);
  rc = aa_close(aardvark);
  printf("## Aardvark Info: aa_close returned %d\n", rc);

  return (0);
} // int aardvark_close()
