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
	WORD *bp;			// vabitmap
} _COMPOSEWORK;

static	_COMPOSEWORK	work;

void scrndrawva_compose_begin(void) {
	work.bp = vabitmap;
}


void scrndrawva_compose_raster(void) {
	int x;
	WORD *bp;
	BYTE c;

	bp = work.bp;
	for (x = 0; x < SURFACE_WIDTH; x++) {
		c = sprraster[x];
		if (c == 0) c = textraster[x];
		if (c == 0) c = grph1p_raster[x];
		if (c == 0) c = grph0p_raster[x];

		*bp = videova.palette[c];
		bp++;
	}

	work.bp = bp;
}


#endif
