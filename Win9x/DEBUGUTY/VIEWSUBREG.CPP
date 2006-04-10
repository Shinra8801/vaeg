#include	"compiler.h"
#include	"resource.h"
#include	"np2.h"
#include	"debugsub.h"
#include	"viewer.h"
#include	"viewcmn.h"
#include	"viewmenu.h"
#include	"viewmem.h"
#include	"viewsubreg.h"
#include	"subsystem.h"
#include	"cpucva/Z80.h"

#if defined(SUPPORT_PC88VA)

#define V16(x) ((x) & 0xffff)

static void viewsubreg_paint(NP2VIEW_T *view, RECT *rc, HDC hdc) {

	LONG		y;
	DWORD		pos;
	char		str[128];
	HFONT		hfont;
	struct Z80Reg	*r;

	hfont = CreateFont(16, 0, 0, 0, 0, 0, 0, 0, 
					SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
					DEFAULT_QUALITY, FIXED_PITCH, "‚l‚r ƒSƒVƒbƒN");
	SetTextColor(hdc, 0xffffff);
	SetBkColor(hdc, 0x400000);
	hfont = (HFONT)SelectObject(hdc, hfont);
/*
	if (view->lock) {
		if (view->buf1.type != ALLOCTYPE_REG) {
			if (viewcmn_alloc(&view->buf1, sizeof(i286core.s))) {
				view->lock = FALSE;
				viewmenu_lock(view);
			}
			else {
				view->buf1.type = ALLOCTYPE_REG;
				CopyMemory(view->buf1.ptr, &i286core.s, sizeof(i286core.s));
			}
			viewcmn_putcaption(view);
		}
	}
*/
	pos = view->pos;
	if (view->lock) {
//		r = (I286STAT *)view->buf1.ptr;
	}
	else {
		r = subsystem_getcpuregs();
	}

	for (y=0; y<rc->bottom && pos<3; y+=16, pos++) {
		switch(pos) {
			case 0:
				wsprintf(str, "AF=%.4x  DE=%.4x  BC=%.4x  HL=%.4x",
							V16(r->r.w.af), V16(r->r.w.de), 
							V16(r->r.w.bc), V16(r->r.w.hl));
				break;

			case 1:
				wsprintf(str, "SP=%.4x           IX=%.4x  IY=%.4x",
							V16(r->r.w.sp), 
							V16(r->r.w.ix), V16(r->r.w.iy));
				break;

			case 2:
				wsprintf(str, "PC=%.4x  IM=%.1x   %s",
							V16(r->pc), r->intmode, " "/*debugsub_flags(r->r.w.flag)*/);
				break;
		}
		TextOut(hdc, 0, y, str, strlen(str));
	}
	DeleteObject(SelectObject(hdc, hfont));
}

LRESULT CALLBACK viewsubreg_proc(NP2VIEW_T *view,
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
			viewcmn_paint(view, 0x400000, viewsubreg_paint);
			break;

	}
	return(0L);
}


// ---------------------------------------------------------------------------

void viewsubreg_init(NP2VIEW_T *dst, NP2VIEW_T *src) {

	dst->type = VIEWMODE_SUBREG;
	dst->maxline = 3;
	dst->mul = 1;
	dst->pos = 0;
}

#endif