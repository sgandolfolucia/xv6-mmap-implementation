#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fcntl.h"
#include "fs.h"
#include "file.h"
#include "proc.h"
#include "defs.h"
#include "memlayout.h"
#include "mmap.h"

uint page_refs[PHYSMEM/PGSIZE]; // an array with one index for each physical page of memory after the kernel base

int mmap_readfile(struct file *f, uint64 va, int offset) {
    ilock(f->ip);
    uint step = readi(f->ip, 1, va, offset, PGSIZE); // read 4096 bytes of the file into the user virtual memory space
    iunlock(f->ip);
    return step;
}

// check if parent has a valid mapping at the fault va
// use walk and walkaddr to get the address of the physical page
// of the mmapped region of the parent process
int mmap_trap_handler(struct proc *p, struct vm_area *vma, uint64 va) {
    va = PGROUNDDOWN(va); // get the start of the virtual page that this address belongs to
    int read_offset = va - vma->start_adr; // get the offset from which the user wishes the read the file
    
    struct proc *parent;
    acquire(p->w_lock);
    parent = p->parent;
    acquire(&parent->lock);
    for(int i = 0; i < MAX_MMAPS; i++) {
        if(!parent->vma[i].valid) continue;
        if(p->vma[i].file == parent->vma[i].file && (va >= parent->vma[i].start_adr && va <= parent->vma[i].end_adr)) {
            uint64 pa = walkaddr(parent->pagetable, va);
            if(pa) {
                if(mappages(p->pagetable, va, PGSIZE, pa, (vma->prot << 1)|PTE_U|PTE_W)) return 0;
                page_refs[PGNUMBER(pa)]++; // increment reference count for physical page
                release(&parent->lock);
                release(p->w_lock);
                return 1;
            }
        }
    }
    release(&parent->lock);
    release(p->w_lock);

    char *mem = kalloc(); // physical memory gods be with me
    if(!mem) {
        printf("out of physical memory!\n");
        return 0;
    }

    // zero out the page
    memset(mem, 0, PGSIZE);
    if(mappages(p->pagetable, va, PGSIZE, (uint64)mem, (vma->prot << 1)|PTE_U|PTE_W)) {
        kfree(mem);
        return 0;
    }

    page_refs[PGNUMBER(((uint64)mem))] = 1; // set reference count to 1 for a freshly allocated page of physical memory    
    return mmap_readfile(vma->file, va, read_offset);
}

void* mmap(void *addr, uint32 length, int prot, int flags, int fd, uint32 offset) {
    struct proc *p = myproc();
    struct file *f = p->ofile[fd];
    uint64 map_adr_start;
    uint64 map_adr_end;

    // replace this with a check for if the address is already in a mapped region 
    // or if the length of the mapping collides with another mapped region
    if(p->vma[p->num_maps].valid || p->num_maps == (MAX_MMAPS - 1)) {
        return MMAP_FAILURE; // can't create a mapping
    }

    if(p->num_maps == 0) {
        // first mapping in virtual memory
        map_adr_start = PGROUNDUP(p->sz + 1); // leave one page of space between the process break point and the first memory mapped region
    } else {
        map_adr_start = PGROUNDUP(p->vma[p->num_maps-1].end_adr + length);
    }
    map_adr_end = PGROUNDUP(map_adr_start + length); // set the end address--its the same in either of the above cases


    // bad address, something crazy happened
    if(map_adr_start >= MAXVA) {
        return MMAP_FAILURE;
    }

    // invalid protections
    if( ((prot & PROT_READ) && !f->readable) || (!f->writable && (prot & PROT_WRITE) && !(flags & MAP_PRIVATE)) ) {
        return MMAP_FAILURE;
    }

    // we can insert a mapping
    p->vma[p->num_maps].valid = 1;
    p->vma[p->num_maps].start_adr = map_adr_start;
    p->vma[p->num_maps].end_adr = map_adr_end;
    p->vma[p->num_maps].len = map_adr_end - map_adr_start;
    p->vma[p->num_maps].prot = prot;
    p->vma[p->num_maps].flags = flags;
    p->vma[p->num_maps].fd = fd;
    p->vma[p->num_maps].file = f;
    p->num_maps++;
    filedup(f); // increase reference count for this file so the information doesn't go away when it is closed

    return (void *)map_adr_start; 
}

int munmap(void *addr, int size) {
    // decrement the number of memory mappings this process has
    // mark the mapping as no longer valid
    // shift the array such that the mappings are still ordered based on ascending starting address
    // close the file so the reference count is decremented
    // write dirty pages back to the file only if the MAP_SHARED flag was passed
    uint64 unmap_adr = PGROUNDDOWN((uint64)addr);
    size = PGROUNDUP(size);
    struct proc *p = myproc();
    struct vm_area *vma = 0;
    for(int i = 0; i < MAX_MMAPS; i++) {
        if(p->vma[i].valid && unmap_adr >= p->vma[i].start_adr && unmap_adr + size <= p->vma[i].end_adr) {
            vma = &(p->vma[i]);
            break;
        }
    }
    if(!vma) {
        return -1; // no valid mapping was found
    }

    // clean up the mapping, writing any dirty pages back to disk only if the MAP_SHARED flag was passed in
    for(uint64 page = unmap_adr; page < unmap_adr + size; page += PGSIZE) {
        pte_t *pte = walk(p->pagetable, page, 0);
        if(*pte & PTE_V) {
            if((*pte & PTE_D) && (vma->prot & MAP_SHARED)) {
                filewrite(vma->file, page, PGSIZE);
            }

            uint64 pa = walkaddr(p->pagetable, page);
            uint refs = --page_refs[PGNUMBER(pa)];
            if(refs == 0) {
                uvmunmap(p->pagetable, page, 1, 1);
            }
        }
    }
    if(size == vma->len) {
        vma->valid = 0;
        fileclose(vma->file);
    }
    return 0; // success
}