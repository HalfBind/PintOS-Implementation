#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

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
  thread_exit ();
}

void validate_user_vaddr (const void *vaddr) {
  if (!is_user_vaddr(vaddr))
    exit(-1); // TODO implement
  if (vaddr == NULL)
    exit(-1); // TODO implement
  if (pagedir_get_page(thread_current()->pagedir, vaddr) == NULL)
    exit(-1); // TODO implement
}

uint32_t get_argument(int32_t *esp, int offset) {
  validate_user_vaddr(esp + offset);
  return *(esp + offset);
}