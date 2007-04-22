#include	"compiler.h"
#include	"resource.h"
#include	"np2.h"
#include	"viewer.h"
#include	"viewcmn.h"
#include	"viewmenu.h"
#include	"viewmem.h"
#include	"viewreg.h"
#include	"viewseg.h"
#include	"view1mb.h"
#include	"viewasm.h"
#include	"viewsnd.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"bios.h"
#include	"bmsio.h"

#if EXT
#include	"dialogs.h"
#endif

#if defined(SUPPORT_PC88VA)
#include	"memoryva.h"
#include	"va91.h"
#include	"viewvabank.h"
#include	"viewvideova.h"
#include	"viewgactrlva.h"
#include	"viewsubmem.h"
#include	"viewsubreg.h"
#include	"viewsubasm.h"
#endif

char viewcmn_hex[16] = {
				'0', '1', '2', '3', '4', '5', '6', '7',
				'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};


void viewcmn_caption(NP2VIEW_T *view, char *buf) {

	int		num;
	char	*p;

	num = ((int)view - (int)np2view) / sizeof(NP2VIEW_T);

	if (view->lock) {
		p = "Locked";
	}
	else {
		p = "Realtime";
	}
	wsprintf(buf, "%d.%s - NP2 Debug Utility", num+1, p);
}


void viewcmn_putcaption(NP2VIEW_T *view) {

	char	buf[256];

	viewcmn_caption(view, buf);
	SetWindowText(view->hwnd, buf);
}



// ----

BOOL viewcmn_alloc(VIEWMEMBUF *buf, DWORD size) {

	if (buf->type == ALLOCTYPE_ERROR) {
		return(FAILURE);
	}
	if (buf->size < size) {
		if (buf->ptr) {
			free(buf->ptr);
		}
		ZeroMemory(buf, sizeof(VIEWMEMBUF));
		buf->ptr = malloc(size);
		if (!buf->ptr) {
			buf->type = ALLOCTYPE_ERROR;
			return(FAILURE);
		}
		buf->size = size;
	}
	return(SUCCESS);
}


void viewcmn_free(VIEWMEMBUF *buf) {

	if (buf->ptr) {
		free(buf->ptr);
	}
	ZeroMemory(buf, sizeof(VIEWMEMBUF));
}


// ----

NP2VIEW_T *viewcmn_find(HWND hwnd) {

	int			i;
	NP2VIEW_T	*view;

	view = np2view;
	for (i=0; i<NP2VIEW_MAX; i++, view++) {
		if ((view->alive) && (view->hwnd == hwnd)) {
			return(view);
		}
	}
	return(NULL);
}

#if EXT
NP2VIEW_T *viewcmn_findbydialog(HWND hwnd) {

	int			i;
	NP2VIEW_T	*view;

	view = np2view;
	for (i=0; i<NP2VIEW_MAX; i++, view++) {
		if ((view->alive) && (view->hDialogWnd == hwnd)) {
			return(view);
		}
	}
	return(NULL);
}
#endif

void viewcmn_setmode(NP2VIEW_T *dst, NP2VIEW_T *src, BYTE type) {

	switch(type) {
		case VIEWMODE_REG:
			viewreg_init(dst, src);
			break;

		case VIEWMODE_SEG:
			viewseg_init(dst, src);
			break;

		case VIEWMODE_1MB:
			view1mb_init(dst, src);
			break;

		case VIEWMODE_ASM:
			viewasm_init(dst, src);
			break;

		case VIEWMODE_SND:
			viewsnd_init(dst, src);
			break;

#if defined(SUPPORT_PC88VA)
		case VIEWMODE_VABANK:
			viewvabank_init(dst, src);
			break;

		case VIEWMODE_VIDEOVA:
			viewvideova_init(dst, src);
			break;

		case VIEWMODE_GACTRLVA:
			viewgactrlva_init(dst, src);
			break;

		case VIEWMODE_SUBMEM:
			viewsubmem_init(dst, src);
			break;

		case VIEWMODE_SUBREG:
			viewsubreg_init(dst, src);
			break;

		case VIEWMODE_SUBASM:
			viewsubasm_init(dst, src);
			break;
#endif
	}
	viewmenu_mode(dst);
}

LRESULT CALLBACK viewcmn_dispat(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {

	NP2VIEW_T *dbg;

	dbg = viewcmn_find(hwnd);
	if (dbg) {
		switch(dbg->type) {
			case VIEWMODE_REG:
				return(viewreg_proc(dbg, hwnd, msg, wp, lp));

			case VIEWMODE_SEG:
				return(viewseg_proc(dbg, hwnd, msg, wp, lp));

			case VIEWMODE_1MB:
				return(view1mb_proc(dbg, hwnd, msg, wp, lp));

			case VIEWMODE_ASM:
				return(viewasm_proc(dbg, hwnd, msg, wp, lp));

			case VIEWMODE_SND:
				return(viewsnd_proc(dbg, hwnd, msg, wp, lp));

#if defined(SUPPORT_PC88VA)
			case VIEWMODE_VABANK:
				return(viewvabank_proc(dbg, hwnd, msg, wp, lp));

			case VIEWMODE_VIDEOVA:
				return(viewvideova_proc(dbg, hwnd, msg, wp, lp));

			case VIEWMODE_GACTRLVA:
				return(viewgactrlva_proc(dbg, hwnd, msg, wp, lp));

			case VIEWMODE_SUBMEM:
				return(viewsubmem_proc(dbg, hwnd, msg, wp, lp));

			case VIEWMODE_SUBREG:
				return(viewsubreg_proc(dbg, hwnd, msg, wp, lp));

			case VIEWMODE_SUBASM:
				return(viewsubasm_proc(dbg, hwnd, msg, wp, lp));
#endif
		}
	}
	return(0L);
}

void viewcmn_setbank(NP2VIEW_T *view) {

	VIEWMEM_T	*dmem;

	dmem = &view->dmem;
	dmem->vram = gdcs.disp;
	dmem->itf = CPU_ITFBANK;
	dmem->A20 = (BYTE)((CPU_ADRSMASK >> 20) & 1);
#if defined(SUPPORT_PC88VA)
	if (!dmem->lock) {
#if defined(SUPPORT_BMS)
		dmem->bmsio_bank = bmsio.bank;
#endif
		dmem->sysm_bank = memoryva.sysm_bank;
		dmem->rom0_bank = memoryva.rom0_bank;
		dmem->rom1_bank = memoryva.rom1_bank;
		dmem->va91rom0_bank = va91.rom0_bank;
		dmem->va91rom1_bank = va91.rom1_bank;
		dmem->va91sysm_bank = va91.sysm_bank;
	}
#endif
}


// ----

static void modmenu(HMENU hmenu, int pos, WORD id,
											const char *seg, WORD value) {

	char	buf[256];

	wsprintf(buf, "Seg = &%s [%04x]", seg, value);
	ModifyMenu(hmenu, pos, MF_BYPOSITION | MF_STRING, id, buf);
}

void viewcmn_setmenuseg(HWND hwnd) {

	HMENU	hparent;
	HMENU	hmenu;

	hparent = GetMenu(hwnd);
	if (hparent == NULL) {
		return;
	}
	hmenu = GetSubMenu(hparent, 2);

	if (hmenu) {
		modmenu(hmenu, 2, IDM_SEGCS, "CS", CPU_CS);
		modmenu(hmenu, 3, IDM_SEGDS, "DS", CPU_DS);
		modmenu(hmenu, 4, IDM_SEGES, "ES", CPU_ES);
		modmenu(hmenu, 5, IDM_SEGSS, "SS", CPU_SS);
		DrawMenuBar(hwnd);
	}
}

// -----

void viewcmn_setvscroll(HWND hWnd, NP2VIEW_T *view) {

	ZeroMemory(&(view->si), sizeof(SCROLLINFO));
	view->si.cbSize = sizeof(SCROLLINFO);
	view->si.fMask = SIF_ALL;
	view->si.nMin = 0;
	view->si.nMax = ((view->maxline + view->mul - 1) / view->mul) - 1;
	view->si.nPos = view->pos / view->mul;
	view->si.nPage = view->step / view->mul;
	SetScrollInfo(hWnd, SB_VERT, &(view->si), TRUE);
}

// -----

void viewcmn_paint(NP2VIEW_T *view, DWORD bkgcolor,
					void (*callback)(NP2VIEW_T *view, RECT *rc, HDC hdc)) {

	HDC			hdc;
	PAINTSTRUCT	ps;
	RECT		rc;
	HDC			hmemdc;
	HBITMAP		hbitmap;
	HBRUSH		hbrush;

	hdc = BeginPaint(view->hwnd, &ps);
	GetClientRect(view->hwnd, &rc);
	hmemdc = CreateCompatibleDC(hdc);
	hbitmap = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
	hbitmap = (HBITMAP)SelectObject(hmemdc, hbitmap);
	hbrush = (HBRUSH)SelectObject(hmemdc, CreateSolidBrush(bkgcolor));
	PatBlt(hmemdc, 0, 0, rc.right, rc.bottom, PATCOPY);
	DeleteObject(SelectObject(hmemdc, hbrush));

	callback(view, &rc, hmemdc);

	BitBlt(hdc, 0, 0, rc.right, rc.bottom, hmemdc, 0, 0, SRCCOPY);
	DeleteObject(SelectObject(hmemdc, hbitmap));
	DeleteDC(hmemdc);
	EndPaint(view->hwnd, &ps);
}


// -----

#if EXT

LRESULT CALLBACK ViewerAddressDialogProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {


	switch (msg) {
		case WM_INITDIALOG:
			{
				char work[16];
				NP2VIEW_T	*view;

				view = (NP2VIEW_T *)lp;
				if (view) {
					view->hDialogWnd = hWnd;
					wsprintf(work, "%04x", view->seg);
					SetDlgItemText(hWnd, IDC_SEG, work);
					wsprintf(work, "%04x", view->off);
					SetDlgItemText(hWnd, IDC_OFF, work);
#if defined(SUPPORT_BMS)
					if (bmsio.cfg.numbanks) {
						EnableWindow(GetDlgItem(hWnd, IDC_BMSIOBANK), TRUE);
						wsprintf(work, "%d", view->dmem.bmsio_bank);
						SetDlgItemText(hWnd, IDC_BMSIOBANK, work);
					}
					else {
						SetDlgItemText(hWnd, IDC_BMSIOBANK, "");
						EnableWindow(GetDlgItem(hWnd, IDC_BMSIOBANK), FALSE);
					}
#endif
#if defined(SUPPORT_PC88VA)
					wsprintf(work, "%d", view->dmem.sysm_bank);
					SetDlgItemText(hWnd, IDC_SYSMBANK, work);
					wsprintf(work, "%d", view->dmem.rom0_bank);
					SetDlgItemText(hWnd, IDC_ROM0BANK, work);
					wsprintf(work, "%d", view->dmem.rom1_bank);
					SetDlgItemText(hWnd, IDC_ROM1BANK, work);
					if (va91.cfg.enabled) {
						EnableWindow(GetDlgItem(hWnd, IDC_VA91SYSMBANK), TRUE);
						wsprintf(work, "%d", view->dmem.va91sysm_bank);
						SetDlgItemText(hWnd, IDC_VA91SYSMBANK, work);
						EnableWindow(GetDlgItem(hWnd, IDC_VA91ROM0BANK), TRUE);
						wsprintf(work, "%d", view->dmem.va91rom0_bank);
						SetDlgItemText(hWnd, IDC_VA91ROM0BANK, work);
						EnableWindow(GetDlgItem(hWnd, IDC_VA91ROM1BANK), TRUE);
						wsprintf(work, "%d", view->dmem.va91rom1_bank);
						SetDlgItemText(hWnd, IDC_VA91ROM1BANK, work);
					}
					else {
						EnableWindow(GetDlgItem(hWnd, IDC_VA91SYSMBANK), FALSE);
						SetDlgItemText(hWnd, IDC_VA91SYSMBANK, "");
						EnableWindow(GetDlgItem(hWnd, IDC_VA91ROM0BANK), FALSE);
						SetDlgItemText(hWnd, IDC_VA91ROM0BANK, "");
						EnableWindow(GetDlgItem(hWnd, IDC_VA91ROM1BANK), FALSE);
						SetDlgItemText(hWnd, IDC_VA91ROM1BANK, "");
					}

					SetDlgItemCheck(hWnd, IDC_LOCKBANK, view->dmem.lock);
#endif
				}
			}
			return(FALSE);

		case WM_COMMAND:
			switch (LOWORD(wp)) {
				case IDOK:
					{
						NP2VIEW_T	*view;
						char work[16];
						unsigned long seg, off;
						BYTE sbank, r0bank, r1bank, va91sbank, va91r0bank, va91r1bank, bmsiobank;
						int result = IDCANCEL;

						view = viewcmn_findbydialog(hWnd);
						if (view) {
							GetDlgItemText(hWnd, IDC_SEG, work, sizeof(work));
							seg = strtoul(work, NULL, 16);
							GetDlgItemText(hWnd, IDC_OFF, work, sizeof(work));
							off = strtoul(work, NULL, 16);
							GetDlgItemText(hWnd, IDC_BMSIOBANK, work, sizeof(work));
							bmsiobank = (BYTE)strtoul(work, NULL, 10);
							GetDlgItemText(hWnd, IDC_SYSMBANK, work, sizeof(work));
							sbank = (BYTE)strtoul(work, NULL, 10);
							GetDlgItemText(hWnd, IDC_ROM0BANK, work, sizeof(work));
							r0bank = (BYTE)strtoul(work, NULL, 10);
							GetDlgItemText(hWnd, IDC_ROM1BANK, work, sizeof(work));
							r1bank = (BYTE)strtoul(work, NULL, 10);
							GetDlgItemText(hWnd, IDC_VA91SYSMBANK, work, sizeof(work));
							va91sbank = (BYTE)strtoul(work, NULL, 10);
							GetDlgItemText(hWnd, IDC_VA91ROM0BANK, work, sizeof(work));
							va91r0bank = (BYTE)strtoul(work, NULL, 10);
							GetDlgItemText(hWnd, IDC_VA91ROM1BANK, work, sizeof(work));
							va91r1bank = (BYTE)strtoul(work, NULL, 10);
							if (seg < 0x10000 && off < 0x10000 && 
								bmsiobank < 0x100 &&
								sbank < 0x10 && r0bank < 0x20 && r1bank < 0x10 &&
								va91sbank < 0x10 && va91r0bank < 0x10 && va91r1bank < 0x10) {

								view->seg = (WORD)seg;
								view->off = (WORD)off;
								view->dmem.bmsio_bank = bmsiobank;
								view->dmem.sysm_bank = sbank;
								view->dmem.rom0_bank = r0bank;
								view->dmem.rom1_bank = r1bank;
								view->dmem.va91sysm_bank = va91sbank;
								view->dmem.va91rom0_bank = va91r0bank;
								view->dmem.va91rom1_bank = va91r1bank;
								view->dmem.lock = GetDlgItemCheck(hWnd, IDC_LOCKBANK);
								result = IDOK;
							}
							view->hDialogWnd = 0;
						}
						EndDialog(hWnd, result);
					}
					break;

				case IDCANCEL:
					{
						NP2VIEW_T	*view;

						view = viewcmn_findbydialog(hWnd);
						if (view) {
							view->hDialogWnd = 0;
						}
						EndDialog(hWnd, IDCANCEL);
					}
					break;

				default:
					return(FALSE);
			}
			break;

		case WM_CLOSE:
			PostMessage(hWnd, WM_COMMAND, IDCANCEL, 0);
			break;

		default:
			return(FALSE);
	}
	return(TRUE);
}

#endif
