#include	"compiler.h"
#include	"resource.h"
#include	"np2.h"
#include	"viewer.h"
#include	"viewcmn.h"
#include	"viewmenu.h"
#include	"viewmem.h"
#include	"viewsubasm.h"

#include	"cpucva/Z80.h"
#include	"subsystem.h"

#if defined(SUPPORT_PC88VA)

static void viewsubasm_paint(NP2VIEW_T *view, RECT *rc, HDC hdc) {

	LONG		y;
	WORD		off;
	char		str[32];
	HFONT		hfont;

	hfont = CreateFont(16, 0, 0, 0, 0, 0, 0, 0, 
					SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
					DEFAULT_QUALITY, FIXED_PITCH, "‚l‚rƒSƒVƒbƒN");
	SetTextColor(hdc, 0xffffff);
	SetBkColor(hdc, 0x400000);
	hfont = (HFONT)SelectObject(hdc, hfont);

	off = view->off;
	if (view->pos) {
		unsigned int i;
		for (i = 0; i < view->pos; i++) {
			off = subsystem_disassemble(off, str);
		}
	}


	for (y=0; y<rc->bottom; y+=16) {
		wsprintf(str, "     %04x", off);
		TextOut(hdc, 0, y, str, 9);
		off &= 0xffff;

		off = subsystem_disassemble(off, str);
		TextOut(hdc, 11 * 8, y, str, strlen(str));
	}

	DeleteObject(SelectObject(hdc, hfont));
}


LRESULT CALLBACK viewsubasm_proc(NP2VIEW_T *view,
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
#if EXT
				case IDM_SETSEG:
					view = viewcmn_find(hwnd);
					if (view) {
						int	ret;
						ret = DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_VIEW_ADDRESS),
									hwnd, (DLGPROC)ViewerAddressDialogProc, (long)view);
						if (ret == IDOK) {
							InvalidateRect(hwnd, NULL, TRUE);
						}
					}
					break;
#endif
			}
			break;

		case WM_PAINT:
			viewcmn_paint(view, 0x400000, viewsubasm_paint);
	}
	return(0L);
}


// ---------------------------------------------------------------------------

void viewsubasm_init(NP2VIEW_T *dst, NP2VIEW_T *src) {

	if (src) {
		switch(src->type) {
			case VIEWMODE_SUBMEM:
				dst->off = (WORD)(dst->pos << 4);
				break;

			case VIEWMODE_SUBASM:
				dst->off = src->off;
				break;

			default:
				src = NULL;
				break;
		}
	}
	if (!src) {
		dst->off = subsystem_getcpureg()->pc;
	}
	dst->type = VIEWMODE_SUBASM;
	dst->maxline = 256;
	dst->mul = 1;
	dst->pos = 0;
}

#endif

