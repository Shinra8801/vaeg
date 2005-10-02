#include	"compiler.h"
#include	<windowsx.h>
#include	"resource.h"
#include	"np2.h"
#include	"pccore.h"

#include	"sysmng.h"
#include	"ini.h"

#include	"breakpoint.h"
#include	"debugctrl.h"

#if defined(VAEG_EXT)

enum {
	IDC_DCTRL_BP_ENABLE	= 0,		// ブレークポイントを有効にするチェックボックス
	IDC_DCTRL_BP_COND	= 1,		// ブレークする条件
	IDC_DCTRL_BP_EDIT	= 2,		// 条件の入力開始、終了
	IDC_DCTRL_BP_MAXITEMS,			// ブレークポイント1個あたりのコントロールの数

	MAXBREAKPOINTS		= 5,		// ブレークポイントの数
	MAXCONDLEN			= 80,		// 条件の文字列の長さ
};

enum {
	IDC_DCTRL_PLAY		= 0,
	IDC_DCTRL_SINGLESTEP= 1,
	IDC_DCTRL_BP		= 20,
	IDC_DCTRL_BP_END	= IDC_DCTRL_BP + IDC_DCTRL_BP_MAXITEMS * MAXBREAKPOINTS -1,
	IDC_DCTRL_MAXITEMS,

	IDC_DCTRL_BASE		= 3000,

};

typedef struct {
	BOOL		editing;
	BOOL		enabled;
	char		cond[MAXCONDLEN];
} DEBUGCTRLBP;

typedef struct {
	int			posx;
	int			posy;
	int			width;
	int			height;
	
	DEBUGCTRLBP	bp[MAXBREAKPOINTS];

	HWND		hWnd;
	HWND		sub[IDC_DCTRL_MAXITEMS];
	BOOL		stopped;
	BOOL		singlestep;


} DEBUGCTRL;


#if defined(SUPPORT_PC88VA)
static const char np2debugctrlclass[] = "VAEG-debugctrlwin";
static const char np2debugctrltitle[] = "VA-EG Debug Control";
#else
static const char np2debugctrlclass[] = "np2-debugctrlwin";
static const char np2debugctrltitle[] = "NP2 Debug Control";
#endif

static	DEBUGCTRL	debugctrl;


static void debugctrl_updatesubcontrols(void) {
	SetWindowText(debugctrl.sub[IDC_DCTRL_PLAY], (debugctrl.stopped ? ">>" : "||")); 
}


static void debugctrl_createsubcontrols(HWND hWnd) {
	int			id;
	int			x, y, w, h;
	int			i;
	HWND		sub;
	DWORD		style;
	const char *text;
	const char *cls;

	id = IDC_DCTRL_PLAY;
	cls = "BUTTON";
	x = 0;	y = 0; w = 32; h = 32;
	style = BS_PUSHBUTTON;
	text = "";
	sub = CreateWindow(cls, text, WS_CHILD | WS_VISIBLE | style,
							x, y, w, h,
							hWnd, (HMENU)(id + IDC_DCTRL_BASE), hInst, NULL);
	debugctrl.sub[id] = sub;


	id = IDC_DCTRL_SINGLESTEP;
	cls = "BUTTON";
	x +=w;	y = 0; w = 32; h = 32;
	style = BS_PUSHBUTTON;
	text = "||>";
	sub = CreateWindow(cls, text, WS_CHILD | WS_VISIBLE | style,
							x, y, w, h,
							hWnd, (HMENU)(id + IDC_DCTRL_BASE), hInst, NULL);
	debugctrl.sub[id] = sub;


	id = IDC_DCTRL_BP;
	y += h;
	x = 0;
	for (i = 0; i < MAXBREAKPOINTS; i++) {
		h = 32;

		cls = "BUTTON";
		style = BS_AUTOCHECKBOX;
		text = "";
		sub = CreateWindow(cls, text, WS_CHILD | WS_VISIBLE | style,
							x, y + 2, 16, 16,
							hWnd, (HMENU)(id + IDC_DCTRL_BP_ENABLE + IDC_DCTRL_BASE), hInst, NULL);
		Button_SetCheck(sub, debugctrl.bp[i].enabled);
		debugctrl.sub[id + IDC_DCTRL_BP_ENABLE] = sub;

		cls = "EDIT";
		style = WS_BORDER;
		text = debugctrl.bp[i].cond;
		sub = CreateWindow(cls, text, WS_CHILD | WS_VISIBLE | style,
							x + 32, y, 256, 24,
							hWnd, (HMENU)(id + IDC_DCTRL_BP_COND + IDC_DCTRL_BASE), hInst, NULL);
		EnableWindow(sub ,FALSE);
		debugctrl.sub[id + IDC_DCTRL_BP_COND] = sub;

		cls = "BUTTON";
		style = BS_PUSHBUTTON;
		text = "Edit";
		sub = CreateWindow(cls, text, WS_CHILD | WS_VISIBLE | style,
							x + 32 + 256, y, 48, 24,
							hWnd, (HMENU)(id + IDC_DCTRL_BP_EDIT + IDC_DCTRL_BASE), hInst, NULL);
		debugctrl.sub[id + IDC_DCTRL_BP_EDIT] = sub;

		y += h;
		id += IDC_DCTRL_BP_MAXITEMS;
	}

	debugctrl_updatesubcontrols();
}

static void debugctrl_destroysubcontrols(void) {
	int		i;
	HWND	sub;

	if (debugctrl.hWnd) {
		for (i=0; i<IDC_DCTRL_MAXITEMS; i++) {
			sub = debugctrl.sub[i];
			if (sub) {
				DestroyWindow(sub);
			}
		}
/*
		DeleteObject(toolwin.access[0]);
		DeleteObject(toolwin.access[1]);
		DeleteObject(toolwin.hdcfont);
		DeleteObject(toolwin.hfont);
		DeleteObject(toolwin.hbmp);
		toolwin.hbmp = NULL;
*/
	}
}

static void debugctrl_bpproc(HWND hWnd, WPARAM wp, LPARAM lp) {
	int index;
	int	subidc, bpidc, bpidcbase;
	//int ctrl;
	int dmy;
	//unsigned int wpl;
	BREAKPOINT *bp;

	subidc = LOWORD(wp) - IDC_DCTRL_BASE;
	index = (subidc - IDC_DCTRL_BP) / IDC_DCTRL_BP_MAXITEMS;
	bpidc = (subidc - IDC_DCTRL_BP) % IDC_DCTRL_BP_MAXITEMS;
	bpidcbase = IDC_DCTRL_BP + IDC_DCTRL_BP_MAXITEMS * index;
	bp = breakpoint_get(index);
	if (bp == NULL) return;

	switch(bpidc) {
	case IDC_DCTRL_BP_ENABLE:
		if (Button_GetCheck(debugctrl.sub[bpidcbase + IDC_DCTRL_BP_ENABLE])) {
			bp->enabled = TRUE;
		}
		else {
			bp->enabled = FALSE;
		}
		debugctrl.bp[index].enabled = bp->enabled;
		sysmng_update(SYS_UPDATEOSCFG);
		break;
	case IDC_DCTRL_BP_COND:
		dmy = HIWORD(wp);
		switch(HIWORD(wp)) {
		case EN_SETFOCUS:
			dmy = 0;
			break;
		case EN_KILLFOCUS:
			dmy = 0;
			break;
		}
		break;
	case IDC_DCTRL_BP_EDIT:

		if (debugctrl.bp[index].editing) {
			GetWindowText(debugctrl.sub[bpidcbase + IDC_DCTRL_BP_COND], debugctrl.bp[index].cond ,MAXCONDLEN);
			sysmng_update(SYS_UPDATEOSCFG);

			breakpoint_clear(index);
			if (breakpoint_parse(debugctrl.bp[index].cond, bp)) {
				EnableWindow(debugctrl.sub[bpidcbase + IDC_DCTRL_BP_COND] ,FALSE);
				SetWindowText(debugctrl.sub[subidc], "Edit"); 
				debugctrl.bp[index].editing = FALSE;
				bp->enabled = debugctrl.bp[index].enabled;
			}
			else {
				// parse失敗
			}
		}
		else {
			EnableWindow(debugctrl.sub[bpidcbase + IDC_DCTRL_BP_COND] ,TRUE);
			SetWindowText(debugctrl.sub[subidc], "OK"); 
			debugctrl.bp[index].editing = TRUE;
		}
		break;
	}
}

static LRESULT CALLBACK debugctrl_proc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {

	HMENU		hMenu;
	BOOL		r;
	UINT		idc;

	switch(msg) {
		case WM_CREATE:
			debugctrl_createsubcontrols(hWnd);
			break;
		/*
		case WM_CLOSE:
			DestroyWindow(hWnd);
			break;
		*/
		case WM_DESTROY:
			breakpoint_check_setenabled(FALSE);
			debugctrl_destroysubcontrols();
			debugctrl.hWnd = NULL;
			break;

		case WM_MOVE:
			if (!(GetWindowLong(hWnd, GWL_STYLE) &
									(WS_MAXIMIZE | WS_MINIMIZE))) {
				RECT rc;
				GetWindowRect(hWnd, &rc);
				debugctrl.posx = rc.left;
				debugctrl.posy = rc.top;
				sysmng_update(SYS_UPDATEOSCFG);
			}
			break;

		case WM_SIZE:
			if (wp == SIZE_RESTORED) {
				RECT rc;
				GetWindowRect(hWnd, &rc);
				debugctrl.width = rc.right - rc.left;
				debugctrl.height = rc.bottom - rc.top;
				sysmng_update(SYS_UPDATEOSCFG);
			}
			break;

		case WM_COMMAND:
			if (LOWORD(wp) >= IDC_DCTRL_BASE + IDC_DCTRL_BP) {
				debugctrl_bpproc(hWnd, wp, lp);
				break;
			}
			switch(LOWORD(wp)) {
				case IDC_DCTRL_BASE + IDC_DCTRL_PLAY:
					if (debugctrl.singlestep) {
						pccore_debugpause(FALSE);
						pccore_debugsinglestep(FALSE);
						debugctrl.singlestep = FALSE;
						debugctrl.stopped = FALSE;
					}
					else {
						debugctrl.stopped = !debugctrl.stopped;
						pccore_debugpause(debugctrl.stopped);
					}
					debugctrl_updatesubcontrols();
					break;
				case IDC_DCTRL_BASE + IDC_DCTRL_SINGLESTEP:
					debugctrl.singlestep = TRUE;
					debugctrl.stopped = TRUE;
					pccore_debugsinglestep(TRUE);
					pccore_debugpause(FALSE);
					debugctrl_updatesubcontrols();
					break;
					/*
					debugctrl.singlestep = !debugctrl.singlestep;
					if (debugctrl.singlestep) debugctrl.stopped = TRUE;
					pccore_debugsinglestep(debugctrl.singlestep);
					debugctrl_updatesubcontrols();
					break;
					*/
			}
			break;

		default:
			return(DefWindowProc(hWnd, msg, wp, lp));
	}
	return(0);
}


BOOL debugctrl_initapp(HINSTANCE hInstance) {

	WNDCLASS wc;

	wc.style = CS_BYTEALIGNCLIENT | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = debugctrl_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
	wc.lpszMenuName = NULL; //MAKEINTRESOURCE(IDR_TOOLWIN);
	wc.lpszClassName = np2debugctrlclass;
	return(RegisterClass(&wc));
}


void debugctrl_create(void) {

	HWND hWnd;
	int i;

	if (debugctrl.hWnd) {
		return;
	}

	hWnd = CreateWindow(
			np2debugctrlclass,				// クラス名
			np2debugctrltitle,				// ウィンドウ名
			WS_OVERLAPPEDWINDOW,			// スタイル
			debugctrl.posx, debugctrl.posy,	// x, y
			debugctrl.width,debugctrl.height, // w, h
			NULL ,							// 親ウィンドウ
			NULL ,							// メニュー
			hInst ,
			NULL							// lParam
	);
	debugctrl.hWnd = hWnd;

	if (hWnd == NULL) {
		goto dcope_err2;
	}

	for (i = 0; i < MAXBREAKPOINTS; i++) {
		BREAKPOINT *bp;

		bp = breakpoint_get(i);
		if (bp != NULL) {
			if (breakpoint_parse(debugctrl.bp[i].cond, bp)) {
				bp->enabled = debugctrl.bp[i].enabled;
			}
			else {
				bp->enabled = FALSE;
				debugctrl.bp[i].enabled = FALSE;
			}
		}
	}

	breakpoint_check_setenabled(TRUE);

	//UpdateWindow(hWnd);
	ShowWindow(hWnd, SW_SHOWNOACTIVATE);
	SetForegroundWindow(hWndMain);
	return;

dcope_err2:
	return;
}


void debugctrl_destroy(void) {

	if (debugctrl.hWnd) {
		DestroyWindow(debugctrl.hWnd);
	}
}

/*
pccoreの実行状態に合わせて表示を更新する
*/
void debugctrl_update(void) {
	if (debugctrl.hWnd) {
		debugctrl.stopped = pccore_getdebugpause();
		debugctrl_updatesubcontrols();
	}
}


// ----

static const char ini_title[] = "Debug Control";

#define INI_ITEM_BP(index)														\
	{"BP" #index "Enable",	INITYPE_BOOL,	&debugctrl.bp[index].enabled,	0},	\
	{"BP" #index "Cond",	INITYPE_STR,	&debugctrl.bp[index].cond,	MAXCONDLEN}

static const INITBL iniitem[] = {
	{"WindposX", INITYPE_SINT32,	&debugctrl.posx,			0},
	{"WindposY", INITYPE_SINT32,	&debugctrl.posy,			0},
	{"WindW",	 INITYPE_UINT32,	&debugctrl.width,			0},
	{"WindH",	 INITYPE_UINT32,	&debugctrl.height,			0},

	INI_ITEM_BP(0),
	INI_ITEM_BP(1),
	INI_ITEM_BP(2),
	INI_ITEM_BP(3),
	INI_ITEM_BP(4),
};

void debugctrl_readini(void) {

	char	path[MAX_PATH];

	ZeroMemory(&debugctrl, sizeof(debugctrl));
	debugctrl.posx = CW_USEDEFAULT;
	debugctrl.posy = CW_USEDEFAULT;
	debugctrl.width = 400;
	debugctrl.height = 200;
	initgetfile(path, sizeof(path));
	ini_read(path, ini_title, iniitem, sizeof(iniitem)/sizeof(INITBL));
}

void debugctrl_writeini(void) {

	char	path[MAX_PATH];

	initgetfile(path, sizeof(path));
	ini_write(path, ini_title, iniitem, sizeof(iniitem)/sizeof(INITBL));
}


#endif