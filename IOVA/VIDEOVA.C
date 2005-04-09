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
	PORT_PAL = 0x300,
};

	_VIDEOVA	videova;



static int port2pal(UINT port) {
	return (port - PORT_PAL) / 2;
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
	videova.grmode = videova.grmode & 0x00ff | (dat << 8);
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
	videova.palette[n] = videova.palette[n] & 0x00ff | (dat << 8);
	adjustpal(n);
}


// ---- I/F

void videova_reset(void) {
	ZeroMemory(&videova, sizeof(videova));

	// ToDo: ƒpƒŒƒbƒg‚Ì‰Šú‰»
}

void videova_bind(void) {
	int i;

	for (i = 0; i < VIDEOVA_PALETTES * 2; i+=2) {
		iocoreva_attachout(PORT_PAL + i, videova_o_palette_l);
		iocoreva_attachout(PORT_PAL + i+1, videova_o_palette_h);
	}

	iocoreva_attachinp(0x100, videova_i100);
	iocoreva_attachinp(0x101, videova_i101);
	iocoreva_attachout(0x100, videova_o100);
	iocoreva_attachout(0x101, videova_o101);
}

#endif
