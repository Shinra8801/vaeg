/*
 * SGP.C: PC-88VA Super Graphic Processor
 */

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"
#include	"memoryva.h"

#if defined(SUPPORT_PC88VA)

		_SGP	sgp;

// ---- I/O


/*
ステータス読み出し
*/
static REG8 IOINPCALL sgp_i506(UINT port) {
	(void)port;
	return 0;
}

// ---- I/F

void sgp_reset(void) {
	ZeroMemory(&sgp, sizeof(sgp));
}

void sgp_bind(void) {
	iocoreva_attachinp(0x506, sgp_i506);
}

#endif
