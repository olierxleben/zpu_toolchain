
#define DEBUG 1

#ifdef DEBUG
# define OUT_DBG(msg, ...)   printk(msg, ##__VA_ARGS__)
#else
# define OUT_DBG(msg, ...)
#endif
#define OUT_WARN(msg, ...) printk(KERN_WARNING msg, ##__VA_ARGS__)

// Constants

#define MINOR_COUNT 1
#define MINOR_FIRST 0

#define VENDOR_ID 0x10EE
#define DEVICE_ID 0x0100

#define ADDR_RAM     0x0000000
#define ADDR_SYSCTL  0x0002000
#define ADDR_DATA    0x0002010
#define ADDR_CONTROL 0x0002014
#define ADDR_STATUS  0x0002018

#define SYSCTL_STOP  0x01
#define SYSCTL_START 0x00

#define CTRL_RXIE    (1 << 1)
#define CTRL_TXIE    (1     )
#define CTRL_STDIN_READY  CTRL_TXIE
#define CTRL_STDOUT_READY CTRL_RXIE

#define STAT_RXINT   (1 << 3)
#define STAT_TXINT   (1 << 2)
#define STAT_RXV     (1 << 1)
#define STAT_TXE     (1     )
#define STAT_STDIN_READY  STAT_TXINT
#define STAT_STDOUT_READY STAT_RXINT

#define ZPU_IO_BUFFER_SIZE 4096
#define ZPU_RAM_SIZE 8192

#define ZPU_IO_READ(offs)        ioread32(pcidev_config + offs)
#define ZPU_IO_WRITE(offs, what) iowrite32(what, pcidev_config + offs)

// driver specific init and exit functions
static int  __init init    (void);
static void __exit cleanup (void);

// PCI functions
static int  mypci_probe  (struct pci_dev *dev, const struct pci_device_id *id);
static void mypci_remove (struct pci_dev *dev);

// Character device  functions
static int     mychr_open  (struct inode* inodep, struct file* filep);
static int     mychr_close (struct inode* inodep, struct file* filep);
static int     mychr_mmap  (struct file *filep, struct vm_area_struct *vma);
static int     mychr_ioctl (struct inode *inodep, struct file* filep, unsigned int cmd, unsigned long param);
static ssize_t mychr_write (struct file *filep, const char __user *data, size_t count, loff_t *offset);
static ssize_t mychr_read  (struct file *filep, char __user *data, size_t count, loff_t *offset);

// Memory management functions
void myvma_open  (struct vm_area_struct *vma);
void myvma_close (struct vm_area_struct *vma);

// Interrupt function
irqreturn_t myirq_handler(int irq, void *dev_id);

struct file_operations mychr_fops = {
	.owner   = THIS_MODULE,
	.open    = mychr_open,
	.release = mychr_close,
	.read    = mychr_read,
	.write   = mychr_write,
	.mmap    = mychr_mmap,
	.ioctl   = mychr_ioctl
};

static struct vm_operations_struct myvma_ops = {
	.open = myvma_open,
	.close = myvma_close
};