/* hbs1 - Host Basic Services - ver. 1
 *
 * Main header file
 *
 * Changelog:
 *  - 2013/01/06 Costin Ionescu: initial release
 *
 */
#ifndef _HBS1_H_
#define _HBS1_H_

#include <c41.h>

#define HBS1_LIB_MINOR 3
#define HBS1_LIB_IS_COMPATIBLE() (HBS1_LIB_MINOR <= hbs1_lib_minor())

#if HBS1_STATIC
# define HBS1_API
#elif HBS1_DL_BUILD
# define HBS1_API C41_DL_EXPORT
#else
# define HBS1_API C41_DL_IMPORT
#endif

#define HBS1_OK                 0x00
#define HBS1_NO_SUP             0x01
#define HBS1_NO_RES             0x02

HBS1_API char const * C41_CALL hbs1_lib_name ();
HBS1_API uint_t C41_CALL hbs1_lib_minor ();
HBS1_API char const * C41_CALL hbs1_status_name (uint_t status);

HBS1_API uint_t C41_CALL hbs1_ma_init (c41_ma_t * ma_p);
HBS1_API uint_t C41_CALL hbs1_ma_finish (c41_ma_t * ma_p);

HBS1_API uint_t C41_CALL hbs1_stdin (c41_io_t * * stdin_pp);
HBS1_API uint_t C41_CALL hbs1_stdout (c41_io_t * * stdout_pp);
HBS1_API uint_t C41_CALL hbs1_stderr (c41_io_t * * stderr_pp);
HBS1_API uint_t C41_CALL hbs1_destroy_std_io (c41_io_t * io_p);

HBS1_API uint_t C41_CALL hbs1_fsi_init (c41_fsi_t * fsi_p);
HBS1_API uint_t C41_CALL hbs1_fsi_finish (c41_fsi_t * fsi_p);

HBS1_API uint_t C41_CALL hbs1_smt_init (c41_smt_t * smt_p, char const * impl);
/*^ initialises all functions for simple multi-threading
 *  impl can be NULL for the default implementation
 *  impl values are specific to the supported platform
 **/


uint8_t C41_CALL hmain (c41_cli_t * cli_p);

#endif /* _HBS1_H_ */

