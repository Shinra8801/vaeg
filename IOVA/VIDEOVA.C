/*
 * VIDEOVA.C: PC-88VA Video Control
 */

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"
#include	"memoryva.h"

#if defined(SUPPORT_PC88VA)

// ---- I/O

/*
static void IOOUTCALL memctrlva_o152(UINT port, REG8 dat) {
	rom0_bank = ((dat & 0x40) >> 2) | dat & 0x0f;
	rom1_bank = (dat & 0xb0) >> 4;
	(void)port;
}

static void IOOUTCALL memctrlva_o153(UINT port, REG8 dat) {
	sysm_bank = dat & 0x0f;
	(void)port;
}
*/
static REG8 IOINPCALL videova_i040(UINT port) {
	UINT8 ret;

	ret = gdc.vsync;

	(void)port;
	return ret;
}

// ---- I/F

void videova_reset(void) {
}

void videova_bind(void) {
	/*
	iocoreva_attachout(0x152, memctrlva_o152);
	iocoreva_attachout(0x153, memctrlva_o153);
	iocoreva_attachout(0x198, memctrlva_o198);
	iocoreva_attachout(0x19a, memctrlva_o19a);
	*/
	iocoreva_attachinp(0x040, videova_i040);
}

#endif
