#include	"compiler.h"
#include	"scrnmng.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"scrndraw.h"
#include	"sdrawva.h"

#if defined(SUPPORT_PC88VA)

	BYTE	np2_tbitmap[SURFACE_SIZE];

static int	initialized = 0;

void scrndrawva_initialize(void) {
	ZeroMemory(np2_tbitmap, sizeof(np2_tbitmap));
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

#endif
