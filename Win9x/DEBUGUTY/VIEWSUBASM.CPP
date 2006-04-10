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
	DWORD		seg4;
	WORD		off;
	DWORD		pos;
	BYTE		*p;
	BYTE		buf[16];
	char		str[32];
	HFONT		hfont;
	BOOL		opsize;
	//_UNASM		una;
	int			step;

	hfont = CreateFont(16, 0, 0, 0, 0, 0, 0, 0, 
					SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
					DEFAULT_QUALITY, FIXED_PITCH, "‚l‚rƒSƒVƒbƒN");
	SetTextColor(hdc, 0xffffff);
	SetBkColor(hdc, 0x400000);
	hfont = (HFONT)SelectObject(hdc, hfont);
/*
	if (view->lock) {
		if ((view->buf1.type != ALLOCTYPE_SEG) ||
			(view->buf1.arg != view->seg)) {
			if (viewcmn_alloc(&view->buf1, 0x10000)) {
				view->lock = FALSE;
				viewmenu_lock(view);
			}
			else {
				view->buf1.type = ALLOCTYPE_SEG;
				view->buf1.arg = view->seg;
				viewmem_read(&view->dmem, view->buf1.arg << 4,
											(BYTE *)view->buf1.ptr, 0x10000);
				view->buf2.type = ALLOCTYPE_NONE;
			}
			viewcmn_putcaption(view);
		}
	}
*/

/*
	seg4 = view->seg << 4;
	pos = view->pos;
	if (view->pos) {
		if ((view->buf2.type != ALLOCTYPE_ASM) ||
			(view->buf2.arg != (seg4 + view->off))) {
			if (viewcmn_alloc(&view->buf2, 256*2)) {
				pos = 0;
			}
			else {
				int i;
				WORD *r;
				r = (WORD *)view->buf2.ptr;
				view->buf2.type = ALLOCTYPE_ASM;
				view->buf2.arg = seg4 + view->off;
				off = view->off;
				for (i=0; i<255; i++) {
					off &= 0xffff;
					*r++ = off;
					if (view->lock) {
						p = (BYTE *)view->buf1.ptr;
						p += off;
						if (off > 0xfff0) {
							DWORD pos = 0x10000 - off;
							CopyMemory(buf, p, pos);
							CopyMemory(buf + pos, view->buf1.ptr, 16 - pos);
							p = buf;
						}
					}
					else {
						p = buf;
						viewmem_read(&(view->dmem), seg4 + off, buf, 16);
					}
					step = unasm(NULL, p, 16, FALSE, off);
					off += (WORD)step;
				}
				*r = off;
			}
		}
	}

	if ((pos) && (pos < 256)) {
		off = *(((WORD *)view->buf2.ptr) + pos);
	}
	else {
		off = view->off;
	}
*/
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
/*
		if (view->lock) {
			p = (BYTE *)view->buf1.ptr;
			p += off;
			if (off > 0xfff0) {
				DWORD pos = 0x10000 - off;
				CopyMemory(buf, p, pos);
				CopyMemory(buf + pos, view->buf1.ptr, 16 - pos);
				p = buf;
			}
		}
		else {
*/
			//p = buf;
			//viewmem_read(&(view->dmem), seg4 + off, buf, 16);
/*
		}
*/
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
		dst->off = subsystem_getcpuregs()->pc;
	}
	dst->type = VIEWMODE_SUBASM;
	dst->maxline = 256;
	dst->mul = 1;
	dst->pos = 0;
}

#endif

