/*
 * UPD9002.C: PC-88VA CPU port
 */


#include	"compiler.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"
#include	"upd9002.h"

#if defined(SUPPORT_PC88VA)

	_UPD9002		upd9002 = {0};



static void IOOUTCALL upd9002_offf0(UINT port, REG8 dat) {
	upd9002.tcks = dat;
	pit_ontckschanged();
}



static REG8 IOINPCALL upd9002_ifff0(UINT port) {
	(void)port;
	return(upd9002.tcks);
}


// ---- I/F

void upd9002_reset(void) {
	ZeroMemory(&upd9002, sizeof(upd9002));
}

void upd9002_bind(void) {

	iocoreva_attachout(0xfff0, upd9002_offf0);
	iocoreva_attachinp(0xfff0, upd9002_ifff0);
}

#endif