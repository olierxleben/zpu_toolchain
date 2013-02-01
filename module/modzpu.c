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

#include "zpu_commands.h"
#include "fifo.c"

#define MINOR_COUNT 1
#define MINOR_FIRST 0

#define VENDOR_ID 0x10EE
#define DEVICE_ID 0x0100

#define ADDR_RAM     0x0000000
#define ADDR_SYSCTL  0x0002000
#define ADDR_DATA    0x0002010
#define ADDR_CONTROL 0x0002014
#define ADDR_STATUS  0x0002018

#define CTRL_RXIE    0x02
#define CTRL_TXIE    0x01

#define STAT_RXINT   1 << 3
#define STAT_TXINT   1 << 2
#define STAT_RXV     1 << 1
#define STAT_TXE     1

#define ZPU_IO_BUFFER_SIZE 4096
#define ZPU_RAM_SIZE 8192
	
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
unsigned long paddr, plen;
void *pcidev_config;
unsigned int mypci_ircount = 0;

struct cdev c_dev;
dev_t dev_number = 0;

fifo_t zpu_io_stdin;
fifo_t zpu_io_stdout;

DECLARE_MUTEX(mutex_w);
DECLARE_MUTEX(mutex_r);

//
// METHODEN-DEKLARATIONEN
//


#include "zpu_irq.c"


// Inititialisierungs- und Aufräumfunktionen.
static int  __init init(void);
static void __exit cleanup(void);

// PCI-bezogene Funktionen.
static int  mypci_probe(struct pci_dev *dev, const struct pci_device_id *id);
static void mypci_remove(struct pci_dev *dev);

// Character device-bezogene Funktionen.
static int     mychr_open  (struct inode* inodep, struct file* filep);
static int     mychr_close (struct inode* inodep, struct file* filep);
static int     mychr_mmap  (struct file *filep, struct vm_area_struct *vma);
static int     mychr_ioctl (struct inode *inodep, struct file* filep, unsigned int cmd, unsigned long param);
static ssize_t mychr_write (struct file *filep, char __user *data, size_t count, loff_t *offset);
static ssize_t mychr_read  (struct file *filep, char __user *data, size_t count, loff_t *offset);

struct file_operations mychr_fops = {
	.owner   = THIS_MODULE,
	.open    = mychr_open,
	.release = mychr_close,
	.read    = mychr_read,
	.write   = mychr_write,
	.mmap    = mychr_mmap,
	.ioctl   = mychr_ioctl
};

// Speicherverwaltungs-bezogene Funktionen.
void myvma_open(struct vm_area_struct *vma);
void myvma_close(struct vm_area_struct *vma);

static struct vm_operations_struct myvma_ops = {
	.open = myvma_open,
	.close = myvma_close
};

// Interrupt-Kram
irqreturn_t myirq_handler(int irq, void *dev_id, struct pt_regs *regs);


//
// METHODEN-IMPLEMENTIERUNGEN (SPEICHERMANAGEMENT)
//


void myvma_open(struct vm_area_struct *vma) { printk("VMA Open.\n"); }

void myvma_close(struct vm_area_struct *vma) { printk("VMA Close.\n"); }


//
// METHODEN-IMPLEMENTIERUNGEN (CHARACTER DEVICE)
//


static int mychr_open(struct inode* inodep, struct file* filep)
{
	// Nur je einmal zum Lesen und Schreiben öffnen!
	if (filep->f_mode & FMODE_READ && down_interruptible(&mutex_r))
	{
		return -ERESTARTSYS;	
	}
	
	if (filep->f_mode & FMODE_WRITE && down_interruptible(&mutex_w))
	{
		if (filep->f_mode & FMODE_READ) up(&mutex_r);
		return -ERESTARTSYS;
	}
	
	printk("Datei geöffnet.\n");
	return 0;
}

static int mychr_close(struct inode* inodep, struct file* filep)
{
	// Scrhranken zum lesen/schreiben wieder öffnen
	if( filep->f_mode & FMODE_READ)
	{
		up(&mutex_r);
	}
	
	if( filep->f_mode & FMODE_WRITE)
	{
		up(&mutex_w);
	}
	
	printk("Datei geschlossen.\n");
	return 0;
}

static ssize_t mychr_write (struct file *filep, char __user *data, size_t count, loff_t *offset)
{
	fifo_t *f = &(zpu_io_stdin);
	unsigned int n;
	
	printk("Count: %i\n", f->count);
	printk("Size : %i\n", f->size);
	
	if (f->count == f->size)
	{
		zpu_enable_read_interrupt();
	
		if (filep->f_flags & O_NONBLOCK)
		{
			return -EAGAIN;
		}
		else
		{
			if (wait_event_interruptible(f->queue, f->count < f->size) != 0)
			{
				// TODO: Anderer Error-Code!
				return -EAGAIN;
			}
		}
	}
	
	n = count < (f->size - f->count) ? count : (f->size - f->count);
	f->count = f->count + n;
	
	if (f->write + n <= f->size)
	{
		copy_from_user(f->data + f->write, data, n);
		f->write = (f->write + n) % f->size;
	}
	else
	{
		unsigned int size1 = (f->size - f->write);
		unsigned int size2 = (n - size1);
		
		copy_from_user(f->data + f->write, data, size1);
		copy_from_user(f->data, data + size1, size2);
		
		f->write = size2;
	}
	
	zpu_enable_read_interrupt();
	
	return n;
}

static ssize_t mychr_read  (struct file *filep, char __user *data, size_t count, loff_t *offset)
{
	
	fifo_t *f = &(zpu_io_stdout);
	unsigned int n;
	
	if (f->count == 0)
	{
		zpu_enable_write_interrupt();
		
		if (filep->f_flags & O_NONBLOCK)
		{
			return -EAGAIN;
		}
		else
		{
			wait_event_interruptible(f->queue, f->count > 0);
		}
	}
	
	n = count < f->count ? count : f->count;
	f->count = f->count - n;
	
	if (f->read + n <= f->size)
	{
		copy_to_user(data, &(f->data[f->read]), n);
		f->read = (f->read + n) % f->size;
	}
	else
	{
		unsigned int size1 = f->size - f->read;
		unsigned int size2 = n - size1;
		
		copy_to_user(data, f->data + f->read, size1);
		copy_to_user(data + size1, f->data, size2);
		
		f->read = size2;
	}
	
	zpu_enable_write_interrupt();
	
	return n;
}

static int mychr_mmap(struct file *filep, struct vm_area_struct *vma)
{
	int vm_size, map_size;
	int r;

	vm_size = vma->vm_end - vma->vm_start;
	map_size = vm_size < ZPU_RAM_SIZE ? vm_size : ZPU_RAM_SIZE;

	printk("Mappe Seite %lx auf virtuelle Adresse %lx (%i Bytes werden gemapped).\n", (paddr + ADDR_RAM) >> PAGE_SHIFT, vma->vm_start, map_size);

	r = io_remap_pfn_range(vma,
		vma->vm_start, // Ziel-Adresse
		(paddr + ADDR_RAM) >> PAGE_SHIFT, // Adresse der Quell-Seite
		map_size, // Größe des zu mappenden Bereichs
		vma->vm_page_prot // ?
	);
	if (r != 0)
	{
		printk("Fehler beim Mappen.\n");
		return -EAGAIN;
	}

	vma->vm_ops = &myvma_ops;
	myvma_open(vma);

	return 0;
}

static int mychr_ioctl(struct inode *inodep, struct file* filep, unsigned int cmd, unsigned long param)
{
	printk("Starten: 0x%x\n", RAGGED_ZPU_START);
	printk("Stoppen: 0x%x\n", RAGGED_ZPU_STOP);

	switch(cmd)
	{
		case RAGGED_ZPU_STOP:
			printk("Prozessor angehalten.\n");
			iowrite32(1, pcidev_config + ADDR_SYSCTL);
			break;
		case RAGGED_ZPU_START:
			printk("Prozessor läuft weiter.\n");
			iowrite32(0, pcidev_config + ADDR_SYSCTL);
			
			printk("Kontroll-Register: 0x%08x\n", ioread32(pcidev_config + ADDR_CONTROL));
			
			break;
		default:
			printk(KERN_WARNING "Unbekanntes ioctl-Kommando: %x\n", cmd);
			return -ENOTTY;
	}
	
	return 0;
}


//
// METHODEN-IMPLEMENTIERUNGEN (INTERRUPT-KRAM)
//


irqreturn_t myirq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned int status = ioread32(pcidev_config + ADDR_STATUS);
	
	if ((status & STAT_TXINT) > 0)
	{
		fifo_t *f = &(zpu_io_stdin);
		
		while ((status & STAT_TXINT) > 0 && (f->count > 0))
		{
			iowrite32(f->data[f->read], pcidev_config + ADDR_DATA);
			
			f->read = (f->read + 1) % f->size;
			f->count --;
			
			wake_up_all(&(f->queue));
			
			printk("1 Byte in stdin geschrieben.\n");
			
			udelay(75);
			status = ioread32(pcidev_config + ADDR_STATUS);
		}
		
		if (f->count == 0)
		{
			printk("Es gibt nichts mehr zu schreiben.\n");
			zpu_disable_read_interrupt();
		}
		
		return IRQ_HANDLED;
	}
	else if ((status & STAT_RXINT) > 0)
	{
		fifo_t *f = &(zpu_io_stdout);
		
		printk("Los gehts\n");
		
		while ((status & STAT_RXV) > 0 && f->count < f->size)
		{
			f->data[f->write] = ioread32(pcidev_config + ADDR_DATA);
			
			f->write = (f->write + 1) % f->size;
			f->count ++;
			
			printk("1 Byte aus stdout gelesen.\n");
			
			udelay(75);
			status = ioread32(pcidev_config + ADDR_STATUS);
		}
		printk("Fertig\n");
		
		if (f->count == f->size)
		{
			printk("Empfangs-Puffer ist voll.\n");
			zpu_disable_write_interrupt();
		}
			
		wake_up_all(&(f->queue));
		
		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}


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
	int r, i;
	struct resource *res;

	if ((r = pci_enable_device(dev)) != 0) goto pci_enable_device_failed;

	// Alle möglichen Speicherbereiche auslesen.
	for (i = 0; i < 6; i ++)
	{
		unsigned long start, end, flags;
		char *type = "???";

		start = pci_resource_start(dev, i);
		end = pci_resource_end(dev, i);
		flags = pci_resource_flags(dev, i);
		
		if (flags & IORESOURCE_IO) type = " IO";
		else if (flags & IORESOURCE_MEM) type = "MEM";

		printk("Ressource (%s): %lx-%lx, Größe: %lu\n", type, start, end, pci_resource_len(dev, i));
	}

	// Physikalische Adresse des Speicherbereichs und dessen Größe auslesen.
	paddr = pci_resource_start(dev, 0);
	plen  = pci_resource_len(dev, 0);

	printk("Interrupt-Nummer: %i\n", dev->irq);
	
	// Komplettes Konfigurationsregister in den Kernel-Speicher mappen.
	if ((r = pci_request_regions(dev, "zpu")) != 0) goto pci_request_regions_failed;

	pcidev_config = ioremap(paddr, plen);
	printk("Konfigurationsregister auf %p gemappt.\n", pcidev_config);

	// Gerätenummer alloziieren.
	if ((r = alloc_chrdev_region(&dev_number, MINOR_FIRST, 1, "zpu")) != 0) goto alloc_chrdev_region_failed;

	cdev_init(&c_dev, &mychr_fops);
	c_dev.owner = THIS_MODULE;

	// Gerät hinzufügen.
	if ((r = cdev_add(&c_dev, dev_number, 1)) != 0) goto cdev_add_failed;
	
	// Fifos initialisieren
	fifo_init(&(zpu_io_stdin) , ZPU_IO_BUFFER_SIZE);
	fifo_init(&(zpu_io_stdout), ZPU_IO_BUFFER_SIZE);
	
	if ((r = request_irq(dev->irq, myirq_handler, IRQF_SHARED, "zpu_ir", pcidev_config)) != 0) goto request_irq_failed;
	
	zpu_disable_interrupts();
	zpu_enable_write_interrupt();

	return 0;

	request_irq_failed:
		printk(KERN_WARNING "Interrupt-Handler konnte nicht registriert werden.\n");
	cdev_add_failed:
		printk(KERN_WARNING "Gerät konnte nicht registriert werden.\n");
		unregister_chrdev_region(dev_number, MINOR_COUNT);
	alloc_chrdev_region_failed:
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
