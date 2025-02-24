# Mmap Outline:
- find unused address (outside region sbrk can modify). user can request starting address, but this is ultimately still up to the kernel. however, the manpages indicate that the first virtual address of the mapping will always be at or above the address passed in by the user

- return address to user, handle everything else when they try to access this memory
    - first access should cause a pagefault to occur
        - allocate a page of physical memory 
        - create a virtual memory mapping to this newly allocated physical memory starting at the address returned to the user
        - if the mapping is file backed (aka not called with the flag MAP_ANON), read the a page (4096 bytes) of the file into this region of virtual memory
    - subsequent accesses may also cause page faults because only a sinlge page of physical memory is allocated and given a virtual mapping at once
        - this allows for a more efficient implementation because it means large files can be mapped quickly and files larger than physical memory can be mapped

# Munmap Outline:
- for the most part, just undoing what mmap has done
    - marking virtual memory region invalid
    - decreasing counter for the number of mapped regions
    - decrement reference count for mapped file (so the struct doesn't disappear if the file is closed)
- if the MAP_SHARED flag is passed to mmap, munmap should write any changes made to the mapped region back to the underlying file
    - it is also possible that a kernel thread is periodically checking if the mapped pages are dirty and writing them back to the underlying file. in this case, changes may be reflected even without calling munmap. 
    - it is also possible to create a memory mapping directly from a virtual address to the disk (virtio.h has some information about this), but I am unsure how this relates to implementations of mmap.


# My Strategy:
- each process has an array of 16 virtual memory area structs that contain metadata about the mapped region.
- mmap itself does not do much besides find the address and allocate from the array of virtual memory structs, setting the correct flags and protections.
    - to find an address, my approach is to keep the array of mapped regions sorted. 
        - the first mapping is placed one full page past the programs break point (this gives sbrk a bit of room to continue to build the heap)
        - subsequent mappings are simply aligned to the nearest page past the end address of the previous mapping
    - when munmap is called, shift all elements in the vm_are array such that there are no gaps between elements. as a result, the array remains sorted in ascending order as long as new mappings are always placed after the newest element
        - this is far from a perfect strategy because it means the address selected for a new mapping must always be greater than the address of the last element in the array, which could cause the mapped addresses to grow excessively large. It would be possible to adjust this and maintain the sorted array in a more complex way, but it should work for a simple implementation