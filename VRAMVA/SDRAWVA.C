
#include	"compiler.h"
#include	"scrnmng.h"
#include	"scrndraw.h"
#include	"sdrawva.h"

#include	"palettesva.h"

#if defined(SUPPORT_PC88VA)

extern	BYTE np2_tbitmap[];

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

	// –{“–‚ÍAsrc‚à•Û‘¶‚Å‚«‚È‚¢‚Æƒ_ƒ
	sdraw->dst = q;
	sdraw->y = y;
}

static const SDRAWFNVA tbl[] = {
	sdrawva32
};
const SDRAWFNVA sdrawva_getproctbl(const SCRNSURF *surf) {

	return tbl[0];
}

#endif