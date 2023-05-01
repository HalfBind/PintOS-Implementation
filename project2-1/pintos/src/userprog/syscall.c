#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "console.h"
#include "filesys/filesys.h"

static void syscall_handler (struct intr_frame *);
bool create (const char *, unsigned );
int open (const char *);

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

  // printf ("system call: %d\n", syscall_num);

  switch (syscall_num)
  {
    case SYS_HALT:
    {
      halt();
      break;
    }

    case SYS_EXIT:
    {
      int status = *((int *) get_argument(f->esp, 1));
      exit(status);
      break;
    }

    case SYS_WRITE:
    {
      int fd = *((int *) get_argument(f->esp, 5));
      // void ** buffer_pointer = *(void ***) get_argument(f->esp, 6);

      uint32_t *buffer_ptr = (uint32_t *) get_argument(f->esp, 6);
      validate_user_vaddr(buffer_ptr);

      void *buffer = *buffer_ptr;
      unsigned size = *((unsigned *) get_argument(f->esp, 7));

      f->eax = write(fd, buffer, size);
      break;
    }

    case SYS_CREATE:
    {
      char *file = *(char **) get_argument(f->esp, 4);
      unsigned initial_size = *(unsigned *) get_argument(f->esp, 5);
       f->eax = create(file, initial_size);

      break;
    }

    case SYS_OPEN:
    {
      char *file = *(char **) get_argument(f->esp, 1);
      f->eax = open(file);
      break;
    }

    case SYS_CLOSE:
    {

    }

    default:
    {
      printf("Invalid system call number: %d\n", syscall_num);
      exit(-1);
      break;
    }
  }
}

void validate_user_vaddr (const void *vaddr) {
  if (!is_user_vaddr(vaddr))
    exit(-1);
  if (vaddr == NULL)
    exit(-1);
  if (pagedir_get_page(thread_current()->pagedir, vaddr) == NULL)
    exit(-1);
}

void *get_argument(void *esp, int offset) {
  validate_user_vaddr(esp + 4 * offset);
  return esp + 4 * offset;
}

void halt (void)
{
  shutdown_power_off();
}

void exit (int status)
{
  // printf("exit program. status: %d", status);
  thread_exit();
}

int write (int fd, const void *buffer, unsigned size)
{
  if (fd == 1)
  {
    putbuf(buffer, size);
    return size;
  }
}

bool create (const char *file, unsigned initial_size)
{
  return filesys_create(file, initial_size);
}

int open (const char *file)
{
 struct file * cur_file;
 int ret = -1;
 int i; 
 cur_file = filesys_open(file);
 if(cur_file == NULL) {
    return -1;
 } else {
  for(i=3; i<128; i++) {
    if(thread_current()->file_descriptor[i] == NULL) {
      thread_current()->file_descriptor[i] = cur_file;
      ret = i;
      break;
    }
  }
 }
 return ret;

}

void close (int fd)
{
  
}