/*
 * NP2VASUP.C: PC-88VA support control
 */

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"

#if defined(SUPPORT_PC88VA)




// ---- I/O

static void IOOUTCALL np2vasup_offd0(UINT port, REG8 dat) {
	memmode_va = dat & 0x01;
	iomode_va =  dat & 0x02;
	(void)port;
}

static REG8 IOINPCALL np2vasup_iffd0(UINT port) {
	(void)port;
	return memmode_va & 0x01 | iomode_va & 0x02;
}

// ---- I/F

void np2vasup_reset(void) {
	if (pccore.model_va == PCMODEL_NOTVA) {
		np2vasup_offd0(0, 0);
	}
	else {
		np2vasup_offd0(0, 0xff);
	}
}

void np2vasup_bind(void) {
	iocore_attachout(0xffd0, np2vasup_offd0);
	iocore_attachinp(0xffd0, np2vasup_iffd0);

	iocoreva_attachout(0xffd0, np2vasup_offd0);
	iocoreva_attachinp(0xffd0, np2vasup_iffd0);
}

#endif
