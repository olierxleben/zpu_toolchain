void zpu_enable_read_interrupt()
{
	iowrite32(ioread32(pcidev_config + ADDR_CONTROL) | CTRL_TXIE, pcidev_config + ADDR_CONTROL);
}

void zpu_disable_read_interrupt()
{
	iowrite32(ioread32(pcidev_config + ADDR_CONTROL) & ~(CTRL_TXIE), pcidev_config + ADDR_CONTROL);
}

void zpu_enable_write_interrupt()
{
	iowrite32(ioread32(pcidev_config + ADDR_CONTROL) | CTRL_RXIE, pcidev_config + ADDR_CONTROL);
}

void zpu_disable_write_interrupt()
{
	iowrite32(ioread32(pcidev_config + ADDR_CONTROL) & ~(CTRL_RXIE), pcidev_config + ADDR_CONTROL);
}

void zpu_disable_interrupts()
{
	iowrite32(0, pcidev_config + ADDR_CONTROL);
}
