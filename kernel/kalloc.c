// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct {
  struct spinlock lock;
  uint cnt[(PHYSTOP - KERNBASE) / PGSIZE]; // 引用计数数组
} refcnt;

#define PA2IDX(pa) (((uint64)pa - KERNBASE) / PGSIZE) // 索引计算逻辑

// add cnt
void refcnt_add(uint64 pa) {
  acquire(&refcnt.lock);
  refcnt.cnt[PA2IDX(pa)]++;
  release(&refcnt.lock);
}

// set cnt
void refcnt_setter(uint64 pa, uint n) {
  refcnt.cnt[PA2IDX(pa)] = n;
}

// get cnt
uint refcnt_getter(uint64 pa) {
  return refcnt.cnt[PA2IDX(pa)];
}

// kalloc() without lock
void *
kalloc_nolock(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  
  if (r)
    refcnt.cnt[PA2IDX((uint64)r)]++;
  return (void*)r;
}

// create new physical page
int refcnt_new(uint64 va, pagetable_t pagetable) {
  pte_t *pte;
  uint64 pa;
  uint flags, cnt;

  va = PGROUNDDOWN(va);
  pte = walk(pagetable, va, 0);
  pa = PTE2PA(*pte);
  flags = PTE_FLAGS(*pte);

  if (!(flags & PTE_COW)) // 非COW页，不予处理
    return -2;

  acquire(&refcnt.lock);
  cnt = refcnt_getter(pa);
  if (cnt > 1) { // 多页则需要创建新页
    char *mem = kalloc_nolock();
    if (mem == 0) // 空闲内存不足
      goto bad;
    memmove(mem, (char *)pa, PGSIZE); // 复制旧页到新页
    uvmunmap(pagetable, va, 1, 0); // 需要旧页原有的映射
    if (mappages(pagetable, va, PGSIZE, (uint64)mem, (flags & ~PTE_COW) | PTE_W) != 0) { // 设置新映射
      kfree(mem);
      goto bad;
    }
    refcnt_setter(pa, cnt - 1); // 旧页引用计数-1
  } else { // 单页直接写入
    *pte = (*pte & ~PTE_COW) | PTE_W;
  }
  release(&refcnt.lock);
  return 0;

  bad:
    release(&refcnt.lock);
    return -1;
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&refcnt.lock, "refcnt");
  memset(refcnt.cnt, 0, sizeof(refcnt.cnt)); // 数组初始化为0
  freerange(end, (void*)PHYSTOP);
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

  acquire(&refcnt.lock);
  int cnt = refcnt_getter((uint64)pa);
  if (cnt > 1) { // 存在多个引用，不释放内存
    refcnt_setter((uint64)pa, cnt - 1);
    release(&refcnt.lock);
    return;
  }

  // 清零计数
  refcnt_setter((uint64)pa, 0);
  release(&refcnt.lock);

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
    memset((char*)r, 5, PGSIZE); // fill with junk
  
  if (r)
    refcnt_add((uint64)r);
  return (void*)r;
}
