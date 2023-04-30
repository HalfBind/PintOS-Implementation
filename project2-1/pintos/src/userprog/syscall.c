#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  // validate address
  validate_user_vaddr (f->esp);

  // system call number
  int syscall_num = (int) *((int *) f->esp);
  
  switch (syscall_num)
  {
    default:
      break;
  }

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

void validate_user_vaddr (const void *vaddr) {
  if (!is_user_vaddr(vaddr))
    exit(-1);
  if (vaddr == NULL)
    exit(-1);
  if (pagedir_get_page(thread_current()->pagedir, vaddr) == NULL)
    exit(-1);
}

uint32_t get_argument(int32_t *esp, int offset) {
  validate_user_vaddr(esp + offset);
  return *(esp + offset);
}

void exit (int status)
{
  printf("exit program. status: %d", status);
  thread_exit();
}
