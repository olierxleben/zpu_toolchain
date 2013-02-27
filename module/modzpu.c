/** ZPU driver module
 * 
 *  Oliver Erxleben <oliver.erxleben@hs-osnabrueck.de>
 *  Martin Helmich  <martin.helmich@hs-osnabrueck.de>
 * 
 *  University of applied sciences Osnabrück
 */
 
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/sched.h>

#include <asm/io.h>
#include <asm/page.h>
#include <asm/uaccess.h>
#include <linux/semaphore.h>

#include "modzpu.h"

#include "zpu_commands.h"
#include "fifo.c"
	
MODULE_AUTHOR("Erxleben, Helmich");
MODULE_LICENSE("None");
MODULE_DESCRIPTION("Control Driver for ZPU PCI device");
MODULE_SUPPORTED_DEVICE("ZPU-Kram");

DEFINE_PCI_DEVICE_TABLE(id_table) = {
	{ PCI_DEVICE(VENDOR_ID, DEVICE_ID) }, 
	{ 0, }
}; 

struct pci_driver mypci_driver;
unsigned long     paddr, plen;
void             *pcidev_config;
unsigned int      mypci_ircount = 0;

struct cdev       c_dev;
dev_t             dev_number = 0;

fifo_t            zpu_io_stdin;
fifo_t            zpu_io_stdout;

DECLARE_MUTEX(mutex_w);
DECLARE_MUTEX(mutex_r);


//
// METHODEN-DEKLARATIONEN
//

#include "zpu_irq.c"
#include "zpu_io.c"
#include "zpu_mem.c"
#include "zpu_open.c"


//
// METHODEN-IMPLEMENTIERUNGEN (INITIALISIERUNG UND CLEANUP)
//

static int __init init(void)
{
	int r;
	struct pci_driver *d = &(mypci_driver);

	d->name     = "zpu";
	d->id_table = (const struct pci_device_id*) &(id_table);
	d->probe    = mypci_probe;
	d->remove   = mypci_remove;

	if ((r = pci_register_driver(d)) != 0)
	{
		OUT_WARN("Could not register ZPU device driver.\n");
		return r;
	}

	OUT_DBG("Initialized module.\n");

	return 0;
}

void __exit cleanup(void)
{
	pci_unregister_driver(&mypci_driver);
	OUT_DBG("Unregistered PCI driver.\n");
}

//
// METHODEN-IMPLEMENTIERUNGEN (PCI-BEZOGEN)
//

static int mypci_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	int r;

	if ((r = pci_enable_device(dev)) != 0)
	{
		goto pci_enable_device_failed;
	}

	// Read physical address of the device's memory and it's size.
	paddr = pci_resource_start (dev, 0);
	plen  = pci_resource_len   (dev, 0);

	OUT_DBG("PCI-Device 0x%x enabled.\n", DEVICE_ID);
	
	// Map the entire memory resource into the kernel's address space.
	if ((r = pci_request_regions(dev, "zpu")) != 0)
	{
		goto pci_request_regions_failed;
	}

	pcidev_config = ioremap(paddr, plen);
	OUT_DBG("Mapped memory resource to %p.\n", pcidev_config);

	// Allocate character device number.
	if ((r = alloc_chrdev_region(&dev_number, MINOR_FIRST, 1, "zpu")) != 0)
	{
		goto alloc_chrdev_region_failed;
	}

	cdev_init(&c_dev, &mychr_fops);
	c_dev.owner = THIS_MODULE; // add actual module as owner in c_dev

	// Add character device to kernel.
	if ((r = cdev_add(&c_dev, dev_number, 1)) != 0) 
	{
		goto cdev_add_failed;
	}
	
	// Initialize fifo buffers.
	fifo_init(&(zpu_io_stdin) , ZPU_IO_BUFFER_SIZE);
	fifo_init(&(zpu_io_stdout), ZPU_IO_BUFFER_SIZE);
	
	OUT_DBG("ZPU device ready to use.\n");
	
	if ((r = request_irq(dev->irq, myirq_handler, IRQF_SHARED, "zpu_ir", pcidev_config)) != 0)
	{
		goto request_irq_failed;
	}
	
	// Disable stdin interrupt, enable stdout interrupt.
	ZPU_IO_WRITE(ADDR_CONTROL, CTRL_STDOUT_READY);

	return 0;

	request_irq_failed:
		OUT_WARN("Could not register interrupt handler for interrupt %i.\n", dev->irq);
		fifo_delete(&(zpu_io_stdin));
		fifo_delete(&(zpu_io_stdout));
		cdev_del(&c_dev);
		
	cdev_add_failed:
		OUT_WARN("Could not register character device.\n");
		unregister_chrdev_region(dev_number, MINOR_COUNT);
		
	alloc_chrdev_region_failed:
		iounmap(pcidev_config);
		pci_release_regions(dev);
		OUT_WARN("Could not allocate character device number.\n");
		
	pci_request_regions_failed:
		pci_disable_device(dev);
		
	pci_enable_device_failed:
		OUT_WARN("Could not enable character device.\n");
		
	return r;
}

static void mypci_remove(struct pci_dev *dev)
{
	// Free interrupt handler
	free_irq(dev->irq, pcidev_config);

	// Unmap PCI address mapping
	iounmap(pcidev_config);
	pci_release_regions(dev);

	// Disable PCI device
	pci_disable_device(dev);

	// Disable and deallocate character devices.
	cdev_del(&c_dev);
	unregister_chrdev_region(dev_number, MINOR_COUNT);

	OUT_DBG("Goodbye.\n");
}

module_init(init);
module_exit(cleanup);
