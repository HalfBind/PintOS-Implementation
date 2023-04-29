#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "lib/user/syscall.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");

  int16_t system_call_number = f->eax;

  switch (system_call_number)
  {
  case SYS_WRITE:
    write(f->edi, f->esi, f->edx);
    break;
  
  default:
    printf("Invalid system call number.");
    // TODO error handling
    break;
  }
  
  thread_exit ();
}
