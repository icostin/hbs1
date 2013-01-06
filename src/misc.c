/* hbs1 - Host Basic Services - ver. 1
 *
 * Misc functions
 *
 * Changelog:
 *  - 2013/01/06 Costin Ionescu: initial release
 *
 */
#include <hbs1.h>

HBS1_API c41_uint_t C41_CALL hbs1_lib_minor ()
{
  return HBS1_LIB_MINOR;
}

HBS1_API char const * C41_CALL hbs1_status_name (c41_uint_t status)
{
#define N(_x) case _x: return #_x
  switch (status)
  {
    N(HBS1_OK     );
    N(HBS1_NO_SUP );
    N(HBS1_NO_RES );
  default:
    return "HBS1_UNSPECIFIED";
  }
#undef N
}

