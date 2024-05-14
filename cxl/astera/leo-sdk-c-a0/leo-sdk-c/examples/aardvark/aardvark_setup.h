
#ifndef __aardvark_SETUP_h__
#define __aardvark_SETUP_h__

#include "../../include/leo_connection.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEVICE Aardvark

int aardvark_setup(int port, int i2c_bitrate, int spi_bitrate,
                   Aardvark *aardvark);
int aardvark_setup_ext(int i2c_bitrate, int spi_bitrate, Aardvark *aardvark,
                       conn_t conn);
int aardvark_close(Aardvark aardvark);

#ifdef __cplusplus
}
#endif

#endif /* __aardvark_SETUP_h__ */
