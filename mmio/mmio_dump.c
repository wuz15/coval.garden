#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/mman.h>

#define IORESOURCE_MEM        0x1

typedef struct mmio_region {
    uint64_t start_addr;
    uint64_t end_addr;
} mmio_region_t;

static void usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s BDF \n\n", prog_name);
    fprintf(stderr, "BDF format should be 'DDDD:BB:DD.F' which staged under /sys/bus/pci/devices\n\n");
}

void region_dump(mmio_region_t *mmio_region)
{
    int fd;
    void *mmap_base;
    FILE *fp;
    char output_file_path[PATH_MAX];
    uint64_t region_size = mmio_region->end_addr - mmio_region->start_addr + 1;

    fd = open("/dev/mem", O_RDWR);
    if (fd < 0) {
        printf("ERROR: Could not open /dev/mem due to \"%s\"\n", strerror(errno));
        return;
    }

    mmap_base = mmap(NULL, region_size, PROT_READ, MAP_SHARED, fd, mmio_region->start_addr);
    if (mmap_base == MAP_FAILED) {
        printf("ERROR: MMAP failed due to \"%s\"\n", strerror(errno));
        goto exit;
    }

    snprintf(output_file_path, sizeof(output_file_path), "mmio_dump_%lx.bin", mmio_region->start_addr);
    fp = fopen(output_file_path, "wb+");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: Couldn't open %s due to \"%s\"\n", output_file_path, strerror(errno));
        goto exit;
    }
    fwrite(mmap_base, 1, region_size, fp);
    fclose(fp);

exit:
    munmap(mmap_base, region_size);
    close(fd);
}

void mmio_dump(const char *bdf)
{
    char resource_path[PATH_MAX];
    mmio_region_t mmio_regions[16];
    uint64_t start_addr, end_addr, data;
    FILE *fp;
    int index = 0;

    snprintf(resource_path, sizeof(resource_path), "/sys/bus/pci/devices/%s/resource", bdf);
    // Open PCIe device resource to get MMIO region address
    fp = fopen(resource_path, "r");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: Couldn't open %s due to \"%s\"\n", resource_path, strerror(errno));
        return;
    }

    memset(mmio_regions, 0, sizeof(mmio_regions));
    while (fscanf(fp, "%lx %lx %lx\n", &start_addr, &end_addr, &data) == 3) {
        if ( start_addr != 0 && end_addr != 0 && (data & IORESOURCE_MEM) == 0) {
            printf("INFO: Find valid MMIO region from %lx to %lx\n", start_addr, end_addr);
            mmio_regions[index].start_addr = start_addr;
            mmio_regions[index].end_addr = end_addr;
            index++;
        }
    }
    fclose(fp);

    if (index == 0 && mmio_regions[0].start_addr == 0 && mmio_regions[0].end_addr == 0) {
        printf("INFO: No valid MMIO region found!");
        return;
    }

    // Start to dump valid MMIO region
    index = 0;
    while (mmio_regions[index].start_addr != 0 && mmio_regions[index].end_addr != 0) {
        printf("INFO: Start to dump MMIO region from %lx to %lx\n", mmio_regions[index].start_addr, mmio_regions[index].end_addr);
        region_dump(&mmio_regions[index]);
        printf("INFO: Dump successfully\n");
        index++;
    }
}

int main(int argc, char *argv[])
{
    int opt;
    uint32_t domain, bus, dev, func;
    const char *optstring = "h";

    if (argc < 2) {
        usage(argv[0]);
        return -1;
    }

    while ((opt = getopt(argc, argv, optstring)) != -1) {
        switch (opt) {
            case 'h':
            default:
                usage(argv[0]);
                break;
        }
    }

    if (sscanf((char *)argv[1], "%4x:%2x:%2x.%x", &domain, &bus, &dev, &func) == 4) {
        printf("INFO: PCIe device BDF -- %s\n", argv[1]);
    } else {
        printf("ERROR: %s - Invalid BDF format\n", (char *)argv[1]);
        usage(argv[0]);
        return 0;
    }

    mmio_dump(argv[1]);

    return 0;
}
