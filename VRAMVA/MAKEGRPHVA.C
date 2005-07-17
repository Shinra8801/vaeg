/*
 * MAKEGRPHVA.C: PC-88VA Graphics
 */

#include	"compiler.h"
#include	"scrndraw.h"
#include	"gvramva.h"
#include	"videova.h"
#include	"makegrphva.h"

enum {
	GRPHVA_SCREENS	= 2,
};
typedef struct {
	UINT16	y;						// 現在処理中のラスタ(グラフィック画面の座標系で)
	BOOL	r200lines;				// 解像度200/204ラインならTRUE
	BOOL	r320dots;				// 解像度320ドットならTRUE
	int		pixelmode;				// 0..1bit, 1..4bit, 2..8bit, 3..16bit
									// bit3 0..パレット 1..直接色指定
	BYTE	*rasterbuf;
	UINT32	addrmask;
	UINT32	addrofs;

	UINT32	lineaddr;				// 次回表示GVRAMアドレス
	UINT32	wrappedaddr;			// ラップアラウンド後のGVRAMアドレス
	UINT16	wrapcount;
	FRAMEBUFFER	framebuffer;		// 現在表示しているフレームバッファ
	int		nextframebuffer;		// 次に使用するフレームバッファの番号。無い場合は-1
} _SCREEN, *SCREEN;

typedef struct {
	UINT16		screeny;			// 現在処理中のラスタ(画面共通の座標系で)
	_SCREEN		screen[GRPHVA_SCREENS];
} _GRPHVAWORK;

static	WORD rgb8to16[256];			// VA 8bit RGB→16bit RGB

static	_GRPHVAWORK	work;
		BYTE grph0p_raster[SURFACE_WIDTH + 32];
		BYTE grph1p_raster[SURFACE_WIDTH + 32];
											// 1ラスタ分のピクセルデータ
											// 各ピクセルはパレット番号(0～15)
											// 画面横幅 + 
											// 最大4バイト(ex.1bit/pixelなら32dot)分
											// 使用する
		WORD grph0c_raster[SURFACE_WIDTH + 32];
		WORD grph1c_raster[SURFACE_WIDTH + 32];
											// 1ラスタ分のピクセルデータ
											// 各ピクセルはカラーコード(16bit)

#define addr18(scrn, x) ( (x) & ((scrn)->addrmask) | ((scrn)->addrofs) )

static void selectframe(SCREEN screen, int no) {
	FRAMEBUFFER	f;

	if (no < 0 || no >= VIDEOVA_FRAMEBUFFERS) {
		screen->framebuffer = NULL;
		return;
	}

	f = &videova.framebuffer[no];
	screen->framebuffer = f;
	screen->lineaddr = addr18(screen, f->dsa);
	screen->wrappedaddr = addr18(screen, f->dsa - f->ofx);
	if (no == 0) {
		screen->nextframebuffer = 2;
	}
	else if (no == 2) {
		screen->nextframebuffer = 3;
	}
	else {
		screen->nextframebuffer = -1;
	}
}

// シングルプレーン4bit/pixel (パレット)
static void drawraster_s4p(SCREEN screen) {
	UINT16		xp;
	UINT16		wrapcount;
	UINT32		addr;
	BYTE		*b;

	WORD		d, d2;

	addr = screen->lineaddr;
	b = screen->rasterbuf;
	if (screen->framebuffer->ofx == 0xffff) {
		// screen 1 (ラップアラウンドなし)
		wrapcount = 0;
	}
	else {
		wrapcount = screen->framebuffer->fbw - screen->framebuffer->ofx;
	}


	if (screen->r320dots) {
		// 320 dots
		d = LOADINTELWORD(grphmem + addr);
		d2 = LOADINTELWORD(grphmem + addr + 2);
		addr = addr18(screen, addr + 4);

		switch (screen->framebuffer->dot & 0x13) {
		case 0:
			b[0] = b[1] = (d >>  4) & 0x0f;
			b += 2;
		case 1:
			b[0] = b[1] = (d      ) & 0x0f;
			b += 2;
		case 2:
			b[0] = b[1] = (d >> 12) & 0x0f;
			b += 2;
		case 3:
			b[0] = b[1] = (d >>  8) & 0x0f;
			b += 2;
		case 0x10:
			b[0] = b[1] = (d2 >>  4) & 0x0f;
			b += 2;
		case 0x11:
			b[0] = b[1] = (d2      ) & 0x0f;
			b += 2;
		case 0x12:
			b[0] = b[1] = (d2 >> 12) & 0x0f;
			b += 2;
		case 0x13:
			b[0] = b[1] = (d2 >>  8) & 0x0f;
			b += 2;
		}
		for (xp = 0; xp < 320/8; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			d = LOADINTELWORD(grphmem + addr);
			d2 = LOADINTELWORD(grphmem + addr + 2);
			addr = addr18(screen, addr + 4);

			b[0] = b[1] = (d >>  4) & 0x0f;
			b += 2;
			b[0] = b[1] = (d      ) & 0x0f;
			b += 2;
			b[0] = b[1] = (d >> 12) & 0x0f;
			b += 2;
			b[0] = b[1] = (d >>  8) & 0x0f;
			b += 2;

			b[0] = b[1] = (d2 >>  4) & 0x0f;
			b += 2;
			b[0] = b[1] = (d2      ) & 0x0f;
			b += 2;
			b[0] = b[1] = (d2 >> 12) & 0x0f;
			b += 2;
			b[0] = b[1] = (d2 >>  8) & 0x0f;
			b += 2;
		}
	}
	else {
		// 640 dots

		d = LOADINTELWORD(grphmem + addr);
		d2 = LOADINTELWORD(grphmem + addr + 2);
		addr = addr18(screen, addr + 4);

		switch (screen->framebuffer->dot & 0x13) {
		case 0:
			*b++ = (d >>  4) & 0x0f;
		case 1:
			*b++ = (d      ) & 0x0f;
		case 2:
			*b++ = (d >> 12) & 0x0f;
		case 3:
			*b++ = (d >>  8) & 0x0f;
		case 0x10:
			*b++ = (d2 >>  4) & 0x0f;
		case 0x11:
			*b++ = (d2      ) & 0x0f;
		case 0x12:
			*b++ = (d2 >> 12) & 0x0f;
		case 0x13:
			*b++ = (d2 >>  8) & 0x0f;
		}
		for (xp = 0; xp < 640/8; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			d = LOADINTELWORD(grphmem + addr);
			d2 = LOADINTELWORD(grphmem + addr + 2);
			addr = addr18(screen, addr + 4);

			*b++ = (d >>  4) & 0x0f;
			*b++ = (d      ) & 0x0f;
			*b++ = (d >> 12) & 0x0f;
			*b++ = (d >>  8) & 0x0f;

			*b++ = (d2 >>  4) & 0x0f;
			*b++ = (d2      ) & 0x0f;
			*b++ = (d2 >> 12) & 0x0f;
			*b++ = (d2 >>  8) & 0x0f;
		}
	}
	screen->lineaddr = addr18(screen, screen->lineaddr + screen->framebuffer->fbw);
	screen->wrappedaddr = addr18(screen, screen->wrappedaddr + screen->framebuffer->fbw);
}


// シングルプレーン8bit/pixel (直接色指定)
static void drawraster_s8c(SCREEN screen) {
	UINT16		xp;
	UINT16		wrapcount;
	UINT32		addr;
	WORD		*b;

	WORD		d, d2;

	addr = screen->lineaddr;
	b = (WORD *)screen->rasterbuf;
	if (screen->framebuffer->ofx == 0xffff) {
		// screen 1 (ラップアラウンドなし)
		wrapcount = 0;
	}
	else {
		wrapcount = screen->framebuffer->fbw - screen->framebuffer->ofx;
	}


	if (screen->r320dots) {
		// 320 dots
		d = LOADINTELWORD(grphmem + addr);
		d2 = LOADINTELWORD(grphmem + addr + 2);
		addr = addr18(screen, addr + 4);

		switch (screen->framebuffer->dot & 0x11) {
		case 0:
			b[0] = b[1] = rgb8to16[(d     ) & 0xff];
			b += 2;
		case 1:
			b[0] = b[1] = rgb8to16[(d >> 8) & 0xff];
			b += 2;
		case 0x10:
			b[0] = b[1] = rgb8to16[(d2     ) & 0xff];
			b += 2;
		case 0x11:
			b[0] = b[1] = rgb8to16[(d2 >> 8) & 0xff];
			b += 2;
		}
		for (xp = 0; xp < 320/4; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			d = LOADINTELWORD(grphmem + addr);
			d2 = LOADINTELWORD(grphmem + addr + 2);
			addr = addr18(screen, addr + 4);

			b[0] = b[1] = rgb8to16[(d     ) & 0xff];
			b += 2;
			b[0] = b[1] = rgb8to16[(d >> 8) & 0xff];
			b += 2;

			b[0] = b[1] = rgb8to16[(d2     ) & 0xff];
			b += 2;
			b[0] = b[1] = rgb8to16[(d2 >> 8) & 0xff];
			b += 2;
		}
	}
	else {
		// 640 dots

		d = LOADINTELWORD(grphmem + addr);
		d2 = LOADINTELWORD(grphmem + addr + 2);
		addr = addr18(screen, addr + 4);

		switch (screen->framebuffer->dot & 0x11) {
		case 0:
			*b++ = rgb8to16[(d     ) & 0xff];
		case 1:
			*b++ = rgb8to16[(d >> 8) & 0xff];
		case 0x10:
			*b++ = rgb8to16[(d2     ) & 0xff];
		case 0x11:
			*b++ = rgb8to16[(d2 >> 8) & 0xff];
		}
		for (xp = 0; xp < 640/4; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			d = LOADINTELWORD(grphmem + addr);
			d2 = LOADINTELWORD(grphmem + addr + 2);
			addr = addr18(screen, addr + 4);

			*b++ = rgb8to16[(d     ) & 0xff];
			*b++ = rgb8to16[(d >> 8) & 0xff];

			*b++ = rgb8to16[(d2     ) & 0xff];
			*b++ = rgb8to16[(d2 >> 8) & 0xff];
		}
	}
	screen->lineaddr = addr18(screen, screen->lineaddr + screen->framebuffer->fbw);
	screen->wrappedaddr = addr18(screen, screen->wrappedaddr + screen->framebuffer->fbw);
}

// シングルプレーン16bit/pixel (直接色指定)
static void drawraster_s16c(SCREEN screen) {
	UINT16		xp;
	UINT16		wrapcount;
	UINT32		addr;
	WORD		*b;

	WORD		d, d2;

	addr = screen->lineaddr;
	b = (WORD *)screen->rasterbuf;
	if (screen->framebuffer->ofx == 0xffff) {
		// screen 1 (ラップアラウンドなし)
		wrapcount = 0;
	}
	else {
		wrapcount = screen->framebuffer->fbw - screen->framebuffer->ofx;
	}


	if (screen->r320dots) {
		// 320 dots
		d = LOADINTELWORD(grphmem + addr);
		d2 = LOADINTELWORD(grphmem + addr + 2);
		addr = addr18(screen, addr + 4);

		switch (screen->framebuffer->dot & 0x10) {
		case 0:
			b[0] = b[1] = d;
			b += 2;
		case 0x10:
			b[0] = b[1] = d2;
			b += 2;
		}

		for (xp = 0; xp < 320/2; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			d = LOADINTELWORD(grphmem + addr);
			d2 = LOADINTELWORD(grphmem + addr + 2);
			addr = addr18(screen, addr + 4);

			b[0] = b[1] = d;
			b += 2;
			b[0] = b[1] = d2;
			b += 2;
		}
	}
	else {
		// 640 dots

		d = LOADINTELWORD(grphmem + addr);
		d2 = LOADINTELWORD(grphmem + addr + 2);
		addr = addr18(screen, addr + 4);

		switch (screen->framebuffer->dot & 0x10) {
		case 0:
			*b++ = d;
		case 0x10:
			*b++ = d2;
		}
		for (xp = 0; xp < 640/2; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			d = LOADINTELWORD(grphmem + addr);
			d2 = LOADINTELWORD(grphmem + addr + 2);
			addr = addr18(screen, addr + 4);

			*b++ = d;
			*b++ = d2;
		}
	}
	screen->lineaddr = addr18(screen, screen->lineaddr + screen->framebuffer->fbw);
	screen->wrappedaddr = addr18(screen, screen->wrappedaddr + screen->framebuffer->fbw);
}


static void drawraster(SCREEN screen) {

	if (!screen->r200lines || (work.screeny & 1) == 0) {
		if (screen->framebuffer != NULL) {
			if (screen->framebuffer->dsp + screen->framebuffer->dsh == screen->y) {
				screen->framebuffer = NULL;
			}
		}

		// 必要なら、次のフレームバッファに切り替える
		if (screen->framebuffer == NULL && screen->nextframebuffer >= 0) {
			if (videova.framebuffer[screen->nextframebuffer].dsp < screen->y) {
				// これより後のフレームバッファは表示しない
				screen->nextframebuffer = -1;
			}
			else if (videova.framebuffer[screen->nextframebuffer].dsp == screen->y) {
				selectframe(screen, screen->nextframebuffer);
			}
		}

		if (screen->rasterbuf) {
			if (screen->framebuffer == NULL) {
				UINT16		xp;
				// 何も表示しない
				if (screen->pixelmode & 0x04) {
					WORD		*b;
					b = (WORD *)screen->rasterbuf;
					for (xp = 0; xp < 640; xp++) *b++ = 0;
				}
				else {
					BYTE		*b;
					b = screen->rasterbuf;
					for (xp = 0; xp < 640; xp++) *b++ = 0;
				}
			}
			else {
				switch (screen->pixelmode) {
				case 0:
					break;
				case 1:
					drawraster_s4p(screen);
					break;
				case 2:
					break;

				case 6:
					drawraster_s8c(screen);
					break;
				case 7:
					drawraster_s16c(screen);
					break;
				}
			}
		}
		screen->y++;

	}
}
		
void makegrphva_initialize(void) {
	int i, r, g, b;

	for (i = 0; i < 256; i++) {
		b = i & 3;
		r = (i >> 2) & 7;
		g = (i >> 5) & 7;
		rgb8to16[i] = (g << 13) | (g == 0 ? 0 : 0x1c00) |
			(r << 7) | (r == 0 ? 0 : 0x0060) |
			(b << 3) | (b == 0 ? 0 : 0x0007);
	}
}

void makegrphva_begin(void) {
	WORD comp;
	int i;

	work.screeny = 0;

	work.screen[0].rasterbuf = NULL;
	work.screen[1].rasterbuf = NULL;
	work.screen[0].pixelmode = videova.grres & 0x0003;
	work.screen[1].pixelmode = (videova.grres >> 8) & 0x0003;

	comp = videova.colcomp;
	for (i = 0; i < 4; i++) {
		switch(comp & 0x000f) {
		case 0x0a:
			work.screen[0].rasterbuf = grph0p_raster;
			break;
		case 0x0b:
			work.screen[1].rasterbuf = grph1p_raster;
			break;
		}
		comp >>= 4;
	}

	comp = videova.rgbcomp;
	for (i = 0; i < 4; i++) {
		switch(comp & 0x000f) {
		case 0x08:
			work.screen[0].rasterbuf = (BYTE *)grph0c_raster;
			work.screen[0].pixelmode |= 0x04;
			break;
		case 0x09:
			work.screen[1].rasterbuf = (BYTE *)grph1c_raster;
			work.screen[1].pixelmode |= 0x04;
			break;
		}
		comp >>= 4;
	}

	//work.screen[0].rasterbuf = grph0p_raster;
	//work.screen[1].rasterbuf = grph1p_raster;
	work.screen[0].r320dots = videova.grres & 0x0010;
	work.screen[1].r320dots = videova.grres & 0x1000;
	work.screen[0].r200lines = videova.grmode & 0x0002;
	work.screen[1].r200lines = videova.grmode & 0x0002;
	if (videova.grmode & 0x0800)  {
		// 2画面モード
		work.screen[0].addrmask = 0x0001ffffL;
	}
	else {
		// 1画面モード
		work.screen[0].addrmask = 0x0003ffffL;
	}
	work.screen[0].addrofs  = 0x00000000L;
	work.screen[1].addrmask = 0x0001ffffL;
	work.screen[1].addrofs  = 0x00020000L;
	work.screen[0].nextframebuffer = 0;
	work.screen[1].nextframebuffer = 1;
	work.screen[0].y = 0;
	work.screen[1].y = 0;
	selectframe(&work.screen[0], -1);
	selectframe(&work.screen[1], -1);
}

void makegrphva_raster(void) {

	if (!(videova.grmode & 0x8000)) {
		// グラフィック表示禁止
		UINT16 xp;
		int i;

		for (i = 0; i < GRPHVA_SCREENS; i++) {
			if (work.screen[i].rasterbuf) {
				if (work.screen[i].pixelmode & 0x04) {
					for (xp = 0; xp < SURFACE_WIDTH; xp++) {
						((WORD *)work.screen[i].rasterbuf)[xp] = 0;
					}
				}
				else {
					for (xp = 0; xp < SURFACE_WIDTH; xp++) {
						work.screen[i].rasterbuf[xp] = 0;
					}
				}
			}
		}
	}
	else if (videova.grmode & 0x0400) {
		// シングルプレーンモード
		drawraster(&work.screen[0]);
		drawraster(&work.screen[1]);
		/*
		switch (videova.grres & 0x0003) {
		case 0:
			break;
		case 1:
			drawraster(&work.screen[0], videova.grres & 0x0003);
			break;
		case 2:
			break;
		case 3:
			break;
		}
		switch ((videova.grres >> 8) & 0x0003) {
		case 0:
			break;
		case 1:
			drawraster(&work.screen[1], (videova.grres >> 8) & 0x0003);
			break;
		case 2:
			break;
		case 3:
			break;
		}
		*/
	}
	else {
		// マルチプレーンモード
	}
	
	work.screeny++;
}
