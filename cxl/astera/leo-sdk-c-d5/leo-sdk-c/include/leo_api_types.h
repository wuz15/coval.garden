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
 * @file leo_api_types.h
 * @brief Definition of enums and structs used by leo_api.
 */

#ifndef ASTERA_LEO_SDK_API_TYPES_H_
#define ASTERA_LEO_SDK_API_TYPES_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LEO_MEM_CHANNEL_COUNT 2

typedef enum LeoBoard { LEO_SVB = 1, LEO_A1 = 2, LEO_A2 = 3 } LeoBoardType;

typedef enum { LEO_ASIC_D5 = 0, LEO_ASIC_A0 = 1 } LeoAsicMinorVersionType;

/**
 * @brief Leo device part number enum.
 *
 * Enumeration of values for Leo products.
 */
typedef enum LeoDevicePart {
  LEO_PTX16 = 0x0, /**< PTx16xxx part numbers */
  LEO_PTX08 = 0x1  /**< PTx08xxx part numbers */
} LeoDevicePartType;

/**
 * @brief Enumeration of Packet Error Checking (PEC) options for Leo
 */
typedef enum LeoFWImageFormat {
  LEO_FW_IMAGE_FORMAT_IHX, /**< Intel hex firmware image format */
  LEO_FW_IMAGE_FORMAT_BIN  /**< Binary firmware image format */
} LeoFWImageFormatType;

/**
 * @brief Struct defining pesudo port physical info
 */
typedef struct LeoPseudoPortPins {
  char rxPin[9];          /**< Rx pin location */
  char txPin[9];          /**< Tx pin location */
  int rxPackageInversion; /**< Rx package inversion flag */
  int txPackageInsersion; /**< Tx package inversion flag */
} LeoPseudoPortPinsType;

/**
 * @brief Struct defining one set of pseudo port pins for a give lane
 */
typedef struct LeoPins {
  int lane;                      /**< Lane number */
  LeoPseudoPortPinsType pinSet1; /**< pseudo port pins set upstream */
  LeoPseudoPortPinsType pinSet2; /**< pseudo port pins set downstream */
} LeoPinsType;

/**
 * @brief Enumeration of I2C transaction format options for Leo.
 */
typedef enum LeoI2cFormat {
  LEO_I2C_FORMAT_ASTERA, /**< Astera short format read/write transactions */
  LEO_I2C_FORMAT_INTEL,  /**< Intel long format read/write transactions */
  LEO_I2C_FORMAT_SMBUS   /** standard SMBus interface */
} LeoI2CFormatType;

/**
 * @brief Enumeration of Packet Error Checking (PEC) options for Leo
 */
typedef enum LeoI2cPecEnable {
  LEO_I2C_PEC_ENABLE, /**< Enable PEC during I2C transactions */
  LEO_I2C_PEC_DISABLE /**< Disable PEC during I2C transactions */
} LeoI2CPECEnableType;

typedef enum LeoPRBSPattern {
  DISABLED = 0,
  LFSR31 = 1,
  LFSR23 = 2,
  LFSR23_ALT = 3,
  LFSR16 = 4,
  LFSR15 = 5,
  LFSR11 = 6,
  LFSR9 = 7,
  LFSR7 = 8,
  FIXED_PAT0 = 9,
  FIXED_PAT0_ALT = 10,
  FIXED_PAT0_ALT_PAD = 11
} LeoPRBSPatternType;


/**
 * @brief Struct defining I2C/SMBus connection with a Leo device.
 */
typedef struct LeoI2CDriver {
  int handle;                    /**< File handle to I2C connection */
  int slaveAddr;                 /**< Slave Address */
  LeoI2CFormatType i2cFormat;    /**< I2C format (Astera or Intel) */
  LeoI2CPECEnableType pecEnable; /**< Enable PEC (Packet Error Checking) */
  int lock;             /** Flag indicating if device reads are locked */
  bool lockInit;        /** Flag indicating if lock has been initialized */
  const char *pciefile; /**< PCIe connection to Leo device */
} LeoI2CDriverType;

/**
 * @brief Struct defining FW version loaded on a Leo device.
 */
typedef struct LeoFWVersion {
  uint8_t major;      /** FW version major release value */
  uint8_t minor;      /** FW version minor release value */
  unsigned int build; /** FW version build release value */
} LeoFWVersionType;

/**
 * @brief Struct defining FW version loaded on a Leo device.
 */
typedef struct LeoAsicVersion {
  uint8_t major;      /** ASIC version major release value */
  uint8_t minor;      /** ASIC version minor release value */
} LeoAsicVersionType;

/**SPI
 * @brief SPI types
 * SPI is a synchronous serial interface communications are controlled by SPI
 * master and used to access the serial Quad I/O Flash memory.
 */
typedef struct LeoSPIDriver {
  int handle;   /**< File handle to SPI Connection */
  int addr;     /**< SPI device address */
  int lock;     /**< Flag indicating if device reads are locked */
  int lockInit; /**< Flag indicating if lock has been initialized */
  bool inUse;   /**< Flag indicating if the device is available or currently
                   engaged */
} LeoSPIDriverType;

// linkSpeed:0=illegal,1=Gen1...5=Gen5,6=reserved,7=reserved
// linkWidth:0=1x16,1=1x8(Lane0), 2=1x8(Lane8), 3=2x8
// linkMode:0=CXL,1=PCIE
// linkStatus:0=Inactive, 1=Active
// linkFoM:0..0xff (0xff higher the better max 255)
typedef struct {
  uint8_t linkSpeed : 3;  /**< Link speed 1,2,3,4,5 for the speeds*/
  uint8_t linkWidth : 5;  /**< Link width */
  uint8_t linkMode : 1;   /**< Link Mode PCIE/CXL */
  uint8_t linkStatus : 1; /**< Link Active/Inactive */
  uint8_t linkFoM[16];    /**< Link FoM */
} LeoLinkStatusType;

// In Progress - will be filled up
// training and initialization done
typedef struct {
  uint8_t initDone : 1;     /**< memory initialization Done */
  uint8_t trainingDone : 1; /**< memory initialization Done */
} LeoMemChannelStatusType;

typedef struct {
  LeoMemChannelStatusType ch[LEO_MEM_CHANNEL_COUNT];
} LeoMemoryStatusType;

typedef enum LeoSPIStatus {
  LEO_SPI_ISIN_TXMODE, /**< SPI is in transmit mode */
  LEO_SPI_ISIN_RXMODE  /**< SPI is in receive mode */
} LeoSPIStatusType;

/**
 * @brief Leo Bifurcation enum.
 *
 * Enumeration of values for Link bifurcation modes.
 */
typedef enum LeoBifurcation {
  LEO_BIFUR_1X16 = 0x0, /** 1x16 mode */
  LEO_BIFUR_2X8 = 0x1   /** 2x8 mode */
} LeoBifurcationType;

/**
 * @brief Leo DDR Phy Training enum
 *
 * Enumeration of DDR PHY training modes
 */
typedef enum LeoDDRPhyTrainingMode {
  DDR_PHY_TRAIN_NORMAL = 0,
  DDR_PHY_TRAIN_SKIP = 1,
  DDR_PHY_TRAIN_INIT_ONLY = 2
} LeoDDRPhyTrainingModeType;

/**
 * @brief Leo Per DIMM Capacity
 *
 * Enumeration of per Dimm Capacity
 *
 */
typedef enum LeoPerDimmCapacity {
  PER_DIMM_CAPACITY_16GB,
  PER_DIMM_CAPACITY_32GB,
  PER_DIMM_CAPACITY_64GB,
  PER_DIMM_CAPACITY_128GB
} LeoPerDimmCapacityType;

/**
 * @brief Leo Lane Margin Inforamtion
 *
 * struct defining Lane Margin information
 *
 */
typedef struct LeoPcieLaneMarginInfo {
  uint32_t marginStatus; /**< Read margin control status */
  uint32_t eyeHeight;
  uint32_t eyeWidth;
} LeoPcieLaneMarginInfoType;

/**
 * @brief Leo ECS Information
 *
 * struct defining Error Check and Scrub
 *
 */
typedef struct LeoECSInfo {
  uint32_t ecsEnable;
  uint32_t errCount;
} LeoECSInfoType;

/**
 * @brief Leo Scrub Config Information
 *
 * struct configuration for DDR scrubbing
 *
 */
typedef struct LeoScrubConfigInfo {
  uint32_t bkgrdScrbEn;
  uint32_t bkgrdScrbTimeout;
  uint64_t bkgrdDpaEndAddr;
  uint32_t bkgrdRoundInterval;
  uint32_t bkgrdCmdInterval;
  uint32_t bkgrdWrPoisonIfUncorrErr;
  uint32_t reqScrbEn;
  uint64_t ReqScrbDpaStartAddr;
  uint64_t ReqScrbDpaEndAddr;
  uint32_t ReqScrbCmdInterval;
  uint32_t onDemandScrbEn;
  uint32_t onDemandWrPoisonIfUncorrErr;
} LeoScrubConfigInfoType;

/**
 * @brief Leo SPD Status Information
 *
 * struct defining DDR SPD data and information
 *
 */
typedef struct LeoSpdInfo {
  uint32_t diePerPackage;
  uint32_t densityInGb;
  uint32_t ddrSpeedMax;
  uint32_t ddrSpeedMin;
  uint32_t rank;
  uint32_t capacityInGb;
  uint32_t channel;
  uint32_t channelWidth;
  uint32_t ioWidth;
  uint32_t dimmType;
  uint32_t ddr5SDRAM;

  uint32_t manufacturerIdCode;
  uint32_t dimmDateCodeBcd;
  char partNumber[31];
  uint32_t manufactureringLoc;
  uint32_t serialNumber;
} LeoSpdInfoType;

/**
 * @brief struct to hold the temperature values
 * we use four variables to hold the whole number and decimal portions
 * of the temperature data.
 * for example 33.56 degrees is :
 * ts0WholeNum = 33
 * ts0Decimal = 56
 * ts0WholeNum.ts0Decimal
 */
typedef struct LeoDimmTsodData {
  uint32_t ts0WholeNum;
  uint32_t ts1WholeNum;
  uint32_t ts0Decimal;
  uint32_t ts1Decimal;
} LeoDimmTsodDataType;

/**
 * @brief Leo PPR Information
 *
 * struct defining PPR post package repair for DDR
 *
 */
typedef struct LeoPPRInfo {
  uint32_t PPRtype;     /**< Hard ppr or Software ppr */
  uint32_t dimmSlotNum; /**< send the DIMM slot# to schedule the PPR on next
                           reboot */
  uint32_t status;      /**< PPR operation status - Success/Fail */
} LeoPPRInfoType;

/**
 * @brief Leo Traffic Generator Checker Information
 *
 * struct defining the TGC traffic gen/checker Configuration
 *
 */

typedef struct LeoConfigTgc {
  uint32_t wrEn;        // Write operation Enable
  uint32_t rdEn;        // Read operation Enable
  uint32_t tranCnt;     // transaction count
  uint32_t startAddr;   //
  uint32_t dataPattern; //
} LeoConfigTgc_t;

/* results and status
 */
/**
 * @brief Struct defining Leo CXL device TGC results
 */
typedef struct LeoResultsTgc {
  uint32_t tgcStatus;    // Write operation Enable
  uint32_t wrRdupperCnt; // Read operation Enable
  uint32_t rdReqLowCnt;  //
  uint32_t rdSubchnlCnt;
  uint32_t wrSubchnlCnt; //
  uint32_t wrReqLowCnt;  // transaction count
  uint32_t tgcError0;    // B0,B1 csr expected meta B2, B3 csr received meta
                         // B4 csr expected poison B5 csr received poison
                         // B8 - B16 error count B16-B32 upper address
  uint32_t tgcError1;    // lower address
  uint32_t tgcDataMismatchVec; // tgc data mismatch vec
  uint32_t tgcDataMismatch;    // tgc data mismatch rx data 16bits
} LeoResultsTgc_t;

/**
 * @brief Struct defining Leo CXL device
 */
typedef struct LeoDevice {
  LeoI2CDriverType *i2cDriver; /**< I2C connection to Leo device */
#ifndef ES0
  // LeoPIODriverType pio; /**< PIO connection to Leo device */
  // FIXME LeoUARTDriverType uart; /**< UART connection to Leo device */
#endif
  LeoSPIDriverType spi;             /**< SPI connection to Leo device */
  LeoFWVersionType fwVersion;       /**< FW version loaded on Leo device */
  LeoAsicVersionType asicVersion;   /**< Asic version loaded on Leo device */
  int i2cBus;                 /**< I2C Bus connection to host BMC */
  int revNumber;              /**< Leo revision num */
  int deviceId;               /**< Leo device ID */
  int vendorId;               /**< Leo vendor ID */
  int controllerIndex;        /**< controller Index */
  uint8_t lotNumber[6];       /**< 6-byte silicon lot number */
  uint8_t chipID[12];         /**< 12-byte unique chip ID */
  bool arpEnable;             /**< Flag indicating if ARP is enabled or not */
  bool codeLoadOkay; /**< Flag to indicate if code load reg value is expected */
  bool mmHeartbeatOkay; /**< Flag indicating if Main Micro heartbeat is good */
  int mm_print_info_struct_addr;  /**< AL print info struct offset (Main Micro)
                                   */
  int pm_print_info_struct_addr;  /**< AL print info struct offset (Path Micro)
                                   */
  int mm_gp_ctrl_sts_struct_addr; /**< GP ctrl status struct offset (Main Micro)
                                   */
  int pm_gp_ctrl_sts_struct_addr; /**< GP ctrl status struct offset (Path Micro)
                                   */
  int linkPathStructSize;         /** Num bytes in link path struct */
  uint8_t tempCalCodePmaA[4];     /**< temp calibration codes for PMA A */
  uint8_t tempCalCodePmaB[4];     /**< temp calibration codes for PMA B */
  uint8_t tempCalCodeAvg;         /**< average temp calibration code */
  float tempAlertThreshC;         /**< temp alert (celsius) */
  float tempWarnThreshC;          /**< temp warning (celsius) */
  float maxTempC;                 /**< Max. temp seen across all temp sensors */
  float currentTempC;           /**< Current average temp across all sensors */
  uint8_t minLinkFoMAlert;      /**< Min. FoM value expected */
  bool deviceOkay;              /**< Device state */
  bool overtempAlert;           /**< Over temp alert indicated by reg */
  LeoDevicePartType partNumber; /** Retimer part number - x16 or x8 */
  LeoPinsType pins[16];         /** Device physical pin information */
  uint16_t minDPLLFreqAlert;    /** Min. DPLL frequency expected */
  uint16_t maxDPLLFreqAlert;    /** Max. DPLL frequency expected */
  uint8_t ignorePersistentDataFlag; /** Overwrite persistent data when updating firmware */
  uint8_t ignoreCompatibilityCheckFlag; /** Ignore asic version check when updating firmware */
  int spiDevice;
} LeoDeviceType;

/**
 * @brief Struct defining detailed Transmitter status, including electrical
 * parameters.
 */
typedef struct LeoTxState {
  int logicalLaneNum;    /**< Captured logical Lane number */
  char *physicalPinName; /**< Physical pin name */
  int elecIdleStatus;    /**< Electrical idle status */
  int de;                /**< Current de-emphasis value (Gen-1/2 and above) */
  int pre;               /**< Current pre-cursor value (Gen-3 and above) */
  int cur;               /**< Current main-cursor value (Gen-3 and above) */
  float pst;             /**< Current post-cursor value (Gen-3 and above) */
  int lastEqRate;    /**< Last speed (e.g. 4 for Gen4) at which EQ was done */
  int lastPresetReq; /**< If final request was a preset, this is the value */
  int lastPreReq;    /**< If final request was a coefficient, this is the pre */
  int lastCurReq;    /**< If final request was a coefficient, this is the cur */
  int lastPstReq;    /**< If final request was a coefficient, this is the pst */
} LeoTxStateType;

/**
 * @brief Struct defining detailed Receiver status, including electrical
 * parameters.
 */
typedef struct LeoRxState {
  int logicalLaneNum;    /**< Captured logical Lane number */
  char *physicalPinName; /**< Physical pin name */
  int termination;       /**< Termination enable (1) or disable (0) status */
  int polarity;          /**< Polarity normal (0) or inverted (1) status */
  float attdB;           /** Rx Attenuator, in DB */
  float vgadB; /** Rx VGA, in DB */ //
  float ctleBoostdB;                /**< CTLE boost, in dB */
  int ctlePole;                     /**< CTLE pole code setting */
  float dfe1;                       /**< DFE tap 1 value, in mV */
  float dfe2;                       /**< DFE tap 2 value, in mV */
  float dfe3;                       /**< DFE tap 3 value, in mV */
  float dfe4;                       /**< DFE tap 4 value, in mV */
  float dfe5;                       /**< DFE tap 5 value, in mV */
  float dfe6;                       /**< DFE tap 6 value, in mV */
  float dfe7;                       /**< DFE tap 7 value, in mV */
  float dfe8;                       /**< DFE tap 8 value, in mV */
  int FoM;                          /**< Current FoM value */
  int lastEqRate;    /**< Last speed (e.g. 4 for Gen4) at which EQ was done */
  int lastPresetReq; /**< Final preset request */
  int lastPresetReqFom;   /**< Final preset request FoM value */
  int lastPresetReqM1;    /**< Final-1 preset request */
  int lastPresetReqFomM1; /**< Final-1 preset request FoM value */
  int lastPresetReqM2;    /**< Final-2 preset request */
  int lastPresetReqFomM2; /**< Final-2 preset request FoM value */
  int lastPresetReqM3;    /**< Final-3 preset request */
  int lastPresetReqFomM3; /**< Final-3 preset request FoM value */
  uint16_t DPLLCode;      /**< Current DPLL code */
} LeoRxStateType;

typedef enum LeoMaxDataRate { Leo_GEN1 = 1, Leo_GEN2 = 2 } LeoMaxDataRateType;

/**
 * @brief Leo LTSSM Logger verbosity enum.
 *
 * Enumeration of values to define verbosity.
 */
typedef enum LeoLTSSMVerbosity {
  LEO_LTSSM_VERBOSITY_HIGH = 0x1, /**< High verbosity */
} LeoLTSSMVerbosityType;

/**
 * @brief Leo Link state enum.
 *
 * Enumeration of values for Link states.
 */
typedef enum LeoLinkStateEnum {
  LEO_STATE_RESET = 0x0,            /**< Reset state */
  LEO_STATE_PROTOCOL_RESET_0 = 0x1, /**< Protocol Reset state */
  LEO_STATE_PROTOCOL_RESET_1 = 0x2, /**< Protocol Reset state */
  LEO_STATE_PROTOCOL_RESET_2 = 0x3, /**< Protocol Reset state */
  LEO_STATE_PROTOCOL_RESET_3 = 0x4, /**< Protocol Reset state */
  LEO_STATE_DETECT_OR_DESKEW = 0x5, /**< Receiver Detect or Deskew state */
  LEO_STATE_FWD = 0x6,              /**< Forwarding state (i.e. L0) */
  LEO_STATE_EQ_P2_0 = 0x7,          /**< Equalization Phase 2 state */
  LEO_STATE_EQ_P2_1 = 0x8,          /**< Equalization Phase 2 state */
  LEO_STATE_EQ_P3_0 = 0x9,          /**< Equalization Phase 3 state */
  LEO_STATE_EQ_P3_1 = 0xa,          /**< Equalization Phase 3 state */
  LEO_STATE_HOT_PLUG = 0xb,         /**< Hot Plug state */
  LEO_STATE_PROTOCOL_RESET_4 = 0xc, /**< Protocol Reset state */
  LEO_STATE_OTHER = 0xd             /**< Other (unexpected) state */
} LeoLinkStateEnumType;

/**
 * @brief Leo LTSSM Logger type enum.
 *
 * Enumeration of values for the Link and Path log types.
 */
typedef enum {
  LEO_LTSSM_LINK_LOGGER = 0xff,    /**< Link-level logger */
  LEO_LTSSM_DS_LN_0_1_LOG = 0x0,   /**< Downstream, lanes 0 & 1 */
  LEO_LTSSM_DS_LN_2_3_LOG = 0x2,   /**< Downstream, lanes 2 & 3 */
  LEO_LTSSM_DS_LN_4_5_LOG = 0x4,   /**< Downstream, lanes 4 & 5 */
  LEO_LTSSM_DS_LN_6_7_LOG = 0x6,   /**< Downstream, lanes 6 & 7 */
  LEO_LTSSM_DS_LN_8_9_LOG = 0x8,   /**< Downstream, lanes 8 & 9 */
  LEO_LTSSM_DS_LN_10_11_LOG = 0xa, /**< Downstream, lanes 10 & 11 */
  LEO_LTSSM_DS_LN_12_13_LOG = 0xc, /**< Downstream, lanes 12 & 13 */
  LEO_LTSSM_DS_LN_14_15_LOG = 0xe, /**< Downstream, lanes 14 & 15 */
  LEO_LTSSM_US_LN_0_1_LOG = 0x1,   /**< Upstream, lanes 0 & 1 */
  LEO_LTSSM_US_LN_2_3_LOG = 0x3,   /**< Upstream, lanes 2 & 3 */
  LEO_LTSSM_US_LN_4_5_LOG = 0x5,   /**< Upstream, lanes 4 & 5 */
  LEO_LTSSM_US_LN_6_7_LOG = 0x7,   /**< Upstream, lanes 6 & 7 */
  LEO_LTSSM_US_LN_8_9_LOG = 0x9,   /**< Upstream, lanes 8 & 9 */
  LEO_LTSSM_US_LN_10_11_LOG = 0xb, /**< Upstream, lanes 10 & 11 */
  LEO_LTSSM_US_LN_12_13_LOG = 0xd, /**< Upstream, lanes 12 & 13 */
  LEO_LTSSM_US_LN_14_15_LOG = 0xf, /**< Upstream, lanes 14 & 15 */
} LeoLTSSMLoggerEnumType;

/**
 * @brief Enumeration of values for the SRAM Program memory check
 */
typedef enum LeoSramMemoryCheck {
  LEO_SRAM_MM_CHECK_IDLE = 0,        /**< Idle state */
  LEO_SRAM_MM_CHECK_IN_PROGRESS = 1, /**< Memory self-check in progress */
  LEO_SRAM_MM_CHECK_PASS = 2,        /**< Memory self-check has passed */
  LEO_SRAM_MM_CHECK_FAIL = 3,        /**< Memory self-check has failed */
} LeoSramMemoryCheckType;

/**
 * @brief Struct defining an Leo chip Link configuration.
 *
 * Note that each Leo chip can support multiple multi-Lane Links.
 */
typedef struct LeoLinkConfig {
  LeoDevicePartType partNumber; /**< Leo part number */
  int maxWidth;                 /**< Maximum Link width (e.g. 16) */
  int startLane;                /**< Starting (physical) Lane number */
  float tempAlertThreshC;       /**< Temperature alert threshold, in celsius */
  int linkId;                   /**< Link identifier (starts from 0) */
} LeoLinkConfigType;

/**
 * @brief Struct defining detailed Pseudo Port status, including electrical
 * parameters.
 *
 * Note that each Leo Link has two Pseudo Ports, and Upstream Pseudo Port
 * (USPP) and a Downstream Pseudo Port (DSPP).
 */
typedef struct LeoPseudoPortState {
  LeoTxStateType txState[16]; /**< Detailed Tx state */
  LeoRxStateType rxState[16]; /**< Detailed Tx state */
  uint16_t minDPLLCode;       /**< Min DPLL code seen */
  uint16_t maxDPLLCode;       /**< Max DPLL code seen */
} LeoPseudoPortStateType;
/**
 * @brief Struct defining detailed Link status, including electrical
 * parameters.
 *
 */
typedef struct LeoLinkState {
  int width;                  /**< Current width of the Link */
  LeoLinkStateEnumType state; /**< Current Link state */
  int rate;                   /**< Current data rate (1=Gen1, 5=Gen5) */
  bool linkOkay;              /**< Link is healthy (true) or not (false) */
  LeoPseudoPortStateType usppState; /**< Current USPP state */
  // LeoRetimerCoreStateType coreState; /**< Current RT core state */
  LeoPseudoPortStateType dsppState; /**< Current DSPP state */
  int linkMinFoM; /**< Minimum FoM value across all Lanes on USPP and DSPP */
  char *linkMinFoMRx; /** Receiver corresponding to minimum FoM */
  int recoveryCount;  /** Count of entries to Recovery since L0 at current speed
                       */
} LeoLinkStateType;

/**
 * @brief Struct defining an Leo Link.
 *
 * Note that each Leo Chip can support multiple multi-Lane Links.
 */
typedef struct LeoLink {
  LeoDeviceType *device;    /**< Pointer to device struct */
  LeoLinkConfigType config; /**< struct with link config settings */
  LeoLinkStateType state;   /**< struct with link stats returned back */
} LeoLinkType;

/**
 * @brief Struct defining an Leo LTSSM Logger entry.
 */
typedef struct LeoLTSSMEntry {
  uint8_t data; /**< Data entry from LTSSM module */
  int logType;  /**< Log type i.e. if Main Mirco (0xff) or Path Micro ID */
  int offset;   /**< Logger offset inside LTSSM module */
} LeoLTSSMEntryType;

/**
 * @brief Struct defining detailed the Link Status register (offset 12h) about
 * the PCIExpress link specific parameters.
 */
typedef union LeoLinkStatusRegisterType {
  struct {
    uint8_t linkSpeed : 2;               /**< Link speed */
    uint8_t linkWidth : 2;               /**< Link width */
    uint8_t undefined : 2;               /**< Undefined */
    uint8_t linkTraining : 2;            /**< Link training state */
    uint8_t slotClockConfig : 2;         /**< Slot clock configuration */
    uint8_t dataLinkLayerLinkActive : 1; /**< Data Link Layer Link Active */
    uint8_t linkBandwidthManagementStatus : 1; /**< Link Bandwidth Management
                                                  Status */
  };
  uint8_t
      linkAutonomousBandwidthStatus; /**< Link Autonomous Bandwidth Status */
} LeoLinkStatusRegisterType;

/**
 * @brief Struct defining detailed the Link Control register (offset 10h) which
 * controls PCIe link parameters.
 */
typedef union LeoLinkControlRegisterType {
  struct {
    uint8_t ASPMControl : 2; /**< ASPM -Active State Power Management Control */
    uint8_t RsvdP1 : 2;      /**< Reserved */
    uint8_t readCompletionBoundary : 1; /**< Read completion boundary value for
                                           root port or upstream from endpoint
                                           0->64 byte and 1-> 128 byte */
    uint8_t linkDisable : 1;            /**< Link training state */
    uint8_t retrainLink : 2;            /**< Slot clock configuration */
    uint8_t commonClockConfig : 1;      /**< Data Link Layer Link Active */
    uint8_t extendedSync : 1;           /**< Link Bandwidth Management Status */
    uint8_t
        enableClockPowerManagement : 1; /**< Clock power management enabled */
    uint8_t RsvdP2 : 7;                 /**< Reserved */
  };
  uint8_t
      linkAutonomousBandwidthStatus; /**< Link Autonomous Bandwidth Status */
} LeoLinkControlRegisterType;

typedef enum LeoSvbI2cSWMode {
  I2C_SW_UNKNOW = 0,
  I2C_SW_NORMAL = 1,
  I2C_SW_DDRC_0 = 2,
  I2C_SW_DDRC_1 = 3
} LeoSvbI2cSWModeType;

typedef struct LeoDdrConfig {
  uint32_t numRanks;
  uint32_t ddrSpeed;
  uint32_t dqWidth;
  uint32_t dpc;
  uint32_t capacity;
} LeoDdrConfigType;

typedef struct LeoCxlConfig {
  uint32_t numLinks;
  uint32_t linkWidth;
} LeoCxlConfigType;

typedef struct LeoIdInfo {
  uint32_t leoId;
  LeoBoardType boardType;
} LeoIdInfoType;

typedef struct LeoCmalStats {
  uint8_t rrspDdrReturnedPoisonCount[2];
  uint8_t rrspRdpCausedPoisonCount[2];
} LeoCmalStatsType;

typedef enum LeoFailVectorIndex {
  LEO_FAIL_VEC_CATRIP 		  = 0x3,
  LEO_FAIL_VEC_DDR_CLEAR_COUNTERS = 0x4,
  LEO_FAIL_VEC_BAD_DSID 	  = 0x5,
  LEO_FAIL_VEC_GUARD_CH0_FAIL     = 0x6,
  LEO_FAIL_VEC_GUARD_CH1_FAIL     = 0x7,
  LEO_FAIL_VEC_GUARD_CHECK_FAIL   = 0x8
} LeoFailVectorIndexType;

typedef struct LeoFailVector {
  uint32_t failVector;
} LeoFailVectorType;

typedef struct LeoCxlDvsecHdr1Info {
  uint32_t dvsecVendorId : 16;
  uint32_t dvsecRev : 4;
  uint32_t dvsecLen : 12;
} LeoCxlDvSecHdr1InfoType;

typedef struct LeoCxlFlexbusRange2SizeLow {
  uint32_t memInfoValid : 1;
  uint32_t memActive : 1;
  uint32_t mediaType : 3;
  uint32_t memClass : 3;
  uint32_t desiredInterleave : 5;
  uint32_t hwInit : 3;
  uint32_t rsvd : 12;
  uint32_t memSizeLow : 4;
} LeoCxlFlexbusRange2SizeLowType;

typedef struct LeoCxlStats {
  uint64_t s2mNdrCount[2];
  uint64_t s2mDrsCount[2];
  uint64_t m2sReqCount[2];
  uint64_t m2sRwdCount[2];
  uint64_t linkBandwidth[2];
} LeoCxlStatsType;

typedef struct LeoDdrTelemetry {
  uint8_t ddrS0waec;
  uint8_t ddrS1waec;
  uint8_t ddrS2waec;
  uint8_t ddrS3waec;
  uint8_t ddrchwaec;
  uint8_t ddrS0rdcrc;
  uint8_t ddrS1rdcrc;
  uint8_t ddrS2rdcrc;
  uint8_t ddrS3rdcrc;
  uint8_t ddrchrdcrc;
  uint8_t ddrS0rduec;
  uint8_t ddrS1rduec;
  uint8_t ddrS2rduec;
  uint8_t ddrS3rduec;
  uint8_t ddrchrduec;
  uint8_t ddrS0rdcec;
  uint8_t ddrS1rdcec;
  uint8_t ddrS2rdcec;
  uint8_t ddrS3rdcec;
  uint8_t ddrchrdcec;
  uint64_t ddrRefCount;
  uint64_t ddrRdActCount;
  uint64_t ddrPreChCount;
} LeoDdrTelemetryType;

typedef struct LeoDatapathTelemetry {
  uint8_t ddrTgcCe;
  uint8_t ddrTgcUe;
  uint8_t ddrOssc;
  uint8_t ddrOsdc;
  uint32_t ddrbscec;
  uint32_t ddrbsuec;
  uint32_t ddrbsoec;
} LeoDatapathTelemetryType;

typedef struct LeoCxlTelemetry {
  uint64_t clock_ticks;
  uint64_t s2m_ndr_c;
  uint64_t s2m_drs_c;
  uint64_t m2s_req_c;
  uint64_t m2s_rwd_c;
  uint64_t rasRxCe;
  uint64_t rasRxUeRwdHdr;
  uint64_t rasRxUeReqHdr;
  uint64_t rasRxUeRwdBe;
  uint64_t rasRxUeRwdData;
  uint64_t rwdHdrUe;
  uint64_t rwdHdrHdm;
  uint64_t rwdHdrUfe;
  uint64_t reqHdrUe;
  uint64_t reqHdrHdm;
  uint64_t reqHdrUfe;
} LeoCxlTelemetryType;

enum LeoPersistentDataId {
  PERSISTENT_DATA_ID_VERSION = 1,
  PERSISTENT_DATA_ID_CATTRIP = 3,
};

typedef struct LeoPersistentDataValue {
  union {
    struct {
      uint32_t countTotal : 16;
      uint32_t countSinceBoot : 16;
    };
    uint32_t value;
  };
} LeoPersistentDataValueType;

typedef struct LeoDDRSubChannelThrottleContrlConfig {
  uint32_t throtCtrlEnable : 1;
  uint32_t maxCmdCntPerWindow : 12;
  uint32_t windowLength : 12;
} LeoDDRSubChannelThrottleContrlConfigType;

typedef union {
  uint32_t dw[8];

  struct {
    uint8_t rxClkDlyMargin[2][4]; // [ch][rank]
    uint8_t vrefDacMargin[2][4];  // [ch][rank]
    uint8_t txDqDlyMargin[2][4];  // [ch][rank]
    uint8_t devVrefMargin[2][4];  // [ch][rank]
  };
} LeoDdrphyRxTxMarginType;

typedef union {
  uint32_t dw[8];

  struct {
    uint8_t csDlyMargin[2][4];  // [ch][rank]
    uint8_t csVrefMargin[2][4]; // [ch][rank]
    uint8_t caDlyMargin[2][4];  // [ch][rank]
    uint8_t caVrefMargin[2][4]; // [ch][rank]
  };
} LeoDdrphyCsCaMarginType;

typedef union 
{
  uint32_t dw[4];

  struct
  {
    uint8_t qcsDlyMargin[2][4]; // [ch][rank]  
    uint8_t qcaDlyMargin[2][4]; // [ch][rank]  
  };
} LeoDdrphyQcsQcaMarginType;

/**
 * @brief Struct defining EventRecord for repair
 */
typedef struct LeoRepairRecord {
  uint8_t  channel;      /** DDR Channel **/
  uint8_t  rank;         /** DIMM Rank **/
  uint8_t  bank;         /** DRAM Bank  **/
  uint32_t row;          /** DRAM Row  **/
  uint8_t  bankGroup;    /** DRAM Bank Group **/
  uint8_t  CID;          /** Future: 3DS: DRAM Logical ID for future **/
  uint32_t nibbleMask;   /** DDR nibble mask **/
  uint8_t  pResource;    /** Future: HPPR: BGA resource usage **/
  uint8_t  pStatus;      /** success or failure of repair **/
} LeoRepairRecordType;

#ifdef __cplusplus
}
#endif

#endif /* ASTERA_LEO_SDK_API_TYPES_H_ */
