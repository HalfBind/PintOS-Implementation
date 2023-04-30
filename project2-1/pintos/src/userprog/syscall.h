#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void *get_argument(void *esp, int offset);

#endif /* userprog/syscall.h */
