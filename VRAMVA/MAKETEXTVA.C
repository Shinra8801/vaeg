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

	TEXTVA_FRAMES			= 4,		// 分割画面の最大数

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

typedef void (*SYNATTRFN)(BYTE, CHARATTR);	// アトリビュート合成ルーチン


typedef struct {
	UINT16	texttable;			// テキスト画面制御テーブル(TVRAM先頭からのオフセット)
	UINT16	attroffset;			// アトリビュート領域(TVRAM先頭からのオフセット)
	UINT8	lineheight;
	UINT8	hlinepos;
} _TSP, *TSP;

typedef struct {		// テキスト分割画面制御テーブル(のコピー)
	UINT16	vw;			// フレームバッファ横幅(バイト)
	UINT8	mode;		// 表示モード
	UINT8	fg;			// フォアグラウンドカラー
	UINT8	bg;			// バックグラウンドカラー
	UINT8	rasteroffset;	// ラスタアドレスオフセット
	UINT32	rsa;		// 分割画面スタートアドレス(TVRAM先頭を0としたバイトアドレス)
	UINT16	rh;			// 分割画面の高さ(ラスタ) (16以上の偶数)
	UINT16	rw;			// 分割画面の横幅(ドット)(32の倍数、設定値/8+2文字が表示される)
	UINT16	rwchar;		// 分割画面の横幅(文字) = rw / 8 + 2 
	SINT16	rxp;		// 分割画面水平開始位置(ドット)
} _TEXTVAFRAME, *TEXTVAFRAME;

typedef struct {
	UINT	y;			// 現在処理中のラスタ
	UINT	raster;
	UINT	texty;
/*
	UINT16	rsa;		// 分割画面スタートアドレス
	UINT16	vw;			// フレームバッファの横幅(バイト)
	UINT16	rw;			// 分割画面の横幅(ドット) 32の倍数 この値/8+2 文字表示される
	UINT16	rwchar;		// 分割画面の横幅(文字) = rw / 8 + 2 
*/
	BOOL	linebitmap_ready;

	UINT		frameno;			// 現在参照している分割画面の番号
	UINT		framelimit;			// 次の分割画面の開始位置(ラスタ)
	SYNATTRFN	synattr;			// アトリビュート合成ルーチン
	TEXTVAFRAME		frame;			// 現在参照している分割画面へのポインタ	
	_TEXTVAFRAME	_frame[TEXTVA_FRAMES];
} _TEXTVAWORK;

static	_TEXTVAWORK	work;

static	_TSP	tsp;

static	BYTE linebitmap[SURFACE_WIDTH * TEXTVA_LINEHEIGHTMAX];	// テキスト1行分のbitmap

		BYTE textraster[SURFACE_WIDTH];		// 1ラスタ分のピクセルデータ
											// 各ピクセルはパレット番号(0～15)







void maketextva_initialize(void) {
	tsp.texttable = 0x7f00;
	tsp.attroffset = 0x8000;
	tsp.lineheight = 16;
	tsp.hlinepos = 15;
}


// attribute mode 0
static void synattr0(BYTE attr, CHARATTR charattr) {
	charattr->bg = attr >> 4;
	charattr->fg = attr & 0x0f;
	charattr->attr = 0;
}

// attribute mode 1
static void synattr1(BYTE attr, CHARATTR charattr) {
	charattr->bg = work.frame->bg;
	charattr->fg = attr >> 4;
	charattr->attr = attr & 0x0f;
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
		work.synattr(attr, &charattr);

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


static void selectframe(int no) {
	static SYNATTRFN synattrtbl[]={
		synattr0, synattr1, synattr0, synattr0, synattr0, synattr0, synattr0, synattr0
	};

	work.frameno = no;
	work.frame = &work._frame[no];
	work.framelimit = work.y + work.frame->rh;
	if (no == TEXTVA_FRAMES - 1) {	// 最後の分割画面なら
		work.framelimit = 0x1fe;	// rh に指定可能な最大値
	}
	work.texty = 0;
	work.raster = work.frame->rasteroffset;
	work.linebitmap_ready = FALSE;
	work.synattr = synattrtbl[work.frame->mode & 0x07];
}

void maketextva_begin(void) {
	int i;
	BYTE *fbinfo;

/*
	work.rsa = 0;
	work.vw = 80*2;
	work.rw = 640;
	work.rwchar = work.rw / 8 + 2;
*/
	work.y = 0;

	// 分割画面制御テーブルの情報をコピー
	fbinfo = textmem + tsp.texttable;
	for (i = 0; i < TEXTVA_FRAMES; i++) {
		TEXTVAFRAME f;
		WORD d;

		f = &work._frame[i];

		f->vw = LOADINTELWORD(fbinfo + 0x08) & 0x03ff;
		d = LOADINTELWORD(fbinfo + 0x0a);
		f->mode = d & 0x1f;
		f->bg = (d & 0x0f00) >> 8;
		f->fg = (d & 0xf000) >> 12;
		f->rasteroffset = fbinfo[0x0d] & 0x1f;
		f->rsa = LOADINTELWORD(fbinfo + 0x10);	// ToDo: テクマニでは16bitだが、18bitではないか？
		f->rh = LOADINTELWORD(fbinfo + 0x14) & 0x01fe;
		if (f->rh == 0) f->rh = 0x01fe;
		f->rw = LOADINTELWORD(fbinfo + 0x16) & 0x03ff;
		f->rwchar = f->rw / TEXTVA_CHARWIDTH + 2;
		f->rxp = LOADINTELWORD(fbinfo + 0x1a) & 0x03ff;

		fbinfo += 0x0020;
	}

	selectframe(0);
}

void maketextva_raster(void) {
	UINT	x;
	BYTE	*v;			// TVRAM
	BYTE	*b;			// bitmap
	BYTE	*lb;		// linebitmap
	TEXTVAFRAME	f;

	if (work.y >= SURFACE_HEIGHT) return;

	while (work.y >= work.framelimit) {
		work.frameno++;
		selectframe(work.frameno);
	}

	f = work.frame;

	if (!work.linebitmap_ready) {
		v = textmem + f->rsa + f->vw * work.texty;
		makeline(v, f->rwchar);

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
