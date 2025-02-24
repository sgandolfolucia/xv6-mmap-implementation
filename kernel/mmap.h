#define MMAP_FAILURE (char *) -1
#define PHYSMEM 128*1028*1028
#define PGNUMBER(pa) ((pa - KERNBASE) / PGSIZE)
extern uint page_refs[]; // an array with one index for each physical page of memory after the kernel base
