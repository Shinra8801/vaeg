
#include	"compiler.h"
#include	"scrnmng.h"
#include	"scrndraw.h"
#include	"sdrawva.h"
#include	"scrndrawva.h"

#include	"palettesva.h"

#if defined(SUPPORT_PC88VA)





static void SCRNCALL _sdrawva32(SDRAWVA sdraw, int maxy) {

	const WORD	*p;
	BYTE	*q;
	int		y;
	int		x;

	p = vabitmap;
	q = sdraw->dst;
	y = sdraw->y;
	do {
		if (1) {
			*(UINT32 *)q = 0;
			for (x=0; x<sdraw->width; x++) {
				WORD c;

				q += sdraw->xalign;
				c = p[x];
				*(UINT32 *)q = RGB32D(
					colorlevel5[(c & 0x03e0) >> 5], 
					colorlevel6[(c & 0xfc00) >> 10],
					colorlevel5[c & 0x1f]);
			}
			q -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += sdraw->yalign;
	} while(++y < maxy);

	// 本当は、srcも保存できないとダメ
	sdraw->dst = q;
	sdraw->y = y;
}

#if 0
static void SCRNCALL sdrawva32(SDRAWVA sdraw, int maxy) {

	const BYTE	*p;
	BYTE	*q;
	int		y;
	int		x;

	p = np2_tbitmap;
	q = sdraw->dst;
	y = sdraw->y;
	do {
		if (1) {
			*(UINT32 *)q = 0;
			for (x=0; x<sdraw->width; x++) {
				BYTE c;

				q += sdraw->xalign;
				c = p[x];
				if (c) {
					c &= 0x0f;
					*(UINT32 *)q = np2_palva32[c].d;
				}
			}
			q -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += sdraw->yalign;
	} while(++y < maxy);

	// 本当は、srcも保存できないとダメ
	sdraw->dst = q;
	sdraw->y = y;
}

static const SDRAWFNVA tbl[] = {
	sdrawva32
};

#endif

static const SDRAWFNVA _tbl[] = {
	_sdrawva32
};

const SDRAWFNVA sdrawva_getproctbl(const SCRNSURF *surf) {

//	return tbl[0];
	return _tbl[0];
}

#endif