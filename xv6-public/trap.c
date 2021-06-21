#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);
  SETGATE(idt[128], 0, SEG_KCODE<<3, vectors[128], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  if(tf->trapno == 128){
    cprintf("user interrupt 128 called!\n");
    exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      if(myproc() && myproc()->state == RUNNING) myproc()->rTime++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
#ifdef FCFS_SCHED
  if(myproc() && myproc()->state == RUNNING && (ticks - myproc()->sTime) >= 200) {
    myproc()->killed = 1;
    cprintf("killed the process %d because it has been over 200 ticks\n", myproc()->pid);
  }

#elif MULTILEVEL_SCHED
  if(myproc() && myproc()->state == RUNNING && (myproc()->pid % 2) == 0) { // pid 짝수면 RR
    if(tf->trapno == T_IRQ0+IRQ_TIMER)
      yield();
  }
  // pid 홀수면 FCFS

#elif MLFQ_SCHED
  if(myproc() && myproc()->state == RUNNING && myproc()->monopoly != 1) { // monopoly가 1인 것은 무시하고 진행
    if(myproc()->qLevel == 0 && myproc()->rTime >= 4) { // L0 큐에 있던 프로세스 4 ticks 넘었을 때
      myproc()->qLevel = 1;
      yield();
    } else if(myproc()->qLevel == 1 && myproc()->rTime >= 8) { // L1 큐에 있던 프로세스 8 ticks 넘었을 때
      if(myproc()->priority > 0) myproc()->priority--;
      yield();
    }
  }

  if(ticks%200 == 0) priorityBoosting(); // 처음 ticks은 0부터 시작하므로 200의 나머지로 계산하면, 매 200 ticks 마다 boost 가능

#else
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();
#endif

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
