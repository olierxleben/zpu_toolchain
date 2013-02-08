

void myvma_open(struct vm_area_struct *vma) { printk("VMA Open.\n"); }


void myvma_close(struct vm_area_struct *vma) { printk("VMA Close.\n"); }


static int mychr_mmap(struct file *filep, struct vm_area_struct *vma)
{
	unsigned long vm_size, map_size;
	int r;

	vm_size  = vma->vm_end - vma->vm_start;
	map_size = vm_size < ZPU_RAM_SIZE ? vm_size : ZPU_RAM_SIZE;

	OUT_DBG("Mapping %li bytes to %lx\n", map_size, vma->vm_start);
	
	r = io_remap_pfn_range(vma,
		vma->vm_start, // Ziel-Adresse
		(paddr + ADDR_RAM) >> PAGE_SHIFT, // Adresse der Quell-Seite
		map_size, // Größe des zu mappenden Bereichs
		vma->vm_page_prot // Zugriffsschutz
	);
	if (r != 0)
	{
		return -EAGAIN;
	}

	vma->vm_ops = &myvma_ops;
	myvma_open(vma);

	return 0;
}