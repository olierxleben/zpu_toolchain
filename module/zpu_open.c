static int mychr_open(struct inode* inodep, struct file* filep)
{
	// Nur je einmal zum Lesen und Schreiben öffnen!
	if (filep->f_mode & FMODE_READ && down_interruptible(&mutex_r))
	{
		return -ERESTARTSYS;	
	}
	
	if (filep->f_mode & FMODE_WRITE && down_interruptible(&mutex_w))
	{
		if (filep->f_mode & FMODE_READ)
		{
			up(&mutex_r);
		}
		return -ERESTARTSYS;
	}
	
	return 0;
}

static int mychr_close(struct inode* inodep, struct file* filep)
{
	// Scrhranken zum lesen/schreiben wieder öffnen
	if (filep->f_mode & FMODE_READ)
	{
		up(&mutex_r);
	}
	
	if (filep->f_mode & FMODE_WRITE)
	{
		up(&mutex_w);
	}
	
	return 0;
}