#include	"compiler.h"
#include	"scrnmng.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"scrndraw.h"
#include	"scrndrawva.h"
#include	"palettesva.h"

#if defined(SUPPORT_PC88VA)
#if 0


		RGB32		np2_palva32[NP2PALVA_TOTAL];
#if defined(SUPPORT_16BPP)
		RGB16		np2_palva16[NP2PALVA_TOTAL];
#endif

static	UINT8		anapal1[16];


/*
テーブルの初期化
  エミュレータ起動後に1回実行すればよい。
*/
void palva_maketable(void) {

	int		i;
	UINT8	ana;

	for (i=0; i<16; i++) {
		ana = (UINT8)(i * 0x11);
		anapal1[i] = ana;
	}
}

/*
VAのパレット(RGB各4ビット)を描画用の色に変換
  IN:	pal	VAのパレット NP2PAL_TOTAL個数分	
*/
void palva_make(RGB32 *pal) {
	UINT	i;

	for (i=0; i<NP2PALVA_TOTAL; i++, pal++) {
			np2_palva32[i].p.b = anapal1[pal->p.b & 15];
			np2_palva32[i].p.g = anapal1[pal->p.g & 15];
			np2_palva32[i].p.r = anapal1[pal->p.r & 15];

	}
#if defined(SUPPORT_16BPP)
	if (scrnmng_getbpp() == 16) {
		for (i=0; i<NP2PALVA_TOTAL; i++) {
				np2_palva16[i] = scrnmng_makepal16(np2_palva32[i]);
		}
	}
#endif

}

#endif

#endif
