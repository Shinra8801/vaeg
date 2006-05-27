/*
 * GACTRLVA.C: PC-88VA GVRAM access control
 */

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"

#if defined(SUPPORT_PC88VA)

#define SETLOWBYTE(x, y) (x) = ( (x) & 0xff00 | (y) )
#define SETHIGHBYTE(x, y) (x) = ( (x) & 0x00ff | ((WORD)(y) << 8) )
#define LOWBYTE(x)  ( (x) & 0xff )
#define HIGHBYTE(x) ( (x) >> 8 )

// ---- I/O

static void IOOUTCALL gactrlva_o510(UINT port, REG8 dat) {
	gactrlva.m.accessmode = dat & 0x01;
}

static void IOOUTCALL gactrlva_o512(UINT port, REG8 dat) {
	gactrlva.m.accessblock = dat & 0x01;
}

static void IOOUTCALL gactrlva_o514(UINT port, REG8 dat) {
	gactrlva.m.readplane = dat | 0xf0;
}

static void IOOUTCALL gactrlva_o516(UINT port, REG8 dat) {
	gactrlva.m.writeplane = dat | 0xf0;
}

static void IOOUTCALL gactrlva_o518(UINT port, REG8 dat) {
	if (gactrlva.m.advancedaccessmode ^ dat & 0x04) {
		if (!dat & 0x04) {
			//パターンレジスタを8bitに変更
			gactrlva.m.patternreadpointer  = 0xf0;
			gactrlva.m.patternwritepointer = 0xf0;
		}
	}
	gactrlva.m.advancedaccessmode = dat & 0xbf;
}

static void IOOUTCALL gactrlva_o520(UINT port, REG8 dat) {
	int i;

	i = (port >> 1) & 3;
	gactrlva.m.cmpdata[i] = dat;
}

static void IOOUTCALL gactrlva_o528(UINT port, REG8 dat) {
	int i;

	gactrlva.m.cmpdatacontrol = dat & 0x0f;
	for (i = 0; i < 4; i++) {
		gactrlva.m.cmpdata[i] = (dat & 1) ? 0xff : 0;
		dat >>= 1;
	}
}

static void IOOUTCALL gactrlva_o530(UINT port, REG8 dat) {
	int i;

	i = (port >> 1) & 3;
	//SETLOWBYTE(gactrlva.m.pattern[i], dat);
	gactrlva.m.pattern[i][0] = dat;
}

static void IOOUTCALL gactrlva_o540(UINT port, REG8 dat) {
	int i;

	i = (port >> 1) & 3;
	//SETHIGHBYTE(gactrlva.m.pattern[i], dat);
	gactrlva.m.pattern[i][1] = dat;
}

static void IOOUTCALL gactrlva_o550(UINT port, REG8 dat) {
	if (!gactrlva.m.advancedaccessmode & 0x04) {
		// パターンレジスタ 8bit
		dat = 0;
	}
	gactrlva.m.patternreadpointer = dat & 0x0f | 0xf0;
}

static void IOOUTCALL gactrlva_o552(UINT port, REG8 dat) {
	if (!gactrlva.m.advancedaccessmode & 0x04) {
		// パターンレジスタ 8bit
		dat = 0;
	}
	gactrlva.m.patternwritepointer = dat & 0x0f | 0xf0;
}

static void IOOUTCALL gactrlva_o560(UINT port, REG8 dat) {
	int i;

	i = (port >> 1) & 3;
	gactrlva.m.rop[i] = dat;
}


static void IOOUTCALL gactrlva_o580(UINT port, REG8 dat) {
	gactrlva.s.writemode = dat & 0x98;
	//TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}

static void IOOUTCALL gactrlva_o590(UINT port, REG8 dat) {
	int	i;

	i = (port >> 1) & 1;
	SETLOWBYTE(gactrlva.s.pattern[i], dat);
	//TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}

static void IOOUTCALL gactrlva_o591(UINT port, REG8 dat) {
	int	i;

	i = (port >> 1) & 1;
	SETHIGHBYTE(gactrlva.s.pattern[i], dat);
	//TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}

static void IOOUTCALL gactrlva_o5a0(UINT port, REG8 dat) {
	int	i;

	i = (port >> 1) & 1;
	SETLOWBYTE(gactrlva.s.rop[i], dat);
	//TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}


// ---- I/F

void gactrlva_reset(void) {
	ZeroMemory(&gactrlva, sizeof(gactrlva));
}

void gactrlva_bind(void) {
	int i;

	iocoreva_attachout(0x510, gactrlva_o510);
	iocoreva_attachout(0x512, gactrlva_o512);
	iocoreva_attachout(0x514, gactrlva_o514);
	iocoreva_attachout(0x516, gactrlva_o516);
	iocoreva_attachout(0x518, gactrlva_o518);
	iocoreva_attachout(0x528, gactrlva_o528);
	iocoreva_attachout(0x550, gactrlva_o550);
	iocoreva_attachout(0x552, gactrlva_o552);
	for (i = 0; i < 4; i++) {
		iocoreva_attachout(0x520 + i * 2, gactrlva_o520);
		iocoreva_attachout(0x530 + i * 2, gactrlva_o530);
		iocoreva_attachout(0x540 + i * 2, gactrlva_o540);
		iocoreva_attachout(0x560 + i * 2, gactrlva_o560);
	}
	iocoreva_attachout(0x580, gactrlva_o580);
	for (i = 0; i < 2; i++) {
		iocoreva_attachout(0x590 + i * 2, gactrlva_o590);
		iocoreva_attachout(0x591 + i * 2, gactrlva_o591);
		iocoreva_attachout(0x5a0 + i * 2, gactrlva_o5a0);
		//iocoreva_attachout(0x5a1 + i * 2, gactrlva_o5a1);
	}
}

#endif
