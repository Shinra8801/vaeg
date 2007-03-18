/*
 * IOCOREVA.H: PC-88VA I/O
 */

#include	"memctrlva.h"
#include	"tsp.h"
#include	"videova.h"
#include	"sysportva.h"
#include	"mouseifva.h"
#include	"gactrlva.h"
#include	"cgromva.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL iocoreva_attachout(UINT port, IOOUT func);
BOOL iocoreva_attachinp(UINT port, IOINP func);

void iocoreva_create(void);
void iocoreva_destroy(void);
BOOL iocoreva_build(void);

//void iocoreva_cb(const IOCBFN *cbfn, UINT count);
//void iocoreva_reset(void);
void iocoreva_bind(void);

void IOOUTCALL iocoreva_out8(UINT port, REG8 dat);
REG8 IOINPCALL iocoreva_inp8(UINT port);

void IOOUTCALL iocoreva_out16(UINT port, REG16 dat);
REG16 IOINPCALL iocoreva_inp16(UINT port);



#ifdef __cplusplus
}
#endif
