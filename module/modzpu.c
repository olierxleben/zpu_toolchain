/**
@file modul.c
@version 0.1 pretty alpha
**/
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
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Control Driver for ZPU PCI device");
MODULE_SUPPORTED_DEVICE("ZPU-Kram");

// pci data structure
// see script page 252
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
	d->id_table = &(id_table);
	d->probe    = mypci_probe;
	d->remove   = mypci_remove;

	if ((r = pci_register_driver(d)) != 0)
	{
		printk("PCI-Treiber konnte nicht registriert werden.\n");
		return r;
	}

	printk("Modul initialisiert.\n");

	return 0;
}

void __exit cleanup(void)
{
	pci_unregister_driver(&mypci_driver);
	printk("PCI-Treiber entladen.\n");
}


//
// METHODEN-IMPLEMENTIERUNGEN (PCI-BEZOGEN)
//


static int mypci_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	int              r, i;
	struct resource *res;

	if ((r = pci_enable_device(dev)) != 0)
	{
		goto pci_enable_device_failed;
	}

	// Alle möglichen Speicherbereiche auslesen.
	for (i = 0; i < 6; i ++)
	{
		unsigned long  start, end, flags;
		char          *type = "???";

		start = pci_resource_start (dev, i);
		end   = pci_resource_end   (dev, i);
		flags = pci_resource_flags (dev, i);
		
		     if (flags & IORESOURCE_IO)  type = " IO";
		else if (flags & IORESOURCE_MEM) type = "MEM";

		printk("Ressource (%s): %lx-%lx, Größe: %lu\n", type, start, end, pci_resource_len(dev, i));
	}

	// Physikalische Adresse des Speicherbereichs und dessen Größe auslesen.
	paddr = pci_resource_start (dev, 0);
	plen  = pci_resource_len   (dev, 0);

	printk("Interrupt-Nummer: %i\n", dev->irq);
	
	// Komplettes Konfigurationsregister in den Kernel-Speicher mappen.
	if ((r = pci_request_regions(dev, "zpu")) != 0)
	{
		goto pci_request_regions_failed;
	}

	pcidev_config = ioremap(paddr, plen);
	printk("Konfigurationsregister auf %p gemappt.\n", pcidev_config);

	// Gerätenummer alloziieren.
	if ((r = alloc_chrdev_region(&dev_number, MINOR_FIRST, 1, "zpu")) != 0)
	{
		goto alloc_chrdev_region_failed;
	}

	cdev_init(&c_dev, &mychr_fops);
	c_dev.owner = THIS_MODULE;

	// Gerät hinzufügen.
	if ((r = cdev_add(&c_dev, dev_number, 1)) != 0)
	{
		goto cdev_add_failed;
	}
	
	// Fifos initialisieren
	fifo_init(&(zpu_io_stdin) , ZPU_IO_BUFFER_SIZE);
	fifo_init(&(zpu_io_stdout), ZPU_IO_BUFFER_SIZE);
	
	if ((r = request_irq(dev->irq, myirq_handler, IRQF_SHARED, "zpu_ir", pcidev_config)) != 0)
	{
		goto request_irq_failed;
	}
	
	// Disable stdin interrupt, enable stdout interrupt.
	ZPU_IO_WRITE(ADDR_CONTROL, CTRL_STDOUT_READY);

	return 0;

	request_irq_failed:
		printk(KERN_WARNING "Interrupt-Handler konnte nicht registriert werden.\n");
		fifo_delete(&(zpu_io_stdin));
		fifo_delete(&(zpu_io_stdout));
		cdev_del(&c_dev);
		
	cdev_add_failed:
		printk(KERN_WARNING "Gerät konnte nicht registriert werden.\n");
		unregister_chrdev_region(dev_number, MINOR_COUNT);
		
	alloc_chrdev_region_failed:
		iounmap(pcidev_config);
		pci_release_regions(dev);
		printk(KERN_WARNING "Gerätenummer konnte nicht ermittelt werden.\n");
		
	pci_request_regions_failed:
		pci_disable_device(dev);
		
	pci_enable_device_failed:
		printk(KERN_WARNING "Konnte Gerät nicht aktivieren.\n");
		
	return r;
}

static void mypci_remove(struct pci_dev *dev)
{
	free_irq(dev->irq, pcidev_config);

	iounmap(pcidev_config);
	pci_release_regions(dev);

	pci_disable_device(dev);

	cdev_del(&c_dev);
	unregister_chrdev_region(dev_number, MINOR_COUNT);

	printk("Tschuess.\n");
}

module_init(init);
module_exit(cleanup);
