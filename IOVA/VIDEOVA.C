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

enum {
	PORT_FRAMEBUFFER = 0x200,
	PORT_PALETTE = 0x300,
};

	_VIDEOVA	videova;



static int port2pal(UINT port) {
	return (port - PORT_PALETTE) / 2;
}

static void adjustpal(int palno) {
	WORD c;

	c = videova.palette[palno];

	if (c & 0xf000) c |= 0x0c00;
	if (c & 0x03c0) c |= 0x0020;
	if (c & 0x001e) c |= 0x0001;

	videova.palette[palno] = c;
}

// ---- I/O

static REG8 IOINPCALL videova_i100(UINT port) {
	return videova.grmode & 0xff;
}

static REG8 IOINPCALL videova_i101(UINT port) {
	return videova.grmode >> 8;
}

static void IOOUTCALL videova_o100(UINT port, REG8 dat) {
	videova.grmode = videova.grmode & 0xff00 | dat;
}

static void IOOUTCALL videova_o101(UINT port, REG8 dat) {
	videova.grmode = videova.grmode & 0x00ff | ((WORD)dat << 8);
}


static void IOOUTCALL videova_o102(UINT port, REG8 dat) {
	videova.grres = videova.grres & 0xff00 | dat;
}

static void IOOUTCALL videova_o103(UINT port, REG8 dat) {
	videova.grres = videova.grres & 0x00ff | ((WORD)dat << 8);
}


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

// ---- I/F

void videova_reset(void) {
	ZeroMemory(&videova, sizeof(videova));

	videova.framebuffer[1].fsa = 0xffffffffL;
	videova.framebuffer[1].fbl = 0xffff;
	videova.framebuffer[1].ofx = 0xffff;
	videova.framebuffer[1].ofy = 0xffff;
	// ToDo: ƒpƒŒƒbƒg‚Ì‰Šú‰»
}

void videova_bind(void) {
	int i;

	for (i = 0; i < VIDEOVA_PALETTES * 2; i+=2) {
		iocoreva_attachout(PORT_PALETTE + i, videova_o_palette_l);
		iocoreva_attachout(PORT_PALETTE + i+1, videova_o_palette_h);
	}

	iocoreva_attachinp(0x100, videova_i100);
	iocoreva_attachinp(0x101, videova_i101);

	iocoreva_attachout(0x100, videova_o100);
	iocoreva_attachout(0x101, videova_o101);
	iocoreva_attachout(0x102, videova_o102);
	iocoreva_attachout(0x103, videova_o103);

	for (i = 0; i < VIDEOVA_FRAMEBUFFERS; i++) {
		int base;
		base = PORT_FRAMEBUFFER + 0x20 * i;
		iocoreva_attachout(base + 0x00, videova_o_fb_00);
		iocoreva_attachout(base + 0x01, videova_o_fb_01);
		iocoreva_attachout(base + 0x02, videova_o_fb_02);

		iocoreva_attachout(base + 0x04, videova_o_fb_04);
		iocoreva_attachout(base + 0x05, videova_o_fb_05);

		iocoreva_attachout(base + 0x06, videova_o_fb_06);
		iocoreva_attachout(base + 0x07, videova_o_fb_07);

		iocoreva_attachout(base + 0x08, videova_o_fb_08);

		iocoreva_attachout(base + 0x0a, videova_o_fb_0a);
		iocoreva_attachout(base + 0x0b, videova_o_fb_0b);

		iocoreva_attachout(base + 0x0c, videova_o_fb_0c);
		iocoreva_attachout(base + 0x0d, videova_o_fb_0d);

		iocoreva_attachout(base + 0x0e, videova_o_fb_0e);
		iocoreva_attachout(base + 0x0f, videova_o_fb_0f);
		iocoreva_attachout(base + 0x10, videova_o_fb_10);

		iocoreva_attachout(base + 0x12, videova_o_fb_12);
		iocoreva_attachout(base + 0x13, videova_o_fb_13);

		iocoreva_attachout(base + 0x16, videova_o_fb_16);
		iocoreva_attachout(base + 0x17, videova_o_fb_17);
	}
}

#endif
