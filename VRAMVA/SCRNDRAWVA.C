#include	"compiler.h"
#include	"scrnmng.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"
#include	"scrndraw.h"
#include	"sdrawva.h"
#include	"maketextva.h"

#if defined(SUPPORT_PC88VA)

	WORD	vabitmap[SURFACE_SIZE];		// 画面合成結果(VA 16bit/pixel)
	BYTE	colorlevel5[32];			// 出力レベル変換 5bit→8bit
	BYTE	colorlevel6[64];			// 出力レベル変換 6bit→8bit

#if 0
	BYTE	np2_tbitmap[SURFACE_SIZE];
#endif

static int	initialized = 0;

void scrndrawva_initialize(void) {
	int i;

#if 0
	ZeroMemory(np2_tbitmap, sizeof(np2_tbitmap));
#endif
	ZeroMemory(vabitmap, sizeof(vabitmap));

	for (i=0; i<32; i++) {
		colorlevel5[i]= (i << 3) | ((i) ?  0x07 : 0);
	}
	for (i=0; i<64; i++) {
		colorlevel6[i]= (i << 2) | ((i) ?  0x03 : 0);
	}
/*
	{
		int i,j;
		for (i=0; i<200; i++) {
			for (j=0; j<16; j++) {
				np2_tbitmap[SURFACE_WIDTH*i+j]=i%15;

			}
		}
	}
*/
}

void scrndrawva_draw_sub(const SCRNSURF	*surf) {
	SDRAWFNVA	sdrawfn;
	_SDRAWVA		sdraw;
	int				height;

	if (!initialized) {
		scrndrawva_initialize();
		initialized=1;
	}
	sdrawfn = sdrawva_getproctbl(surf);
	height = surf->height;

	sdraw.dst = surf->ptr;
	sdraw.width = surf->width;
	sdraw.xbytes = surf->xalign * surf->width;
	sdraw.y = 0;
	sdraw.xalign = surf->xalign;
	sdraw.yalign = surf->yalign;
	sdrawfn(&sdraw, height);
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

	bp = work.bp;
	for (x = 0; x < SURFACE_WIDTH; x++) {
		*bp = videova.palette[textraster[x]];
		bp++;
	}

	work.bp = bp;
}


#endif
