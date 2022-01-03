// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define REFINDEX(pa) ((pa - (uint64)KERNBASE) / PGSIZE)

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.
int ref[(PHYSTOP - KERNBASE) / PGSIZE];

void freerange(void *pa_start, void *pa_end);

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
	printf("pa_start: %p, pa_end: %p, ref size: %d\n", pa_start, pa_end, NELEM(ref));
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  int index = REFINDEX((uint64)pa);
	if (index >= NELEM(ref)) 
		panic("ref index error");
	if (ref[index] > 0) 
		ref[index]--;
	// if ref count is zero, add to freelist
	if (!ref[index]) {
  	r->next = kmem.freelist;
  	kmem.freelist = r;
	}
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r) {
    kmem.freelist = r->next;
		int index = REFINDEX((uint64)r);
		if (index >= NELEM(ref))
			panic("kalloc");
		ref[index]++;
	}
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

// Just add ref for pa
void
kref(void* pa) {
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kref");
  acquire(&kmem.lock);
  int index = REFINDEX((uint64)pa);
	if (index >= NELEM(ref))
		panic("kref");
	ref[index]++;	
	printf("add ref index: %d pa: %p to %d\n", index, pa, ref[index]);
  release(&kmem.lock);
}
