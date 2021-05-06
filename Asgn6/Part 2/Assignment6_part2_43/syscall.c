#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "filesys/file.h"
#include "filesys/filesys.h"


void exit_syscall ();
pid_t exec_syscall ();
int write_syscall ();
void check_addr (void *address);
int get_int (int **sp);

bool create_syscall ();
bool remove_syscall ();
int open_syscall ();
int filesize_syscall ();
int read_syscall ();
void close_syscall ();

void* sp;
struct lock fs_lock;

static void syscall_handler (struct intr_frame *);

void
syscall_init (void)
{
  lock_init (&fs_lock);
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
    case SYS_EXIT:
        exit_syscall ();
        break;
    case SYS_EXEC:
        f->eax = exec_syscall ();
        break;
    case SYS_CREATE:
        f->eax = create_syscall ();
        break;
    case SYS_READ:
        f->eax = read_syscall ();
        break;
    case SYS_OPEN:
        f->eax = open_syscall ();
        break;
    case SYS_FILESIZE:
        f->eax = filesize_syscall ();
        break;
    case SYS_REMOVE:
        f->eax = remove_syscall ();
        break;
    case SYS_CLOSE:
        close_syscall ();
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


struct file_descriptor *
get_file_descriptor (struct thread *t, int fid)
{
  struct list_elem *e;
  for (e = list_begin (&t->files); e != list_end (&t->files); e = list_next(e))
  {
    struct file_descriptor *file_desc = list_entry (e, struct file_descriptor, thread_elem);
    if (file_desc->fid == fid)
      return file_desc;
  }
  return NULL;
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
exit_syscall ()
{
  int status = get_int ((int**) &sp);
  exit (status);
}

pid_t
exec_syscall ()
{
  char*** esp = (char***) &sp;
  check_addr (sp);
  char* cmd_line = **esp;
  (*esp)++;
  check_addr (cmd_line);
 
  int pid = process_execute (cmd_line);
  if (pid == -1) return -1;

  sema_down (&thread_current ()->esema);
  struct child_info* child = parent_child(thread_current(), pid);
  if (child != NULL && child->is_loaded == false)
     return -1;
  
  return pid;
}

int
write_syscall ()
{
  int fd = get_int ((int**) &sp);
  void ***esp = ((void***) &sp);
  check_addr (sp);
  void* buffer = **esp;
  (*esp)++;
  check_addr (buffer);
  
  int length = get_int ((int**) &sp);
  int ret_value = length;

  lock_acquire (&fs_lock);

  if (fd == 1)
    putbuf (buffer, length);
  else
  {
    struct thread* cur = thread_current();
    struct file_descriptor* file_desc = get_file_descriptor (cur, fd);
    if (file_desc == NULL)
      ret_value = -1;
    else 
      ret_value = file_write (file_desc->file, buffer, length);
  }

  lock_release (&fs_lock);
  return ret_value;
}


bool
create_syscall ()
{
  char*** esp = (char***) &sp;
  check_addr (sp);
  char* file = **esp;
  (*esp)++;
  check_addr (file);

  unsigned size = get_int ((int**) &sp);
  lock_acquire (&fs_lock);
  bool status = filesys_create (file, size);
  lock_release (&fs_lock);
  return status;
}

bool
remove_syscall ()
{
  void ***esp = ((void***) &sp);
  check_addr (sp);
  void* file = **esp;
  (*esp)++;
  check_addr (file);
  
  lock_acquire (&fs_lock);
  bool status = filesys_remove (file);
  lock_release (&fs_lock);
  return status;
}


int
open_syscall ()
{
  struct thread* cur = thread_current ();
  
  char*** esp = (char***) &sp;
  check_addr (sp);
  char* file_name = **esp;
  (*esp)++;
  check_addr (file_name);

  lock_acquire (&fs_lock);
  struct file* file = filesys_open (file_name);
  int ret_val = -1;
  if (file != NULL)
  {
    struct file_descriptor *file_des = malloc (sizeof (struct file_descriptor));
    file_des->file = file;
    file_des->fid = cur->fd_gen++;
    list_push_back (&cur->files, &file_des->thread_elem);
    ret_val = file_des->fid;
  }
  lock_release (&fs_lock);
  return ret_val;
}

int
filesize_syscall ()
{
  struct thread* cur = thread_current ();
  int fid = get_int ((int**) &sp);
  lock_acquire (&fs_lock);
  struct file_descriptor* file_desc = get_file_descriptor (cur, fid);
  int length = -1;

  if (file_desc != NULL)
    length = file_length (file_desc->file);
    
  lock_release (&fs_lock);
  return length;
}

int
read_syscall ()
{
  int fd = get_int ((int**) &sp);
  
  void ***esp = ((void***) &sp);
  check_addr (sp);
  void* buffer = **esp;
  (*esp)++;
  check_addr (buffer);
  
  int ret_value = get_int ((int**) &sp);
  // int ret_value = length;
  lock_acquire (&fs_lock);

  if (fd == 0) 
  {
    int i;
    for (i = 0; i < ret_value; i++)
    {
      *((uint8_t*) buffer) = input_getc ();
      buffer += sizeof(uint8_t);
    }
  }
  else
  {
    struct file_descriptor* file_desc = get_file_descriptor (thread_current (), fd);
    if (file_desc == NULL) 
      ret_value = -1;
    else
      ret_value = file_read (file_desc->file, buffer, ret_value);
  }

  lock_release (&fs_lock);
  return ret_value;
}

void
close_syscall ()
{
  int fd = get_int ((int**) &sp);
  struct file_descriptor* file_desc = get_file_descriptor (thread_current (), fd);
  if (file_desc != NULL)
  {
    list_remove (&file_desc->thread_elem);
    file_close (file_desc->file);
    free (file_desc);
  }
}
