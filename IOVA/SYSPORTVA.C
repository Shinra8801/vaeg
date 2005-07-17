/*
 * SYSPORTVA.C: PC-88VA System port/Calendar/Printer/Dip switch
 */


#include	"compiler.h"
#include	"cpucore.h"
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
		// すべてOFF
		led = 0x00;		// すべてON
	}
	for (i = 0; i < 3; i++) {
		sysmng_modeled((BYTE)i, (BYTE)(~led & 1));
		led >>= 1;
	}
}

static void calendar_ondataset() {
	upd4990_o20(0,
		(REG8) ((sysportva.port010 & 0x07) |		// C0-C2
		        ((sysportva.port010 & 0x08) << 2) |	// データ出力
		        ((sysportva.port040 & 0x06) << 2))	// STB,CLK
	);
}

// ---- I/O

static void IOOUTCALL sysp_o010(UINT port, REG8 dat) {
	sysportva.port010 = dat;
	calendar_ondataset();
	// ToDo: プリンタ 未実装
}

static void IOOUTCALL sysp_o032(UINT port, REG8 dat) {
	// ToDo: サウンド割り込みマスク
	TRACEOUT(("sysp_o032 - %x %x %.4x:%.4x", port, dat, CPU_CS, CPU_IP));
	sysportva.port032 = dat & 0xbf;		// GVAMは0にする
}

static REG8 IOINPCALL sysp_i032(UINT port) {
	// ToDo: サウンド割り込みマスク
	TRACEOUT(("sysp_i032 - %x %.4x:%.4x", port, CPU_CS, CPU_IP));
	return sysportva.port032;
}

/*
	bit7	FBEEP
	bit6	JOP1
	bit5	BEEP
	bit4	1
	bit3	0
	bit2	CCLK
	bit1	CSTB
	bit0	XPSTB
*/
static void IOOUTCALL sysp_o040(UINT port, REG8 dat) {
	sysportva.port040 = dat;
	calendar_ondataset();
	// ToDo: カレンダ以外
}

/*
	bit7	1
	bit6	1
	bit5	VRTC
	bit4	CDI
	bit3	SW7
	bit2	DCD
	bit1	SW1
	bit0	PBSY
*/
static REG8 IOINPCALL sysp_i040(UINT port) {
	UINT8 ret;

	ret =
		0xc0 |							// 常に1
		(tsp.vsync & 0x20) |			// VSYNC
		((uPD4990.cdat & 0x01) << 4) |	// CDI(カレンダ時計)
		0x01;							// PBSY

	return ret;
}

static REG8 IOINPCALL sysp_i150(UINT port) {
	return sysportva.modesw & 0x00ff;
}

static REG8 IOINPCALL sysp_i151(UINT port) {
	return sysportva.modesw >> 8;
}

static void IOOUTCALL sysp_o1c6(UINT port, REG8 dat) {
	sysportva.modesw = (dat & 0x01) ? 0xfffe : 0xfffd;
	if (dat & 0x02) {
		sysportva.a |= 0x20;
	}
	else {
		sysportva.a &= ~0x20;
	}
	if ((dat & 0xfc) != 0x04) {
		TRACEOUT(("o1c6: unsupported bits are specified: 0x%.2x", dat));
	}
}

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
		sysport.c = (sysport.c & 0xf0) | (sysportva.c & 0x0f);
		if (bit == 0x04) {									// ver0.29
			rs232c.send = 1;
		}
		else if (bit == 0x08) {
			beep_oneventset();
		}
		modeled_oneventset();
	}
	(void)port;
}


static REG8 IOINPCALL sysp_i1c9(UINT port) {
	return sysportva.a;
}

/*
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
	sysportva.a |= 0xc1;
	sysportva.c = 0xf9;
	sysportva.port010 = 0;
	sysportva.port040 = 0;
	//beep_oneventset();
	modeled_oneventset();
}

void systemportva_bind(void) {

	iocoreva_attachout(0x010, sysp_o010);
	iocoreva_attachout(0x032, sysp_o032);
	iocoreva_attachinp(0x032, sysp_i032);
	iocoreva_attachout(0x040, sysp_o040);
	iocoreva_attachinp(0x040, sysp_i040);

	iocoreva_attachinp(0x150, sysp_i150);
	iocoreva_attachinp(0x151, sysp_i151);

	iocoreva_attachout(0x1c6, sysp_o1c6);

	iocoreva_attachout(0x1cd, sysp_o1cd);
	iocoreva_attachout(0x1cf, sysp_o1cf);
	iocoreva_attachinp(0x1c9, sysp_i1c9);
//	iocoreva_attachinp(0x1cb, sysp_i1cb);
	iocoreva_attachinp(0x1cd, sysp_i1cd);
}

