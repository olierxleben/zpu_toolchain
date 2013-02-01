
#include "linux/vmalloc.h"

typedef struct {
	int  read;
	int  write;
	int  size;
	unsigned int count;
	char *data;
	wait_queue_head_t queue;
} fifo_t;

void fifo_init(fifo_t *f, size_t s);
void fifo_delete(fifo_t *f);
int  fifo_transfer(fifo_t *f1, fifo_t *f2, int n);

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
