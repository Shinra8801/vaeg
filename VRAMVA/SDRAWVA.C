
#include	"compiler.h"
#include	"scrnmng.h"
#include	"scrndraw.h"
#include	"sdrawva.h"
#include	"scrndrawva.h"

#if defined(SUPPORT_PC88VA)



#if defined(SUPPORT_16BPP)
static void SCRNCALL sdrawva16(SDRAWVA sdraw, int maxy) {

	const WORD	*p;
	BYTE	*q;
	int		y;
	int		x;

	p = vabitmap;
	q = sdraw->dst;
	y = sdraw->y;
	do {
		if (1) {
			*(UINT16 *)q = 0;
			for (x=0; x<sdraw->width; x++) {
				WORD c;
//				RGB32 rgb32;

				q += sdraw->xalign;
				c = p[x];
				//if (c) {
/*
					rgb32.d = 
						RGB32D(
							colorlevel5[(c & 0x03e0) >> 5], 
							colorlevel6[(c & 0xfc00) >> 10],
							colorlevel5[c & 0x1f] );
					*(UINT16 *)q = scrnmng_makepal16(rgb32);
*/
					*(UINT16 *)q = drawcolor16[c];
				//}
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
#endif

#if defined(SUPPORT_24BPP)

static void SCRNCALL sdrawva24(SDRAWVA sdraw, int maxy) {

	const WORD	*p;
	BYTE	*q;
	int		y;
	int		x;

	p = vabitmap;
	q = sdraw->dst;
	y = sdraw->y;
	do {
		if (1) {
			((UINT8 *)q)[RGB24_R] = 0;
			((UINT8 *)q)[RGB24_G] = 0;
			((UINT8 *)q)[RGB24_B] = 0;
			for (x=0; x<sdraw->width; x++) {
				WORD c;

				q += sdraw->xalign;
				c = p[x];
				//if (c) {
/*
					((UINT8 *)q)[RGB24_R] = colorlevel5[(c & 0x03e0) >> 5];
					((UINT8 *)q)[RGB24_G] = colorlevel6[(c & 0xfc00) >> 10];
					((UINT8 *)q)[RGB24_B] = colorlevel5[c & 0x1f];
*/
					((UINT8 *)q)[RGB24_R] = drawcolor32[c].p.r;
					((UINT8 *)q)[RGB24_G] = drawcolor32[c].p.g;
					((UINT8 *)q)[RGB24_B] = drawcolor32[c].p.b;
				//}
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

#endif


#if defined(SUPPORT_32BPP)

static void SCRNCALL sdrawva32(SDRAWVA sdraw, int maxy) {

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
				//if (c) {
/*
					*(UINT32 *)q = RGB32D(
						colorlevel5[(c & 0x03e0) >> 5], 
						colorlevel6[(c & 0xfc00) >> 10],
						colorlevel5[c & 0x1f]);
*/
					*(UINT32 *)q = drawcolor32[c].d;
				//}
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

#endif


static const SDRAWFNVA tbl[] = {
	NULL,	// 8bit/pixel

#if defined(SUPPORT_16BPP)
	sdrawva16,
#else
	NULL,
#endif

#if defined(SUPPORT_24BPP)
	sdrawva24,
#else
	NULL,
#endif

#if defined(SUPPORT_32BPP)
	sdrawva32,
#else
	NULL,
#endif
};

const SDRAWFNVA sdrawva_getproctbl(const SCRNSURF *surf) {
	return tbl[((surf->bpp >> 3) - 1) & 3];
}

#endif