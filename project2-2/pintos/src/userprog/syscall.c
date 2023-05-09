#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "console.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "user/syscall.h"
#define DEBUG false // TODO make 'DEBUG's 1 value

static void syscall_handler (struct intr_frame *);
bool create (const char *, unsigned );
int open (const char *);

bool create (const char *file, unsigned initial_size);
int open(const char *file);
struct file* get_file_with_fd(int fd);

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

  if (DEBUG)
    printf ("system call: %d\n", syscall_num);

  switch (syscall_num)
  {
    case SYS_HALT:
    {
      halt();
      break;
    }

    case SYS_EXIT:
    {
      int status = (int) (*(uint32_t *)get_argument(f->esp, 1));
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

      // case of null file
      if (file == NULL)
        exit(-1);

      // case of bad pointer
      validate_user_vaddr(file);

      f->eax = create(file, initial_size);

      break;
    }

    case SYS_OPEN:
    {
      char **file_address = get_argument(f->esp, 1);
      validate_user_vaddr(*file_address);
      char *file = *file_address;

      if (file == NULL)
        exit(-1);

      // validate_user_vaddr(file);

      int fd = open(file);
      f->eax = fd;

      break;
    }

    case SYS_CLOSE:
    {
       int fd = *((int *) get_argument(f->esp, 1));
       close(fd);
       break;
    }

    case SYS_EXEC : 
    {
      char* cmd_line = *((char **) get_argument(f->esp,1));
      //todo : implement

      break;
    }

    case SYS_WAIT : 
    {
      pid_t pid = *((pid_t *) get_argument(f->esp,1));

      //todo : implement

      break;
    }
    
    case SYS_REMOVE : 
    {
      char* file_name = *((char **) get_argument(f->esp, 1));
      return filesys_remove(file_name);
      break;
      
    }

    case SYS_FILESIZE : 
    {
      struct file* target_file;
      int fd = get_argument(f->esp, 1);
      target_file = thread_current()->file_descriptor[fd];
      if (target_file == NULL) {
        exit(-1);
      } else {
        return (int)file_length(target_file);
      }
      break;
    }

    case SYS_SEEK : 
    {
      struct file* target_file;

      int fd = *((int*) get_argument(f->esp, 4));
      unsigned position = *((unsigned*) get_argument(f->esp, 5));

      target_file = thread_current()->file_descriptor[fd];
      if (target_file == NULL) {
        exit(-1);
      } else {
        file_seek(target_file, position);
      }
      break;
    }

    case SYS_TELL : 
    {
      struct file* target_file;

      int fd = *((int*) get_argument(f->esp, 1));

      target_file = thread_current()->file_descriptor[fd];
      if(target_file == NULL) {
        exit(-1);
      } else {
        return (unsigned)file_tell(target_file);
      }
      break;

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
  if (DEBUG)
    printf("exit program. status: %d\n", status);
  printf("%s: exit(%d)\n", thread_name(), status);

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
  int i; 
  cur_file = filesys_open(file);
  if (cur_file == NULL) {
    return -1;
  }
  for (i = 3; i < 128; i++) {
    if (thread_current()->file_descriptor[i] == NULL) 
    {
      thread_current()->file_descriptor[i] = cur_file;
      return i;
    }
  }
  exit(-1); // file descriptor is full
}

void close (int fd)
{
  struct file* target_file;

  if (!(fd >= 2 && fd < 128))
    exit(-1);

  target_file = thread_current()->file_descriptor[fd];

  if (target_file == NULL) {
    exit(-1);
  } 
  
  target_file = NULL;
}


