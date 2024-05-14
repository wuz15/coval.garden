/* 
 * PTUSYS - the purpose of this driver is to add ability to access system physical memory spaces.
 * 
 * Use of this source code is governed by a GPL license that can be
 * found in the LICENSE file.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/kallsyms.h>
#include <linux/moduleparam.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/tty.h>
#include <linux/ptrace.h>
#include <linux/device.h>
#include <linux/highmem.h>
//#include <linux/bootmem.h>
#include <linux/pfn.h>
#include <linux/version.h>

#ifdef CONFIG_IA64
# include <linux/efi.h>
#endif

#define  DEVICE_NAME "ptusys"   	///< The device will appear as /dev/ptusys
#define  CLASS_NAME  "ptusys_class"   ///< This is a character device driver class
#define PTUSYS_MINOR 1

MODULE_LICENSE("GPL");

static int major_number;
static struct class* ptusysClass = NULL; ///< The device-driver class struct pointer
static struct device* ptusysDevice = NULL; ///< The device-driver device struct pointer

/*
 * This is a stripped version of linux/drivers/char/mem.c and it supports
 * only memory mapping operation.
 */

static const struct vm_operations_struct mmap_mem_ops = { .access =
		generic_access_phys };

/** @brief The standard mmap_mem function from mem.c
 *  This function stripped down to map any given physical address range in system
 *
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param vma A pointer to an vm_area_struct object (defined in linux/mm_types.h)
 *
 *  @return returns 0 if successful
 *
 * */
static int mmap_mem(struct file * file, struct vm_area_struct * vma) {
	size_t size = vma->vm_end - vma->vm_start;
	phys_addr_t offset = (phys_addr_t) vma->vm_pgoff << PAGE_SHIFT;

	//printk("PTUSYS: mmap_mem for offset %llx, vm_start = %lx \n", offset, vma->vm_start);
	
	/* It's illegal to wrap around the end of the physical address space. */
	if (offset + (phys_addr_t) size - 1 < offset)
		return -EINVAL;

	vma->vm_page_prot = vma->vm_page_prot;
	vma->vm_ops = &mmap_mem_ops;

	/* Remap-pfn-range will mark the range VM_IO */
	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size,
			vma->vm_page_prot)) {
		printk("PTUSYS: Error in remapping given Physical Address -> Virtual Address...\n");
		return -EAGAIN;
	}
	return 0;
}

/** @brief Function  checks if the use have the right privileges to open the drive.
 *
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *
 *   @return returns 0 if successful
 */
static int open_port(struct inode * inode, struct file * filp) {
	return capable(CAP_SYS_RAWIO) ? 0 : -EPERM;
}

static const struct file_operations mem_fops = { .mmap = mmap_mem, .open =
		open_port, };

/** @brief The device open function that is called each time the device is opened
 *
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *
 *  @return
 */
static int memory_open(struct inode * inode, struct file * filp) {
	int ret = 0;
	switch (iminor(inode)) {
	case 1:
		filp->f_op = &mem_fops;
		break;

	default:
		return -ENXIO;
	}
	if (filp->f_op && filp->f_op->open)
		ret = filp->f_op->open(inode, filp);
	return ret;
}

static const struct file_operations memory_fops = {
	.open = memory_open, /* just a selector for the real open */
};

/** @brief The PTUSYS LKM initialization function
 *  This function actually creates device itself by allocating a major number
 *  for itself and registering a class. This function known to work with kernel
 *  version 2.6.26 and above.
 *  @return returns 0 if successful
 */
static int __init chr_dev_init(void) {
	printk(KERN_INFO "PTUSYS: Initializing the PTUSYS LKM\n");

	// Try to dynamically allocate a major number
	major_number = register_chrdev(0, DEVICE_NAME, &memory_fops);
	if (major_number<0) {
		printk(KERN_ALERT "PTUSYS: failed to register a major number\n");
		return major_number;
	}
	printk(KERN_INFO "PTUSYS: registered successfully with major # %d\n", major_number);

	// Register the device class
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,5,0)
	ptusysClass = class_create(CLASS_NAME);
#else
	ptusysClass = class_create(THIS_MODULE, CLASS_NAME);
#endif
	if (IS_ERR(ptusysClass)) {    // Check for error and clean up if there is
		unregister_chrdev(major_number, DEVICE_NAME);
		printk(KERN_ALERT "PTUSYS: Failed to register device class\n");
		return PTR_ERR(ptusysClass);// Correct way to return an error on a pointer
	}
	printk(KERN_INFO "PTUSYS: device class registered correctly\n");

	// Register the device driver
	ptusysDevice = device_create(ptusysClass, NULL, MKDEV(major_number, PTUSYS_MINOR),
			NULL, DEVICE_NAME);
	if (IS_ERR(ptusysDevice)) { // Clean up when there is error in creating device
		class_destroy(ptusysClass);
		unregister_chrdev(major_number, DEVICE_NAME);
		printk(KERN_ALERT "Failed to create the device\n");
		return PTR_ERR(ptusysDevice);
	}
	printk(KERN_INFO "PTUSYS: device class created correctly\n");
	return 0;
}

/// Function called upon loading module
int __init init_module(void) {
	chr_dev_init();
	printk("PTUSYS: init completed\n");
	return 0;
}

/// Function called upon unloading module
void __exit cleanup_module(void) {
	device_destroy(ptusysClass, MKDEV(major_number, PTUSYS_MINOR));
	class_unregister(ptusysClass);
	class_destroy(ptusysClass);
	if(major_number>0)
	unregister_chrdev(major_number, DEVICE_NAME);
	printk(KERN_INFO "PTUSYS: clean up completed, quitting...\n");
}
