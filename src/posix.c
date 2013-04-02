/* hbs1 - Host Basic Services - ver. 1
 *
 * Code specific to POSIX platforms
 *
 * Changelog:
 *  - 2013/01/06 Costin Ionescu: initial release
 *
 */
#define _LARGEFILE64_SOURCE
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>

#include <hbs1.h>

typedef struct io_file_s io_file_t;
struct io_file_s
{
  c41_io_t io;
  int fd;
};

static uint_t C41_CALL io_read
(
  c41_io_t *    io_p,
  void *        data,
  size_t        size,
  size_t *      used_size_p
);
static uint_t C41_CALL io_write
(
  c41_io_t *    io_p,
  void const *  data,
  size_t        size,
  size_t *      used_size_p
);
static uint_t C41_CALL io_seek64
(
  c41_io_t *    io_p,
  int64_t       disp,
  int           anchor
);
static uint_t C41_CALL io_truncate (c41_io_t * io_p);
static uint_t C41_CALL io_close (c41_io_t * io_p);

static c41_io_t * C41_CALL iof_create (int fd);

static uint_t C41_CALL file_open
(
  c41_io_t * *              io_pp,
  uint8_t const *           path_a,
  size_t                    path_n,
  uint32_t                  mode,
  void *                    context
);
static uint_t C41_CALL file_destroy
(
  c41_io_t *                io_p,
  void *                    context
);

static c41_io_type_t iof_type =
{
  io_read,
  io_write,
  io_seek64,
  io_truncate,
  io_close,
  1
};

/* hbs1_lib_name ************************************************************/
HBS1_API char const * C41_CALL hbs1_lib_name ()
{
  return "hbs1-posix";
}

/* alloc ********************************************************************/
static c41_uint_t ma_alloc
(
  void * * data_pp,
  size_t len,
  void * ctx
)
{
  (void) ctx;
  if ((*data_pp = malloc(len))) return 0;
  return C41_MA_NO_MEM;
}

/* realloc ******************************************************************/
static c41_uint_t ma_realloc
(
  void * * data_pp,
  size_t new_len,
  size_t old_len,
  void * ctx
)
{
  void * data_p;
  (void) ctx;
  (void) old_len;

  data_p = realloc(*data_pp, new_len);
  if (!data_p) return C41_MA_NO_MEM;
  *data_pp = data_p;
  return 0;
}

/* free *********************************************************************/
static c41_uint_t ma_free
(
  void * data_p,
  size_t len,
  void * ctx
)
{
  (void) ctx;
  (void) len;
  free(data_p);
  return 0;
}

/* hbs1_ma_init *************************************************************/
HBS1_API c41_uint_t C41_CALL hbs1_ma_init (c41_ma_t * ma_p)
{
  ma_p->ctx = NULL;
  ma_p->alloc = ma_alloc;
  ma_p->realloc = ma_realloc;
  ma_p->free = ma_free;
  return 0;
}

/* hbs1_ma_finish ***********************************************************/
HBS1_API c41_uint_t C41_CALL hbs1_ma_finish (c41_ma_t * ma_p)
{
  (void) ma_p;
  return 0;
}

#define E()                                                                  \
  switch (errno)                                                             \
  {                                                                          \
  case EAGAIN:                  return C41_IO_WOULD_BLOCK;                   \
  case EINTR:                   return C41_IO_SIGNAL;                        \
  case EIO:                     return C41_IO_MEDIA_ERROR;                   \
  default:                      return C41_IO_FAILED;                        \
  }

/* io_read ******************************************************************/
static uint_t C41_CALL io_read
(
  c41_io_t *    io_p,
  void *        data,
  size_t        size,
  size_t *      used_size_p
)
{
  io_file_t * f = (io_file_t *) io_p;
  ssize_t s;

  s = read(f->fd, data, size);
  if (s >= 0)
  {
    *used_size_p = s;
    return 0;
  }
  E();
}

/* io_write *****************************************************************/
static uint_t C41_CALL io_write
(
  c41_io_t *    io_p,
  void const *  data,
  size_t        size,
  size_t *      used_size_p
)
{
  io_file_t * f = (io_file_t *) io_p;
  ssize_t s;

  s = write(f->fd, data, size);
  if (s >= 0)
  {
    *used_size_p = s;
    return 0;
  }
  E();
}


/* io_seek64 ****************************************************************/
static uint_t C41_CALL io_seek64
(
  c41_io_t *    io_p,
  int64_t       disp,
  int           anchor
)
{
  io_file_t * f = (io_file_t *) io_p;
  f->io.pos = lseek64(f->fd, disp, anchor);
  if (f->io.pos >= 0) return 0;
  E();
}

/* io_truncate **************************************************************/
static uint_t C41_CALL io_truncate (c41_io_t * io_p)
{
  io_file_t * f = (io_file_t *) io_p;
  if (!ftruncate(f->fd, f->io.pos)) return 0;
  E();
}

/* io_close *****************************************************************/
static uint_t C41_CALL io_close (c41_io_t * io_p)
{
  io_file_t * f = (io_file_t *) io_p;
  if (!close(f->fd)) return 0;
  E();
}

/* iof_create ***************************************************************/
static c41_io_t * C41_CALL iof_create (int fd)
{
  io_file_t * f = malloc(sizeof(io_file_t));
  if (!f) return NULL;
  memset(f, 0, sizeof(io_file_t));
  f->io.io_type_p = &iof_type;
  f->fd = fd;
  return &f->io;
}

/* hbs1_io_destroy **********************************************************/
HBS1_API uint_t C41_CALL hbs1_destroy_std_io (c41_io_t * io_p)
{
  free(io_p);
  return 0;
}

/* hbs1_stdin ***************************************************************/
HBS1_API uint_t C41_CALL hbs1_stdin (c41_io_t * * stdin_pp)
{
  *stdin_pp = iof_create(0);
  if (!*stdin_pp) return HBS1_NO_RES;
  return 0;
}

/* hbs1_stdout **************************************************************/
HBS1_API uint_t C41_CALL hbs1_stdout (c41_io_t * * stdout_pp)
{
  *stdout_pp = iof_create(1);
  if (!*stdout_pp) return HBS1_NO_RES;
  return 0;
}

/* hbs1_stderr **************************************************************/
HBS1_API uint_t C41_CALL hbs1_stderr (c41_io_t * * stderr_pp)
{
  *stderr_pp = iof_create(2);
  if (!*stderr_pp) return HBS1_NO_RES;
  return 0;
}

/* file_open ****************************************************************/
static uint_t C41_CALL file_open
(
  c41_io_t * *              io_pp,
  uint8_t const *           path_a,
  size_t                    path_n,
  uint32_t                  mode,
  void *                    context
)
{
  int oflags, omode, fd;

  (void) context;
  switch (mode & (C41_FSI_READ | C41_FSI_WRITE))
  {
  case C41_FSI_READ:
    oflags = O_RDONLY;
    break;
  case C41_FSI_WRITE:
    oflags = O_WRONLY;
    break;
  case C41_FSI_READ | C41_FSI_WRITE:
    oflags = O_RDWR;
    break;
  default:
    return C41_FSI_MISSING_ACCESS;
  }
  switch (mode & C41_FSI_EXF_MASK)
  {
  case C41_FSI_EXF_REJECT:
    // this must be create
    oflags |= O_CREAT | O_EXCL;
    break;
  // case C41_EXF_OPEN:
  //   break;
  case C41_FSI_EXF_TRUNC:
    oflags |= O_TRUNC;
    break;
  }
  if ((mode & C41_FSI_NEWF_MASK) == C41_FSI_NEWF_CREATE) oflags |= O_CREAT;
  if ((mode & C41_FSI_NON_BLOCKING)) oflags |= O_NONBLOCK;
  //if (!(mode & C41_FSI_INHERITABLE)) oflags |= O_CLOEXEC;
  if ((mode & C41_FSI_WRITE_THROUGH)) oflags |= O_SYNC;

  omode = (mode >> C41_FSI_PERM_SHIFT) & 0777;

  if (path_n == 0 || path_a[path_n - 1] != 0)
    return C41_FSI_BAD_PATH;
  fd = open((char const *) path_a, oflags, omode);
  if (fd < 0)
  {
    // failed
    return C41_FSI_OPEN_FAILED;
  }
  *io_pp = iof_create(fd);
  if (!*io_pp) return C41_FSI_NO_RES;
  return 0;
}

/* file_destroy *************************************************************/
static uint_t C41_CALL file_destroy
(
  c41_io_t *                io_p,
  void *                    context
)
{
  (void) context;
  free(io_p);
  return 0;
}

/* hbs1_fsi_init ************************************************************/
HBS1_API uint_t C41_CALL hbs1_fsi_init (c41_fsi_t * fsi_p, c41_fspi_t * fspi_p)
{
  C41_VAR_ZERO(*fsi_p);
  fsi_p->file_open = file_open;
  fsi_p->file_destroy = file_destroy;
  c41_fsp_unix(fspi_p);
  return 0;
}

/* hbs1_fsi_finish **********************************************************/
HBS1_API uint_t C41_CALL hbs1_fsi_finish (c41_fsi_t * fsi_p)
{
  (void) fsi_p;
  return 0;
}

/* thread_proc_ctx_t ********************************************************/
typedef struct thread_proc_ctx_s thread_proc_ctx_t;
struct thread_proc_ctx_s
{
  c41_smt_thread_f func;
  void * arg;
};

/* thread_proc **************************************************************/
static void * thread_proc (void * arg)
{
  thread_proc_ctx_t * c = arg;
  c41_smt_thread_f func;
  void * fa;

  func = c->func;
  fa = c->arg;
  free(c);
  return (void *) (intptr_t) func(fa);
}

/* thread_create ************************************************************/
static uint_t C41_CALL thread_create
  (
    c41_smt_t * smt_p,
    c41_smt_tid_t * tid_p,
    c41_smt_thread_f func,
    void * arg
  )
{
  thread_proc_ctx_t * c;
  pthread_t t;
  int r;

  (void) smt_p;
  c = malloc(sizeof(thread_proc_ctx_t));
  if (!c) return C41_SMT_NO_RES;
  c->func = func;
  c->arg = arg;

  r = pthread_create(&t, NULL, thread_proc, c);
  if (r)
  {
    free(c);
    return C41_SMT_FAIL;
  }

  *tid_p = (c41_smt_tid_t) (intptr_t) t;

  return 0;
}

/* thread_join **************************************************************/
static int C41_CALL thread_join (c41_smt_t * smt_p, c41_smt_tid_t tid)
{
  int r;
  void * v;

  (void) smt_p;
  r = pthread_join((pthread_t) tid, &v);
  if (r) return -C41_SMT_FAIL;

  return (int) (intptr_t) v;
}

/* mutex_init ***************************************************************/
static uint_t C41_CALL mutex_init (c41_smt_t * smt_p, c41_smt_mutex_t * mutex_p)
{
  int i;
  (void) smt_p;
  i = pthread_mutex_init((pthread_mutex_t *) mutex_p, NULL);
  if (i) return C41_SMT_FAIL;
  return 0;
}

/* mutex_finish *************************************************************/
static uint_t C41_CALL mutex_finish (c41_smt_t * smt_p, c41_smt_mutex_t * mutex_p)
{
  int i;
  (void) smt_p;
  i = pthread_mutex_destroy((pthread_mutex_t *) mutex_p);
  if (i) return C41_SMT_FAIL;
  return 0;
}

/* mutex_lock ***************************************************************/
static uint_t C41_CALL mutex_lock (c41_smt_t * smt_p, c41_smt_mutex_t * mutex_p)
{
  int i;
  (void) smt_p;
  i = pthread_mutex_lock((pthread_mutex_t *) mutex_p);
  if (i) return C41_SMT_FAIL;
  return 0;
}

/* mutex_trylock ************************************************************/
static uint_t C41_CALL mutex_trylock (c41_smt_t * smt_p, c41_smt_mutex_t * mutex_p)
{
  int i;
  (void) smt_p;
  i = pthread_mutex_trylock((pthread_mutex_t *) mutex_p);
  if (i) return C41_SMT_FAIL;
  return 0;
}

/* mutex_unlock *************************************************************/
static uint_t C41_CALL mutex_unlock (c41_smt_t * smt_p, c41_smt_mutex_t * mutex_p)
{
  int i;
  (void) smt_p;
  i = pthread_mutex_unlock((pthread_mutex_t *) mutex_p);
  if (i) return C41_SMT_FAIL;
  return 0;
}

/* cond_init ****************************************************************/
static uint_t C41_CALL cond_init (c41_smt_t * smt_p, c41_smt_cond_t * cond_p)
{
  (void) smt_p;
  if (pthread_cond_init((pthread_cond_t *) cond_p, NULL)) return C41_SMT_FAIL;
  return 0;
}

/* cond_finish **************************************************************/
static uint_t C41_CALL cond_finish (c41_smt_t * smt_p, c41_smt_cond_t * cond_p)
{
  (void) smt_p;
  if (pthread_cond_destroy((pthread_cond_t *) cond_p)) return C41_SMT_FAIL;
  return 0;
}

/* cond_signal **************************************************************/
static uint_t C41_CALL cond_signal (c41_smt_t * smt_p, c41_smt_cond_t * cond_p)
{
  (void) smt_p;
  if (pthread_cond_signal((pthread_cond_t *) cond_p)) return C41_SMT_FAIL;
  return 0;
}

/* cond_wait ****************************************************************/
static uint_t C41_CALL cond_wait (c41_smt_t * smt_p, c41_smt_cond_t * cond_p,
                                 c41_smt_mutex_t * mutex_p)
{
  (void) smt_p;
  if (pthread_cond_wait((pthread_cond_t *) cond_p, (pthread_mutex_t *) mutex_p))
    return C41_SMT_FAIL;
  return 0;
}

/* hbs1_smt_init ************************************************************/
HBS1_API uint_t C41_CALL hbs1_smt_init (c41_smt_t * smt_p, char const * impl)
{
  if (impl && !C41_STR_EQUAL(impl, "posix")) return HBS1_NO_SUP;
  smt_p->thread_create = thread_create;
  smt_p->thread_join = thread_join;

  smt_p->mutex_size = sizeof(pthread_mutex_t);
  smt_p->mutex_init = mutex_init;
  smt_p->mutex_finish = mutex_finish;
  smt_p->mutex_lock = mutex_lock;
  smt_p->mutex_trylock = mutex_trylock;
  smt_p->mutex_unlock = mutex_unlock;

  smt_p->cond_size = sizeof(pthread_cond_t);
  smt_p->cond_init = cond_init;
  smt_p->cond_finish = cond_finish;
  smt_p->cond_signal = cond_signal;
  smt_p->cond_wait = cond_wait;

  return 0;
}

