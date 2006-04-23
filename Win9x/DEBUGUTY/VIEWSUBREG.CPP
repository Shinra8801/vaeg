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

static const char s_pl[] = "P";
static const char s_ng[] = "M";
static const char s_nz[] = "-";
static const char s_zr[] = "Z";
static const char s_nh[] = "-";
static const char s_hc[] = "H";
static const char s_po[] = "O";
static const char s_pe[] = "E";
static const char s_ns[] = "-";
static const char s_su[] = "N";
static const char s_nc[] = "-";
static const char s_cy[] = "C";
static const char s_xx[] = "-";

static const char *flagstr[8][2] = {
				{s_pl, s_ng},		// 0x0080
				{s_nz, s_zr},		// 0x0040
				{s_xx, s_xx},		// 0x0020
				{s_nh, s_hc},		// 0x0010
				{s_xx, s_xx},		// 0x0008
				{s_po, s_pe},		// 0x0004
				{s_ns, s_su},		// 0x0002
				{s_nc, s_cy}};		// 0x0001

static const char *flags(UINT16 flag) {

static char	work[128];
	int		i;
	UINT16	bit;

	work[0] = 0;
	for (i=0, bit=0x80; bit; i++, bit>>=1) {
		if (flagstr[i][0]) {
			if (flag & bit) {
				milstr_ncat(work, flagstr[i][1], sizeof(work));
			}
			else {
				milstr_ncat(work, flagstr[i][0], sizeof(work));
			}
		}
	}
	return(work);
}

static void viewsubreg_paint(NP2VIEW_T *view, RECT *rc, HDC hdc) {

	LONG		y;
	DWORD		pos;
	char		str[128];
	HFONT		hfont;
	const struct Z80Reg	*r;

	hfont = CreateFont(16, 0, 0, 0, 0, 0, 0, 0, 
					SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
					DEFAULT_QUALITY, FIXED_PITCH, "‚l‚r ƒSƒVƒbƒN");
	SetTextColor(hdc, 0xffffff);
	SetBkColor(hdc, 0x400000);
	hfont = (HFONT)SelectObject(hdc, hfont);

	if (view->lock) {
		if (view->buf1.type != ALLOCTYPE_SUBREG) {
			if (viewcmn_alloc(&view->buf1, sizeof(Z80Reg))) {
				view->lock = FALSE;
				viewmenu_lock(view);
			}
			else {
				view->buf1.type = ALLOCTYPE_SUBREG;
				CopyMemory(view->buf1.ptr, subsystem_getcpureg(), sizeof(Z80Reg));
			}
			viewcmn_putcaption(view);
		}
	}

	pos = view->pos;
	if (view->lock) {
		r = (Z80Reg *)view->buf1.ptr;
	}
	else {
		r = subsystem_getcpureg();
	}

	for (y=0; y<rc->bottom && pos<5; y+=16, pos++) {
		switch(pos) {
			case 0:
				wsprintf(str, "A  =%.2x    BC =%.4x  DE =%.4x  HL =%.4x",
							V16(r->r.b.a), V16(r->r.w.bc), 
							V16(r->r.w.de), V16(r->r.w.hl));
				break;

			case 1:
				wsprintf(str, "A' =%.2x    BC'=%.4x  DE'=%.4x  HL'=%.4x",
							(r->r_af >> 8) & 0xff, V16(r->r_bc), 
							V16(r->r_de), V16(r->r_hl));
				break;

			case 2:
				wsprintf(str, "SP =%.4x            IX =%.4x  IY =%.4x",
							V16(r->r.w.sp), 
							V16(r->r.w.ix), V16(r->r.w.iy));
				break;

			case 3:
				wsprintf(str, "PC =%.4x  IM =%.1x     F  =%s",
							V16(r->pc), r->intmode, flags(r->r.b.flags));
				break;

			case 4:
				wsprintf(str, "                    F' =%s",
							flags(r->r_af & 0xff));
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
	dst->maxline = 5;
	dst->mul = 1;
	dst->pos = 0;
}

#endif