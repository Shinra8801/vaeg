#include	"compiler.h"
#include	"scrnmng.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"
#include	"scrndraw.h"
#include	"sdrawva.h"
#include	"dispsync.h"
#include	"maketextva.h"
#include	"makesprva.h"
#include	"makegrphva.h"

#if defined(SUPPORT_PC88VA)

enum {
	COLORCODES = 0x10000,
};


	WORD	vabitmap[SURFACE_SIZE];		// 画面合成結果(VA 16bit/pixel)

#if defined(SUPPORT_24BPP) || defined(SUPPORT_32BPP)
	RGB32		drawcolor32[COLORCODES];
#endif

#if defined(SUPPORT_16BPP)
	RGB16		drawcolor16[COLORCODES];
#endif



static void scrndrawva_initialize(void) {
	static int	initialized = 0;

	if (initialized) return;
	initialized=1;


	ZeroMemory(vabitmap, sizeof(vabitmap));
}

static void scrndrawva_makedrawcolor(void) {
	BYTE	colorlevel5[32];			// 出力レベル変換 5bit→8bit
	BYTE	colorlevel6[64];			// 出力レベル変換 6bit→8bit
	int i;
	int c;

	for (i=0; i<32; i++) {
		colorlevel5[i]= (i << 3) | ((i) ?  0x07 : 0);
	}
	for (i=0; i<64; i++) {
		colorlevel6[i]= (i << 2) | ((i) ?  0x03 : 0);
	}

	for (c = 0; c < COLORCODES; c++) {
#if defined(SUPPORT_24BPP) || defined(SUPPORT_32BPP)
		drawcolor32[c].d = RGB32D(
						colorlevel5[(c & 0x03e0) >> 5], 
						colorlevel6[(c & 0xfc00) >> 10],
						colorlevel5[c & 0x1f]);
#endif
#if defined(SUPPORT_16BPP)
		{
			RGB32 rgb32;
			rgb32.d = RGB32D(
							colorlevel5[(c & 0x03e0) >> 5], 
							colorlevel6[(c & 0xfc00) >> 10],
							colorlevel5[c & 0x1f]);
			drawcolor16[c] = scrnmng_makepal16(rgb32);
		}
#endif
	}
}

RGB32 scrndrawva_drawcolor32(WORD colorva16) {
#if defined(SUPPORT_24BPP) || defined(SUPPORT_32BPP)
	return drawcolor32[colorva16];
#else
	RGB32	rgb;
	return rgb;		// ToDo:
#endif
}

BYTE scrndrawva_draw(BYTE redraw) {

	BYTE		ret;
	const	SCRNSURF	*surf;
	SDRAWFNVA	sdrawfn;
	_SDRAWVA	sdraw;
	int			height;

	scrndrawva_initialize();

	ret = 0;
	surf = scrnmng_surflock();
	if (surf == NULL) {
		goto sddr_exit1;
	}

	sdrawfn = sdrawva_getproctbl(surf);
	if (sdrawfn == NULL) {
		goto sddr_exit2;
	}

	height = surf->height;

	sdraw.dst = surf->ptr;
	sdraw.width = surf->width;
	sdraw.xbytes = surf->xalign * surf->width;
	sdraw.y = 0;
	sdraw.xalign = surf->xalign;
	sdraw.yalign = surf->yalign;
	sdrawfn(&sdraw, height);

sddr_exit2:
	scrnmng_surfunlock(surf);

sddr_exit1:
	return ret;

}


void scrndrawva_redraw(void) {

	scrnmng_allflash();
	dispsync_renewalmode();
	scrndrawva_makedrawcolor();
	scrndrawva_draw(1);
}

typedef struct {
	int		type;			// VIDEOVA_xxxxSCREEN (未使用 -1)

	BYTE	*raster;		// 1ライン分の色データ(パレット番号orVA16bitカラーコード)
	BYTE	palflip;		// パレット番号に XORするデータ
							// パレットセット0使用時は0, 1使用時は0x10
	DWORD	xpar;			// 透明色フラグ(bit0:パレット0～bit31:パレット31)
} _COMPSCRN, *COMPSCRN;

typedef struct {
	WORD		*bp;			// vabitmap
	_COMPSCRN	scrn[VIDEOVA_PALETTE_SCREENS + VIDEOVA_RGB_SCREENS];
} _COMPOSEWORK;

static	_COMPOSEWORK	work;

void scrndrawva_compose_begin(void) {
	work.bp = vabitmap;
}


void scrndrawva_compose_raster(void) {
	int	x;
	int	i;
	int palmode;
	int palset1scrn;
	int type;
	WORD *bp;
	COMPSCRN scrn;
	WORD c;
	DWORD dd;
//	BYTE *palscrn[4];
//	BYTE pri[VIDEOVA_SCREENS];
	BYTE defaultflip;
/*
	pri[VIDEOVA_TEXTSCREEN] = 1;
	pri[VIDEOVA_SPRITESCREEN] = 0;
	pri[VIDEOVA_GRAPHICSCREEN0] = 3;
	pri[VIDEOVA_GRAPHICSCREEN1] = 2;
*/
	palmode = (videova.palmode >> 6) & 3;
	defaultflip = palmode == 1 ? 0x10 : 0x00;
	palset1scrn = (videova.palmode >> 4) & 3;

	dd = videova.colcomp | ((DWORD)videova.rgbcomp << 16);
/*
	for (i = 0; i < VIDEOVA_PALETTE_SCREENS + VIDEOVA_RGB_SCREENS; i++) {
		work.scrn[i].raster = NULL;
	}
*/
	//for (i = 0; i < VIDEOVA_SCREENS; i++) {
	scrn = work.scrn;
	for (i = 0; i < VIDEOVA_PALETTE_SCREENS; i++, scrn++) {
		type = (int)(dd & 0x0f);
		dd >>= 4;
		if (type < 8) {
			scrn->raster = NULL;
			continue;
		}
		if (i < VIDEOVA_PALETTE_SCREENS) {
			type &= 0x03;
		}
		else {
			// ToDo: 直接色指定
			//type = (type & 0x01) + VIDEOVA_GRAPHICSCREEN0;
		}
		scrn->type = type;
//		scrn = &work.scrn[pri[i]];
		scrn->palflip = defaultflip;
		if (palmode == 2 && type == palset1scrn) scrn->palflip = 0x10;

		switch (type) {
		case VIDEOVA_TEXTSCREEN:
			scrn->raster = textraster;
			scrn->xpar = videova.xpar_txtspr | ((DWORD)videova.xpar_txtspr << 16);
			break;
		case VIDEOVA_SPRITESCREEN:
			scrn->raster = sprraster;
			scrn->xpar = videova.xpar_txtspr | ((DWORD)videova.xpar_txtspr << 16);
			break;
		case VIDEOVA_GRAPHICSCREEN0:
			scrn->raster = grph0p_raster;
			scrn->xpar = videova.xpar_g0 | ((DWORD)videova.xpar_g0 << 16);
			break;
		case VIDEOVA_GRAPHICSCREEN1:
			scrn->raster = grph1p_raster;
			scrn->xpar = videova.xpar_g1 | ((DWORD)videova.xpar_g1 << 16);
			break;
		}
		// ToDo: palette mode 3
	}
/*
	for (i = 0; i < VIDEOVA_PALETTE_SCREENS; i++) work.scrn[i].palflip = 0;
	switch ((videova.palmode >> 6) & 3) {
	case 1:
		for (i = 0; i < VIDEOVA_PALETTE_SCREENS; i++) work.scrn[i].palflip = 0x10;
		break;
	case 2:
		work.scrn[pri[(videova.palmode >> 4) & 3]].palflip = 0x10;
		break;
	case 3:	// ToDo
		break;
	}
*/
	bp = work.bp;
	for (x = 0; x < SURFACE_WIDTH; x++) {
		for (i = 0; i < VIDEOVA_PALETTE_SCREENS; i++) {
			scrn = &work.scrn[i];
			if (scrn->raster != NULL) {
				c = scrn->raster[x] ^ scrn->palflip;
				if ((scrn->xpar & (1 << c)) == 0) {
					// 不透明色
					c = videova.palette[c];
					goto opaque;
				}
			}
		}
		// ToDo: 直接色指定画面

		c = videova.dropcol;
/*
		c = work.raster[1][x] ^ work.flipset[1];
		if ((c & 0x0f) == 0) c = work.raster[0][x] ^ work.flipset[0];
		if ((c & 0x0f) == 0) c = work.raster[3][x] ^ work.flipset[3];
		if ((c & 0x0f) == 0) c = work.raster[2][x] ^ work.flipset[2];
		*bp = videova.palette[c];
*/
 /*
		c = sprraster[x];
		if (c == 0) c = textraster[x];
		if (c == 0) c = grph1p_raster[x];
		if (c == 0) c = grph0p_raster[x];
		*bp = videova.palette[c];
*/
	opaque:
		*bp = c;
		bp++;
	}

	work.bp = bp;
}


#endif
