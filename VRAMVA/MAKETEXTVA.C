#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
//#include	"vram.h"
#include	"scrnmng.h"
#include	"scrndraw.h"
#include	"scrndrawva.h"
//#include	"dispsync.h"

#include	"cgromva.h"
#include	"memoryva.h"
#include	"maketextva.h"

enum {
	TEXTVA_LINEHEIGHTMAX	= 16,
	TEXTVA_CHARWIDTH		= 8,

	TEXTVA_ATR_ST	= 0x01,		// シークレット
	TEXTVA_ATR_BL	= 0x02,		// ブリンク
	TEXTVA_ATR_RV	= 0x04,		// リバース
	TEXTVA_ATR_HL	= 0x08,		// ホリゾンタルライン(mode 1)
	TEXTVA_ATR_HL2	= 0x10,		// ホリゾンタルライン(mode 2,3)
	TEXTVA_ATR_UL	= 0x20,		// アンダーライン
	TEXTVA_ATR_DWID	= 0x40,		// ダブルウィドス
	TEXTVA_ATR_DWIDC= 0x80,		// ダブルウィドスコントロール
};

typedef struct {
	UINT8	bg;
	UINT8	fg;
	UINT8	attr;
} _CHARATTR, *CHARATTR;

typedef struct {
	UINT16	attroffset;			// アトリビュート領域(TVRAM先頭からのオフセット)
	UINT8	lineheight;
	UINT8	hlinepos;
} _TSP, *TSP;

static _TSP tsp;

// テキスト1行分のbitmap
static BYTE linebitmap[SURFACE_WIDTH * TEXTVA_LINEHEIGHTMAX];


	BYTE textraster[SURFACE_WIDTH];		// 1ラスタ分のピクセルデータ
										// 各ピクセルはパレット番号(0～15)







void maketextva_initialize(void) {
	tsp.attroffset = 0x8000;
	tsp.lineheight = 16;
	tsp.hlinepos = 15;
}


static _CHARATTR synattr(BYTE attr) {
	_CHARATTR	charattr;
	// attribute mode 1
	charattr.bg = 0;
	charattr.fg = attr >> 4;
	charattr.attr = attr & 0x0f;
	return charattr;
}

static void makeline(BYTE *v, UINT16 rwchar) {
	int		x;
	int		r;
	int		i;
	BYTE	*b;
	WORD	hccode;
	BYTE	*font;
	BYTE	fontdata;
	BYTE	attr;
	int		fontw;
	_CHARATTR	charattr;
	UINT8	bg;
	UINT8	fg;

	if (rwchar > SURFACE_WIDTH / TEXTVA_CHARWIDTH) {
		rwchar = SURFACE_WIDTH / TEXTVA_CHARWIDTH;
	}
	b = linebitmap;

	for (x = 0; x < rwchar; x++) {
		hccode = LOADINTELWORD(v);
		attr = *(v + tsp.attroffset);
		v+=2;
		font = cgromva_font(hccode);
		fontw = cgromva_width(hccode);
		charattr = synattr(attr);

		if (charattr.attr & TEXTVA_ATR_RV) {
			bg = charattr.fg;
			fg = charattr.bg;
		}
		else {
			bg = charattr.bg;
			fg = charattr.fg;
		}
		if (charattr.attr & TEXTVA_ATR_ST) {
			fg = bg;
		}

		for (r = 0; r < tsp.lineheight; r++) {
			fontdata = *font;
			font += fontw;
			if ((charattr.attr & (TEXTVA_ATR_HL | TEXTVA_ATR_HL2)) && r == tsp.hlinepos) {
				for (i = 0; i < 8; i++) {
					b[i] = fg;
				}
			}
			else {
				for (i = 0; i < 8; i++) {
					b[i] = (fontdata & 0x80) ? fg : bg;
					fontdata <<= 1;
				}
			}
			b += SURFACE_WIDTH;
		}
		b -= SURFACE_WIDTH * tsp.lineheight - TEXTVA_CHARWIDTH;
	}

}

#if 0
void maketextva(void) {
	UINT	x;
	UINT	y;
	UINT	raster;
	UINT	texty;
	UINT16	rsa;		// 分割画面スタートアドレス
	UINT16	vw;			// フレームバッファの横幅(バイト)
	UINT16	rw;			// 分割画面の横幅(ドット) 32の倍数 この値/8+2 文字表示される
	UINT16	rwchar;		// 分割画面の横幅(文字) = rw / 8 + 2 
	BYTE	*v;			// TVRAM
	BYTE	*b;			// bitmap
	BYTE	*lb;		// linebitmap
	BOOL	linebitmap_ready;

	linebitmap_ready = FALSE;
	rsa = 0;
	vw = 80*2;
	rw = 640;
	rwchar = rw / 8 + 2;
	raster = 0;
	texty = 0;
	for (y=0; y<SURFACE_HEIGHT; y++) {
		if (!linebitmap_ready) {
			v = textmem + rsa + vw * texty;
			makeline(v, rwchar);

			texty++;
			linebitmap_ready = TRUE;
		}

		b = np2_tbitmap + SURFACE_WIDTH * y;
		lb = linebitmap + SURFACE_WIDTH * raster;
		for (x = 0; x < SURFACE_WIDTH; x++) {
			*b = *lb;
			b++;
			lb++;
		}

		raster++;
		if (raster >= tsp.lineheight) {
			linebitmap_ready = FALSE;
			raster = 0;
		}
	}


}
#endif


typedef struct {
	UINT	y;
	UINT	raster;
	UINT	texty;
	UINT16	rsa;		// 分割画面スタートアドレス
	UINT16	vw;			// フレームバッファの横幅(バイト)
	UINT16	rw;			// 分割画面の横幅(ドット) 32の倍数 この値/8+2 文字表示される
	UINT16	rwchar;		// 分割画面の横幅(文字) = rw / 8 + 2 
	BOOL	linebitmap_ready;
} _TEXTVAWORK;

static _TEXTVAWORK	work;


void maketextva_begin(void) {
	work.linebitmap_ready = FALSE;
	work.rsa = 0;
	work.vw = 80*2;
	work.rw = 640;
	work.rwchar = work.rw / 8 + 2;
	work.raster = 0;
	work.texty = 0;
	work.y = 0;
}

void maketextva_raster(void) {
	UINT	x;
	BYTE	*v;			// TVRAM
	BYTE	*b;			// bitmap
	BYTE	*lb;		// linebitmap

	if (work.y >= SURFACE_HEIGHT) return;

		if (!work.linebitmap_ready) {
			v = textmem + work.rsa + work.vw * work.texty;
			makeline(v, work.rwchar);

			work.texty++;
			work.linebitmap_ready = TRUE;
		}

		b = textraster;
		lb = linebitmap + SURFACE_WIDTH * work.raster;
		for (x = 0; x < SURFACE_WIDTH; x++) {
			*b = *lb;
			b++;
			lb++;
		}

		work.raster++;
		if (work.raster >= tsp.lineheight) {
			work.linebitmap_ready = FALSE;
			work.raster = 0;
		}

	work.y++;

}
