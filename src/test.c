/* hbs1 - Host Basic Services - ver. 1
 *
 * Test program for hbs1 library.
 *
 * Changelog:
 *  - 2013/01/06 Costin Ionescu: initial release
 *
 */
#include <string.h>
#include <stdio.h>
#include <c41.h>
#include <hbs1.h>

#define Z(_expr) do { if ((_expr)) { printf("%s:%u:%s(): (%s) is non-zero\n", __FILE__, __LINE__, __FUNCTION__, #_expr); return 1; } } while (0)
#define T(_expr) Z(!(_expr))

int io_test ();
int smt_test ();
int u8v_test (c41_ma_t * ma_p);
int esm_test (c41_ma_t * ma_p);

static c41_ma_t ma;

/* main *********************************************************************/
int main (int argc, char const * const * argv)
{
  void * p;
  void * q;
  void * r;
  uint_t ui;
  int i;

  (void) argc;
  (void) argv;

  printf("hbs1 test\n");
  printf("- c41 lib:                            %s.%u\n",
         c41_lib_name(), c41_lib_minor());
  printf("- hbs1 lib:                           %s.%u\n",
         hbs1_lib_name(), hbs1_lib_minor());

  ui = hbs1_ma_init(&ma);
  printf("- ma.init:                            %s\n",
         hbs1_status_name(ui));
  if (ui) return 1;

  ui = c41_ma_alloc(&ma, &p, 0);
  printf("- ma.alloc(0):                        %s %p\n",
         c41_ma_status_name(ui), p);
  if (ui || p) goto l_ma_finish;

  ui = c41_ma_realloc(&ma, &p, 10, 0);
  printf("- ma.realloc(NULL, 10, 0):            %s %p\n",
         c41_ma_status_name(ui), p);
  if (ui || !p) goto l_ma_finish;

  ui = c41_ma_alloc(&ma, &r, 30);
  printf("- ma.alloc(30):                       %s %p\n",
         c41_ma_status_name(ui), r);

  q = p;
  ui = c41_ma_realloc(&ma, &p, 200, 10);
  printf("- ma.realloc(p, 200, 10):             %s %p->%p\n",
         c41_ma_status_name(ui), q, p);

  q = r;
  ui = c41_ma_realloc(&ma, &r, 0, 30);
  printf("- ma.realloc(p, 0, 30):               %s %p->%p\n",
         c41_ma_status_name(ui), q, r);

  ui = c41_ma_free(&ma, p, 200);
  printf("- ma.free(p, 200):                    %s (p=%p)\n",
         c41_ma_status_name(ui), p);

  ui = c41_ma_free(&ma, p, 0);
  printf("- ma.free(p, 0):                      %s (p=%p)\n",
         c41_ma_status_name(ui), p);

  if ((i = smt_test())) return i;
  if ((i = io_test())) return i;
  if ((i = u8v_test(&ma))) return i;
  if ((i = esm_test(&ma))) return i;
l_ma_finish:
  ui = hbs1_ma_finish(&ma);
  printf("- ma.finish:                          %s\n",
         hbs1_status_name(ui));
  if (ui) return 1;

  return 0;
}

/* io_test ******************************************************************/
int io_test ()
{
  c41_fsi_t fsi;
  c41_fspi_t fspi;
  c41_io_t * io_p;
  uint_t sc;
  ssize_t z;
  uint8_t buf[0x20];
  uint8_t const fn[] = "hbs1.tmp";

  printf("io test:\n");
  sc = hbs1_fsi_init(&fsi, &fspi);
  printf("- fsi: %s\n", hbs1_status_name(sc));
  z = fspi.fsp_from_utf8(buf, sizeof(buf), fn, C41_STR_LEN(fn));
  sc = c41_file_open(&fsi, &io_p, buf, z,
                     C41_FSI_WRITE | C41_FSI_EXF_OPEN | C41_FSI_NEWF_CREATE |
                     C41_FSI_UR | C41_FSI_UW);
  printf("- file open: %s\n", c41_fsi_status_name(sc));
  if (sc) return 1;
  sc = c41_io_write(io_p, "test", 4, NULL);
  printf("- io write: %s\n", c41_io_status_name(sc));
  sc = c41_file_destroy(&fsi, io_p);
  printf("- file destroy: %s\n", c41_fsi_status_name(sc));
  return 0;
}

static c41_smt_t smt;

/* worker *******************************************************************/
uint8_t C41_CALL worker (void * arg)
{
  uint_t c;

  c41_smt_mutex_t * mutex_p = arg;
  printf("- worker thread started! locking mutex...\n");

  c = c41_smt_mutex_lock(&smt, mutex_p);
  if (c)
  {
    printf("- mutex_lock failed: %u\n", c);
    return 1;
  }

  printf("- worker thread: mutex locked! unlocking...\n");
  c = c41_smt_mutex_unlock(&smt, mutex_p);
  if (c)
  {
    printf("- mutex_unlock failed: %u\n", c);
    return 1;
  }

  printf("- worker thread: exiting...\n");
  return 0;
}

/* smt_test *****************************************************************/
int smt_test ()
{
  uint_t c;
  int i;
  c41_smt_tid_t t;
  //uint8_t buf[0x100];
  c41_smt_mutex_t * mutex_p; // = (c41_smt_mutex_t *) &buf[0];

  printf("smt test:\n");
  c = hbs1_smt_init(&smt, NULL);
  if (c)
  {
    printf("- init: %u\n", c);
    return 1;
  }

  //c = c41_smt_mutex_init(&smt, mutex_p);
  c = c41_smt_mutex_create(&mutex_p, &smt, &ma);
  if (c)
  {
    printf("- mutex_init failed: %u\n", c);
    return 1;
  }

  c = c41_smt_mutex_trylock(&smt, mutex_p);
  if (c)
  {
    printf("- mutex_trylock failed: %u\n", c);
    return 1;
  }

  printf("- creating thread...\n");
  c = c41_smt_thread_create(&smt, &t, worker, mutex_p);
  if (c)
  {
    printf("- thread_create failed: %u\n", c);
    return 1;
  }

  printf("- unlocking mutex...\n");
  c = c41_smt_mutex_unlock(&smt, mutex_p);
  if (c)
  {
    printf("- mutex_unlock failed: %u\n", c);
    return 1;
  }

  printf("- waiting for worker...\n");
  i = c41_smt_thread_join(&smt, t);
  if (i)
  {
    printf("- thread_join: %d\n", i);
    return 1;
  }
  c = c41_smt_mutex_destroy(mutex_p, &smt, &ma);
  if (c)
  {
    printf("- failed to destroy mutex: %d\n", c);
    return 1;
  }

  printf("- another job well done!\n");

  return 0;
}

/* u8v_test *****************************************************************/
int u8v_test (c41_ma_t * ma_p)
{
  uint_t r;
  c41_u8v_t v;
  uint8_t * p;
  printf("u8v test\n");
  c41_u8v_init(&v, ma_p, 20);
  r = c41_u8v_extend(&v, 10);
  if (r)
  {
    printf("- failed extending\n");
    return 1;
  }
  printf("- extend(10) -> m=0x%lX\n", v.m);
  p = c41_u8v_append(&v, 17);
  printf("- append(17) -> p=%p\n", p);
  if (!p) return 1;
  C41_MEM_FILL(p, 17, 'A');
  r = c41_u8v_opt(&v);
  printf("- opt -> r=%d, m=0x%lX\n", r, v.m);
  if (r) return 1;
  c41_u8v_afmt(&v, "123 is $Xi - ", 123);
  c41_u8v_afmt(&v, "123 is $Xi", 123);
  c41_u8v_afmt(&v, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  printf("- test string: '%.*s'\n", (int) v.n, v.a);
  r = c41_u8v_free(&v);
  printf("- free -> r=%d\n", r);
  printf("- passed!\n");
  return 0;
}

/* esm_test *****************************************************************/
int esm_test (c41_ma_t * ma_p)
{
    c41_esm_t esm;
    int c;

    printf("esm test\n");
    c41_esm_init(&esm, ma_p, 3, 14, 2, 6);
    Z(c41_esm_alloc(&esm)); T(esm.lbx == 0); T(esm.lex == 0);
    //printf("ebc=%d pbc=%d fbc=%d\n", esm.ebc, esm.pbc, esm.fbc);
    //printf("b0n=%u\n", esm.bv.a[0].n);
    Z(c41_esm_alloc(&esm)); T(esm.lbx == 0); T(esm.lex == 1);
    //printf("ebc=%d pbc=%d fbc=%d\n", esm.ebc, esm.pbc, esm.fbc);
    Z(c41_esm_alloc(&esm)); T(esm.lbx == 1); T(esm.lex == 0);
    //printf("bx=%u, ex=%u\n", esm.lbx, esm.lex);
    //printf("ebc=%d pbc=%d fbc=%d\n", esm.ebc, esm.pbc, esm.fbc);
    Z(c41_esm_alloc(&esm)); T(esm.lbx == 1); T(esm.lex == 1);
    Z(c41_esm_alloc(&esm)); T(esm.lbx == 1); T(esm.lex == 2);
    Z(c41_esm_alloc(&esm)); T(esm.lbx == 1); T(esm.lex == 3);
    Z(c41_esm_alloc(&esm)); T(esm.lbx == 2); T(esm.lex == 0);
    Z(c41_esm_alloc(&esm)); T(esm.lbx == 2); T(esm.lex == 1);
    Z(c41_esm_alloc(&esm)); T(esm.lbx == 2); T(esm.lex == 2);
    Z(c41_esm_alloc(&esm)); T(esm.lbx == 2); T(esm.lex == 3);
    Z(c41_esm_alloc(&esm)); T(esm.lbx == 2); T(esm.lex == 4);
    Z(c41_esm_alloc(&esm)); T(esm.lbx == 2); T(esm.lex == 5);
    Z(c41_esm_alloc(&esm)); T(esm.lbx == 3); T(esm.lex == 0);
    Z(c41_esm_alloc(&esm)); T(esm.lbx == 3); T(esm.lex == 1);
    T(c41_esm_alloc(&esm) == C41_ESM_LIMIT);
    Z(c41_esm_deref(&esm, 0, 0));
    Z(c41_esm_deref(&esm, 0, 1));
    Z(c41_esm_deref(&esm, 3, 0));
    Z(c41_esm_deref(&esm, 3, 1));
    Z(c41_esm_alloc(&esm)); T(esm.lbx == 3); T(esm.lex == 1);
    Z(c41_esm_alloc(&esm)); T(esm.lbx == 3); T(esm.lex == 0);
    Z(c41_esm_alloc(&esm)); T(esm.lbx == 0); T(esm.lex == 1);

    c = c41_esm_finish(&esm);
    if (c) { printf("- esm_finish: %d\n", c); return c; }
    printf("- passed!\n");
    return 0;
}

