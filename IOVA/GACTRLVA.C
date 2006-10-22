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

static REG8 IOINPCALL gactrlva_i_notimpl(UINT port) {
	REG8 dat;

	if (port & 1) {
		// high
		// 実機では必ずしも安定していない
		if (port < 0x580) {
			dat = (port & 0x02) ? 0xfd : 0xff;
		}
		else {
			dat = (port & 0x02) ? 0x7d : 0x7f;
		}
	}
	else {
		// low
		// 実機では必ずしも安定していない
		dat = ((port & 0x0f) == 0x0a) ? 0xfa : 0xfe;
	}

	TRACEOUT(("gactrlva(in, not implemented) - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));

	return dat;
}

static REG8 IOINPCALL gactrlva_i_notactive(UINT port) {
	REG8 dat;

	if (port & 1) {
		// high
		// 実機では必ずしも安定していない
		if (port < 0x580) {
			dat = (port & 0x02) ? 0xfd : 0xff;
		}
		else {
			dat = (port & 0x02) ? 0x7d : 0x7f;
		}
	}
	else {
		// low
		// 実機では必ずしも安定していない
		dat = ((port & 0x0f) == 0x0a) ? 0xfa : 0xfe;
	}

	TRACEOUT(("gactrlva(in, not active) - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));

	return dat;
}

static REG8 IOINPCALL gactrlva_i_high_m(UINT port) {

	if (gactrlva.gmsp) return gactrlva_i_notactive(port);

	TRACEOUT(("gactrlva(in, not implemented) - %x %x %.4x %.4x", port, 0xff, CPU_CS, CPU_IP));
	return 0xff;
}


static void IOOUTCALL gactrlva_o510(UINT port, REG8 dat) {
	gactrlva.m.accessmode = dat & 0x01;
	TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}

static REG8 IOINPCALL gactrlva_i510(UINT port) {
	REG8 dat;

	if (gactrlva.gmsp) return gactrlva_i_notactive(port);

	dat = gactrlva.m.accessmode;
	TRACEOUT(("gactrlva(in) - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));

	return dat;
}

static void IOOUTCALL gactrlva_o512(UINT port, REG8 dat) {
	gactrlva.m.accessblock = dat & 0x01;
	TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}

static REG8 IOINPCALL gactrlva_i512(UINT port) {
	REG8 dat;

	if (gactrlva.gmsp) return gactrlva_i_notactive(port);

	dat = gactrlva.m.accessblock;
	TRACEOUT(("gactrlva(in) - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));

	return dat;
}

static void IOOUTCALL gactrlva_o514(UINT port, REG8 dat) {
	gactrlva.m.readplane = dat | 0xf0;
	TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}

static REG8 IOINPCALL gactrlva_i514(UINT port) {
	REG8 dat;

	if (gactrlva.gmsp) return gactrlva_i_notactive(port);

	dat = gactrlva.m.readplane;
	TRACEOUT(("gactrlva(in) - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));

	return dat;
}

static void IOOUTCALL gactrlva_o516(UINT port, REG8 dat) {
	gactrlva.m.writeplane = dat | 0xf0;
	TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}

static REG8 IOINPCALL gactrlva_i516(UINT port) {
	REG8 dat;

	if (gactrlva.gmsp) return gactrlva_i_notactive(port);

	dat = gactrlva.m.writeplane;
	TRACEOUT(("gactrlva(in) - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));

	return dat;
}

static void IOOUTCALL gactrlva_o518(UINT port, REG8 dat) {
	if (gactrlva.m.advancedaccessmode ^ dat & 0x04) {
		if (!(dat & 0x04)) {
			//パターンレジスタを8bitに変更
			gactrlva.m.patternreadpointer  = 0xf0;
			gactrlva.m.patternwritepointer = 0xf0;
		}
	}
	gactrlva.m.advancedaccessmode = dat & 0x3f; //0xbf;
			// VAクラブ版テクマニはbit7:RBUSYだが、VA2だとbit7は0固定
	TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}

static REG8 IOINPCALL gactrlva_i518(UINT port) {
	REG8 dat;

	if (gactrlva.gmsp) return gactrlva_i_notactive(port);

	dat = gactrlva.m.advancedaccessmode;
	TRACEOUT(("gactrlva(in) - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));

	return dat;
}

static void IOOUTCALL gactrlva_o520(UINT port, REG8 dat) {
	int i;

	i = (port >> 1) & 3;
	gactrlva.m.cmpdata[i] = dat;
	TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}

static REG8 IOINPCALL gactrlva_i520(UINT port) {
	REG8 dat;
	int i;

	if (gactrlva.gmsp) return gactrlva_i_notactive(port);

	i = (port >> 1) & 3;
	dat = gactrlva.m.cmpdata[i];
	TRACEOUT(("gactrlva(in) - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));

	return dat;
}

static void IOOUTCALL gactrlva_o528(UINT port, REG8 dat) {
	int i;

	TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
	//gactrlva.m.cmpdatacontrol = dat & 0x0f | 0xf0;
			// VAクラブ版テクマニでは上位4bit=0だがVA2では1111b
	for (i = 0; i < 4; i++) {
		gactrlva.m.cmpdata[i] = (dat & 1) ? 0xff : 0;
		dat >>= 1;
	}
}

static REG8 IOINPCALL gactrlva_i528(UINT port) {
	REG8 dat;
	int i;

	if (gactrlva.gmsp) return gactrlva_i_notactive(port);

	dat = 0;
	for (i = 3; i >=0; i--) {
		dat <<= 1;
		if (gactrlva.m.cmpdata[i] == 0xff) dat |= 1;
	}
	dat |= 0xf0;
		// VAクラブ版テクマニでは上位4bit=0だがVA2では1111b
	//dat = gactrlva.m.cmpdatacontrol;
	TRACEOUT(("gactrlva(in) - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));

	return dat;
}

static void IOOUTCALL gactrlva_o530(UINT port, REG8 dat) {
	int i;

	i = (port >> 1) & 3;
	gactrlva.m.pattern[i][0] = dat;
	TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}

static REG8 IOINPCALL gactrlva_i530(UINT port) {
	int i;
	REG8 dat;

	if (gactrlva.gmsp) return gactrlva_i_notactive(port);

	i = (port >> 1) & 3;
	dat = gactrlva.m.pattern[i][0];
	TRACEOUT(("gactrlva(in) - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));

	return dat;
}

static void IOOUTCALL gactrlva_o540(UINT port, REG8 dat) {
	int i;

	i = (port >> 1) & 3;
	gactrlva.m.pattern[i][1] = dat;
	TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}

static REG8 IOINPCALL gactrlva_i540(UINT port) {
	int i;
	REG8 dat;

	if (gactrlva.gmsp) return gactrlva_i_notactive(port);

	i = (port >> 1) & 3;
	dat = gactrlva.m.pattern[i][1];
	TRACEOUT(("gactrlva(in) - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));

	return dat;
}

static void IOOUTCALL gactrlva_o550(UINT port, REG8 dat) {
	if (!(gactrlva.m.advancedaccessmode & 0x04)) {
		// パターンレジスタ 8bit
		dat = 0;
	}
	gactrlva.m.patternreadpointer = dat & 0x0f | 0xf0;
			// VAクラブ版テクマニでは上位4bit=0だがVA2では1111b
	TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}

static REG8 IOINPCALL gactrlva_i550(UINT port) {
	REG8 dat;

	if (gactrlva.gmsp) return gactrlva_i_notactive(port);

	dat = gactrlva.m.patternreadpointer;
	TRACEOUT(("gactrlva(in) - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));

	return dat;
}

static void IOOUTCALL gactrlva_o552(UINT port, REG8 dat) {
	if (!(gactrlva.m.advancedaccessmode & 0x04)) {
		// パターンレジスタ 8bit
		dat = 0;
	}
	gactrlva.m.patternwritepointer = dat & 0x0f | 0xf0;
			// VAクラブ版テクマニでは上位4bit=0だがVA2では1111b
	TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}

static REG8 IOINPCALL gactrlva_i552(UINT port) {
	REG8 dat;

	if (gactrlva.gmsp) return gactrlva_i_notactive(port);

	dat = gactrlva.m.patternwritepointer;
	TRACEOUT(("gactrlva(in) - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));

	return dat;
}

static void IOOUTCALL gactrlva_o560(UINT port, REG8 dat) {
	int i;

	i = (port >> 1) & 3;
	gactrlva.m.rop[i] = dat;
	TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}

static REG8 IOINPCALL gactrlva_i560(UINT port) {
	REG8 dat;
	int i;

	if (gactrlva.gmsp) return gactrlva_i_notactive(port);

	i = (port >> 1) & 3;
	dat = gactrlva.m.rop[i];
	TRACEOUT(("gactrlva(in) - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));

	return dat;
}

static void IOOUTCALL gactrlva_o580(UINT port, REG8 dat) {
	gactrlva.s.writemode = dat & 0x18;	//0x98;
			// VAクラブ版テクマニはbit7:RBUSYだが、VA2だとbit7は0固定
	//TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}

static REG8 IOINPCALL gactrlva_i580(UINT port) {
	REG8 dat;

	if (!gactrlva.gmsp) return gactrlva_i_notactive(port);

	dat = gactrlva.s.writemode;
	TRACEOUT(("gactrlva(in) - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));

	return dat;
}

static void IOOUTCALL gactrlva_o590(UINT port, REG8 dat) {
	int	i;

	i = (port >> 1) & 1;
	SETLOWBYTE(gactrlva.s.pattern[i], dat);
	//TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}

static REG8 IOINPCALL gactrlva_i590(UINT port) {
	REG8 dat;
	int	i;

	if (!gactrlva.gmsp) return gactrlva_i_notactive(port);

	i = (port >> 1) & 1;
	dat = LOWBYTE(gactrlva.s.pattern[i]);
	TRACEOUT(("gactrlva(in) - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));

	return dat;
}

static void IOOUTCALL gactrlva_o591(UINT port, REG8 dat) {
	int	i;

	i = (port >> 1) & 1;
	SETHIGHBYTE(gactrlva.s.pattern[i], dat);
	//TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}

static REG8 IOINPCALL gactrlva_i591(UINT port) {
	REG8 dat;
	int	i;

	if (!gactrlva.gmsp) return gactrlva_i_notactive(port);

	i = (port >> 1) & 1;
	dat = HIGHBYTE(gactrlva.s.pattern[i]);
	TRACEOUT(("gactrlva(in) - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));

	return dat;
}

static void IOOUTCALL gactrlva_o5a0(UINT port, REG8 dat) {
	int	i;

	i = (port >> 1) & 1;
	gactrlva.s.rop[i] = dat;
	//TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}

static REG8 IOINPCALL gactrlva_i5a0(UINT port) {
	REG8 dat;
	int	i;

	if (!gactrlva.gmsp) return gactrlva_i_notactive(port);

	i = (port >> 1) & 1;
	dat = gactrlva.s.rop[i];
	TRACEOUT(("gactrlva(in) - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));

	return dat;
}


// ---- I/F

void gactrlva_reset(void) {
	ZeroMemory(&gactrlva, sizeof(gactrlva));
	gactrlva.m.readplane = 0xff;
	gactrlva.m.writeplane = 0xff;
	gactrlva.m.patternreadpointer = 0xf0;
	gactrlva.m.patternwritepointer = 0xf0;
}

void gactrlva_bind(void) {
	int i;

	for (i = 0x510; i < 0x600; i++) {
		iocoreva_attachinp(i, gactrlva_i_notimpl);
	}

	iocoreva_attachout(0x510, gactrlva_o510);
	iocoreva_attachinp(0x510, gactrlva_i510);
	iocoreva_attachinp(0x511, gactrlva_i_high_m);
	iocoreva_attachout(0x512, gactrlva_o512);
	iocoreva_attachinp(0x512, gactrlva_i512);
	iocoreva_attachinp(0x513, gactrlva_i_high_m);
	iocoreva_attachout(0x514, gactrlva_o514);
	iocoreva_attachinp(0x514, gactrlva_i514);
	iocoreva_attachout(0x516, gactrlva_o516);
	iocoreva_attachinp(0x516, gactrlva_i516);
	iocoreva_attachout(0x518, gactrlva_o518);
	iocoreva_attachinp(0x518, gactrlva_i518);
	iocoreva_attachinp(0x519, gactrlva_i_high_m);
	iocoreva_attachout(0x528, gactrlva_o528);
	iocoreva_attachinp(0x528, gactrlva_i528);
	iocoreva_attachout(0x550, gactrlva_o550);
	iocoreva_attachinp(0x550, gactrlva_i550);
	iocoreva_attachout(0x552, gactrlva_o552);
	iocoreva_attachinp(0x552, gactrlva_i552);
	for (i = 0; i < 4; i++) {
		iocoreva_attachout(0x520 + i * 2, gactrlva_o520);
		iocoreva_attachout(0x530 + i * 2, gactrlva_o530);
		iocoreva_attachout(0x540 + i * 2, gactrlva_o540);
		iocoreva_attachout(0x560 + i * 2, gactrlva_o560);

		iocoreva_attachinp(0x520 + i * 2, gactrlva_i520);
		iocoreva_attachinp(0x521 + i * 2, gactrlva_i_high_m);
		iocoreva_attachinp(0x530 + i * 2, gactrlva_i530);
		iocoreva_attachinp(0x531 + i * 2, gactrlva_i_high_m);
		iocoreva_attachinp(0x540 + i * 2, gactrlva_i540);
		iocoreva_attachinp(0x541 + i * 2, gactrlva_i_high_m);
		iocoreva_attachinp(0x560 + i * 2, gactrlva_i560);
		iocoreva_attachinp(0x561 + i * 2, gactrlva_i_high_m);
	}
	iocoreva_attachout(0x580, gactrlva_o580);
	iocoreva_attachinp(0x580, gactrlva_i580);
	for (i = 0; i < 2; i++) {
		iocoreva_attachout(0x590 + i * 2, gactrlva_o590);
		iocoreva_attachout(0x591 + i * 2, gactrlva_o591);
		iocoreva_attachout(0x5a0 + i * 2, gactrlva_o5a0);

		iocoreva_attachinp(0x590 + i * 2, gactrlva_i590);
		iocoreva_attachinp(0x591 + i * 2, gactrlva_i591);
		iocoreva_attachinp(0x5a0 + i * 2, gactrlva_i5a0);
	}
}

#endif
