/* hbs1 - Host Basic Services - ver. 1
 *
 * hbs1cli static library code
 *
 * This initialises interfaces for basic services and passes them to user's
 * hmain() function.
 *
 * Changelog:
 *  - 2013/01/06 Costin Ionescu: initial release
 *
 */
#include <stdio.h>
#include <c41.h>
#include <hbs1.h>

int main (int argc, char const * const * argv)
{
  c41_ma_t ma;
  c41_smt_t smt;
  c41_fsi_t fsi;
  c41_fspi_t fspi;
  c41_cli_t cli;
  uint8_t h;
  uint_t hs;

  if (!C41_LIB_IS_COMPATIBLE())
  {
    fprintf(stderr, "Error: incompatible c41 lib\n");
    return 0x40;
  }

  if (!HBS1_LIB_IS_COMPATIBLE())
  {
    fprintf(stderr, "Error: incompatible hbs1 lib\n");
    return 0x41;
  }

  C41_VAR_ZERO(cli);

  hs = hbs1_ma_init(&ma);
  if (hs)
  {
    fprintf(stderr, "Error: failed initialising memory allocator "
            "(%s, code %u)", hbs1_status_name(hs), hs);
    return 0x42;
  }

  hs = hbs1_smt_init(&smt, NULL);
  if (hs)
  {
    fprintf(stderr, "Error: failed initialising multithreading support "
            "(%s, code %u)", hbs1_status_name(hs), hs);
    return 0x42;
  }

  hs = hbs1_stdin(&cli.stdin_p);
  if (hs)
  {
    fprintf(stderr, "Error: failed obtaining std input object "
            "(%s, code %u)", hbs1_status_name(hs), hs);
    return 0x42;
  }

  hs = hbs1_stdout(&cli.stdout_p);
  if (hs)
  {
    fprintf(stderr, "Error: failed obtaining std output object "
            "(%s, code %u)", hbs1_status_name(hs), hs);
    return 0x43;
  }

  hs = hbs1_stderr(&cli.stderr_p);
  if (hs)
  {
    fprintf(stderr, "Error: failed obtaining std error object "
            "(%s, code %u)", hbs1_status_name(hs), hs);
    return 0x43;
  }

  hs = hbs1_fsi_init(&fsi, &fspi);
  if (hs)
  {
    fprintf(stderr, "Error: failed initialising file system interface "
            "(%s, code %u)", hbs1_status_name(hs), hs);
    return 0x42;
  }

  cli.ma_p = &ma;
  cli.smt_p = &smt;
  cli.fsi_p = &fsi;
  cli.fspi_p = &fspi;
  cli.program = argv[0];
  cli.arg_a = argv + 1;
  cli.arg_n = argc - 1;
  h = hmain(&cli);
  if (h >= 0x40)
  {
    fprintf(stderr, "Error: bad return code 0x%02X\n", h);
    h = 0x7F;
  }

  hs = hbs1_fsi_finish(&fsi);
  if (hs)
  {
    fprintf(stderr, "Warning: failed finalising file system interface "
            "(%s, code %u)", hbs1_status_name(hs), hs);
  }

  hs = hbs1_ma_finish(&ma);
  if (hs)
  {
    fprintf(stderr, "Warning: failed finalising memory allocator "
            "(%s, code %u)", hbs1_status_name(hs), hs);
    return 0x42;
  }


  return h;
}

