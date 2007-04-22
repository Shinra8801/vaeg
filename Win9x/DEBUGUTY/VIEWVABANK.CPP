#include	"compiler.h"
#include	"resource.h"
#include	"np2.h"
#include	"debugsub.h"
#include	"viewer.h"
#include	"viewcmn.h"
#include	"viewmenu.h"
#include	"viewmem.h"
#include	"viewvabank.h"
#include	"memoryva.h"
#include	"bmsio.h"
#include	"va91.h"

#if defined(SUPPORT_PC88VA)

typedef struct {
	BYTE	rom0_bank;
	BYTE	rom1_bank;
	BYTE	sysm_bank;
	BYTE	bms_bank;
	BYTE	va91rom0_bank;
	BYTE	va91rom1_bank;
	BYTE	va91sysm_bank;
} _BANKSTAT, *BANKSTAT;

static void viewvabank_paint(NP2VIEW_T *view, RECT *rc, HDC hdc) {

	LONG		y;
	DWORD		pos;
	char		str[128];
	HFONT		hfont;
	BANKSTAT	r;
	_BANKSTAT	_bankstat;

	hfont = CreateFont(16, 0, 0, 0, 0, 0, 0, 0, 
					SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
					DEFAULT_QUALITY, FIXED_PITCH, "‚l‚r ƒSƒVƒbƒN");
	SetTextColor(hdc, 0xffffff);
	SetBkColor(hdc, 0x400000);
	hfont = (HFONT)SelectObject(hdc, hfont);

	_bankstat.rom0_bank = memoryva.rom0_bank;
	_bankstat.rom1_bank = memoryva.rom1_bank;
	_bankstat.sysm_bank = memoryva.sysm_bank;
//	_bankstat.bms_bank = bmsio.bank;
	_bankstat.va91rom0_bank = va91.rom0_bank;
	_bankstat.va91rom1_bank = va91.rom1_bank;
	_bankstat.va91sysm_bank = va91.sysm_bank;

	if (view->lock) {
		if (view->buf1.type != ALLOCTYPE_VABANK) {
			if (viewcmn_alloc(&view->buf1, sizeof(_bankstat))) {
				view->lock = FALSE;
				viewmenu_lock(view);
			}
			else {
				view->buf1.type = ALLOCTYPE_VABANK;
				CopyMemory(view->buf1.ptr, &_bankstat, sizeof(_bankstat));
			}
			viewcmn_putcaption(view);
		}
	}

	pos = view->pos;

	if (view->lock) {
		r = (BANKSTAT)view->buf1.ptr;
	}
	else {
		r = &_bankstat;
	}

	for (y=0; y<rc->bottom && pos<6; y+=16, pos++) {
		switch(pos) {
			case 0:
				wsprintf(str, "System memory bank      = %.2x",
								r->sysm_bank);
				break;
			case 1:
				wsprintf(str, "ROM0 bank               = %.2x",
								r->rom0_bank);
				break;
			case 2:
				wsprintf(str, "ROM1 bank               = %.2x",
								r->rom1_bank);
				break;
/*
			case 3:
				wsprintf(str, "BMS(ECh) bank           = %.2x",
								r->bms_bank);
				break;
*/
			case 3:
				wsprintf(str, "VA91 System memory bank = %.2x",
								r->va91sysm_bank);
				break;
			case 4:
				wsprintf(str, "VA91 ROM0 bank          = %.2x",
								r->va91rom0_bank);
				break;
			case 5:
				wsprintf(str, "VA91 ROM1 bank          = %.2x",
								r->va91rom1_bank);
				break;
		}
		TextOut(hdc, 0, y, str, strlen(str));
	}
	DeleteObject(SelectObject(hdc, hfont));
}

LRESULT CALLBACK viewvabank_proc(NP2VIEW_T *view,
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
			viewcmn_paint(view, 0x400000, viewvabank_paint);
			break;

	}
	return(0L);
}


// ---------------------------------------------------------------------------

void viewvabank_init(NP2VIEW_T *dst, NP2VIEW_T *src) {

	dst->type = VIEWMODE_VABANK;
	dst->maxline = 6;
	dst->mul = 1;
	dst->pos = 0;
}

#endif
