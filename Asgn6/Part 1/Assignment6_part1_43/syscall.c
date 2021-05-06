#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"


void exit_syscall (void);
pid_t exec_syscall (void);
int write_syscall (void);
void check_addr (void *address);
int get_int (int **sp);

void* sp;

static void syscall_handler (struct intr_frame *);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  sp = f->esp;
  check_addr (f->esp);
  int sys_call = get_int ((int**) &sp);
  switch (sys_call)
  {
    case SYS_WRITE:
        f->eax = write_syscall ();
        break;
    case SYS_EXEC:
        f->eax = exec_syscall ();
        break;
    case SYS_EXIT:
        exit_syscall ();
        break;
    default:
      break;
  }
}


int
get_int (int **esp)
{
  check_addr (sp);
  int ret = **esp;
  (*esp)++;
  return ret;
}

void
exit (int status)
{
  struct thread* parent = get_thread (thread_current ()->parent_id);
  if (parent != NULL)
  {
    struct child_info* child_info_elem = parent_child (parent, thread_current ()->tid);
     lock_acquire (&parent->waiting_lock);
    if (child_info_elem != NULL)
    {
      child_info_elem->exit_status = status;
    }
    lock_release (&parent->waiting_lock);
  }

  thread_current ()->exit_status = status;
  thread_exit ();
}

void
check_addr (void *address)
{
  if (address == NULL || is_kernel_vaddr (address) || pagedir_get_page (thread_current ()->pagedir, address) == NULL) 
    exit (-1);
}

void
exit_syscall (void)
{
  int status = get_int ((int**) &sp);
  exit (status);
}

pid_t
exec_syscall (void)
{
  char*** esp = (char***) &sp;
  check_addr (sp);
  char* ret = **esp;
  (*esp)++;
  check_addr (ret);
  char *cmd_line = ret;
 
  int pid = process_execute (cmd_line);
  if (pid == -1) return -1;

  sema_down (&thread_current ()->esema);
  struct child_info* child = parent_child(thread_current(), pid);
  if (child != NULL && child->is_loaded == false)
     return -1;
  
  return pid;
}

int
write_syscall (void)
{
  int fd = get_int ((int**) &sp);
  void ***esp = ((void***) &sp);
  check_addr (sp);
  void* ret = **esp;
  (*esp)++;
  check_addr (ret);
  void* buffer = ret;
  
  int length = get_int ((int**) &sp);
  int ret_value = length;

  if (fd == 1){
    putbuf (buffer, length);
    return ret_value;
  }

  return 0;
}

