#define ZPU_IR_ENABLE(CTRL)  ZPU_IO_WRITE(ADDR_CONTROL, ZPU_IO_READ(ADDR_CONTROL) |   CTRL);
#define ZPU_IR_DISABLE(CTRL) ZPU_IO_WRITE(ADDR_CONTROL, ZPU_IO_READ(ADDR_CONTROL) & (~CTRL));
#define ZPU_IR_DISABLE_ALL   ZPU_IO_WRITE(ADDR_CONTROL, 0); 
	

#define ZPU_ENABLE_STDIN_IR()   ZPU_IR_ENABLE  (CTRL_TXIE);
#define ZPU_DISABLE_STDIN_IR()  ZPU_IR_DISABLE (CTRL_TXIE);
#define ZPU_ENABLE_STDOUT_IR()  ZPU_IR_ENABLE  (CTRL_RXIE);
#define ZPU_DISABLE_STDOUT_IR() ZPU_IR_DISABLE (CTRL_RXIE);


irqreturn_t myirq_handler(int irq, void *dev_id)
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
			OUT_DBG("Wrote 1 byte to STDIN.\n");
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
			int b = ZPU_IO_READ(ADDR_DATA);
			fifo_write_byte(f, b);
			
			udelay(75);
			status = ZPU_IO_READ(ADDR_STATUS);
			OUT_DBG("Read 1 byte (%02x) from STDOUT.\n", b & 0xFF);
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
