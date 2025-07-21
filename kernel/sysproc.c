#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  uint64 start; // 起始虚拟地址
  int len;   // 页面数量
  uint64 mask;// 位掩码缓冲区地址

  // 获取参数
  if (argaddr(0, &start) < 0 || argint(1, &len) < 0 
    || argaddr(2, &mask) < 0)
    return -1;
  
  // 对应掩码缓冲区长度
  if (len < 1 || len > 64)
    return -1;

  struct proc *p = myproc();
  pagetable_t pagetable = p->pagetable;

  // 在内核中创建临时缓冲区存储结果
  uint64 abits = 0;

  for (int i = 0; i < len; ++i) {
    uint64 va = start + i * PGSIZE;
    pte_t *pte = walk(pagetable, va, 0); // 获取对应的PTE
    
    if (pte == 0) continue; // PTE不存在

    if (*pte & PTE_A) {
      abits |= (1 << i);
      *pte &= ~PTE_A; // 清除
    }
  }
  
  // 复制到用户空间
  if (copyout(pagetable, mask, (char *)&abits, sizeof(abits)) < 0)
    return -1;

  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int sys_pageccess(void) {
  uint64 start; // 起始虚拟地址
  int len;   // 页面数量
  uint64 mask;// 位掩码缓冲区地址

  // 获取参数
  if (argaddr(0, &start) < 0 || argint(1, &len) < 0 
    || argaddr(2, &mask) < 0)
    return -1;
  
  // 对应掩码缓冲区长度
  if (len < 1 || len > 64)
    return -1;

  struct proc *p = myproc();
  pagetable_t pagetable = p->pagetable;

  vmprint(pagetable);

  // 在内核中创建临时缓冲区存储结果
  uint64 abits = 0;

  for (int i = 0; i < len; ++i) {
    uint64 va = start + i * PGSIZE;
    pte_t *pte = walk(pagetable, va, 0); // 获取对应的PTE
    
    if (pte == 0) continue; // PTE不存在

    if (*pte & PTE_A) {
      abits |= (1 << i);
      *pte &= ~PTE_A; // 清除
    }
  }
  
  // 复制到用户空间
  if (copyout(pagetable, mask, (char *)&abits, sizeof(abits)) < 0)
    return -1;

  return 0;
}
