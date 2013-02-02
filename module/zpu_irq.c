#define ZPU_IR_ENABLE(CTRL)  ZPU_IO_WRITE(ADDR_CONTROL, ZPU_IO_READ(ADDR_CONTROL) |   CTRL);
#define ZPU_IR_DISABLE(CTRL) ZPU_IO_WRITE(ADDR_CONTROL, ZPU_IO_READ(ADDR_CONTROL) & (~CTRL));
#define ZPU_IR_DISABLE_ALL   ZPU_IO_WRITE(ADDR_CONTROL, 0); 
	

#define ZPU_ENABLE_STDIN_IR   ZPU_IR_ENABLE  (CTRL_TXIE);
#define ZPU_DISABLE_STDIN_IR  ZPU_IR_DISABLE (CTRL_TXIE);
#define ZPU_ENABLE_STDOUT_IR  ZPU_IR_ENABLE  (CTRL_RXIE);
#define ZPU_DISABLE_STDOUT_IR ZPU_IR_DISABLE (CTRL_RXIE);


irqreturn_t myirq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned int status = ZPU_IO_READ(ADDR_STATUS);
	
	if ((status & STAT_STDIN_READY) > 0)
	{
		fifo_t *f = &(zpu_io_stdin);
		
		while ((status & STAT_STDIN_READY) > 0 && FIFO_NOT_EMPTY(f))
		{
			ZPU_IO_WRITE(ADDR_DATA, fifo_read_byte(f));
			
			wake_up_all(&(f->queue));
			
			udelay(75);
			status = ZPU_IO_READ(ADDR_STATUS);
		}
		
		if (FIFO_EMPTY(f))
		{
			ZPU_DISABLE_STDIN_IR();
		}
			
		wake_up_all(&(f->queue));
		
		return IRQ_HANDLED;
	}
	else if ((status & STAT_STDOUT_READY) > 0)
	{
		fifo_t *f = &(zpu_io_stdout);
		
		while ((status & STAT_STDOUT_READY) > 0 && FIFO_NOT_FULL(f))
		{
			fifo_write_byte(f, ZPU_IO_READ(ADDR_DATA);
			
			udelay(75);
			status = ZPU_IO_READ(ADDR_STATUS);
		}
		
		if (FIFO_FULL(f))
		{
			ZPU_DISABLE_STDOUT_IR();
		}
			
		wake_up_all(&(f->queue));
		
		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}