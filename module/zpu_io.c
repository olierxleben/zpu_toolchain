/**
 * zpu_io.c
 *
 * functions collection for reading and writing zpu-fifos used by modzpu
 *
 * oliver.erxleben@hs-osnabrueck.de
 * martin.helmlich@hs-osnabrueck.de
 * 
 */
	 
/**
 * TODO: Add method description
 */	 
static ssize_t mychr_write (struct file *filep, const char __user *data, size_t count, loff_t *offset)
{
	fifo_t			*f	= &(zpu_io_stdin);
	unsigned int	n;
	
	if (FIFO_FULL(f))
	{
		ZPU_ENABLE_STDIN_IR();
	
		if (filep->f_flags & O_NONBLOCK)
		{
			return -EAGAIN; // return with "try again"
		}
		else if (wait_event_interruptible(f->queue, FIFO_NOT_FULL(f)) != 0)
		{
			return -EINTR; // return with 
		}
	}
	
	n = count < FIFO_FREE(f) ? count : FIFO_FREE(f);
	
	if (f->write + n <= f->size)
	{
		if (copy_from_user(f->data + f->write, data, n) > 0) return -EAGAIN;
		f->write = (f->write + n) % f->size;
	}
	else
	{
		unsigned int size1 = (f->size - f->write);
		unsigned int size2 = (n - size1);
		
		if (copy_from_user(f->data + f->write, data, size1) +
		    copy_from_user(f->data, data + size1, size2) > 0) return -EAGAIN;
		
		f->write = size2;
	}
	
	f->count = f->count + n;
	ZPU_ENABLE_STDIN_IR();
	
	return n;
}

/*
 * TODO: Add Method description
 */
static ssize_t mychr_read  (struct file *filep, char __user *data, size_t count, loff_t *offset)
{
	fifo_t *f = &(zpu_io_stdout);
	unsigned int n;
	
	if (f->count == 0)
	{
		ZPU_ENABLE_STDOUT_IR();
		
		if (filep->f_flags & O_NONBLOCK)
		{
			return -EAGAIN;
		}
		else if (wait_event_interruptible(f->queue, f->count > 0) != 0)
		{
			return -EINTR;
		}
	}
	
	n = count < f->count ? count : f->count;
	
	if (f->read + n <= f->size)
	{
		if (copy_to_user(data, &(f->data[f->read]), n) > 0) return -EAGAIN;
		f->read = (f->read + n) % f->size;
	}
	else
	{
		unsigned int size1 = f->size - f->read;
		unsigned int size2 = n - size1;
		
		if (copy_to_user(data, f->data + f->read, size1) +
		    copy_to_user(data + size1, f->data, size2) > 0) return -EAGAIN;
		
		f->read = size2;
	}
	
	f->count = f->count - n;
	ZPU_ENABLE_STDOUT_IR();
	
	return n;
}

static int mychr_ioctl(struct inode *inodep, struct file* filep, unsigned int cmd, unsigned long param)
{
	switch(cmd)
	{
		case RAGGED_ZPU_STOP:
			OUT_DBG("Stopping ZPU.\n");
			ZPU_IO_WRITE(ADDR_SYSCTL, SYSCTL_STOP);
			break;
		case RAGGED_ZPU_START:
			OUT_DBG("Starting ZPU.\n");
			ZPU_IO_WRITE(ADDR_SYSCTL, SYSCTL_START);
			if (FIFO_NOT_FULL((&zpu_io_stdout)))
			{
				OUT_DBG("Enabling STDOUT interrupt.\n");
				ZPU_ENABLE_STDOUT_IR();
				
				OUT_DBG("CTRL-Register: %08x\n", ZPU_IO_READ(ADDR_CONTROL));
				OUT_DBG("STAT-Register: %08x\n", ZPU_IO_READ(ADDR_STATUS));
			}
			break;
		default:
			printk(KERN_WARNING "Unbekanntes ioctl-Kommando: %x\n", cmd);
			return -EINVAL;
	}
	
	return 0;
}
