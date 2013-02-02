
#include "linux/vmalloc.h"
#include "linux/types.h"


#define FIFO_EMPTY(fifop)     fifop->count == 0
#define FIFO_FULL(fifop)      fifop->count == fifop->size
#define FIFO_NOT_FULL(fifop)  fifop->count < fifop->size
#define FIFO_NOT_EMPTY(fifop) fifop->count > 0
#define FIFO_FREE(fifop)      fifop->size - fifop->count


typedef struct {
	int  read;
	int  write;
	int  size;
	unsigned int count;
	char *data;
	wait_queue_head_t queue;
} fifo_t;


void fifo_init       (fifo_t *f, size_t s);
void fifo_delete     (fifo_t *f);
int  fifo_transfer   (fifo_t *f1, fifo_t *f2, int n);
u8   fifo_read_byte  (fifo_t *f);
void fifo_write_byte (fifo_t *f, u8 b);


void fifo_init(fifo_t *f, size_t s)
{
	// Lese- und Schreibzeiger initialisieren.
	f->read = 0;
	f->write = 0;
	f->count = 0;
	f->size = s;
	
	// Speicher für Datenbereich alloziieren.
	f->data = (char*) vmalloc(s);

	// Queues initialisieren
	init_waitqueue_head(&f->queue);
}

void fifo_delete(fifo_t *f)
{
	vfree(f->data);
}

int fifo_transfer(fifo_t *f1, fifo_t *f2, int n)
{
	int i;
	
	for (i=0; i < n; i ++)
	{
		f2->data[f2->write + i % f2->size] = f1->data[f1->read + i % f2->size];
	}
	
	f1->read  = f1->read  + n % f1->size;
	f2->write = f2->write + n % f2->size;

	printk("Transferred %d bytes.\n", n);
	return 0;
}

u8 fifo_read_byte(fifo_t *f)
{
	if (FIFO_NOT_EMPTY(f))
	{
		u8 r = f->data[f->read];
		f->read = f->read + 1 % f->size;
		f->count --;
		return r;
	}
	return 0;
}

void fifo_write_byte(fifo_t *f, u8 b)
{
	if (FIFO_NOT_FULL(f))
	{
		f->data[f->write] = b;
		f->write = f->write + %f->size;
		f->count ++;
	}
}