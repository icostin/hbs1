/* hbs1 - Host Basic Services - ver. 1
 *
 * Code specific to user-mode Windows
 *
 * Changelog:
 *  - 2013/01/06 Costin Ionescu: initial release
 *
 */
#include <windows.h>
#include <hbs1.h>

typedef struct io_file_s io_file_t;
struct io_file_s
{
  c41_io_t io;
  HANDLE h;
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

static c41_io_t * C41_CALL iof_create (HANDLE h);

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
  return "hbs1-mswin";
}

/* alloc ********************************************************************/
static c41_uint_t C41_CALL ma_alloc
(
  void * * data_pp,
  size_t len,
  void * ctx
)
{
  HANDLE heap_h = ctx;

  if ((*data_pp = HeapAlloc(heap_h, 0, len))) return 0;
  return C41_MA_NO_MEM;
}

/* realloc ******************************************************************/
static c41_uint_t C41_CALL ma_realloc
(
  void * * data_pp,
  size_t new_len,
  size_t old_len,
  void * ctx
)
{
  HANDLE heap_h = ctx;
  void * data_p;
  (void) old_len;

  data_p = HeapReAlloc(heap_h, 0, *data_pp, new_len);
  if (!data_p) return C41_MA_NO_MEM;
  *data_pp = data_p;
  return 0;
}

/* free *********************************************************************/
static c41_uint_t C41_CALL ma_free
(
  void * data_p,
  size_t len,
  void * ctx
)
{
  HANDLE heap_h = ctx;
  (void) len;
  HeapFree(heap_h, 0, data_p);
  return 0;
}

/* hbs1_ma_init *************************************************************/
HBS1_API c41_uint_t C41_CALL hbs1_ma_init (c41_ma_t * ma_p)
{
  ma_p->ctx = GetProcessHeap();
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
  DWORD s, e;

  if (!ReadFile(f->h, data, size, &s, NULL))
  {
    e = GetLastError();
    *used_size_p = 0;
    if (e == ERROR_BROKEN_PIPE) return 0; // treat as EOF
    return C41_IO_FAILED;
  }

  *used_size_p = s;
  return 0;
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
  DWORD s;

  if (!WriteFile(f->h, data, size, &s, NULL))
  {
    *used_size_p = 0;
    return C41_IO_FAILED;
  }
  *used_size_p = s;
  return 0;
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
  LONG lo, hi;

  lo = (LONG) disp;
  hi = (LONG) (disp >> 0x20);

  if (!SetFilePointer(f->h, lo, &hi, anchor)) return C41_IO_FAILED;
  f->io.pos = (((int64_t) hi) << 0x20) | lo;
  if (f->io.pos < 0) return C41_IO_FAILED;
  return 0;
}

/* io_truncate **************************************************************/
static uint_t C41_CALL io_truncate (c41_io_t * io_p)
{
  io_file_t * f = (io_file_t *) io_p;
  if (!SetEndOfFile(f->h)) return C41_IO_FAILED;
  return 0;
}

/* io_close *****************************************************************/
static uint_t C41_CALL io_close (c41_io_t * io_p)
{
  io_file_t * f = (io_file_t *) io_p;
  if (!CloseHandle(f->h)) return C41_IO_FAILED;
  return 0;
}

/* iof_create ***************************************************************/
static c41_io_t * C41_CALL iof_create (HANDLE h)
{
  io_file_t * f = HeapAlloc(GetProcessHeap(), 0, sizeof(io_file_t));
  if (!f) return NULL;
  memset(f, 0, sizeof(io_file_t));
  f->io.io_type_p = &iof_type;
  f->h = h;
  return &f->io;
}

/* hbs1_io_destroy **********************************************************/
HBS1_API uint_t C41_CALL hbs1_destroy_std_io (c41_io_t * io_p)
{
  HeapFree(GetProcessHeap(), 0, io_p);
  return 0;
}

/* hbs1_stdin ***************************************************************/
HBS1_API uint_t C41_CALL hbs1_stdin (c41_io_t * * stdin_pp)
{
  *stdin_pp = iof_create(GetStdHandle(STD_INPUT_HANDLE));
  if (!*stdin_pp) return HBS1_NO_RES;
  return 0;
}

/* hbs1_stdout **************************************************************/
HBS1_API uint_t C41_CALL hbs1_stdout (c41_io_t * * stdout_pp)
{
  *stdout_pp = iof_create(GetStdHandle(STD_OUTPUT_HANDLE));
  if (!*stdout_pp) return HBS1_NO_RES;
  return 0;
}

/* hbs1_stderr **************************************************************/
HBS1_API uint_t C41_CALL hbs1_stderr (c41_io_t * * stderr_pp)
{
  *stderr_pp = iof_create(GetStdHandle(STD_ERROR_HANDLE));
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
  return C41_FSI_OPEN_FAILED;
}

/* file_destroy *************************************************************/
static uint_t C41_CALL file_destroy
(
  c41_io_t *                io_p,
  void *                    context
)
{
  (void) context;
  HeapFree(GetProcessHeap(), 0, io_p);
  return 0;
}

/* hbs1_fsi_init ************************************************************/
HBS1_API uint_t C41_CALL hbs1_fsi_init (c41_fsi_t * fsi_p)
{
  C41_VAR_ZERO(*fsi_p);
  fsi_p->file_open = file_open;
  fsi_p->file_destroy = file_destroy;
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
  c41_smt_t * smt_p;
  c41_smt_thread_f func;
  void * arg;
};

/* thread_proc **************************************************************/
static DWORD WINAPI thread_proc (void * arg)
{
  thread_proc_ctx_t * c = arg;
  c41_smt_thread_f func;
  void * fa;

  func = c->func;
  fa = c->arg;
  HeapFree((HANDLE) c->smt_p->thread_context, 0, c);
  return func(fa);
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
  HANDLE h;
  int r;

  c = HeapAlloc((HANDLE) smt_p->thread_context, 0, sizeof(thread_proc_ctx_t));
  if (!c) return C41_SMT_NO_RES;
  c->smt_p = smt_p;
  c->func = func;
  c->arg = arg;

  h = CreateThread(NULL, 0, thread_proc, c, 0, NULL);
  if (!h)
  {
    HeapFree((HANDLE) smt_p->thread_context, 0, c);
    return C41_SMT_FAIL;
  }

  *tid_p = (c41_smt_tid_t) (intptr_t) h;

  return 0;
}

/* thread_join **************************************************************/
static int C41_CALL thread_join (c41_smt_t * smt_p, c41_smt_tid_t tid)
{
  void * v;
  DWORD r;

  (void) smt_p;

  r = WaitForSingleObject((HANDLE) tid, INFINITE);
  if (!GetExitCodeThread((HANDLE) tid, &r)) return -C41_SMT_FAIL;

  return (int) r;
}

/* mutex_init ***************************************************************/
static uint_t C41_CALL mutex_init (c41_smt_t * smt_p, c41_smt_mutex_t * mutex_p)
{
  (void) smt_p;
  InitializeCriticalSection((CRITICAL_SECTION *) mutex_p);
  return 0;
}

/* mutex_finish *************************************************************/
static uint_t C41_CALL mutex_finish (c41_smt_t * smt_p, c41_smt_mutex_t * mutex_p)
{
  (void) smt_p;
  DeleteCriticalSection((CRITICAL_SECTION *) mutex_p);
  return 0;
}

/* mutex_lock ***************************************************************/
static uint_t C41_CALL mutex_lock (c41_smt_t * smt_p, c41_smt_mutex_t * mutex_p)
{
  (void) smt_p;
  EnterCriticalSection((CRITICAL_SECTION *) mutex_p);
  return 0;
}

/* mutex_trylock ************************************************************/
static uint_t C41_CALL mutex_trylock (c41_smt_t * smt_p, c41_smt_mutex_t * mutex_p)
{
  (void) smt_p;
  (void) mutex_p;
  if (TryEnterCriticalSection((CRITICAL_SECTION *) mutex_p)) return 0;
  else return C41_SMT_OTHER;
  return 0;
}

/* mutex_unlock *************************************************************/
static uint_t C41_CALL mutex_unlock (c41_smt_t * smt_p, c41_smt_mutex_t * mutex_p)
{
  (void) smt_p;
  LeaveCriticalSection((CRITICAL_SECTION *) mutex_p);
  return 0;
}

/* cond_init ****************************************************************/
static uint_t C41_CALL cond_init (c41_smt_t * smt_p, c41_smt_cond_t * cond_p)
{
  (void) smt_p;
  (void) cond_p;
  return C41_SMT_NO_CODE;
}

/* cond_finish **************************************************************/
static uint_t C41_CALL cond_finish (c41_smt_t * smt_p, c41_smt_cond_t * cond_p)
{
  (void) smt_p;
  (void) cond_p;
  return C41_SMT_NO_CODE;
}

/* cond_signal **************************************************************/
static uint_t C41_CALL cond_signal (c41_smt_t * smt_p, c41_smt_cond_t * cond_p)
{
  (void) smt_p;
  (void) cond_p;
  return C41_SMT_NO_CODE;
}

/* cond_wait ****************************************************************/
static uint_t C41_CALL cond_wait (c41_smt_t * smt_p, c41_smt_cond_t * cond_p,
                                 c41_smt_mutex_t * mutex_p)
{
  (void) smt_p;
  (void) mutex_p; (void) cond_p;
  return C41_SMT_NO_CODE;
}

/* hbs1_smt_init ************************************************************/
HBS1_API uint_t C41_CALL hbs1_smt_init (c41_smt_t * smt_p, char const * impl)
{
  if (impl && !C41_STR_EQUAL(impl, "classic")) return HBS1_NO_SUP;
  smt_p->thread_create = thread_create;
  smt_p->thread_join = thread_join;
  smt_p->thread_context = GetProcessHeap();

  smt_p->mutex_size = sizeof(CRITICAL_SECTION);
  smt_p->mutex_init = mutex_init;
  smt_p->mutex_finish = mutex_finish;
  smt_p->mutex_lock = mutex_lock;
  smt_p->mutex_trylock = mutex_trylock;
  smt_p->mutex_unlock = mutex_unlock;

  smt_p->cond_size = sizeof(HANDLE);
  smt_p->cond_init = cond_init;
  smt_p->cond_finish = cond_finish;
  smt_p->cond_signal = cond_signal;
  smt_p->cond_wait = cond_wait;

  return 0;
}



