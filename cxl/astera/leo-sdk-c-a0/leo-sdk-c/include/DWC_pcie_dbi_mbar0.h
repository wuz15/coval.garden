#ifndef MBAR0__DWC_PCIE_DBI_H_
#define MBAR0__DWC_PCIE_DBI_H_

#include "hal.h"


//TODO YW: this is a temp fix, need to merge from the same-name header in leo_sw
#define csr_cxl_mbar0_0_PF0_PCIE_CAP_LINK_CONTROL_LINK_STATUS_REG 0x6c0081
#define csr_cxl_mbar0_1_PF0_PCIE_CAP_LINK_CONTROL_LINK_STATUS_REG 0x7c0081

#define CSR_CXL_DBI_0_PF0_CXL_DEVICE_CAP_RCIEP_DVSEC_HDR_1_OFF         0x6003A4
#define CSR_CXL_DBI_0_PF0_CXL_DEVICE_CAP_RCIEP_FLEXBUS_R2_SIZE_LOW_OFF 0x6003CC

//TODO YW: definition need to be replaced later
#define LEO_CXL0_EQ_CONTROL1_ADDR 0x6002f0
#define LEO_CXL1_EQ_CONTROL1_ADDR 0x7002f0

#define LEO_CXL0_EQ_STATUS2_ADDR 0x600304
#define LEO_CXL1_EQ_STATUS2_ADDR 0x700304


#endif