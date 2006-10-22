#include	"compiler.h"
#include	"resource.h"
#include	"np2.h"
#include	"debugsub.h"
#include	"viewer.h"
#include	"viewcmn.h"
#include	"viewmenu.h"
//#include	"viewmem.h"
#include	"viewgactrlva.h"
#include	"gactrlva.h"
//#include	"../vramva/scrndrawva.h"

#if defined(SUPPORT_PC88VA)

typedef struct {
	_GACTRLVA	gactrlva;
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

static const char *utoa_4bit(UINT v) {
	static char str[5];
	int i;

	str[4] = '\0';
	for (i = 0; i < 4; i++) {
		str[3 - i] = (v & 1) ? '1' : '0'; 
		v >>= 1;
	}

	return str;
}

static void viewgactrlva_paint(NP2VIEW_T *view, RECT *rc, HDC hdc) {
	static char *mes_rbusy[2]={"Enabled", "Disabled"};
	static char *mes_cmpen[2]={"Disabled", "Enabled"};
	static char *mes_wss[4]={"LU output", "Pattern register", "CPU data", "No operation"};
	static char *mes_pmod2[2]={" 8 bit", "16 bit"};
	static char *mes_pmod[4]={"Fixed", "Update on read", "Update on write", "Update on read/write"};

	LONG		y;
	int			lh;
	DWORD		pos;
	HFONT		hfont;
	STAT		r;
	_STAT		_stat;
	WORD		dw;
	_VIEWSCRN	vs;

	hfont = CreateFont(16, 0, 0, 0, 0, 0, 0, 0, 
					SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
					DEFAULT_QUALITY, FIXED_PITCH, "‚l‚r ƒSƒVƒbƒN");
	SetTextColor(hdc, 0xffffff);
	SetBkColor(hdc, 0x400000);
	hfont = (HFONT)SelectObject(hdc, hfont);

	_stat.gactrlva = gactrlva;

	if (view->lock) {
		if (view->buf1.type != ALLOCTYPE_GACTRLVA) {
			if (viewcmn_alloc(&view->buf1, sizeof(_stat))) {
				view->lock = FALSE;
				viewmenu_lock(view);
			}
			else {
				view->buf1.type = ALLOCTYPE_GACTRLVA;
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

	dw = r->gactrlva.gmsp ? 1 : 0;
	println(&vs, "GMSP (152h bit12) = %d   (%s)", dw, dw ? "Single plane" : "Multi plane");
	vs.y++;
	println(&vs, "For Multi Plane");
	println(&vs, "AACC (510h)       = %.2xh (%s)",
		r->gactrlva.m.accessmode,
		r->gactrlva.m.accessmode ? "Advanced" : "Normal");
	println(&vs, "GMAP (512h)       = %.2xh (%s)",
		r->gactrlva.m.accessblock,
		r->gactrlva.m.accessblock ? "Block 1" : "Block 0");
	println(&vs, "XRPM (514h)       = %.2xh (%s: 1..Mask)", 
		r->gactrlva.m.readplane, 
		utoa_4bit(r->gactrlva.m.readplane));
	println(&vs, "XWPM (516h)       = %.2xh (%s: 1..Mask)", 
		r->gactrlva.m.writeplane,
		utoa_4bit(r->gactrlva.m.writeplane));
	dw = r->gactrlva.m.advancedaccessmode;
	println(&vs, "Mode (518h)       = %.2xh", dw);
	println(&vs, "    Read register      : %s", mes_rbusy[(dw >> 7) & 1]);
	println(&vs, "    Comparison register: %s", mes_cmpen[(dw >> 5) & 1]);
	println(&vs, "    Source to write    : %s", mes_wss[(dw >> 3) & 3]);
	println(&vs, "    Pattern register   : %s, %s", mes_pmod2[(dw >> 2) & 1],
												    mes_pmod[dw & 3]);
	println(&vs, "PRRP (550h)       = %.2xh (%s: 1..High, 0..Low)", 
		r->gactrlva.m.patternreadpointer,
		utoa_4bit(r->gactrlva.m.patternreadpointer));
	println(&vs, "PRWP (552h)       = %.2xh (%s: 1..High, 0..Low)",
		r->gactrlva.m.patternwritepointer,
		utoa_4bit(r->gactrlva.m.patternwritepointer));
	println(&vs, "CMPR (520h-526h)  = %.2xh, %.2xh, %.2xh, %.2xh",
		r->gactrlva.m.cmpdata[0],
		r->gactrlva.m.cmpdata[1],
		r->gactrlva.m.cmpdata[2],
		r->gactrlva.m.cmpdata[3]);
	println(&vs, "PATRL(530h-536h)  = %.2xh, %.2xh, %.2xh, %.2xh",
		r->gactrlva.m.pattern[0][0],
		r->gactrlva.m.pattern[1][0],
		r->gactrlva.m.pattern[2][0],
		r->gactrlva.m.pattern[3][0]);
	println(&vs, "PATRH(540h-546h)  = %.2xh, %.2xh, %.2xh, %.2xh",
		r->gactrlva.m.pattern[0][1],
		r->gactrlva.m.pattern[1][1],
		r->gactrlva.m.pattern[2][1],
		r->gactrlva.m.pattern[3][1]);
	println(&vs, "ROP  (560h-566h)  = %.2xh, %.2xh, %.2xh, %.2xh",
		r->gactrlva.m.rop[0],
		r->gactrlva.m.rop[1],
		r->gactrlva.m.rop[2],
		r->gactrlva.m.rop[3]);
	vs.y++;

	println(&vs, "For Single Plane");
	dw = r->gactrlva.s.writemode;
	println(&vs, "Mode (580h)       = %.2xh", dw);
	println(&vs, "    Read register      : %s", mes_rbusy[(dw >> 7) & 1]);
	println(&vs, "    Source to write    : %s", mes_wss[(dw >> 3) & 3]);
	println(&vs, "PATR (590h-592h)  = %.4xh, %.4xh",
		r->gactrlva.s.pattern[0],
		r->gactrlva.s.pattern[1]);
	println(&vs, "ROP  (5a0h-5a2h)  = %.2xh, %.2xh",
		r->gactrlva.s.rop[0],
		r->gactrlva.s.rop[1]);

	DeleteObject(SelectObject(hdc, hfont));
}

LRESULT CALLBACK viewgactrlva_proc(NP2VIEW_T *view,
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
			viewcmn_paint(view, 0x400000, viewgactrlva_paint);
			break;

	}
	return(0L);
}


// ---------------------------------------------------------------------------

void viewgactrlva_init(NP2VIEW_T *dst, NP2VIEW_T *src) {

	dst->type = VIEWMODE_GACTRLVA;
	dst->maxline = 24;
	dst->mul = 1;
	dst->pos = 0;
}

#endif
