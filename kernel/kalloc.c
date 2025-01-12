// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define PGNUM (PHYSTOP - KERNBASE) / PGSIZE

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct {
  struct spinlock lock[PGNUM];
  int counters[PGNUM];
} phys_pages;

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
  for(int i = 0; i < PGNUM; i++) {
    initlock(&phys_pages.lock[i], "phys_page");
    phys_pages.counters[i] = 1;
  }
  freerange(end, (void*)PHYSTOP);
}

void
reference_counter_increment(void *pa)
{
  int index = ((uint64)pa - KERNBASE) / PGSIZE;
  acquire(&phys_pages.lock[index]);
  phys_pages.counters[index]++;
  release(&phys_pages.lock[index]);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
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

  int index = ((uint64)pa - KERNBASE) / PGSIZE;

  acquire(&phys_pages.lock[index]);
  phys_pages.counters[index]--;
  if(phys_pages.counters[index] > 0) {
    release(&phys_pages.lock[index]);
    return;
  }
  release(&phys_pages.lock[index]);

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
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
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
  {
    reference_counter_increment(r);
    memset((char*)r, 5, PGSIZE); // fill with junk
  }
  return (void*)r;
}