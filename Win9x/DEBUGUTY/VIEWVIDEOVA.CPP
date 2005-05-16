#include	"compiler.h"
#include	"resource.h"
#include	"np2.h"
#include	"debugsub.h"
#include	"viewer.h"
#include	"viewcmn.h"
#include	"viewmenu.h"
#include	"viewmem.h"
#include	"viewvideova.h"
#include	"videova.h"
#include	"../vramva/scrndrawva.h"

#if defined(SUPPORT_PC88VA)

typedef struct {
	_VIDEOVA	videova;
} _STAT, *STAT;

typedef struct {
	int		y;
	int		lineheight;
	HDC		hdc;
} _VIEWSCRN, *VIEWSCRN;

static void println(VIEWSCRN viewscrn, const char *format, ...) {
	char	str[256];
	va_list	args;

	va_start(args, format);

	wvsprintf(str, format, args);
	TextOut(viewscrn->hdc, 0, viewscrn->y * viewscrn->lineheight, str, strlen(str));
	viewscrn->y++;
}

static void drawcolorsample(HDC hdc, int x, int y, WORD color) {
	RGB32	rgb;
	RECT	rect;
	HBRUSH	hbr;

	rect.top  = y;
	rect.bottom = y + 15;
	rect.left = x;
	rect.right = x + 15;
	rgb = scrndrawva_drawcolor32(color);
	hbr = CreateSolidBrush(RGB(rgb.p.r, rgb.p.g, rgb.p.b));
	FillRect(hdc, &rect, hbr);
	DeleteObject(hbr);
}

static void viewvideova_paint(NP2VIEW_T *view, RECT *rc, HDC hdc) {
	static char	*mes_lines[4] = {"400", "408", "200", "204"};
	static char	*mes_rsm[4] = {"non-interlace 0", "non-interlace 1", 
									"interlace 0", "interlace 1"};
	static char	*mes_bpp[4] = {"1", "4", "8", "16"};
	static char	*mes_palmode[4] = {
							"use palette-set 0", "use plaette-set 1",
							"use mixed-mode", "use 32 palettes"};
	static char	*mes_scrn[4] = {"text", "sprite", "graphics 0", "graphics 1"};
	static char	*mes_gmp[4] = {"0-1", "1-2", "2-3", "3-4"};
	static char	*mes_mkm[4] = {"do not mask", "mask the next screen", 
							   "mask upper screens", "mask lower screens"};

	int			i;
	LONG		y;
	int			lh;
	DWORD		pos;
	HFONT		hfont;
	STAT		r;
	_STAT		_stat;
	FRAMEBUFFER	f;
	WORD		dw;
	_VIEWSCRN	vs;

	hfont = CreateFont(16, 0, 0, 0, 0, 0, 0, 0, 
					SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
					DEFAULT_QUALITY, FIXED_PITCH, "‚l‚r ƒSƒVƒbƒN");
	SetTextColor(hdc, 0xffffff);
	SetBkColor(hdc, 0x400000);
	hfont = (HFONT)SelectObject(hdc, hfont);

	_stat.videova = videova;

	if (view->lock) {
		if (view->buf1.type != ALLOCTYPE_VIDEOVA) {
			if (viewcmn_alloc(&view->buf1, sizeof(_stat))) {
				view->lock = FALSE;
				viewmenu_lock(view);
			}
			else {
				view->buf1.type = ALLOCTYPE_VIDEOVA;
				CopyMemory(view->buf1.ptr, &_stat, sizeof(_stat));
			}
			viewcmn_putcaption(view);
		}
	}

	pos = view->pos;

	if (view->lock) {
		r = (STAT)view->buf1.ptr;
	}
	else {
		r = &_stat;
	}

	y = - (LONG)view->pos;
	lh = 16;
	vs.lineheight = 16;
	vs.y = - (LONG)view->pos;
	vs.hdc = hdc;

	dw = r->videova.grmode;
	println(&vs, "GrMode  (100h) = %.4xh", dw);
	println(&vs, "  %s lines, %s, %s, %s",
		mes_lines[dw & 0x0003],
		mes_rsm[(dw >> 6) & 0x0003],
		(dw & 0x0400) ? "Single plane" : "Multi plane",
		(dw & 0x0800) ? "2 screens" : "1 screen");

	dw = r->videova.grres;
	println(&vs, "GrRes   (102h) = %.4xh", dw);
	println(&vs, "  Screen 0: %d dots, %s bit/pixel",
		(dw & 0x0010) ? 320 : 640,
		mes_bpp[dw & 3]);
	println(&vs, "  Screen 1: %d dots, %s bit/pixel",
		(dw & 0x1000) ? 320 : 640,
		mes_bpp[(dw >> 8) & 3]);

	println(&vs, "ColComp (106h) = %.4xh", r->videova.colcomp);
	println(&vs, "RGBComp (108h) = %.4xh", r->videova.rgbcomp);

	dw = r->videova.palmode;
	println(&vs, "PalMode (10ch) = %.4xh", dw);
	println(&vs, "  %s, assign palette-set 1 to %s (in mixed-mode)",
		mes_palmode[(dw >> 6) & 3],
		mes_scrn[(dw >> 4) & 3]);

	dw = r->videova.mskmode;
	println(&vs, "MskMode (10ah) = %.4xh", dw);
	println(&vs, "  position %s, %s (inside), %s (outside)" ,
				mes_gmp[(dw >> 4) & 3], mes_mkm[dw & 3], mes_mkm[(dw >> 2) & 3]);

	println(&vs, "MskArea (130h, 134h, 132h, 136h) = (%4d,%4d)-(%4d,%4d)",
				r->videova.mskleft, r->videova.msktop,
				r->videova.mskrit, r->videova.mskbot);

	vs.y++;
	f = r->videova.framebuffer;
	for (i = 0; i < VIDEOVA_FRAMEBUFFERS; i++, f++) {

		println(&vs, "Frame buffer %d", i);
		println(&vs, "  FSA = %.8lxh", f->fsa);
		println(&vs, "  FBW = %.4xh (%d)", f->fbw, f->fbw);
		println(&vs, "  FBL = %.4xh (%d)", f->fbl, f->fbl);
		println(&vs, "  DOT = %.4xh (%d)", f->dot, f->dot);
		println(&vs, "  OFX = %.4xh (%d)", f->ofx, f->ofx);
		println(&vs, "  OFY = %.4xh (%d)", f->ofy, f->ofy);
		println(&vs, "  DSA = %.8lxh", f->dsa);
		println(&vs, "  DSH = %.4xh (%d)", f->dsh, f->dsh);
		println(&vs, "  DSP = %.4xh (%d)", f->dsp, f->dsp);
		vs.y++;
	}

	println(&vs, "Palettes");
	for (i = 0; i < VIDEOVA_PALETTES/2; i++) {
//		RGB32	rgb;
//		RECT	rect;
//		HBRUSH	hbr;
		int		coly;

		coly = vs.y;
//		rect.top  = vs.y * vs.lineheight;
//		rect.bottom = rect.top + 15;
		println(&vs, "  #%02d = %.4xh     #%02d = %.4xh", 
			i, r->videova.palette[i],
			i + VIDEOVA_PALETTES/2, r->videova.palette[i + VIDEOVA_PALETTES/2]);

		drawcolorsample(hdc, 14 * 8, coly * vs.lineheight, r->videova.palette[i]);
		drawcolorsample(hdc, 14 * 8 + 15 * 8, coly * vs.lineheight, r->videova.palette[i + VIDEOVA_PALETTES/2]);
/*
		rect.left = 14 * 8;
		rect.right = rect.left + 15;
		rgb = scrndrawva_drawcolor32(r->videova.palette[i]);
		hbr = CreateSolidBrush(RGB(rgb.p.r, rgb.p.g, rgb.p.b));
		FillRect(hdc, &rect, hbr);
		DeleteObject(hbr);

		rect.left += 15 * 8;
		rect.right = rect.left + 15;
		rgb = scrndrawva_drawcolor32(r->videova.palette[i + VIDEOVA_PALETTES/2]);
		hbr = CreateSolidBrush(RGB(rgb.p.r, rgb.p.g, rgb.p.b));
		FillRect(hdc, &rect, hbr);
		DeleteObject(hbr);
*/
	}
	println(&vs, "  drop= %.4xh", r->videova.dropcol); 
	drawcolorsample(hdc, 14 * 8, (vs.y - 1) * vs.lineheight, r->videova.dropcol);

	DeleteObject(SelectObject(hdc, hfont));
}

LRESULT CALLBACK viewvideova_proc(NP2VIEW_T *view,
								HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {

	switch (msg) {

		case WM_COMMAND:
			switch(LOWORD(wp)) {
				case IDM_VIEWMODELOCK:
					view->lock ^= 1;
					viewmenu_lock(view);
					viewcmn_putcaption(view);
					InvalidateRect(hwnd, NULL, TRUE);
					break;
			}
			break;

		case WM_PAINT:
			viewcmn_paint(view, 0x400000, viewvideova_paint);
			break;

	}
	return(0L);
}


// ---------------------------------------------------------------------------

void viewvideova_init(NP2VIEW_T *dst, NP2VIEW_T *src) {

	dst->type = VIEWMODE_VIDEOVA;
	dst->maxline = 11 * VIDEOVA_FRAMEBUFFERS + 14 + 18;
	dst->mul = 1;
	dst->pos = 0;
}

#endif
