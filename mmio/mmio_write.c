#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/mman.h>

typedef struct mmio_region {
    uint64_t start_addr;
    uint64_t end_addr;
} mmio_region_t;

#define __get_unaligned_t(type, ptr) ({                                                     \
    const struct __attribute__ ((__packed__)) { type x; } *__pptr = (typeof(__pptr))(ptr);  \
    __pptr->x;                                                                              \
})

#define get_unaligned(ptr)  __get_unaligned_t(typeof(*(ptr)), (ptr))

static uint32_t region_id = 0;
static char *default_bdf = "0000:00:00.0";
static char *actual_bdf = NULL;
static char *mmio_reg_bin = NULL;
static char *mmio_reg_bin_bak = "tpmi_dump.bin.bak";

static void usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s [-d BDF] [-r region_id] [-f bin_file] [-b original_file] [-h] \n\n", prog_name);
    fprintf(stderr, "Command Option Description:\n");
    fprintf(stderr, "       [-d BDF]       BDF format should be 'DDDD:BB:DD.F' which staged under /sys/bus/pci/devices, default: 0000:00:03.1\n");
    fprintf(stderr, "       [-r region_id] Target region which required to be writen and could be obtained via 'lspci -vvvv -s BDF', default: 0\n");
    fprintf(stderr, "       [-f bin_file]  BIN file used to overwrite MMIO registers\n");
    fprintf(stderr, "       [-b bin_file]  The original bin file\n");
    fprintf(stderr, "       [-h]           Print command usage\n");
}

static uint32_t get_mmio_bin_size(char *file_path)
{
    uint32_t size = 0;
    FILE *fp = fopen(file_path, "rb");

    if (fp == NULL) {
        fprintf(stderr, "ERROR: Couldn't open %s due to \"%s\"\n", mmio_reg_bin, strerror(errno));
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fclose(fp);
    printf("INFO: The file size of %s is 0x%x\n", file_path, size);
    return size;
}

static uint64_t get_region_size(void)
{
    char resource_path[PATH_MAX];
    uint64_t start_addr, end_addr, data;
    uint64_t region_size = 0;
    FILE *fp;
    int index = 0;
    char *bdf = actual_bdf != NULL ? actual_bdf : default_bdf;

    snprintf(resource_path, sizeof(resource_path), "/sys/bus/pci/devices/%s/resource", bdf);
    // Open TPMI PCIe device resource to get MMIO region address
    fp = fopen(resource_path, "r");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: Couldn't open %s due to \"%s\"\n", resource_path, strerror(errno));
        return 0;
    }

    while (fscanf(fp, "%lx %lx %lx\n", &start_addr, &end_addr, &data) == 3) {
        if (index == region_id) {
            region_size = end_addr - start_addr + 1;
            printf("INFO: Target MMIO region from %lx to %lx, Size: 0x%lx\n", start_addr, end_addr, region_size);
            break;
        }
        index++;
    }
    fclose(fp);

    return region_size;
}

static void mmio_write(void)
{
    char region_path[PATH_MAX];
    uint64_t region_size = 0;
    uint32_t mmio_bin_size = 0;
    FILE *fp, *fp1;
    int fd;
    int index = 0;
    char *bdf = actual_bdf != NULL ? actual_bdf : default_bdf;
    void *mmap_base;

    // Read MMIO field changes from MMIO BIN file
    mmio_bin_size = get_mmio_bin_size(mmio_reg_bin);
    if (mmio_bin_size == 0)
    {
        fprintf(stderr, "ERROR: Failed to get BIN size with %s due to \"%s\"\n", mmio_reg_bin, strerror(errno));
        return;
    }

    fp = fopen(mmio_reg_bin, "rb");
    fp1 = fopen(mmio_reg_bin_bak, "rb");
    if (fp == NULL || fp1 == NULL) {
        char *file = (fp == NULL) ? "mmio_reg_bin_bak" : "mmio_reg_bin";
        fprintf(stderr, "ERROR: Couldn't open %s due to \"%s\"\n", file, strerror(errno));
        return;
    }
    unsigned char *new_mmio_reg_values = (unsigned char *)malloc(mmio_bin_size);
    unsigned char *mmio_bak_head = (unsigned char *)malloc(mmio_bin_size);
    unsigned char *mmio_reg_ptr;
    unsigned char *new_mmio_reg_ptr = new_mmio_reg_values;
    unsigned char *mmio_bak = mmio_bak_head;
    fread(new_mmio_reg_ptr, sizeof(unsigned char), mmio_bin_size, fp);
    fread(mmio_bak, sizeof(unsigned char), mmio_bin_size, fp1);
    fclose(fp);
    fclose(fp1);

    // Open PCIe device resource to get MMIO region data
    region_size = get_region_size();
    snprintf(region_path, sizeof(region_path), "/sys/bus/pci/devices/%s/resource%d", bdf, region_id);
    fd = open(region_path, O_RDWR);
    if (fd < 0) {
        printf("ERROR: Could not open %s due to \"%s\"\n", region_path, strerror(errno));
        return;
    }

    mmap_base = mmap(NULL, region_size, PROT_WRITE, MAP_SHARED, fd, 0);
    if (mmap_base == MAP_FAILED) {
        printf("ERROR: MMAP failed due to \"%s\"\n", strerror(errno));
        goto exit;
    }

    // Start to overwrite MMIO regs
    mmio_reg_ptr = (unsigned char *)mmap_base;
    while (index < region_size && index < mmio_bin_size) {
        //printf("INFO: Checking REG#0x%08x, OLD_VALUE->0x%02x, NEW_VALUE->0x%02x \n", index, *mmio_reg_ptr, *new_mmio_reg_ptr);
        if (*mmio_bak != *new_mmio_reg_ptr) {
            printf("INFO: Try to change REG#0x%08x from 0x%02x to 0x%02x\n", index, *mmio_bak, *new_mmio_reg_ptr);
            memcpy(mmio_reg_ptr, new_mmio_reg_ptr, 1);
        }
        index++;
        mmio_reg_ptr++;
        new_mmio_reg_ptr++;
        mmio_bak++;
    }

exit:
    munmap(mmap_base, region_size);
    close(fd);
    free(new_mmio_reg_values);
    free(mmio_bak_head);
}

int main(int argc, char *argv[])
{
    int opt;
    uint32_t domain, bus, dev, func;
    const char *optstring = "hr:d:f:b:";

    if (argc < 2) {
        usage(argv[0]);
        return -1;
    }

    while ((opt = getopt(argc, argv, optstring)) != -1) {
        switch (opt) {
            case 'd':
                if (sscanf((char *)optarg, "%4x:%2x:%2x.%x", &domain, &bus, &dev, &func) == 4) {
                    actual_bdf = (char *)optarg;
                    printf("INFO: Get new TPMI_BDF -- %s\n", actual_bdf);
                } else {
                    printf("ERROR: %s - Invalid BDF format\n", (char *)optarg);
                    usage(argv[0]);
                    return 0;
                }
                break;
            case 'r':
                region_id = (uint32_t)strtoul(optarg, NULL, 0);
                printf("INFO: Get new TPMI_REGION_ID -- %d\n", region_id);
                break;
            case 'f':
                mmio_reg_bin = (char *)optarg;
                printf("INFO: Use %s to overwrite MMIO registers\n", mmio_reg_bin);
                break;
            case 'b':
                mmio_reg_bin_bak = (char *)optarg;
                printf("INFO: Use %s as the original bin file\n", mmio_reg_bin_bak);
                break;
            case 'h':
            default:
                usage(argv[0]);
                return 0;
        }
    }

    if (mmio_reg_bin == NULL || mmio_reg_bin_bak == NULL) {
        printf("ERROR: Please add MMIO BIN files according to following rules...");
        usage(argv[0]);
    }

    mmio_write();

    return 0;
}
