#include	"compiler.h"
#include	"resource.h"
#include	"np2.h"
#include	"viewer.h"
#include	"viewcmn.h"
#include	"viewmenu.h"
#include	"viewmem.h"
#include	"viewsubmem.h"
#include	"subsystem.h"

#if defined(SUPPORT_PC88VA)


static void viewsubmem_paint(NP2VIEW_T *view, RECT *rc, HDC hdc) {

	int		x;
	LONG	y;
	DWORD	off;
	BYTE	*p;
	BYTE	buf[16];
	char	str[16];
	char	str2[2];
	HFONT	hfont;

	hfont = CreateFont(16, 0, 0, 0, 0, 0, 0, 0, 
					SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
					DEFAULT_QUALITY, FIXED_PITCH, "‚l‚rƒSƒVƒbƒN");
	SetTextColor(hdc, 0xffffff);
	SetBkColor(hdc, 0x400000);
	hfont = (HFONT)SelectObject(hdc, hfont);

	if (view->lock) {
		if (view->buf1.type != ALLOCTYPE_SUBMEM) {
			if (viewcmn_alloc(&view->buf1, 0x10000)) {
				view->lock = FALSE;
				viewmenu_lock(view);
			}
			else {
				view->buf1.type = ALLOCTYPE_SUBMEM;
				for (x=0; x<0x10000; x++) {
					((BYTE *)view->buf1.ptr)[x] = subsystem_readmem(x);
				}
			}
			viewcmn_putcaption(view);
		}
	}

	off = (view->pos) << 4;
	for (y=0; y<rc->bottom && off<0x10000; y+=16, off+=16) {
		wsprintf(str, "    %04x", off);
		TextOut(hdc, 0, y, str, 8);
		if (view->lock) {
			p = (BYTE *)view->buf1.ptr;
			p += off;
		}
		else {
			p = buf;
			for (x=0; x<16; x++) {
				buf[x] = subsystem_readmem((WORD)off + x);
			}
		}
		for (x=0; x<16; x++) {
			str[0] = viewcmn_hex[*p >> 4];
			str[1] = viewcmn_hex[*p & 15];
			str[2] = 0;
			if (*p < 0x20 || *p >= 0x80) {
				str2[0] = '.';
			}
			else {
				str2[0] = *p;
			}
			str2[1] = 0;
			p++;
			TextOut(hdc, (10 + x*3 + (x >= 8 ? 2 : 0))*8, y, str, 2);
			TextOut(hdc, (10 + 16*3 + 2 + 1 + x)*8, y, str2, 1);
		}
		TextOut(hdc, (10 + 8*3)*8, y, "-", 1);
	}

	DeleteObject(SelectObject(hdc, hfont));
}


LRESULT CALLBACK viewsubmem_proc(NP2VIEW_T *view,
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
			viewcmn_paint(view, 0x400000, viewsubmem_paint);
	}
	return(0L);
}


// ---------------------------------------------------------------------------

void viewsubmem_init(NP2VIEW_T *dst, NP2VIEW_T *src) {

	dst->pos = 0;
	dst->type = VIEWMODE_SUBMEM;
	dst->maxline = 0x1000;
	dst->mul = 2;
}

#endif