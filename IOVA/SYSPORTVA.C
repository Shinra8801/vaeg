/*
 * SYSPORTVA.C: PC-88VA System port
 */


#include	"compiler.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"
#include	"sound.h"
#include	"beep.h"
#include	"sysmng.h"

	_SYSPORTVA	sysportva;


static void modeled_oneventset() {
	BYTE led;
	int i;

	led = (sysportva.c >> 4) & 0x07;
	if (led == 0x07) {
		// ‚·‚×‚ÄOFF
		led = 0x00;		// ‚·‚×‚ÄON
	}
	for (i = 0; i < 3; i++) {
		sysmng_modeled((BYTE)i, (BYTE)(~led & 1));
		led >>= 1;
	}
}

// ---- I/O

static void IOOUTCALL sysp_o1cd(UINT port, REG8 dat) {

	if ((sysportva.c ^ dat) & 0x04) {					// ver0.29
		rs232c.send = 1;
	}
	sysportva.c = dat;
	sysport.c = (sysport.c & 0xf0) | (dat & 0x0f);
	beep_oneventset();
	modeled_oneventset();
	(void)port;
}

static void IOOUTCALL sysp_o1cf(UINT port, REG8 dat) {

	REG8	bit;

	if (!(dat & 0xf0)) {
		bit = 1 << (dat >> 1);
		if (dat & 1) {
			sysportva.c |= bit;
		}
		else {
			sysportva.c &= ~bit;
		}
		if (bit == 0x04) {									// ver0.29
			rs232c.send = 1;
		}
		else if (bit == 0x08) {
			beep_oneventset();
		}
	}
	sysport.c = (sysport.c & 0xf0) | (sysportva.c & 0x0f);
	modeled_oneventset();
	(void)port;
}

/*
static REG8 IOINPCALL sysp_i1c9(UINT port) {
	(void)port;
	return(np2cfg.dipsw[1]);
}

static REG8 IOINPCALL sysp_i1cb(UINT port) {

	REG8	ret;

	ret = ((~np2cfg.dipsw[0]) & 1) << 3;
	ret |= rs232c_stat();
	ret |= uPD4990.cdat;
	(void)port;
	return(ret);
}
*/

static REG8 IOINPCALL sysp_i1cd(UINT port) {

	(void)port;
	return(sysportva.c);
}


// ---- I/F

void systemportva_reset(void) {

	sysportva.c = 0xf9;
	//beep_oneventset();
	modeled_oneventset();
}

void systemportva_bind(void) {


	iocoreva_attachout(0x1cd, sysp_o1cd);
	iocoreva_attachout(0x1cf, sysp_o1cf);
//	iocoreva_attachinp(0x1c9, sysp_i1c9);
//	iocoreva_attachinp(0x1cb, sysp_i1cb);
	iocoreva_attachinp(0x1cd, sysp_i1cd);
}

