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
#include "userprog/process.h"
#include "threads/synch.h"

#define DEBUG false // TODO make 'DEBUG's 1 value

static void syscall_handler (struct intr_frame *);
bool create (const char *, unsigned );
int open (const char *);
pid_t execute (char *cmd_line);
bool create (const char *file, unsigned initial_size);
int open(const char *file);
struct file* get_file_with_fd(int fd);
struct lock file_lock;

void
syscall_init (void) 
{
  lock_init(&file_lock);
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
    case SYS_READ:
    {
      int fd = *((int *) get_argument(f->esp, 5));
      if (!(fd >= 0 && fd < 128))
       exit(-1);
      int i = 0, read_size = 0;
      struct file * target_file;

      uint32_t *buffer_ptr = (uint32_t *) get_argument(f->esp, 6);
      void *buffer = *buffer_ptr;
      validate_user_vaddr(buffer);
      lock_acquire(&file_lock);
      unsigned size = *((unsigned *) get_argument(f->esp, 7));
      if (fd == 0) {
        for (i = 0; i < size; i++) {
          if (input_getc () == '/0') {
            break;
          }
          read_size++;
        }
      } else if (fd > 1) {
        target_file = thread_current()->file_descriptor[fd];
        if (target_file == NULL) {
          read_size = -1;
        } else {
          read_size = file_read(target_file, buffer, size);
        }
      }

      lock_release(&file_lock);
      f->eax = (int)read_size;
      break;

    }
    case SYS_WRITE:
    {
      int fd = *((int *) get_argument(f->esp, 5));
      if (!(fd >= 1 && fd < 128))
       exit(-1);
      
      int i = 0, write_size = 0;
      struct file* target_file;

      uint32_t *buffer_ptr = (uint32_t *) get_argument(f->esp, 6);
      void *buffer = *buffer_ptr;

      validate_user_vaddr(buffer);
      lock_acquire(&file_lock);
      unsigned size = *((unsigned *) get_argument(f->esp, 7));

      if (fd == 1) { 
        putbuf(buffer, size);
        write_size =  size;
      } else {
        target_file = thread_current()->file_descriptor[fd];
        if(target_file == NULL) {
          lock_release(&file_lock);
          write_size = -1;
        } else {
          write_size = file_write(target_file, buffer, size);
        }
      }

      lock_release(&file_lock);
      f->eax = (int)write_size;
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
      char *file = *(char **)get_argument(f->esp, 1);
      validate_user_vaddr(file);
      lock_acquire(&file_lock);

      struct file * cur_file;
      int  i = 0, ret_fd = -1; 

      if (file == NULL) {
        lock_release(&file_lock);
        exit(-1);
      }

      cur_file = filesys_open(file);
      if (cur_file == NULL) {
        ret_fd =  -1;
      } else {
        for (i = 3; i < 128; i++) {
          if (thread_current()->file_descriptor[i] == NULL) {
            if(strcmp(thread_current()-> name, file)== 0) {
              file_deny_write(cur_file);
            }
            thread_current()->file_descriptor[i] = cur_file;
            ret_fd= i;
            break;
          }
        }
        if(ret_fd == -1) {
          // printf('::// ret_fd is -1');
          file_close(cur_file);
          lock_release(&file_lock);
          // exit(-1); // file descriptor is full
        }

      }
  
      lock_release(&file_lock);
      f->eax = ret_fd;

      break;
    }

    case SYS_CLOSE:
    {
       int fd = *((int *) get_argument(f->esp, 1));
       close(fd);
       break;
    }

    case SYS_EXEC: 
    {
      char* cmd_line = *((char **) get_argument(f->esp, 1));
      f->eax = execute (cmd_line);

      break;
    }

    case SYS_WAIT: 
    {
      pid_t pid = *((pid_t *) get_argument(f->esp, 1));

      f->eax = wait(pid);

      break;
    }

    case SYS_REMOVE:
    {
      char* file_name = *((char **) get_argument(f->esp, 1));
      f->eax = filesys_remove(file_name);
      break;
      
    }

    case SYS_FILESIZE:
    {
      struct file* target_file;
      int fd = *((int*) get_argument(f->esp, 1));
      target_file = thread_current()->file_descriptor[fd];

      if (target_file == NULL) {
        exit(-1);
      } 
      f->eax = (int)file_length(target_file);
      break;
    }

    case SYS_SEEK:
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

    case SYS_TELL:
    {
      struct file* target_file;

      int fd = *((int*) get_argument(f->esp, 1));

      target_file = thread_current()->file_descriptor[fd];
      if(target_file == NULL) {
        exit(-1);
      } 
      f->eax = (unsigned)file_tell(target_file);
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

  set_exit_thread (status);
  thread_exit ();
}

bool create (const char *file, unsigned initial_size)
{
  return filesys_create(file, initial_size);
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

int wait (pid_t pid)
{
  return process_wait(pid);
}

pid_t execute (char *cmd_line)
{
  return (tid_t) process_execute (cmd_line);
}