/* hbs1 - Host Basic Services - ver. 1
 *
 * Test program for hbs1 and hbs1cli libraries.
 *
 * Changelog:
 *  - 2013/01/06 Costin Ionescu: initial release
 *
 */
#include <c41.h>
#include <hbs1.h>

/* hmain ********************************************************************/
uint8_t C41_CALL hmain (c41_cli_t * cli_p)
{
  c41_io_write(cli_p->stdout_p, "cucu\n", 5, NULL);
  return 0;
}

