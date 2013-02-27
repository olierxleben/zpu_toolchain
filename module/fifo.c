
#include "linux/vmalloc.h"
#include "linux/types.h"


//
// MACROS AND STRUCTURES
//


#define FIFO_EMPTY(fifop)     fifop->count == 0
#define FIFO_FULL(fifop)      fifop->count == fifop->size
#define FIFO_NOT_FULL(fifop)  fifop->count < fifop->size
#define FIFO_NOT_EMPTY(fifop) fifop->count > 0
#define FIFO_FREE(fifop)      fifop->size - fifop->count


/// Structure for a simple FIFO buffer.
typedef struct {
	int  read;               //!< Read pointer.
	int  write;              //!< Write pointer.
	int  size;               //!< Maximum size.
	unsigned int count;      //!< Fill level.
	char *data;              //!< Pointer to dynamically allocated data array.
	wait_queue_head_t queue; //!< Wait queue for processes waiting on updates.
} fifo_t;


//
// FUNCTION DECLARATION
//


/// Initialize fifo.
/** Initializes a fifo buffer with a given size. The memory for the fifo
 *  is dynamically allocated using vmalloc.
 *  @param f A pointer to the fifo to be initialized.
 *  @param s The desired size of the buffer. */
void fifo_init       (fifo_t *f, size_t s);

/// Deletes a fifo.
/** Deletes a fifo and frees allocated memory.
 *  @param f A pointer to the fifo to be deleted. */
void fifo_delete     (fifo_t *f);

/// Transfers data from one fifo to another.
/** This method transfers n bytes from fifo f1 to f2.
 *  @param f1 First fifo.
 *  @param f2 Second fifo.
 *  @param n  Amount of bytes.
 *  @return   0 on success. */
int  fifo_transfer   (fifo_t *f1, fifo_t *f2, int n);

/// Reads a single byte from fifo.
/** @param f The fifo to be read from.
 *  @return  A single byte from fifo. */
u8   fifo_read_byte  (fifo_t *f);

/// Writes a single byte to fifo.
/** @param f The fifo to be written to.
 *  @param b The byte to be written. */
void fifo_write_byte (fifo_t *f, u8 b);


//
// METHOD IMPLEMENTATIONS
//


void fifo_init(fifo_t *f, size_t s)
{
	// Initialize read and write pointers.
	f->read = 0;
	f->write = 0;
	f->count = 0;
	f->size = s;
	
	// Allocate buffer memory.
	f->data = (char*) vmalloc(s);

	// Initialize wait queue.
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

		f->write = (f->write + 1) % f->size;
		f->count ++;
	}
}
