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

#if defined(SUPPORT_PC88VA)

typedef struct {
	_VIDEOVA	videova;
} _STAT, *STAT;

static void viewvideova_paint(NP2VIEW_T *view, RECT *rc, HDC hdc) {
	static		char *mes_lines[4] = {"400", "408", "200", "204"};
	static		char *mes_rsm[4] = {"non-interlace 0", "non-interlace 1", 
									"interlace 0", "interlace 1"};
	static		char *mes_bpp[4] = {"1", "4", "8", "16"};
	int			i;
	LONG		y;
	int			lh;
	DWORD		pos;
	char		str[128];
	HFONT		hfont;
	STAT		r;
	_STAT		_stat;
	FRAMEBUFFER	f;
	WORD		dw;

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

	dw = r->videova.grmode;
	wsprintf(str, "GrMode (100h) = %.4xh", dw);
	TextOut(hdc, 0, y * lh, str, strlen(str));
	y++;

	wsprintf(str, "  %s lines, %s, %s, %s",
		mes_lines[dw & 0x0003],
		mes_rsm[(dw >> 6) & 0x0003],
		(dw & 0x0400) ? "Single plane" : "Multi plane",
		(dw & 0x0800) ? "2 screens" : "1 screen");
	TextOut(hdc, 0, y * lh, str, strlen(str));
	y++;

	wsprintf(str, "GrRes  (102h) = %.4xh", r->videova.grres);
	TextOut(hdc, 0, y * lh, str, strlen(str));
	y++;

	dw = r->videova.grres;
	wsprintf(str, "  Screen 0: %d dots %s bit/pixel",
		(dw & 0x0010) ? 320 : 640,
		mes_bpp[dw & 3]);
	TextOut(hdc, 0, y * lh, str, strlen(str));
	y++;

	wsprintf(str, "  Screen 1: %d dots %s bit/pixel",
		(dw & 0x1000) ? 320 : 640,
		mes_bpp[(dw >> 8) & 3]);
	TextOut(hdc, 0, y * lh, str, strlen(str));
	y++;

	y++;
	f = r->videova.framebuffer;
	for (i = 0; i < VIDEOVA_FRAMEBUFFERS; i++, f++) {

		wsprintf(str, "Frame buffer %d", i);
		TextOut(hdc, 0, y * lh, str, strlen(str));
		y++;

		wsprintf(str, "  FSA = %.8lxh", f->fsa);
		TextOut(hdc, 0, y * lh, str, strlen(str));
		y++;

		wsprintf(str, "  FBW = %.4xh (%d)", f->fbw, f->fbw);
		TextOut(hdc, 0, y * lh, str, strlen(str));
		y++;

		wsprintf(str, "  FBL = %.4xh (%d)", f->fbl, f->fbl);
		TextOut(hdc, 0, y * lh, str, strlen(str));
		y++;

		wsprintf(str, "  DOT = %.4xh (%d)", f->dot, f->dot);
		TextOut(hdc, 0, y * lh, str, strlen(str));
		y++;

		wsprintf(str, "  OFX = %.4xh (%d)", f->ofx, f->ofx);
		TextOut(hdc, 0, y * lh, str, strlen(str));
		y++;

		wsprintf(str, "  OFY = %.4xh (%d)", f->ofy, f->ofy);
		TextOut(hdc, 0, y * lh, str, strlen(str));
		y++;

		wsprintf(str, "  DSA = %.8lxh", f->dsa);
		TextOut(hdc, 0, y * lh, str, strlen(str));
		y++;

		wsprintf(str, "  DSH = %.4xh (%d)", f->dsh, f->dsh);
		TextOut(hdc, 0, y * lh, str, strlen(str));
		y++;

		wsprintf(str, "  DSP = %.4xh (%d)", f->dsp, f->dsp);
		TextOut(hdc, 0, y * lh, str, strlen(str));
		y++;

		y++;

	}

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
	dst->maxline = 11 * VIDEOVA_FRAMEBUFFERS + 6;
	dst->mul = 1;
	dst->pos = 0;
}

#endif
