#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pcre.h>

#define NUM_TPMI_REG 217
#define NUM_TPMI_PFS 14

typedef struct {
    uint64_t start_addr;
    uint64_t end_addr;
} mmio_region_t;

typedef struct {
    uint64_t id;
    uint64_t offset;
    uint64_t register_offset;
} tpmi_instance_t;

typedef struct {
    char name[50];
    uint64_t tpmi_id;
    uint64_t cap_offset;
    uint64_t tpmi_offset;
    uint64_t sub_id;
} tpmi_reg_t;

typedef struct {
    uint64_t tpmi_id;
    uint64_t entry_num;
    uint64_t entry_size;
} tpmi_pfs_t;

static uint32_t tpmi_region_id = 1;
static char *tpmi_default_bdf = "0000:00:03.1";
static char *tpmi_dump_file = "tpmi_dump.bin";
static char *tpmi_actual_bdf = NULL;
static tpmi_pfs_t TPMI_PFS_MAP[NUM_TPMI_PFS] = {
    //TPMI_ID           ENTRY_NUM           ENTRY_SIZE
    {0x0,               1,                  96},
    {0x1,               5,                  10},
    {0x2,               5,                  12},
    {0x3,               2,                  6},
    {0x4,               1,                  20},
    {0x5,               5,                  254},
    {0xa,               5,                  10},
    {0x6,               1,                  6},
    {0xc,               5,                  10},
    {0xd,               1,                  6},
    {0x81,              1,                  4},
    {0xfd,              5,                  291},
    {0xfe,              3,                  291},
    {0xff,              1,                  291},
};

static tpmi_reg_t TPMI_REG_MAP[NUM_TPMI_REG] = {
        //REG_NAME                          REG_TPMI_ID                 REG_CAP_OFFSET                  REG_TPMI_OFFSET              SUB_ID
        {"SOCKET_RAPL_DOMAIN_HEADER",       0x0,                        0x2000,                         0x0,                         0},
        {"SOCKET_RAPL_POWER_UNIT",          0x0,                        0x2000,                         0x8,                         0},
        {"SOCKET_RAPL_PL1_CONTROL",         0x0,                        0x2000,                         0x10,                        0},
        {"SOCKET_RAPL_PL2_CONTROL",         0x0,                        0x2000,                         0x18,                        0},
        {"SOCKET_RAPL_RSVD1",               0x0,                        0x2000,                         0x20,                        0},
        {"SOCKET_RAPL_PL4_CONTROL",         0x0,                        0x2000,                         0x28,                        0},
        {"SOCKET_RAPL_RSVD2",               0x0,                        0x2000,                         0x30,                        0},
        {"SOCKET_RAPL_ENERGY_STATUS",       0x0,                        0x2000,                         0x38,                        0},
        {"SOCKET_RAPL_PERF_STATUS",         0x0,                        0x2000,                         0x40,                        0},
        {"SOCKET_RAPL_PL_INFO",             0x0,                        0x2000,                         0x48,                        0},
        {"SOCKET_RAPL_RSVD3",               0x0,                        0x2000,                         0x50,                        0},
        {"SOCKET_RAPL_RSVD4",               0x0,                        0x2000,                         0x58,                        0},
        {"SOCKET_RAPL_RSVD5",               0x0,                        0x2000,                         0x60,                        0},
        {"SOCKET_RAPL_RSVD6",               0x0,                        0x2000,                         0x68,                        0},
        {"SOCKET_RAPL_RSVD7",               0x0,                        0x2000,                         0x70,                        0},
        {"SOCKET_RAPL_RSVD8",               0x0,                        0x2000,                         0x78,                        0},
        {"DRAM_RAPL_DOMAIN_HEADER",         0x0,                        0x2000,                         0x80,                        0},
        {"DRAM_RAPL_POWER_UNIT",            0x0,                        0x2000,                         0x88,                        0},
        {"DRAM_RAPL_PL1_CONTROL",           0x0,                        0x2000,                         0x90,                        0},
        {"DRAM_RAPL_RSVD1",                 0x0,                        0x2000,                         0x98,                        0},
        {"DRAM_RAPL_RSVD2",                 0x0,                        0x2000,                         0xA0,                        0},
        {"DRAM_RAPL_RSVD3",                 0x0,                        0x2000,                         0xA8,                        0},
        {"DRAM_RAPL_RSVD4",                 0x0,                        0x2000,                         0xB0,                        0},
        {"DRAM_RAPL_ENERGY_STATUS",         0x0,                        0x2000,                         0xB8,                        0},
        {"DRAM_RAPL_PERF_STATUS",           0x0,                        0x2000,                         0xC0,                        0},
        {"DRAM_RAPL_PL_INFO",               0x0,                        0x2000,                         0xC8,                        0},
        {"DRAM_RAPL_RSVD5",                 0x0,                        0x2000,                         0xD0,                        0},
        {"DRAM_RAPL_RSVD6",                 0x0,                        0x2000,                         0xD8,                        0},
        {"DRAM_RAPL_RSVD7",                 0x0,                        0x2000,                         0xE0,                        0},
        {"DRAM_RAPL_RSVD8",                 0x0,                        0x2000,                         0xE8,                        0},
        {"DRAM_RAPL_RSVD9",                 0x0,                        0x2000,                         0xF0,                        0},
        {"DRAM_RAPL_RSVD10",                0x0,                        0x2000,                         0xF8,                        0},
        {"PLATFORM_RAPL_DOMAIN_HEADER",     0x0,                        0x2000,                         0x100,                       0},
        {"PLATFORM_RAPL_POWER_UNIT",        0x0,                        0x2000,                         0x108,                       0},
        {"PLATFORM_RAPL_PL1_CONTROL",       0x0,                        0x2000,                         0x110,                       0},
        {"PLATFORM_RAPL_PL2_CONTROL",       0x0,                        0x2000,                         0x118,                       0},
        {"PLATFORM_RAPL_RSVD1",             0x0,                        0x2000,                         0x120,                       0},
        {"PLATFORM_RAPL_RSVD2",             0x0,                        0x2000,                         0x128,                       0},
        {"PLATFORM_RAPL_RSVD3",             0x0,                        0x2000,                         0x130,                       0},
        {"PLATFORM_RAPL_ENERGY_STATUS",     0x0,                        0x2000,                         0x138,                       0},
        {"PLATFORM_RAPL_PERF_STATUS",       0x0,                        0x2000,                         0x140,                       0},
        {"PLATFORM_RAPL_PL_INFO",           0x0,                        0x2000,                         0x148,                       0},
        {"PLATFORM_RAPL_DOMAIN_INFO",       0x0,                        0x2000,                         0x150,                       0},
        {"PLATFORM_RAPL_RSVD4",             0x0,                        0x2000,                         0x158,                       0},
        {"PLATFORM_RAPL_RSVD5",             0x0,                        0x2000,                         0x160,                       0},
        {"PLATFORM_RAPL_RSVD6",             0x0,                        0x2000,                         0x168,                       0},
        {"PLATFORM_RAPL_RSVD7",             0x0,                        0x2000,                         0x170,                       0},
        {"PLATFORM_RAPL_RSVD8",             0x0,                        0x2000,                         0x178,                       0},
        {"PEM_HEADER",                      0x1,                        0x3000,                         0x0,                         0},
        {"PEM_CONTROL",                     0x1,                        0x3000,                         0x8,                         0},
        {"PEM_STATUS",                      0x1,                        0x3000,                         0x10,                        0},
        {"PEM_PMT_INFO",                    0x1,                        0x3000,                         0x18,                        0},
        {"PEM_RSVD1",                       0x1,                        0x3000,                         0x20,                        0},
        {"UFS_HEADER",                      0x2,                        0x4000,                         0x0,                         0},
        {"UFS_FABRIC_CLUSTER_OFFSET",       0x2,                        0x4000,                         0x8,                         0},
        {"UFS_STATUS",                      0x2,                        0x4000,                         0x0,                         1},
        {"UFS_CONTROL",                     0x2,                        0x4000,                         0x8,                         1},
        {"UFS_ADV_CONTROL_1",               0x2,                        0x4000,                         0x10,                        1},
        {"UFS_ADV_CONTROL_2",               0x2,                        0x4000,                         0x18,                        1},
        {"DRC_HEADER",                      0x4,                        0x6000,                         0x0,                         0},
        {"DRC_CONTROL",                     0x4,                        0x6000,                         0x8,                         0},
        {"DRC_CLOS_TO_MEMCLOS",             0x4,                        0x6000,                         0x10,                        0},
        {"DRC_MCLOS_ATTRIBUTES_0",          0x4,                        0x6000,                         0x18,                        0},
        {"DRC_MCLOS_ATTRIBUTES_1",          0x4,                        0x6000,                         0x20,                        0},
        {"DRC_MCLOS_ATTRIBUTES_2",          0x4,                        0x6000,                         0x28,                        0},
        {"DRC_MCLOS_ATTRIBUTES_3",          0x4,                        0x6000,                         0x30,                        0},
        {"DRC_MCLOS_CONFIG0",               0x4,                        0x6000,                         0x38,                        0},
        {"DRC_MCLOS_CONFIG1",               0x4,                        0x6000,                         0x40,                        0},
        {"DRC_STATUS",                      0x4,                        0x6000,                         0x48,                        0},
        {"SST_HEADER",                      0x5,                        0x7000,                         0x0,                         0},
        {"SST_CP_HEADER",                   0x5,                        0x7000,                         0x0,                         1},
        {"SST_CP_CONTROL",                  0x5,                        0x7000,                         0x8,                         1},
        {"SST_CP_STATUS",                   0x5,                        0x7000,                         0x10,                        1},
        {"SST_CLOS_CONFIG_0",               0x5,                        0x7000,                         0x18,                        1},
        {"SST_CLOS_CONFIG_1",               0x5,                        0x7000,                         0x20,                        1},
        {"SST_CLOS_CONFIG_2",               0x5,                        0x7000,                         0x28,                        1},
        {"SST_CLOS_CONFIG_3",               0x5,                        0x7000,                         0x30,                        1},
        {"SST_CLOS_ASSOC_0",                0x5,                        0x7000,                         0x38,                        1},
        {"SST_CLOS_ASSOC_1",                0x5,                        0x7000,                         0x40,                        1},
        {"SST_CLOS_ASSOC_2",                0x5,                        0x7000,                         0x48,                        1},
        {"SST_CLOS_ASSOC_3",                0x5,                        0x7000,                         0x50,                        1},
        {"SST_PP_HEADER",                   0x5,                        0x7000,                         0x0,                         2},
        {"SST_PP_OFFSET_0",                 0x5,                        0x7000,                         0x8,                         2},
        {"SST_PP_OFFSET_1",                 0x5,                        0x7000,                         0x10,                        2},
        {"SST_PP_CONTROL",                  0x5,                        0x7000,                         0x18,                        2},
        {"SST_PP_STATUS",                   0x5,                        0x7000,                         0x20,                        2},
        {"SST_PP_INFO_0",                   0x5,                        0x7000,                         0x0,                         3},
        {"SST_PP_INFO_1",                   0x5,                        0x7000,                         0x8,                         3},
        {"SST_PP_INFO_2",                   0x5,                        0x7000,                         0x10,                        3},
        {"SST_PP_INFO_3",                   0x5,                        0x7000,                         0x18,                        3},
        {"SST_PP_INFO_4",                   0x5,                        0x7000,                         0x20,                        3},
        {"SST_PP_INFO_5",                   0x5,                        0x7000,                         0x28,                        3},
        {"SST_PP_INFO_6",                   0x5,                        0x7000,                         0x30,                        3},
        {"SST_PP_INFO_7",                   0x5,                        0x7000,                         0x38,                        3},
        {"SST_PP_INFO_8",                   0x5,                        0x7000,                         0x40,                        3},
        {"SST_PP_INFO_9",                   0x5,                        0x7000,                         0x48,                        3},
        {"SST_PP_INFO_10",                  0x5,                        0x7000,                         0x50,                        3},
        {"SST_PP_INFO_11",                  0x5,                        0x7000,                         0x58,                        3},
        {"SST_BF_INFO_0",                   0x5,                        0x7000,                         0x0,                         4},
        {"SST_BF_INFO_1",                   0x5,                        0x7000,                         0x8,                         4},
        {"SST_TF_INFO_0",                   0x5,                        0x7000,                         0x0,                         5},
        {"SST_TF_INFO_1",                   0x5,                        0x7000,                         0x8,                         5},
        {"SST_TF_INFO_2",                   0x5,                        0x7000,                         0x10,                        5},
        {"SST_TF_INFO_3",                   0x5,                        0x7000,                         0x18,                        5},
        {"SST_TF_INFO_4",                   0x5,                        0x7000,                         0x20,                        5},
        {"SST_TF_INFO_5",                   0x5,                        0x7000,                         0x28,                        5},
        {"SST_TF_INFO_6",                   0x5,                        0x7000,                         0x30,                        5},
        {"SST_TF_INFO_7",                   0x5,                        0x7000,                         0x38,                        5},
        {"PP1_SST_PP_INFO_0",               0x5,                        0x7000,                         0x0,                         3},
        {"PP1_SST_PP_INFO_1",               0x5,                        0x7000,                         0x8,                         3},
        {"PP1_SST_PP_INFO_2",               0x5,                        0x7000,                         0x10,                        3},
        {"PP1_SST_PP_INFO_3",               0x5,                        0x7000,                         0x18,                        3},
        {"PP1_SST_PP_INFO_4",               0x5,                        0x7000,                         0x20,                        3},
        {"PP1_SST_PP_INFO_5",               0x5,                        0x7000,                         0x28,                        3},
        {"PP1_SST_PP_INFO_6",               0x5,                        0x7000,                         0x30,                        3},
        {"PP1_SST_PP_INFO_7",               0x5,                        0x7000,                         0x38,                        3},
        {"PP1_SST_PP_INFO_8",               0x5,                        0x7000,                         0x40,                        3},
        {"PP1_SST_PP_INFO_9",               0x5,                        0x7000,                         0x48,                        3},
        {"PP1_SST_PP_INFO_10",              0x5,                        0x7000,                         0x50,                        3},
        {"PP1_SST_PP_INFO_11",              0x5,                        0x7000,                         0x58,                        3},
        {"PP1_SST_BF_INFO_0",               0x5,                        0x7000,                         0x0,                         4},
        {"PP1_SST_BF_INFO_1",               0x5,                        0x7000,                         0x8,                         4},
        {"PP1_SST_TF_INFO_0",               0x5,                        0x7000,                         0x0,                         5},
        {"PP1_SST_TF_INFO_1",               0x5,                        0x7000,                         0x8,                         5},
        {"PP1_SST_TF_INFO_2",               0x5,                        0x7000,                         0x10,                        5},
        {"PP1_SST_TF_INFO_3",               0x5,                        0x7000,                         0x18,                        5},
        {"PP1_SST_TF_INFO_4",               0x5,                        0x7000,                         0x20,                        5},
        {"PP1_SST_TF_INFO_5",               0x5,                        0x7000,                         0x28,                        5},
        {"PP1_SST_TF_INFO_6",               0x5,                        0x7000,                         0x30,                        5},
        {"PP1_SST_TF_INFO_7",               0x5,                        0x7000,                         0x38,                        5},
        {"PP2_SST_PP_INFO_0",               0x5,                        0x7000,                         0x0,                         3},
        {"PP2_SST_PP_INFO_1",               0x5,                        0x7000,                         0x8,                         3},
        {"PP2_SST_PP_INFO_2",               0x5,                        0x7000,                         0x10,                        3},
        {"PP2_SST_PP_INFO_3",               0x5,                        0x7000,                         0x18,                        3},
        {"PP2_SST_PP_INFO_4",               0x5,                        0x7000,                         0x20,                        3},
        {"PP2_SST_PP_INFO_5",               0x5,                        0x7000,                         0x28,                        3},
        {"PP2_SST_PP_INFO_6",               0x5,                        0x7000,                         0x30,                        3},
        {"PP2_SST_PP_INFO_7",               0x5,                        0x7000,                         0x38,                        3},
        {"PP2_SST_PP_INFO_8",               0x5,                        0x7000,                         0x40,                        3},
        {"PP2_SST_PP_INFO_9",               0x5,                        0x7000,                         0x48,                        3},
        {"PP2_SST_PP_INFO_10",              0x5,                        0x7000,                         0x50,                        3},
        {"PP2_SST_PP_INFO_11",              0x5,                        0x7000,                         0x58,                        3},
        {"PP2_SST_BF_INFO_0",               0x5,                        0x7000,                         0x0,                         4},
        {"PP2_SST_BF_INFO_1",               0x5,                        0x7000,                         0x8,                         4},
        {"PP2_SST_TF_INFO_0",               0x5,                        0x7000,                         0x0,                         5},
        {"PP2_SST_TF_INFO_1",               0x5,                        0x7000,                         0x8,                         5},
        {"PP2_SST_TF_INFO_2",               0x5,                        0x7000,                         0x10,                        5},
        {"PP2_SST_TF_INFO_3",               0x5,                        0x7000,                         0x18,                        5},
        {"PP2_SST_TF_INFO_4",               0x5,                        0x7000,                         0x20,                        5},
        {"PP2_SST_TF_INFO_5",               0x5,                        0x7000,                         0x28,                        5},
        {"PP2_SST_TF_INFO_6",               0x5,                        0x7000,                         0x30,                        5},
        {"PP2_SST_TF_INFO_7",               0x5,                        0x7000,                         0x38,                        5},
        {"PP3_SST_PP_INFO_0",               0x5,                        0x7000,                         0x0,                         3},
        {"PP3_SST_PP_INFO_1",               0x5,                        0x7000,                         0x8,                         3},
        {"PP3_SST_PP_INFO_2",               0x5,                        0x7000,                         0x10,                        3},
        {"PP3_SST_PP_INFO_3",               0x5,                        0x7000,                         0x18,                        3},
        {"PP3_SST_PP_INFO_4",               0x5,                        0x7000,                         0x20,                        3},
        {"PP3_SST_PP_INFO_5",               0x5,                        0x7000,                         0x28,                        3},
        {"PP3_SST_PP_INFO_6",               0x5,                        0x7000,                         0x30,                        3},
        {"PP3_SST_PP_INFO_7",               0x5,                        0x7000,                         0x38,                        3},
        {"PP3_SST_PP_INFO_8",               0x5,                        0x7000,                         0x40,                        3},
        {"PP3_SST_PP_INFO_9",               0x5,                        0x7000,                         0x48,                        3},
        {"PP3_SST_PP_INFO_10",              0x5,                        0x7000,                         0x50,                        3},
        {"PP3_SST_PP_INFO_11",              0x5,                        0x7000,                         0x58,                        3},
        {"PP3_SST_BF_INFO_0",               0x5,                        0x7000,                         0x0,                         4},
        {"PP3_SST_BF_INFO_1",               0x5,                        0x7000,                         0x8,                         4},
        {"PP3_SST_TF_INFO_0",               0x5,                        0x7000,                         0x0,                         5},
        {"PP3_SST_TF_INFO_1",               0x5,                        0x7000,                         0x8,                         5},
        {"PP3_SST_TF_INFO_2",               0x5,                        0x7000,                         0x10,                        5},
        {"PP3_SST_TF_INFO_3",               0x5,                        0x7000,                         0x18,                        5},
        {"PP3_SST_TF_INFO_4",               0x5,                        0x7000,                         0x20,                        5},
        {"PP3_SST_TF_INFO_5",               0x5,                        0x7000,                         0x28,                        5},
        {"PP3_SST_TF_INFO_6",               0x5,                        0x7000,                         0x30,                        5},
        {"PP3_SST_TF_INFO_7",               0x5,                        0x7000,                         0x38,                        5},
        {"PP4_SST_PP_INFO_0",               0x5,                        0x7000,                         0x0,                         3},
        {"PP4_SST_PP_INFO_1",               0x5,                        0x7000,                         0x8,                         3},
        {"PP4_SST_PP_INFO_2",               0x5,                        0x7000,                         0x10,                        3},
        {"PP4_SST_PP_INFO_3",               0x5,                        0x7000,                         0x18,                        3},
        {"PP4_SST_PP_INFO_4",               0x5,                        0x7000,                         0x20,                        3},
        {"PP4_SST_PP_INFO_5",               0x5,                        0x7000,                         0x28,                        3},
        {"PP4_SST_PP_INFO_6",               0x5,                        0x7000,                         0x30,                        3},
        {"PP4_SST_PP_INFO_7",               0x5,                        0x7000,                         0x38,                        3},
        {"PP4_SST_PP_INFO_8",               0x5,                        0x7000,                         0x40,                        3},
        {"PP4_SST_PP_INFO_9",               0x5,                        0x7000,                         0x48,                        3},
        {"PP4_SST_PP_INFO_10",              0x5,                        0x7000,                         0x50,                        3},
        {"PP4_SST_PP_INFO_11",              0x5,                        0x7000,                         0x58,                        3},
        {"PP4_SST_BF_INFO_0",               0x5,                        0x7000,                         0x0,                         4},
        {"PP4_SST_BF_INFO_1",               0x5,                        0x7000,                         0x8,                         4},
        {"PP4_SST_TF_INFO_0",               0x5,                        0x7000,                         0x0,                         5},
        {"PP4_SST_TF_INFO_1",               0x5,                        0x7000,                         0x8,                         5},
        {"PP4_SST_TF_INFO_2",               0x5,                        0x7000,                         0x10,                        5},
        {"PP4_SST_TF_INFO_3",               0x5,                        0x7000,                         0x18,                        5},
        {"PP4_SST_TF_INFO_4",               0x5,                        0x7000,                         0x20,                        5},
        {"PP4_SST_TF_INFO_5",               0x5,                        0x7000,                         0x28,                        5},
        {"PP4_SST_TF_INFO_6",               0x5,                        0x7000,                         0x30,                        5},
        {"PP4_SST_TF_INFO_7",               0x5,                        0x7000,                         0x38,                        5},
        {"TPMI_INFO_HEADER",                0x81,                       0xd000,                         0x0,                         0},
        {"TPMI_BUS_INFO",                   0x81,                       0xd000,                         0x8,                         0},
        {"FHM_HEADER",                      0xA,                        0x9000,                         0x0,                         0},
        {"FHM_CONTROL",                     0xA,                        0x9000,                         0x8,                         0},
        {"FHM_PMT_INFO",                    0xA,                        0x9000,                         0x10,                        0},
        {"FHM_RSVD1",                       0xA,                        0x9000,                         0x18,                        0},
        {"FHM_RSVD2",                       0xA,                        0x9000,                         0x20,                        0},
        {"PMAX_HEADER",                     0x3,                        0x5000,                         0x0,                         0},
        {"PMAX_CONTROL",                    0x3,                        0x5000,                         0x8,                         0},
        {"PMAX_STATUS",                     0x3,                        0x5000,                         0x10,                        0},
        {"MISC_CTRL_HEADER",                0x6,                        0xa000,                         0x0,                         0},
        {"MISC_CTRL_DESC",                  0x6,                        0xa000,                         0x8,                         0},
        {"PROCHOT_RESPONSE_POWER",          0x6,                        0xa000,                         0x0,                         0},
        {"PLR_HEADER",                      0xC,                        0xb000,                         0x0,                         0},
        {"PLR_MAILBOX_INTERFACE",           0xC,                        0xb000,                         0x8,                         0},
        {"PLR_MAILBOX_DATA",                0xC,                        0xb000,                         0x10,                        0},
        {"PLR_DIE_LEVEL",                   0xC,                        0xb000,                         0x18,                        0},
        {"PLR_RSVD",                        0xC,                        0xb000,                         0x20,                        0},
        {"BMC_CTL_HEADER",                  0xD,                        0xc000,                         0x0,                         0},
        {"BMC_CTL_MAILBOX_INTERFACE",       0xD,                        0xc000,                         0x8,                         0},
        {"BMC_CTL_MAILBOX_DATA",            0xD,                        0xc000,                         0x10,                        0},
};

static void usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s [-d BDF] [-r region_id] [-h] \n\n", prog_name);
    fprintf(stderr, "Command Option Description:\n");
    fprintf(stderr, "       [-d BDF]       BDF format should be 'DDDD:BB:DD.F' which staged under /sys/bus/pci/devices, default: 0000:00:03.1\n");
    fprintf(stderr, "       [-r region_id] Use to identify region to stage TPMI regs which showed via 'lspci -vvvv -s TPMI_BDF', default: 1\n");
    fprintf(stderr, "       [-h]           Print command usage\n");
}

void get_tpmi_pfs(uint64_t tpmi_id, tpmi_pfs_t *tpmi_pfs)
{
    int i;

    for (i = 0; i < NUM_TPMI_PFS; i++) {
        if (TPMI_PFS_MAP[i].tpmi_id == tpmi_id) {
            tpmi_pfs->tpmi_id = TPMI_PFS_MAP[i].tpmi_id;
            tpmi_pfs->entry_num = TPMI_PFS_MAP[i].entry_num;
            tpmi_pfs->entry_size = TPMI_PFS_MAP[i].entry_size;
            return;
        }
    }

}

void region_dump(mmio_region_t *mmio_region)
{
    int fd;
    void *mmap_base;
    FILE *fp;
    char region_path[PATH_MAX];
    uint64_t region_size = mmio_region->end_addr - mmio_region->start_addr + 1;
    char *tpmi_bdf = tpmi_actual_bdf != NULL ? tpmi_actual_bdf : tpmi_default_bdf;

    snprintf(region_path, sizeof(region_path), "/sys/bus/pci/devices/%s/resource%d", tpmi_bdf, tpmi_region_id);
    fd = open(region_path, O_RDWR);
    if (fd < 0) {
        printf("ERROR: Could not open %s due to \"%s\"\n", region_path, strerror(errno));
        return;
    }

    mmap_base = mmap(NULL, region_size, PROT_READ, MAP_SHARED, fd, 0);
    if (mmap_base == MAP_FAILED) {
        printf("ERROR: MMAP failed due to \"%s\"\n", strerror(errno));
        goto exit;
    }

    fp = fopen(tpmi_dump_file, "wb+");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: Couldn't open %s due to \"%s\"\n", tpmi_dump_file, strerror(errno));
        goto exit;
    }
    fwrite(mmap_base, 1, region_size, fp);
    fclose(fp);

exit:
    munmap(mmap_base, region_size);
    close(fd);
}

int pattern_check(const char *pattern, char *content)
{
    const char *error;
    int erroffset;
    pcre *re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
    if (re == NULL) {
        printf("PCRE compilation failed at offset %d: %s\n", erroffset, error);
        return 1;
    }

    int ovector[30];
    int rc = pcre_exec(re, NULL, content, strlen(content), 0, 0, ovector, sizeof(ovector));
    if (rc >= 0) {
//printf("Match found: '%.*s'\n", ovector[1] - ovector[0], content + ovector[0]);
        return 0;
    } else if (rc == PCRE_ERROR_NOMATCH) {
//printf("No match found for pattern: %s.\n", pattern);
        return 1;
    } else {
        printf("PCRE matching error: %d\n", rc);
        return -1;
    }

}
void parse_tpmi_dump(mmio_region_t *mmio_region)
{
    FILE *fp;
    int i, j;
    // int fd, s_fd, n_fd;
    uint64_t tpmi_reg_value;
    uint64_t region_size = mmio_region->end_addr - mmio_region->start_addr + 1;
    char tpmi_regs_buffer[region_size];
    tpmi_pfs_t tpmi_pfs = {0x0, 1, 96};

    fp = fopen(tpmi_dump_file, "rb");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: Couldn't open %s due to \"%s\"\n", tpmi_dump_file, strerror(errno));
        return;
    }
    fread(tpmi_regs_buffer, 1, region_size, fp);
    fclose(fp);

    // fd = open("tpmi_regs.txt", O_WRONLY | O_CREAT | O_TRUNC);
    // if (fd < 0) {
    //     fprintf(stderr, "ERROR: Couldn't open tpmi_regs.txt due to \"%s\"\n", strerror(errno));
    //     return;
    // }
    fp = fopen("tpmi_regs.txt", "w+");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: Couldn't open tpmi_regs.txt due to \"%s\"\n", strerror(errno));
        return;
    }

    printf("=========================================REG DUMP IN PROGRESS===========================================\n");
    printf("Please wait for a moment......\n\n");
    // sleep(1);
    // s_fd = dup(STDOUT_FILENO);
    // n_fd = dup2(fd, STDOUT_FILENO);
    printf("============================================TPMI REG LIST===============================================\n");
    fprintf(fp, "==============================================TPMI REG LIST=============================================\n");
    printf("Number\t|\t%-*s\t|\t%-*s\t|\tAddress|TPMI_REG_VALUE\n", 30, "TPMI_REG_NAME", 10, "INSTANCE_ID");
    fprintf(fp, "Number\t|\t%-*s\t|\t%-*s\t|\tAddress|TPMI_REG_VALUE\n", 30, "TPMI_REG_NAME", 10, "INSTANCE_ID");
    printf("--------------------------------------------------------------------------------------------------------\n");
    fprintf(fp, "--------------------------------------------------------------------------------------------------------\n");

    for (i = 0; i < NUM_TPMI_REG; i++) {
        get_tpmi_pfs(TPMI_REG_MAP[i].tpmi_id, &tpmi_pfs);

        // Translate instance id and offset via tpmi_pfs
        tpmi_instance_t tpmi_instance[tpmi_pfs.entry_num];
        switch (tpmi_pfs.entry_num) {
            case 1:
                tpmi_instance[0].id = 0;
                tpmi_instance[0].offset = 0x0;
                tpmi_instance[0].register_offset = 0x0;
                break;
            case 2:
                tpmi_instance[0].id = 1;
                tpmi_instance[0].offset = 0x0;
                tpmi_instance[0].register_offset = 0x0;
                tpmi_instance[1].id = 2;
                tpmi_instance[1].offset = 4 * tpmi_pfs.entry_size;
                tpmi_instance[1].register_offset = 0x0;
                break;
            case 3:
                tpmi_instance[0].id = 1;
                tpmi_instance[0].offset = 0x0;
                tpmi_instance[0].register_offset = 0x0;
                tpmi_instance[1].id = 2;
                tpmi_instance[1].offset = 4 * tpmi_pfs.entry_size;
                tpmi_instance[1].register_offset = 0x0;
                tpmi_instance[2].id = 3;
                tpmi_instance[2].offset = 4 * tpmi_pfs.entry_size * 2;
                tpmi_instance[2].register_offset = 0x0;
                break;
            case 5:
                tpmi_instance[0].id = 0;
                tpmi_instance[0].offset = 4 * tpmi_pfs.entry_size * 4;
                tpmi_instance[0].register_offset = 0x0;
                tpmi_instance[1].id = 1;
                tpmi_instance[1].offset = 0x0;
                tpmi_instance[1].register_offset = 0x0;
                tpmi_instance[2].id = 2;
                tpmi_instance[2].offset = 4 * tpmi_pfs.entry_size;
                tpmi_instance[2].register_offset = 0x0;
                tpmi_instance[3].id = 3;
                tpmi_instance[3].offset = 4 * tpmi_pfs.entry_size * 2;
                tpmi_instance[3].register_offset = 0x0;
                tpmi_instance[4].id = 4;
                tpmi_instance[4].offset = 4 * tpmi_pfs.entry_size * 3;
                tpmi_instance[4].register_offset = 0x0;
                switch (tpmi_pfs.tpmi_id) {
                    case 2:
                        if (TPMI_REG_MAP[i].sub_id == 0x1){
                            uint64_t ufs_fabric_cluster_offset[5];
                            for (int n=0; n < 5; n++){
                                ufs_fabric_cluster_offset[n] = *(uint64_t *)(tpmi_regs_buffer + TPMI_REG_MAP[i].cap_offset + 0x8 + tpmi_instance[n].offset);
                                tpmi_instance[n].register_offset = ufs_fabric_cluster_offset[n] * 8;
                            }
                        }
                        break;
                    case 5:
                        //uint64_t instance[5] = {254*4*4, 0,  254*4, 254*4*2, 254*4*3};
                        uint64_t sst_header[5];
                        uint64_t sst_pp_offset_0[5];
                        uint64_t sst_pp_offset_1[5];
                        uint64_t sst_header_sst_cp_offset[5];
                        uint64_t sst_header_sst_pp_offset[5];
                        uint64_t sst_pp_offset[5];
                        uint64_t sst_pp_bf_offset[5];
                        uint64_t sst_pp_tf_offset[5];
                        uint64_t pp0[5];
                        uint64_t pp1[5];
                        uint64_t pp2[5];
                        uint64_t pp3[5];
                        uint64_t pp4[5];
                        uint64_t mask0_7 = 0xFF;
                        uint64_t mask8_15 = 0xFF00;
                        uint64_t mask16_23 = 0xFF0000;
                        uint64_t mask24_31 = 0xFF000000;
                        uint64_t mask32_39 = 0xFF00000000;
                        const char *pattern_1 = "PP1_*";
                        const char *pattern_2 = "PP2_*";
                        const char *pattern_3 = "PP3_*";
                        const char *pattern_4 = "PP4_*";

                        for (int n=0; n < 5; n++){
                            sst_header[n] = *(uint64_t *)(tpmi_regs_buffer + TPMI_REG_MAP[i].cap_offset + tpmi_instance[n].offset);
                            sst_header_sst_cp_offset[n] = (sst_header[n] & mask16_23) >> 16;
                            sst_header_sst_pp_offset[n] = (sst_header[n] & mask24_31) >> 24;
                            sst_pp_offset_0[n] =  *(uint64_t *)(tpmi_regs_buffer + TPMI_REG_MAP[i].cap_offset + sst_header_sst_pp_offset[n]*8 + 0x8 + tpmi_instance[n].offset);
                            sst_pp_offset_1[n] =  *(uint64_t *)(tpmi_regs_buffer + TPMI_REG_MAP[i].cap_offset + sst_header_sst_pp_offset[n]*8 + 0x10 + tpmi_instance[n].offset);
                            sst_pp_offset[n] = sst_pp_offset_0[n] & mask0_7;
                            sst_pp_bf_offset[n] = (sst_pp_offset_0[n] & mask8_15) >> 8;
                            sst_pp_tf_offset[n] = (sst_pp_offset_0[n] & mask16_23) >> 16;
                            pp0[n] = (sst_pp_offset_1[n] & mask0_7);
                            pp1[n] = (sst_pp_offset_1[n] & mask8_15) >> 8;
                            pp2[n] = (sst_pp_offset_1[n] & mask16_23) >> 16;
                            pp3[n] = (sst_pp_offset_1[n] & mask24_31) >> 24;
                            pp4[n] = (sst_pp_offset_1[n] & mask32_39) >> 32;
                        }

                        if (TPMI_REG_MAP[i].sub_id == 1){
                            for (int n=0; n < 5; n++){
                                tpmi_instance[n].register_offset = sst_header_sst_cp_offset[n] * 8;
                            }
                        } else if (TPMI_REG_MAP[i].sub_id == 2){
                            for (int n=0; n < 5; n++){
                                tpmi_instance[n].register_offset = sst_header_sst_pp_offset[n] * 8;
                            }
                        } else if (TPMI_REG_MAP[i].sub_id == 3){
                            for (int n=0; n < 5; n++){
                                int r = pattern_check(pattern_1, TPMI_REG_MAP[i].name);
                                if (r == 0){
                                    tpmi_instance[n].register_offset = sst_header_sst_pp_offset[n] * 8 + pp1[n] * 8 + sst_pp_offset[n] * 8;
                                }else{
                                    int r = pattern_check(pattern_2, TPMI_REG_MAP[i].name);
                                    if (r == 0){
                                        tpmi_instance[n].register_offset = sst_header_sst_pp_offset[n] * 8 + pp2[n] * 8 + sst_pp_offset[n] * 8;
                                    }else{
                                        int r = pattern_check(pattern_3, TPMI_REG_MAP[i].name);
                                        if (r == 0){
                                            tpmi_instance[n].register_offset = sst_header_sst_pp_offset[n] * 8 + pp3[n] * 8 + sst_pp_offset[n] * 8;
                                        }else{
                                            int r = pattern_check(pattern_4, TPMI_REG_MAP[i].name);
                                            if (r == 0){
                                                tpmi_instance[n].register_offset = sst_header_sst_pp_offset[n] * 8 + pp4[n] * 8 + sst_pp_offset[n] * 8;
                                            }else{
                                                tpmi_instance[n].register_offset = sst_header_sst_pp_offset[n] * 8 + pp0[n] * 8 + sst_pp_offset[n] * 8;
                                            }
                                        }
                                    }
                                }
                            }
                        } else if (TPMI_REG_MAP[i].sub_id == 4){
                            for (int n=0; n < 5; n++){
                                int r = pattern_check(pattern_1, TPMI_REG_MAP[i].name);
                                if (r == 0){
                                    tpmi_instance[n].register_offset = sst_header_sst_pp_offset[n] * 8 + pp1[n] * 8 + sst_pp_bf_offset[n] * 8;
                                }else{
                                    int r = pattern_check(pattern_2, TPMI_REG_MAP[i].name);
                                    if (r == 0){
                                        tpmi_instance[n].register_offset = sst_header_sst_pp_offset[n] * 8 + pp2[n] * 8 + sst_pp_bf_offset[n] * 8;
                                    }else{
                                        int r = pattern_check(pattern_3, TPMI_REG_MAP[i].name);
                                        if (r == 0){
                                            tpmi_instance[n].register_offset = sst_header_sst_pp_offset[n] * 8 + pp3[n] * 8 + sst_pp_bf_offset[n] * 8;
                                        }else{
                                            int r = pattern_check(pattern_4, TPMI_REG_MAP[i].name);
                                            if (r == 0){
                                                tpmi_instance[n].register_offset = sst_header_sst_pp_offset[n] * 8 + pp4[n] * 8 + sst_pp_bf_offset[n] * 8;
                                            }else{
                                                tpmi_instance[n].register_offset = sst_header_sst_pp_offset[n] * 8 + pp0[n] * 8 + sst_pp_bf_offset[n] * 8;
                                            }
                                        }
                                    }
                                }
                            }
                        } else if (TPMI_REG_MAP[i].sub_id == 5){
                            for (int n=0; n < 5; n++){
                                int r = pattern_check(pattern_1, TPMI_REG_MAP[i].name);
                                if (r == 0){
                                    tpmi_instance[n].register_offset = sst_header_sst_pp_offset[n] * 8 + pp1[n] * 8 + sst_pp_tf_offset[n] * 8;
                                }else{
                                    int r = pattern_check(pattern_2, TPMI_REG_MAP[i].name);
                                    if (r == 0){
                                        tpmi_instance[n].register_offset = sst_header_sst_pp_offset[n] * 8 + pp2[n] * 8 + sst_pp_tf_offset[n] * 8;
                                    }else{
                                        int r = pattern_check(pattern_3, TPMI_REG_MAP[i].name);
                                        if (r == 0){
                                            tpmi_instance[n].register_offset = sst_header_sst_pp_offset[n] * 8 + pp3[n] * 8 + sst_pp_tf_offset[n] * 8;
                                        }else{
                                            int r = pattern_check(pattern_4, TPMI_REG_MAP[i].name);
                                            if (r == 0){
                                                tpmi_instance[n].register_offset = sst_header_sst_pp_offset[n] * 8 + pp4[n] * 8 + sst_pp_tf_offset[n] * 8;
                                            }else{
                                                tpmi_instance[n].register_offset = sst_header_sst_pp_offset[n] * 8 + pp0[n] * 8 + sst_pp_tf_offset[n] * 8;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        break;
                }
                break;
            default:
                printf("ERROR: Invalid TPMI_PFS_ENTRY_NUM - %ld\n", tpmi_pfs.entry_num);
                return;
        }

        for (j = 0; j < tpmi_pfs.entry_num; j ++) {
            // printf("%lx, %lx, %lx\n", TPMI_REG_MAP[i].tpmi_id, tpmi_instance[j].id, tpmi_instance[j].offset);
            uint64_t address = (uint64_t)(TPMI_REG_MAP[i].cap_offset + tpmi_instance[j].offset + TPMI_REG_MAP[i].tpmi_offset + tpmi_instance[j].register_offset);
            tpmi_reg_value = *(uint64_t *)(tpmi_regs_buffer + address);
            printf("%d\t|\t%-*s\t|\t%-*ld\t|\t0x%lx|\t0x%lx\n", i, 30, TPMI_REG_MAP[i].name, 10, tpmi_instance[j].id, address, tpmi_reg_value);
            fprintf(fp, "%d\t\t|\t%-*s\t|\t%-*ld\t|\t0x%lx|\t0x%lx\n", i, 30, TPMI_REG_MAP[i].name, 10, tpmi_instance[j].id, address, tpmi_reg_value);
            printf("--------------------------------------------------------------------------------------------------------\n");
            fprintf(fp, "--------------------------------------------------------------------------------------------------------\n");
        }
    }
    // dup2(s_fd, n_fd);
    // close(fd);
    fclose(fp);
    // sleep(1);
    printf("=================================================END====================================================\n");
}

void tpmi_dump(void)
{
    char resource_path[PATH_MAX];
    mmio_region_t mmio_region;
    uint64_t start_addr, end_addr, data;
    FILE *fp;
    int index = 0;
    char *tpmi_bdf = tpmi_actual_bdf != NULL ? tpmi_actual_bdf : tpmi_default_bdf;

    snprintf(resource_path, sizeof(resource_path), "/sys/bus/pci/devices/%s/resource", tpmi_bdf);
    // Open TPMI PCIe device resource to get MMIO region address
    fp = fopen(resource_path, "r");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: Couldn't open %s due to \"%s\"\n", resource_path, strerror(errno));
        return;
    }

    while (fscanf(fp, "%lx %lx %lx\n", &start_addr, &end_addr, &data) == 3) {
        if ( start_addr != 0 && end_addr != 0 && index == tpmi_region_id ) {
            printf("INFO: Find valid TPMI MMIO region from %lx to %lx\n", start_addr, end_addr);
            mmio_region.start_addr = start_addr;
            mmio_region.end_addr = end_addr;
            break;
        }
        index++;
    }
    fclose(fp);

    if (mmio_region.start_addr == 0 && mmio_region.end_addr == 0) {
        printf("INFO: No valid TPMI MMIO region found!");
        return;
    }

    // Start to dump TPMI MMIO region
    region_dump(&mmio_region);

    // Start to parse TPMI dump
    parse_tpmi_dump(&mmio_region);
}

int main(int argc, char *argv[])
{
    int opt;
    uint32_t domain, bus, dev, func;
    const char *optstring = "hr:d:";

    while ((opt = getopt(argc, argv, optstring)) != -1) {
        switch (opt) {
            case 'd':
                if (sscanf((char *)optarg, "%4x:%2x:%2x.%x", &domain, &bus, &dev, &func) == 4) {
                    tpmi_actual_bdf = (char *)optarg;
                    printf("INFO: Get new TPMI_BDF -- %s\n", tpmi_actual_bdf);
                } else {
                    printf("ERROR: %s - Invalid BDF format\n", (char *)optarg);
                    usage(argv[0]);
                    return 0;
                }
                break;
            case 'r':
                tpmi_region_id = (uint32_t)strtoul(optarg, NULL, 0);
                printf("INFO: Get new TPMI_REGION_ID -- %d\n", tpmi_region_id);
                break;
            case 'h':
            default:
                usage(argv[0]);
                return 0;
        }
    }

    tpmi_dump();

    return 0;
}
