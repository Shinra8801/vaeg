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

#define SETLOWBYTE(x, y) (x) = ( (x) & 0xff00 | (y) )
#define SETHIGHBYTE(x, y) (x) = ( (x) & 0x00ff | ((WORD)(y) << 8) )
#define LOWBYTE(x)  ( (BYTE)((x) & 0xff) )
#define HIGHBYTE(x) ( (BYTE)(((x) >> 8) & 0xff) )
#define HIGHWORD(x) ( (WORD)(((x) >> 16) & 0xffff) )

enum {
	PORT_FRAMEBUFFER = 0x200,
	PORT_PALETTE = 0x300,
};

	_VIDEOVA	videova;



static int port2pal(UINT port) {
	return (port - PORT_PALETTE) / 2;
}

static WORD adjustcolor12(WORD c) {
	if (c & 0xf000) c |= 0x0c00;
	if (c & 0x03c0) c |= 0x0020;
	if (c & 0x001e) c |= 0x0001;
	return c;
}

static void adjustpal(int palno) {
	videova.palette[palno] = adjustcolor12(videova.palette[palno]);
}

// ---- I/O

//    テキスト制御ポート0
static void IOOUTCALL videova_o030(UINT port, REG8 dat) {
	videova.txtmode8 = dat;
}

//    表示画面制御レジスタ

static REG8 IOINPCALL videova_i100(UINT port) {
//	return videova.grmode & 0xff;
	return LOWBYTE(videova.grmode);
}

static REG8 IOINPCALL videova_i101(UINT port) {
//	return videova.grmode >> 8;
	return HIGHBYTE(videova.grmode);
}

static void IOOUTCALL videova_o100(UINT port, REG8 dat) {
//	videova.grmode = videova.grmode & 0xff00 | dat;
	SETLOWBYTE(videova.grmode, dat);
}

static void IOOUTCALL videova_o101(UINT port, REG8 dat) {
//	videova.grmode = videova.grmode & 0x00ff | ((WORD)dat << 8);
	SETHIGHBYTE(videova.grmode, dat);
}

//    グラフィック画面制御レジスタ

static REG8 IOINPCALL videova_i102(UINT port) {
//	return videova.grres & 0xff;
	return LOWBYTE(videova.grres);
}

static REG8 IOINPCALL videova_i103(UINT port) {
//	return videova.grres >> 8;
	return HIGHBYTE(videova.grres);
}

static void IOOUTCALL videova_o102(UINT port, REG8 dat) {
//	videova.grres = videova.grres & 0xff00 | dat;
	SETLOWBYTE(videova.grres, dat);
}

static void IOOUTCALL videova_o103(UINT port, REG8 dat) {
//	videova.grres = videova.grres & 0x00ff | ((WORD)dat << 8);
	SETHIGHBYTE(videova.grres, dat);
}

//    パレット指定画面制御レジスタ

static void IOOUTCALL videova_o106(UINT port, REG8 dat) {
	SETLOWBYTE(videova.colcomp, dat);
}

static void IOOUTCALL videova_o107(UINT port, REG8 dat) {
	SETHIGHBYTE(videova.colcomp, dat);
}

//    直接色指定画面制御レジスタ

static void IOOUTCALL videova_o108(UINT port, REG8 dat) {
	SETLOWBYTE(videova.rgbcomp, dat);
}

static void IOOUTCALL videova_o109(UINT port, REG8 dat) {
	SETHIGHBYTE(videova.rgbcomp, dat);
}

//    画面マスクモードレジスタ

static void IOOUTCALL videova_o10a(UINT port, REG8 dat) {
	SETLOWBYTE(videova.mskmode, dat);
}

static void IOOUTCALL videova_o10b(UINT port, REG8 dat) {
	SETHIGHBYTE(videova.mskmode, dat);
}

//    カラーパレットモードレジスタ

static REG8 IOINPCALL videova_i10c(UINT port) {
//	return videova.palmode & 0xff;
	return LOWBYTE(videova.palmode);
}

static REG8 IOINPCALL videova_i10d(UINT port) {
//	return videova.palmode >> 8;
	return HIGHBYTE(videova.palmode);
}

static void IOOUTCALL videova_o10c(UINT port, REG8 dat) {
//	videova.palmode = videova.palmode & 0xff00 | dat;
	SETLOWBYTE(videova.palmode, dat);
}

static void IOOUTCALL videova_o10d(UINT port, REG8 dat) {
//	videova.palmode = videova.palmode & 0x00ff | ((WORD)dat << 8);
	SETHIGHBYTE(videova.palmode, dat);
}

//   バックドロップカラー
static void IOOUTCALL videova_o10e(UINT port, REG8 dat) {
	SETLOWBYTE(videova.dropcol, dat);
	videova.dropcol = adjustcolor12(videova.dropcol);
}

static void IOOUTCALL videova_o10f(UINT port, REG8 dat) {
	SETHIGHBYTE(videova.dropcol, dat);
	videova.dropcol = adjustcolor12(videova.dropcol);
}

//   カラーコード/プレーンマスクレジスタ
//     bit15-12 テキスト/スプライト判別境界カラー(1～設定値 がスプライト)
//     bit11- 8 シングルプレーン1bit/pixel時のフォアグラウンドカラー
//     bit 7    マルチプレーン 4bit/pixel プレーン3 スイッチ(1でON)
//     bit 6    グラフィック表示回路モード(1でV1/V2)
//     bit 5- 4 0
//     bit 3- 0 マルチプレーン 1bit/pixel プレーンn画面スイッチ
static void IOOUTCALL videova_o110(UINT port, REG8 dat) {
	SETLOWBYTE(videova.pagemsk, dat);
}

static void IOOUTCALL videova_o111(UINT port, REG8 dat) {
	SETHIGHBYTE(videova.pagemsk, dat);
}


//   グラフィック画面0透明色レジスタ

static void IOOUTCALL videova_o124(UINT port, REG8 dat) {
	SETLOWBYTE(videova.xpar_g0, dat);
}

static void IOOUTCALL videova_o125(UINT port, REG8 dat) {
	SETHIGHBYTE(videova.xpar_g0, dat);
}

//   グラフィック画面1透明色レジスタ

static void IOOUTCALL videova_o126(UINT port, REG8 dat) {
	SETLOWBYTE(videova.xpar_g1, dat);
}

static void IOOUTCALL videova_o127(UINT port, REG8 dat) {
	SETHIGHBYTE(videova.xpar_g1, dat);
}

//   テキスト/スプライト透明色レジスタ

static void IOOUTCALL videova_o12e(UINT port, REG8 dat) {
	SETLOWBYTE(videova.xpar_txtspr, dat | 0x0001);
}

static void IOOUTCALL videova_o12f(UINT port, REG8 dat) {
	SETHIGHBYTE(videova.xpar_txtspr, dat);
}

//    画面マスクパラメータ

static void IOOUTCALL videova_o130(UINT port, REG8 dat) {
	SETLOWBYTE(videova.mskleft, dat);
}

static void IOOUTCALL videova_o131(UINT port, REG8 dat) {
	SETHIGHBYTE(videova.mskleft, dat & 0x03ff);
}

static void IOOUTCALL videova_o132(UINT port, REG8 dat) {
	SETLOWBYTE(videova.mskrit, dat);
}

static void IOOUTCALL videova_o133(UINT port, REG8 dat) {
	SETHIGHBYTE(videova.mskrit, dat & 0x3ff);
}

static void IOOUTCALL videova_o134(UINT port, REG8 dat) {
	SETLOWBYTE(videova.msktop, dat);
}

static void IOOUTCALL videova_o135(UINT port, REG8 dat) {
	SETHIGHBYTE(videova.msktop, dat & 0xff);
}

static void IOOUTCALL videova_o136(UINT port, REG8 dat) {
	SETLOWBYTE(videova.mskbot, dat);
}

static void IOOUTCALL videova_o137(UINT port, REG8 dat) {
	SETHIGHBYTE(videova.mskbot, dat & 0xff);
}

//   テキスト画面制御ポート

static void IOOUTCALL videova_o148(UINT port, REG8 dat) {
	videova.txtmode = dat | 1;
	//TRACEOUT(("videova_o148 - %x %x %.4x:%.4x", port, dat, CPU_CS, CPU_IP));
	if ((dat & 1) == 0) {
		// TVRAM 256Kモード
		TRACEOUT(("videova_o148: WARNING: TVRAM 256K mode is not supported."));
	}
}

//   パレット

static void IOOUTCALL videova_o_palette_l(UINT port, REG8 dat) {
	int n;
	
	n = port2pal(port);
	videova.palette[n] = videova.palette[n] & 0xff00 | dat;
	adjustpal(n);
}

static void IOOUTCALL videova_o_palette_h(UINT port, REG8 dat) {
	int n;
	
	n = port2pal(port);
	videova.palette[n] = videova.palette[n] & 0x00ff | ((WORD)dat << 8);
	adjustpal(n);
}


//   フレームバッファ制御

#define fbno(x) ((x>>5) & 3)

static void IOOUTCALL videova_o_fb_00(UINT port, REG8 dat) {
	int n;

	n = fbno(port);
	if (n == 1) return;
	videova.framebuffer[n].fsa =
		videova.framebuffer[n].fsa & 0xffffff00L | (dat & 0xfc);
}

static void IOOUTCALL videova_o_fb_01(UINT port, REG8 dat) {
	int n;

	n = fbno(port);
	if (n == 1) return;
	videova.framebuffer[n].fsa =
		videova.framebuffer[n].fsa & 0xffff00ffL | ((DWORD)dat << 8);
}

static void IOOUTCALL videova_o_fb_02(UINT port, REG8 dat) {
	int n;

	n = fbno(port);
	if (n == 1) return;
	videova.framebuffer[n].fsa =
		videova.framebuffer[n].fsa & 0xff00ffffL | (((DWORD)dat & 0x03) << 16);
}

static void IOOUTCALL videova_o_fb_03(UINT port, REG8 dat) {
}

static void IOOUTCALL videova_o_fb_04(UINT port, REG8 dat) {
	int n;

	n = fbno(port);
	videova.framebuffer[n].fbw =
		videova.framebuffer[n].fbw & 0xff00 | (dat & 0xfc);
}

static void IOOUTCALL videova_o_fb_05(UINT port, REG8 dat) {
	int n;

	n = fbno(port);
	videova.framebuffer[n].fbw =
		videova.framebuffer[n].fbw & 0x00ff | (((WORD)dat & 0x07) << 8);
}


static void IOOUTCALL videova_o_fb_06(UINT port, REG8 dat) {
	int n;

	n = fbno(port);
	if (n == 1) return;
	videova.framebuffer[n].fbl =
		videova.framebuffer[n].fbl & 0xff00 | dat;
}

static void IOOUTCALL videova_o_fb_07(UINT port, REG8 dat) {
	int n;

	n = fbno(port);
	if (n == 1) return;
	videova.framebuffer[n].fbl =
		videova.framebuffer[n].fbl & 0x00ff | (((WORD)dat & 0x03) << 8);
}


static void IOOUTCALL videova_o_fb_08(UINT port, REG8 dat) {
	int n;

	n = fbno(port);
	videova.framebuffer[n].dot = dat & 0x1f;
}

static void IOOUTCALL videova_o_fb_09(UINT port, REG8 dat) {
}


static void IOOUTCALL videova_o_fb_0a(UINT port, REG8 dat) {
	int n;

	n = fbno(port);
	if (n == 1) return;
	videova.framebuffer[n].ofx =
		videova.framebuffer[n].ofx & 0xff00 | (dat & 0xfc);
}

static void IOOUTCALL videova_o_fb_0b(UINT port, REG8 dat) {
	int n;

	n = fbno(port);
	if (n == 1) return;
	videova.framebuffer[n].ofx =
		videova.framebuffer[n].ofx & 0x00ff | (((WORD)dat & 0x07) << 8);
}



static void IOOUTCALL videova_o_fb_0c(UINT port, REG8 dat) {
	int n;

	n = fbno(port);
	if (n == 1) return;
	videova.framebuffer[n].ofy =
		videova.framebuffer[n].ofy & 0xff00 | dat;
}

static void IOOUTCALL videova_o_fb_0d(UINT port, REG8 dat) {
	int n;

	n = fbno(port);
	if (n == 1) return;
	videova.framebuffer[n].ofy =
		videova.framebuffer[n].ofy & 0x00ff | (((WORD)dat & 0x03) << 8);
}



static void IOOUTCALL videova_o_fb_0e(UINT port, REG8 dat) {
	int n;

	n = fbno(port);
	videova.framebuffer[n].dsa =
		videova.framebuffer[n].dsa & 0xffffff00L | (dat & 0xfc);
}

static void IOOUTCALL videova_o_fb_0f(UINT port, REG8 dat) {
	int n;

	n = fbno(port);
	videova.framebuffer[n].dsa =
		videova.framebuffer[n].dsa & 0xffff00ffL | ((DWORD)dat << 8);
}

static void IOOUTCALL videova_o_fb_10(UINT port, REG8 dat) {
	int n;

	n = fbno(port);
	videova.framebuffer[n].dsa =
		videova.framebuffer[n].dsa & 0xff00ffffL | (((DWORD)dat & 0x03) << 16);
}

static void IOOUTCALL videova_o_fb_11(UINT port, REG8 dat) {
}



static void IOOUTCALL videova_o_fb_12(UINT port, REG8 dat) {
	int n;

	n = fbno(port);
	videova.framebuffer[n].dsh =
		videova.framebuffer[n].dsh & 0xff00 | dat;
}

static void IOOUTCALL videova_o_fb_13(UINT port, REG8 dat) {
	int n;

	n = fbno(port);
	videova.framebuffer[n].dsh =
		videova.framebuffer[n].dsh & 0x00ff | (((WORD)dat & 0x01) << 8);
}


static void IOOUTCALL videova_o_fb_16(UINT port, REG8 dat) {
	int n;

	n = fbno(port);
	videova.framebuffer[n].dsp =
		videova.framebuffer[n].dsp & 0xff00 | dat;
}

static void IOOUTCALL videova_o_fb_17(UINT port, REG8 dat) {
	int n;

	n = fbno(port);
	videova.framebuffer[n].dsp =
		videova.framebuffer[n].dsp & 0x00ff | (((WORD)dat & 0x01) << 8);
}


static REG8 IOINPCALL videova_i_fb_00(UINT port) {
	return LOWBYTE(videova.framebuffer[fbno(port)].fsa);
}

static REG8 IOINPCALL videova_i_fb_01(UINT port) {
	return HIGHBYTE(videova.framebuffer[fbno(port)].fsa);
}

static REG8 IOINPCALL videova_i_fb_02(UINT port) {
	return LOWBYTE(HIGHWORD(videova.framebuffer[fbno(port)].fsa));
}

static REG8 IOINPCALL videova_i_fb_03(UINT port) {
	return HIGHBYTE(HIGHWORD(videova.framebuffer[fbno(port)].fsa));
}

static REG8 IOINPCALL videova_i_fb_04(UINT port) {
	return LOWBYTE(videova.framebuffer[fbno(port)].fbw);
}

static REG8 IOINPCALL videova_i_fb_05(UINT port) {
	return HIGHBYTE(videova.framebuffer[fbno(port)].fbw);
}

static REG8 IOINPCALL videova_i_fb_06(UINT port) {
	return LOWBYTE(videova.framebuffer[fbno(port)].fbl);
}

static REG8 IOINPCALL videova_i_fb_07(UINT port) {
	return HIGHBYTE(videova.framebuffer[fbno(port)].fbl);
}

static REG8 IOINPCALL videova_i_fb_08(UINT port) {
	return LOWBYTE(videova.framebuffer[fbno(port)].dot);
}

static REG8 IOINPCALL videova_i_fb_09(UINT port) {
	return 0;
}

static REG8 IOINPCALL videova_i_fb_0a(UINT port) {
	return LOWBYTE(videova.framebuffer[fbno(port)].ofx);
}

static REG8 IOINPCALL videova_i_fb_0b(UINT port) {
	return HIGHBYTE(videova.framebuffer[fbno(port)].ofx);
}

static REG8 IOINPCALL videova_i_fb_0c(UINT port) {
	return LOWBYTE(videova.framebuffer[fbno(port)].ofy);
}

static REG8 IOINPCALL videova_i_fb_0d(UINT port) {
	return HIGHBYTE(videova.framebuffer[fbno(port)].ofy);
}


static REG8 IOINPCALL videova_i_fb_0e(UINT port) {
	return LOWBYTE(videova.framebuffer[fbno(port)].dsa);
}

static REG8 IOINPCALL videova_i_fb_0f(UINT port) {
	return HIGHBYTE(videova.framebuffer[fbno(port)].dsa);
}

static REG8 IOINPCALL videova_i_fb_10(UINT port) {
	return LOWBYTE(HIGHWORD(videova.framebuffer[fbno(port)].dsa));
}

static REG8 IOINPCALL videova_i_fb_11(UINT port) {
	return HIGHBYTE(HIGHWORD(videova.framebuffer[fbno(port)].dsa));
}

static REG8 IOINPCALL videova_i_fb_12(UINT port) {
	return LOWBYTE(videova.framebuffer[fbno(port)].dsh);
}

static REG8 IOINPCALL videova_i_fb_13(UINT port) {
	return HIGHBYTE(videova.framebuffer[fbno(port)].dsh);
}

static REG8 IOINPCALL videova_i_fb_16(UINT port) {
	return LOWBYTE(videova.framebuffer[fbno(port)].dsp);
}

static REG8 IOINPCALL videova_i_fb_17(UINT port) {
	return HIGHBYTE(videova.framebuffer[fbno(port)].dsp);
}



// ---- I/F

void videova_reset(void) {
	ZeroMemory(&videova, sizeof(videova));

	videova.framebuffer[1].fsa = 0xffffffffL;
	videova.framebuffer[1].fbl = 0xffff;
	videova.framebuffer[1].ofx = 0xffff;
	videova.framebuffer[1].ofy = 0xffff;

	videova.xpar_txtspr = 0x0001;

	videova.crtmode = sysportvacfg.dipsw & 1;

	// ToDo: パレットの初期化
}

void videova_bind(void) {
	int i;

	iocoreva_attachout(0x030, videova_o030);

	iocoreva_attachinp(0x100, videova_i100);
	iocoreva_attachinp(0x101, videova_i101);
	iocoreva_attachout(0x100, videova_o100);
	iocoreva_attachout(0x101, videova_o101);

	iocoreva_attachinp(0x102, videova_i102);
	iocoreva_attachinp(0x103, videova_i103);
	iocoreva_attachout(0x102, videova_o102);
	iocoreva_attachout(0x103, videova_o103);

	iocoreva_attachout(0x106, videova_o106);
	iocoreva_attachout(0x107, videova_o107);
	iocoreva_attachout(0x108, videova_o108);
	iocoreva_attachout(0x109, videova_o109);

	iocoreva_attachout(0x10a, videova_o10a);
	iocoreva_attachout(0x10b, videova_o10b);

	iocoreva_attachinp(0x10c, videova_i10c);
	iocoreva_attachinp(0x10d, videova_i10d);
	iocoreva_attachout(0x10c, videova_o10c);
	iocoreva_attachout(0x10d, videova_o10d);

	iocoreva_attachout(0x10e, videova_o10e);
	iocoreva_attachout(0x10f, videova_o10f);

	iocoreva_attachout(0x110, videova_o110);
	iocoreva_attachout(0x111, videova_o111);

	iocoreva_attachout(0x124, videova_o124);
	iocoreva_attachout(0x125, videova_o125);
	iocoreva_attachout(0x126, videova_o126);
	iocoreva_attachout(0x127, videova_o127);
	iocoreva_attachout(0x12e, videova_o12e);
	iocoreva_attachout(0x12f, videova_o12f);

	iocoreva_attachout(0x130, videova_o130);
	iocoreva_attachout(0x131, videova_o131);
	iocoreva_attachout(0x132, videova_o132);
	iocoreva_attachout(0x133, videova_o133);
	iocoreva_attachout(0x134, videova_o134);
	iocoreva_attachout(0x135, videova_o135);
	iocoreva_attachout(0x136, videova_o136);
	iocoreva_attachout(0x137, videova_o137);

	iocoreva_attachout(0x148, videova_o148);

	for (i = 0; i < VIDEOVA_FRAMEBUFFERS; i++) {
		int base;
		base = PORT_FRAMEBUFFER + 0x20 * i;

		iocoreva_attachout(base + 0x00, videova_o_fb_00);
		iocoreva_attachout(base + 0x01, videova_o_fb_01);
		iocoreva_attachout(base + 0x02, videova_o_fb_02);
		iocoreva_attachout(base + 0x03, videova_o_fb_03);

		iocoreva_attachout(base + 0x04, videova_o_fb_04);
		iocoreva_attachout(base + 0x05, videova_o_fb_05);

		iocoreva_attachout(base + 0x06, videova_o_fb_06);
		iocoreva_attachout(base + 0x07, videova_o_fb_07);

		iocoreva_attachout(base + 0x08, videova_o_fb_08);
		iocoreva_attachout(base + 0x09, videova_o_fb_09);

		iocoreva_attachout(base + 0x0a, videova_o_fb_0a);
		iocoreva_attachout(base + 0x0b, videova_o_fb_0b);

		iocoreva_attachout(base + 0x0c, videova_o_fb_0c);
		iocoreva_attachout(base + 0x0d, videova_o_fb_0d);

		iocoreva_attachout(base + 0x0e, videova_o_fb_0e);
		iocoreva_attachout(base + 0x0f, videova_o_fb_0f);
		iocoreva_attachout(base + 0x10, videova_o_fb_10);
		iocoreva_attachout(base + 0x11, videova_o_fb_11);

		iocoreva_attachout(base + 0x12, videova_o_fb_12);
		iocoreva_attachout(base + 0x13, videova_o_fb_13);

		iocoreva_attachout(base + 0x16, videova_o_fb_16);
		iocoreva_attachout(base + 0x17, videova_o_fb_17);

	
		iocoreva_attachinp(base + 0x00, videova_i_fb_00);
		iocoreva_attachinp(base + 0x01, videova_i_fb_01);
		iocoreva_attachinp(base + 0x02, videova_i_fb_02);
		iocoreva_attachinp(base + 0x03, videova_i_fb_03);

		iocoreva_attachinp(base + 0x04, videova_i_fb_04);
		iocoreva_attachinp(base + 0x05, videova_i_fb_05);

		iocoreva_attachinp(base + 0x06, videova_i_fb_06);
		iocoreva_attachinp(base + 0x07, videova_i_fb_07);

		iocoreva_attachinp(base + 0x08, videova_i_fb_08);
		iocoreva_attachinp(base + 0x09, videova_i_fb_09);

		iocoreva_attachinp(base + 0x0a, videova_i_fb_0a);
		iocoreva_attachinp(base + 0x0b, videova_i_fb_0b);

		iocoreva_attachinp(base + 0x0c, videova_i_fb_0c);
		iocoreva_attachinp(base + 0x0d, videova_i_fb_0d);

		iocoreva_attachinp(base + 0x0e, videova_i_fb_0e);
		iocoreva_attachinp(base + 0x0f, videova_i_fb_0f);
		iocoreva_attachinp(base + 0x10, videova_i_fb_10);
		iocoreva_attachinp(base + 0x11, videova_i_fb_11);

		iocoreva_attachinp(base + 0x12, videova_i_fb_12);
		iocoreva_attachinp(base + 0x13, videova_i_fb_13);

		iocoreva_attachinp(base + 0x16, videova_i_fb_16);
		iocoreva_attachinp(base + 0x17, videova_i_fb_17);
	}

	for (i = 0; i < VIDEOVA_PALETTES * 2; i+=2) {
		iocoreva_attachout(PORT_PALETTE + i, videova_o_palette_l);
		iocoreva_attachout(PORT_PALETTE + i+1, videova_o_palette_h);
	}

}


int videova_hsyncmode(void) {
	int ret;

	if (videova.crtmode) {
		// 24KHzディスプレイ
		if (videova.grmode & 0x0080) {
			// インターレースモード
			ret = VIDEOVA_15_73KHZ;
		}
		else {
			// ノンインターレースモード
			ret = VIDEOVA_24_8KHZ;
		}
	}
	else {
		// 15KHzディスプレイ
		if (videova.grmode & 0x0080) {
			// インターレースモード
			ret = VIDEOVA_15_73KHZ;
		}
		else {
			// ノンインターレースモード
			ret = VIDEOVA_15_98KHZ;
		}
	}
	return ret;
}

#endif
