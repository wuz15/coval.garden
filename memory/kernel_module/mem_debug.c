#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/kobject.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <uapi/linux/stat.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/highmem.h>
#include <linux/vmalloc.h>

MODULE_LICENSE("GPL");

uint64_t physical_start_address = 0x1100000000; // Start from 64GB
static struct kobject *kobj_mem_debug;

// Sysfs to show current physical address to kick off mem_write
static ssize_t physical_start_address_show(struct kobject *kobj, struct kobj_attribute *attr, char *buff)
{
    return sprintf(buff, "0x%016llx\n", physical_start_address);
}

// Sysfs to change current physical address to kick off mem_write
static ssize_t physical_start_address_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buff, size_t count)
{
    int ret;
    uint64_t new_physical_start_address = 0;

    ret = kstrtoull(buff, 16, &new_physical_start_address);
    if (ret) {
        printk(KERN_INFO "Failed to update physical_start_address\n");
        return count;
    }

    physical_start_address = new_physical_start_address;

    return count;
}

static int mem_write(void *arg)
{
    uint8_t mem_src_data[2 * PAGE_SIZE];
    int index = 0;
    uint64_t pfn = physical_start_address / PAGE_SIZE;
    struct page *page = pfn_to_page(pfn);

    // Generate random memory source data which used for memory write
    for (index = 0; index < 2 * PAGE_SIZE; index++) {
        get_random_bytes(&mem_src_data[index], sizeof(uint8_t));
        // printk(KERN_INFO "MEM_SRC_DATA#0x%08x: 0x%x\n", index, mem_src_data[index]);
    }

    void *kmap_address = kmap(page);
    while (!kthread_should_stop()) {
        index = 0;
        while (index < 100) {
            memcpy(kmap_address, mem_src_data + index, 0x1000);
            // printk(KERN_INFO "DST_DATA: 0x%08x, SRC_DATA: 0x%08x\n", *(uint32_t *)kmap_address, *(uint32_t *)(mem_src_data + index));
            asm volatile (
                "wbinvd\n\t"
                ::);
            index++;
            msleep(1);
        }
    }

    kunmap(page);
    printk(KERN_INFO "Terminate kthread to end mem_write from physical_adress: 0x%016llx\n", physical_start_address);
    return 0;
}

// Sysfs to trigger mem_write request
static ssize_t mem_write_request_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buff, size_t count)
{
    struct task_struct *k_thread;

    k_thread = kthread_create(mem_write, NULL, "mem_write_kthread");
    if (k_thread == NULL) {
        printk(KERN_INFO "Failed to create kthread to start mem_write from physical_address: 0x%016llx\n", physical_start_address);
        return count;
    }

    wake_up_process(k_thread);
    printk(KERN_INFO "K_THREAD#%d was created successfully to start mem_write from physical_address: 0x%016llx\n", k_thread->pid, physical_start_address);

    return count;
}

// Sysfs to trigger mem_dump request
static ssize_t mem_dump_show(struct kobject *kobj, struct kobj_attribute *attr, char *buff)
{
    uint64_t pfn = physical_start_address / PAGE_SIZE;
    struct page *page = pfn_to_page(pfn);
    void *kmap_address = kmap(page);
    uint32_t count = 0;
    uint32_t index = 0;

    
    // data = *(uint64_t *)kmap_address;
    while (index < 100) {
        count += sprintf(buff + count, "0x%016llx:\t0x%016llx\n", PAGE_SIZE * pfn + 0x8 * index, *(uint64_t *)(kmap_address + 0x8 * index));
        index++;
    }

    kunmap(page);

    return count;
}

// Define mem_debug sysfs attributes
static struct kobj_attribute physical_start_address_attribute =
    __ATTR(physical_start_address, S_IRUGO | S_IWUSR, physical_start_address_show, physical_start_address_store);

static struct kobj_attribute mem_write_request_attribute =
    __ATTR(mem_write_request, S_IRUGO | S_IWUSR, NULL, mem_write_request_store);

static struct kobj_attribute mem_dump_attribute =
    __ATTR(mem_dump, S_IRUGO | S_IWUSR, mem_dump_show, NULL);

static struct attribute *attrs[] = {
    &physical_start_address_attribute.attr,
    &mem_write_request_attribute.attr,
    &mem_dump_attribute.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = attrs,
};


// Module Init
static int __init mem_debug_init(void)
{
    int32_t ret;

    printk(KERN_INFO "Loading kernel module mem_debug\n");

    kobj_mem_debug = kobject_create_and_add("mem_debug", kernel_kobj);
    if (!kobj_mem_debug) {
        printk(KERN_INFO "Failed to create kernel object for mem_debug module\n");
        return -1;
    }

    ret = sysfs_create_group(kobj_mem_debug, &attr_group);
    if (ret) {
        printk(KERN_INFO "Failed to create kernel object sysfs group for mem_debug module\n");
        return -1;
    }

    printk(KERN_INFO "Kernel module mem_debug was loaded successfully\n");

    return 0;
}

// Module Exit
static void __exit mem_debug_exit(void)
{
    printk(KERN_INFO "Trying to remove kernel module mem_debug\n");

    kobject_put(kobj_mem_debug);

    printk(KERN_INFO "Kernel module mem_debug was removed successfully\n");
}

module_init(mem_debug_init);
module_exit(mem_debug_exit);