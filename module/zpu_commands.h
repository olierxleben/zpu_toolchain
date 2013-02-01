
#include <asm/ioctl.h>

#define RAGGED_MAGIC 'z'

#define RAGGED_ZPU_STOP    _IO(RAGGED_MAGIC, 0x01)
#define RAGGED_ZPU_START   _IO(RAGGED_MAGIC, 0x02)
