#include	"compiler.h"
#include	"resource.h"
#include	"np2.h"
#include	"sysmng.h"
#include	"menu.h"
#include	"np2class.h"
#include	"pccore.h"

#if defined(SUPPORT_PC88VA)
#include	"mouseifva.h"
#include	"va91.h"
#endif

#if defined(SUPPORT_OPRECORD)
#include	"oprecord.h"
#endif

#if defined(VAEG_EXT)
#include	"strres.h"
#endif

#define	MFCHECK(a) ((a)?MF_CHECKED:MF_UNCHECKED)

typedef struct {
	UINT16	id;
	UINT16	str;
} MENUITEMS;


// これってAPIあるのか？
void menu_addmenubar(HMENU popup, HMENU menubar) {

	UINT			cnt;
	UINT			pos;
	UINT			i;
	MENUITEMINFO	mii;
	char			str[128];
	HMENU			hSubMenu;

	cnt = GetMenuItemCount(menubar);
	pos = 0;
	for (i=0; i<cnt; i++) {
		ZeroMemory(&mii, sizeof(mii));
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID | MIIM_SUBMENU |
																	MIIM_DATA;
		mii.dwTypeData = str;
		mii.cch = sizeof(str);
		if (GetMenuItemInfo(menubar, i, TRUE, &mii)) {
			if (mii.hSubMenu) {
				hSubMenu = CreatePopupMenu();
				menu_addmenubar(hSubMenu, mii.hSubMenu);
				mii.hSubMenu = hSubMenu;
			}
			InsertMenuItem(popup, pos, TRUE, &mii);
			pos++;
		}
	}
}

static void insertresmenu(HMENU menu, UINT pos, UINT flag,
													UINT item, UINT str) {

	char	tmp[128];

	if (LoadString(hInst, str, tmp, sizeof(tmp))) {
		InsertMenu(menu, pos, flag, item, tmp);
	}
}

static void insertresmenus(HMENU menu, UINT pos,
									const MENUITEMS *item, UINT items) {

const MENUITEMS *iterm;

	iterm = item + items;
	while(item < iterm) {
		if (item->id) {
			insertresmenu(menu, pos, MF_BYPOSITION | MF_STRING,
													item->id, item->str);
		}
		else {
			InsertMenu(menu, pos, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
		}
		item++;
		pos++;
	}
}


// ----

static const MENUITEMS smenuitem[] = {
			{IDM_TOOLWIN,		IDS_TOOLWIN},
#if defined(SUPPORT_KEYDISP)
			{IDM_KEYDISP,		IDS_KEYDISP},
#endif
#if defined(SUPPORT_SOFTKBD)
			{IDM_SOFTKBD,		IDS_SOFTKBD},
#endif
			{0,					0},
			{IDM_SCREENCENTER,	IDS_SCREENCENTER},
			{IDM_SNAPENABLE,	IDS_SNAPENABLE},
			{IDM_BACKGROUND,	IDS_BACKGROUND},
			{IDM_BGSOUND,		IDS_BGSOUND},
			{0,					0},
			{IDM_SCRNMUL4,		IDS_SCRNMUL4},
			{IDM_SCRNMUL6,		IDS_SCRNMUL6},
			{IDM_SCRNMUL8,		IDS_SCRNMUL8},
			{IDM_SCRNMUL10,		IDS_SCRNMUL10},
			{IDM_SCRNMUL12,		IDS_SCRNMUL12},
			{IDM_SCRNMUL16,		IDS_SCRNMUL16},
			{0,					0}};

static const MENUITEMS smenuitem2[] = {
#if defined(CPUCORE_IA32) && defined(SUPPORT_MEMDBG32)
			{IDM_MEMDBG32,		IDS_MEMDBG32},
#endif
			{IDM_MEMORYDUMP,	IDS_MEMORYDUMP},
			{IDM_DEBUGUTY,		IDS_DEBUGUTY},
#if defined(VAEG_EXT)
			{IDM_DEBUGCTRL,		IDS_DEBUGCTRL},
#endif
			{0,					0}};


void sysmenu_initialize(void) {

	HMENU	hMenu;

	hMenu = GetSystemMenu(hWndMain, FALSE);
	insertresmenus(hMenu, 0, smenuitem, sizeof(smenuitem)/sizeof(MENUITEMS));
	if (np2oscfg.I286SAVE) {
		insertresmenus(hMenu, 0, smenuitem2,
										sizeof(smenuitem2)/sizeof(MENUITEMS));
	}
}

void sysmenu_settoolwin(BYTE value) {

	value &= 1;
	np2oscfg.toolwin = value;
	CheckMenuItem(GetSystemMenu(hWndMain, FALSE),
											IDM_TOOLWIN, MFCHECK(value));
}

void sysmenu_setkeydisp(BYTE value) {

	value &= 1;
	np2oscfg.keydisp = value;
	CheckMenuItem(GetSystemMenu(hWndMain, FALSE),
											IDM_KEYDISP, MFCHECK(value));
}

void sysmenu_setwinsnap(BYTE value) {

	value &= 1;
	np2oscfg.WINSNAP = value;
	CheckMenuItem(GetSystemMenu(hWndMain, FALSE),
											IDM_SNAPENABLE, MFCHECK(value));
}

void sysmenu_setbackground(BYTE value) {

	HMENU	hmenu;

	np2oscfg.background &= 2;
	np2oscfg.background |= (value & 1);
	hmenu = GetSystemMenu(hWndMain, FALSE);
	if (value & 1) {
		CheckMenuItem(hmenu, IDM_BACKGROUND, MF_UNCHECKED);
		EnableMenuItem(hmenu, IDM_BGSOUND, MF_GRAYED);
	}
	else {
		CheckMenuItem(hmenu, IDM_BACKGROUND, MF_CHECKED);
		EnableMenuItem(hmenu, IDM_BGSOUND, MF_ENABLED);
	}
}

void sysmenu_setbgsound(BYTE value) {

	np2oscfg.background &= 1;
	np2oscfg.background |= (value & 2);
	CheckMenuItem(GetSystemMenu(hWndMain, FALSE),
									IDM_BGSOUND, MFCHECK((value & 2) ^ 2));
}

void sysmenu_setscrnmul(BYTE value) {

	HMENU	hmenu;

//	np2cfg.scrnmul = value;
	hmenu = GetSystemMenu(hWndMain, FALSE);
	CheckMenuItem(hmenu, IDM_SCRNMUL4, MFCHECK(value == 4));
	CheckMenuItem(hmenu, IDM_SCRNMUL6, MFCHECK(value == 6));
	CheckMenuItem(hmenu, IDM_SCRNMUL8, MFCHECK(value == 8));
	CheckMenuItem(hmenu, IDM_SCRNMUL10, MFCHECK(value == 10));
	CheckMenuItem(hmenu, IDM_SCRNMUL12, MFCHECK(value == 12));
	CheckMenuItem(hmenu, IDM_SCRNMUL16, MFCHECK(value == 16));
}


// ----

typedef struct {
	UINT16		title;
	MENUITEMS	item[3];
} DISKMENU;

static const DISKMENU fddmenu[4] = {
		{IDS_FDD1,	{{IDM_FDD1OPEN,		IDS_OPEN},
					{0,					0},
					{IDM_FDD1EJECT,		IDS_EJECT}}},
		{IDS_FDD2,	{{IDM_FDD2OPEN,		IDS_OPEN},
					{0,					0},
					{IDM_FDD2EJECT,		IDS_EJECT}}},
		{IDS_FDD3,	{{IDM_FDD3OPEN,		IDS_OPEN},
					{0,					0},
					{IDM_FDD3EJECT,		IDS_EJECT}}},
		{IDS_FDD4,	{{IDM_FDD4OPEN,		IDS_OPEN},
					{0,					0},
					{IDM_FDD4EJECT,		IDS_EJECT}}}};

static void insdiskmenu(HMENU hMenu, UINT pos, const DISKMENU *m) {

	HMENU	hSubMenu;

	hSubMenu = CreatePopupMenu();
	insertresmenus(hSubMenu, 0, m->item, 3);
	insertresmenu(hMenu, pos, MF_BYPOSITION | MF_POPUP,
											(UINT)hSubMenu, m->title);
}

#if defined(SUPPORT_SCSI)
static const DISKMENU scsimenu[4] = {
		{IDS_SCSI0,	{{IDM_SCSI0OPEN,	IDS_OPEN},
					{0,					0},
					{IDM_SCSI0EJECT,	IDS_REMOVE}}},
		{IDS_SCSI1,	{{IDM_SCSI1OPEN,	IDS_OPEN},
					{0,					0},
					{IDM_SCSI1EJECT,	IDS_REMOVE}}},
		{IDS_SCSI2,	{{IDM_SCSI2OPEN,	IDS_OPEN},
					{0,					0},
					{IDM_SCSI2EJECT,	IDS_REMOVE}}},
		{IDS_SCSI3,	{{IDM_SCSI3OPEN,	IDS_OPEN},
					{0,					0},
					{IDM_SCSI3EJECT,	IDS_REMOVE}}}};
#endif

#if defined(SUPPORT_STATSAVE)
static const char xmenu_stat[] = "S&tat";
static const char xmenu_statsave[] = "Save %u";
static const char xmenu_statload[] = "Load %u";

static void addstatsavemenu(HMENU hMenu, UINT pos) {

	HMENU	hSubMenu;
	UINT	i;
	char	buf[16];

	hSubMenu = CreatePopupMenu();
	for (i=0; i<SUPPORT_STATSAVE; i++) {
		SPRINTF(buf, xmenu_statsave, i);
		AppendMenu(hSubMenu, MF_STRING, IDM_FLAGSAVE + i, buf);
	}
	AppendMenu(hSubMenu, MF_MENUBARBREAK, 0, NULL);
	for (i=0; i<SUPPORT_STATSAVE; i++) {
		SPRINTF(buf, xmenu_statload, i);
		AppendMenu(hSubMenu, MF_STRING, IDM_FLAGLOAD + i, buf);
	}
	InsertMenu(hMenu, pos, MF_BYPOSITION | MF_POPUP,
											(UINT32)hSubMenu, xmenu_stat);
}
#endif

#if defined(SUPPORT_OPRECORD)
static const char xmenu_oprec[] = "&Replay";
/*
static const char xmenu_oprecrec[] = "Rec %u";
static const char xmenu_oprecplay[] = "Play %u";
*/

static void addoprecordmenu(HMENU hMenu, UINT pos) {

	HMENU	hSubMenu;
/*
	UINT	i;
	char	buf[16];
*/

	hSubMenu = CreatePopupMenu();
	AppendMenu(hSubMenu, MF_STRING, IDM_OPRECORDSTOP,   "&Stop");
	AppendMenu(hSubMenu, MF_STRING, IDM_OPRECORD_PLAY,  "&Play");
	AppendMenu(hSubMenu, MF_STRING, IDM_OPRECORD_PLAYMULTI,  "&Multi play");
	AppendMenu(hSubMenu, MF_STRING, IDM_OPRECORD_REC,   "&Record");
	AppendMenu(hSubMenu, MF_MENUBARBREAK, 0, NULL);
	AppendMenu(hSubMenu, MF_STRING, IDM_OPRECORDREPEAT, "R&epeat");
/*
	AppendMenu(hSubMenu, MF_MENUBARBREAK, 0, NULL);
	for (i=0; i<SUPPORT_OPRECORD; i++) {
		SPRINTF(buf, xmenu_oprecrec, i);
		AppendMenu(hSubMenu, MF_STRING, IDM_OPRECORDREC + i, buf);
	}
	AppendMenu(hSubMenu, MF_MENUBARBREAK, 0, NULL);
	for (i=0; i<SUPPORT_OPRECORD; i++) {
		SPRINTF(buf, xmenu_oprecplay, i);
		AppendMenu(hSubMenu, MF_STRING, IDM_OPRECORDPLAY + i, buf);
	}
*/
	InsertMenu(hMenu, pos, MF_BYPOSITION | MF_POPUP,
											(UINT32)hSubMenu, xmenu_oprec);
}

void xmenu_setrepeat(int value) {
	CheckMenuItem(np2class_gethmenu(hWndMain), IDM_OPRECORDREPEAT, MFCHECK(value));
}

void xmenu_setoprecord(void) {
	if (oprecord_recording() || oprecord_playing()) {
		// Stop を有効に、Record, Play を無効に
		EnableMenuItem(np2class_gethmenu(hWndMain), IDM_OPRECORDSTOP, MF_ENABLED);
		EnableMenuItem(np2class_gethmenu(hWndMain), IDM_OPRECORD_REC, MF_GRAYED);
		EnableMenuItem(np2class_gethmenu(hWndMain), IDM_OPRECORD_PLAY, MF_GRAYED);
		EnableMenuItem(np2class_gethmenu(hWndMain), IDM_OPRECORD_PLAYMULTI, MF_GRAYED);
	}
	else {
		// Stop を無効に、Record, Play を有効に
		EnableMenuItem(np2class_gethmenu(hWndMain), IDM_OPRECORDSTOP, MF_GRAYED);
		EnableMenuItem(np2class_gethmenu(hWndMain), IDM_OPRECORD_REC, MF_ENABLED);
		EnableMenuItem(np2class_gethmenu(hWndMain), IDM_OPRECORD_PLAY, MF_ENABLED);
		EnableMenuItem(np2class_gethmenu(hWndMain), IDM_OPRECORD_PLAYMULTI, MF_ENABLED);
	}
}
#endif

void xmenu_initialize(void) {

	HMENU	hMenu;
	HMENU	hSubMenu;
	UINT	i;

	hMenu = np2class_gethmenu(hWndMain);
	if (np2oscfg.I286SAVE) {
		hSubMenu = GetSubMenu(hMenu, 4);
		insertresmenu(hSubMenu, 10, MF_BYPOSITION | MF_STRING,
												IDM_I286SAVE, IDS_I286SAVE);
	}
#if defined(SUPPORT_WAVEREC)
	hSubMenu = GetSubMenu(hMenu, 4);
	insertresmenu(hSubMenu, 2, MF_BYPOSITION | MF_STRING,
												IDM_WAVEREC, IDS_WAVEREC);
#endif

#if defined(SUPPORT_SCSI)
	hSubMenu = GetSubMenu(hMenu, 1);
	AppendMenu(hSubMenu, MF_SEPARATOR, 0, NULL);
	for (i=0; i<4; i++) {
		insdiskmenu(hSubMenu, i + 3, scsimenu + i);
	}
#endif

	for (i=4; i--;) {
		if (np2cfg.fddequip & (1 << i)) {
			insdiskmenu(hMenu, 1, fddmenu + i);
		}
	}

#if defined(SUPPORT_STATSAVE)
	if (np2oscfg.statsave) {
		addstatsavemenu(hMenu, 1);
	}
#endif
#if defined(SUPPORT_OPRECORD)
	if (np2oscfg.oprecord) {
		addoprecordmenu(hMenu, 1);
		xmenu_setoprecord();
	}
#endif
}

#if defined(VAEG_EXT)
void xmenu_update(void) {
#if defined(SUPPORT_PC88VA)
	int disabledforva[]={
		IDM_RASTER,
		IDM_JOY2,
		IDM_MEM640, IDM_MEM16, IDM_MEM36, IDM_MEM76, IDM_MEM116, IDM_MEM136,
		IDM_PC9801_14, IDM_PC9801_26K, IDM_PC9801_86, IDM_PC9801_26_86, IDM_PC9801_86_CB,
		IDM_PC9801_118, IDM_SPEAKBOARD, IDM_SPARKBOARD, IDM_AMD98,
		IDM_JASTSOUND,
	};
	int disabledfor98[]={
		IDM_SOUNDBOARD2,
		IDM_PORTJOY, IDM_PORTMOUSE,
		IDM_F12ZENKAKU,
	};
#endif
	HMENU	hmenu;
	int i;

	hmenu = np2class_gethmenu(hWndMain);

#if defined(SUPPORT_PC88VA)
	if (milstr_cmp(np2cfg.model, str_VA1)==0) {
		// VA1
		EnableMenuItem(hmenu, IDM_VA91, MF_ENABLED);
	}
	else {
		EnableMenuItem(hmenu, IDM_VA91, MF_GRAYED);
	}

	if (milstr_cmp(np2cfg.model, str_VA1)==0 || milstr_cmp(np2cfg.model, str_VA2)==0) {
		// VA
		for (i = 0; i < sizeof(disabledforva) / sizeof(int); i++) {
			EnableMenuItem(hmenu, disabledforva[i], MF_GRAYED);
		}
		for (i = 0; i < sizeof(disabledfor98) / sizeof(int); i++) {
			EnableMenuItem(hmenu, disabledfor98[i], MF_ENABLED);
		}
	}
	else {
		// 98
		for (i = 0; i < sizeof(disabledforva) / sizeof(int); i++) {
			EnableMenuItem(hmenu, disabledforva[i], MF_ENABLED);
		}
		for (i = 0; i < sizeof(disabledfor98) / sizeof(int); i++) {
			EnableMenuItem(hmenu, disabledfor98[i], MF_GRAYED);
		}
	}

#endif
}
#endif

void xmenu_disablewindow(void) {

	HMENU	hmenu;

	hmenu = np2class_gethmenu(hWndMain);
	EnableMenuItem(hmenu, IDM_WINDOW, MF_GRAYED);
	EnableMenuItem(hmenu, IDM_FULLSCREEN, MF_GRAYED);
}

void xmenu_setroltate(BYTE value) {

	HMENU	hmenu;

	hmenu = np2class_gethmenu(hWndMain);
	CheckMenuItem(hmenu, IDM_ROLNORMAL, MFCHECK(value == 0));
	CheckMenuItem(hmenu, IDM_ROLLEFT, MFCHECK(value == 1));
	CheckMenuItem(hmenu, IDM_ROLRIGHT, MFCHECK(value == 2));
}

void xmenu_setdispmode(BYTE value) {

	value &= 1;
	np2cfg.DISPSYNC = value;
	CheckMenuItem(np2class_gethmenu(hWndMain), IDM_DISPSYNC, MFCHECK(value));
}

void xmenu_setraster(BYTE value) {

	value &= 1;
	np2cfg.RASTER = value;
	CheckMenuItem(np2class_gethmenu(hWndMain), IDM_RASTER, MFCHECK(value));
}

void xmenu_setwaitflg(BYTE value) {

	value &= 1;
	np2oscfg.NOWAIT = value;
	CheckMenuItem(np2class_gethmenu(hWndMain), IDM_NOWAIT, MFCHECK(value));
}

void xmenu_setframe(BYTE value) {

	HMENU	hmenu;

	np2oscfg.DRAW_SKIP = value;
	hmenu = np2class_gethmenu(hWndMain);
	CheckMenuItem(hmenu, IDM_AUTOFPS, MFCHECK(value == 0));
	CheckMenuItem(hmenu, IDM_60FPS, MFCHECK(value == 1));
	CheckMenuItem(hmenu, IDM_30FPS, MFCHECK(value == 2));
	CheckMenuItem(hmenu, IDM_20FPS, MFCHECK(value == 3));
	CheckMenuItem(hmenu, IDM_15FPS, MFCHECK(value == 4));
}

void xmenu_setkey(BYTE value) {

	HMENU	hmenu;

	if (value >= 4) {
		value = 0;
	}
	np2cfg.KEY_MODE = value;
	hmenu = np2class_gethmenu(hWndMain);
	CheckMenuItem(hmenu, IDM_KEY, MFCHECK(value == 0));
	CheckMenuItem(hmenu, IDM_JOY1, MFCHECK(value == 1));
	CheckMenuItem(hmenu, IDM_JOY2, MFCHECK(value == 2));
}

void xmenu_setxshift(BYTE value) {

	HMENU	hmenu;

	np2cfg.XSHIFT = value;
	hmenu = np2class_gethmenu(hWndMain);
	CheckMenuItem(hmenu, IDM_XSHIFT, MFCHECK(value & 1));
	CheckMenuItem(hmenu, IDM_XCTRL, MFCHECK(value & 2));
	CheckMenuItem(hmenu, IDM_XGRPH, MFCHECK(value & 4));
}

void xmenu_setf12copy(BYTE value) {

	HMENU	hmenu;

#if defined(SUPPORT_PC88VA)
	if (value > 7) {
#else
	if (value > 6) {
#endif
		value = 0;
	}
	np2oscfg.F12COPY = value;
	hmenu = np2class_gethmenu(hWndMain);
	CheckMenuItem(hmenu, IDM_F12MOUSE, MFCHECK(value == 0));
	CheckMenuItem(hmenu, IDM_F12COPY, MFCHECK(value == 1));
	CheckMenuItem(hmenu, IDM_F12STOP, MFCHECK(value == 2));
	CheckMenuItem(hmenu, IDM_F12EQU, MFCHECK(value == 3));
	CheckMenuItem(hmenu, IDM_F12COMMA, MFCHECK(value == 4));
	CheckMenuItem(hmenu, IDM_USERKEY1, MFCHECK(value == 5));
	CheckMenuItem(hmenu, IDM_USERKEY2, MFCHECK(value == 6));
#if defined(SUPPORT_PC88VA)
	CheckMenuItem(hmenu, IDM_F12ZENKAKU, MFCHECK(value == 7));
#endif
}

#if defined(VAEG_EXT)
void xmenu_setaltrkey(BYTE value) {

	HMENU	hmenu;

	if (value > 9) {
		value = 0;
	}
	np2oscfg.ALTRKEY = value;
	hmenu = np2class_gethmenu(hWndMain);
	//CheckMenuItem(hmenu, IDM_ALTRMOUSE, MFCHECK(value == 0));
	//CheckMenuItem(hmenu, IDM_ALTRCOPY, MFCHECK(value == 1));
	//CheckMenuItem(hmenu, IDM_ALTRSTOP, MFCHECK(value == 2));
	CheckMenuItem(hmenu, IDM_ALTREQU, MFCHECK(value == 3));
	CheckMenuItem(hmenu, IDM_ALTRCOMMA, MFCHECK(value == 4));
	CheckMenuItem(hmenu, IDM_ALTRUSERKEY1, MFCHECK(value == 5));
	CheckMenuItem(hmenu, IDM_ALTRUSERKEY2, MFCHECK(value == 6));
	CheckMenuItem(hmenu, IDM_ALTRZENKAKU, MFCHECK(value == 7));
	CheckMenuItem(hmenu, IDM_ALTRGRPH, MFCHECK(value == 8));
	CheckMenuItem(hmenu, IDM_ALTRNOWAIT, MFCHECK(value == 9));
}
#endif

void xmenu_setbeepvol(BYTE value) {

	HMENU	hmenu;

	value &= 3;
	np2cfg.BEEP_VOL = value;
	hmenu = np2class_gethmenu(hWndMain);
	CheckMenuItem(hmenu, IDM_BEEPOFF, MFCHECK(value == 0));
	CheckMenuItem(hmenu, IDM_BEEPLOW, MFCHECK(value == 1));
	CheckMenuItem(hmenu, IDM_BEEPMID, MFCHECK(value == 2));
	CheckMenuItem(hmenu, IDM_BEEPHIGH, MFCHECK(value == 3));
}

#if defined(SUPPORT_PC88VA)
void xmenu_setsound(WORD value) {
#else
void xmenu_setsound(BYTE value) {
#endif

	HMENU	hmenu;

	sysmng_update(SYS_UPDATESBOARD);
	np2cfg.SOUND_SW = value;
	hmenu = np2class_gethmenu(hWndMain);
	CheckMenuItem(hmenu, IDM_NOSOUND, MFCHECK(value == 0x00));
	CheckMenuItem(hmenu, IDM_PC9801_14, MFCHECK(value == 0x01));
	CheckMenuItem(hmenu, IDM_PC9801_26K, MFCHECK(value == 0x02));
	CheckMenuItem(hmenu, IDM_PC9801_86, MFCHECK(value == 0x04));
	CheckMenuItem(hmenu, IDM_PC9801_26_86, MFCHECK(value == 0x06));
	CheckMenuItem(hmenu, IDM_PC9801_86_CB, MFCHECK(value == 0x14));
	CheckMenuItem(hmenu, IDM_PC9801_118, MFCHECK(value == 0x08));
	CheckMenuItem(hmenu, IDM_SPEAKBOARD, MFCHECK(value == 0x20));
	CheckMenuItem(hmenu, IDM_SPARKBOARD, MFCHECK(value == 0x40));
	CheckMenuItem(hmenu, IDM_AMD98, MFCHECK(value == 0x80));
#if defined(SUPPORT_PC88VA)
	CheckMenuItem(hmenu, IDM_SOUNDBOARD2, MFCHECK(value == 0x0200));
#endif
}

void xmenu_setjastsound(BYTE value) {

	value &= 1;
	np2oscfg.jastsnd = value;
	CheckMenuItem(np2class_gethmenu(hWndMain), IDM_JASTSOUND, MFCHECK(value));
}

void xmenu_setmotorflg(BYTE value) {

	value &= 1;
	np2cfg.MOTOR = value;
	CheckMenuItem(np2class_gethmenu(hWndMain), IDM_SEEKSND, MFCHECK(value));
}

void xmenu_setextmem(BYTE value) {

	HMENU	hmenu;

	sysmng_update(SYS_UPDATEMEMORY);
	np2cfg.EXTMEM = value;
	hmenu = np2class_gethmenu(hWndMain);
	CheckMenuItem(hmenu, IDM_MEM640, MFCHECK(value == 0));
	CheckMenuItem(hmenu, IDM_MEM16, MFCHECK(value == 1));
	CheckMenuItem(hmenu, IDM_MEM36, MFCHECK(value == 3));
	CheckMenuItem(hmenu, IDM_MEM76, MFCHECK(value == 7));
	CheckMenuItem(hmenu, IDM_MEM116, MFCHECK(value == 11));
	CheckMenuItem(hmenu, IDM_MEM136, MFCHECK(value == 13));
}

void xmenu_setmouse(BYTE value) {

	value &= 1;
	np2oscfg.MOUSE_SW = value;
	CheckMenuItem(np2class_gethmenu(hWndMain), IDM_MOUSE, MFCHECK(value));
}

#if defined(SUPPORT_S98)
void xmenu_sets98logging(BYTE value) {

	CheckMenuItem(np2class_gethmenu(hWndMain),
											IDM_S98LOGGING, MFCHECK(value));
}
#endif

#if defined(SUPPORT_WAVEREC)
void xmenu_setwaverec(BYTE value) {

	CheckMenuItem(np2class_gethmenu(hWndMain),
											IDM_WAVEREC, MFCHECK(value));
}
#endif

void xmenu_setshortcut(BYTE value) {

	HMENU	hmenu;

	np2oscfg.shortcut = value;
	hmenu = np2class_gethmenu(hWndMain);
	CheckMenuItem(hmenu, IDM_ALTENTER, MFCHECK(value & 1));
	CheckMenuItem(hmenu, IDM_ALTF4, MFCHECK(value & 2));
}

void xmenu_setdispclk(BYTE value) {

	HMENU	hmenu;

	value &= 3;
	np2oscfg.DISPCLK = value;
	hmenu = np2class_gethmenu(hWndMain);
	CheckMenuItem(hmenu, IDM_DISPCLOCK, MFCHECK(value & 1));
	CheckMenuItem(hmenu, IDM_DISPFRAME, MFCHECK(value & 2));
	sysmng_workclockrenewal();
	sysmng_updatecaption(3);
}

void xmenu_setbtnmode(BYTE value) {

	value &= 1;
	np2cfg.BTN_MODE = value;
	CheckMenuItem(np2class_gethmenu(hWndMain), IDM_JOYX, MFCHECK(value));
}

void xmenu_setbtnrapid(BYTE value) {

	value &= 1;
	np2cfg.BTN_RAPID = value;
	CheckMenuItem(np2class_gethmenu(hWndMain), IDM_RAPID, MFCHECK(value));
}

void xmenu_setmsrapid(BYTE value) {

	value &= 1;
	np2cfg.MOUSERAPID = value;
	CheckMenuItem(np2class_gethmenu(hWndMain), IDM_MSRAPID, MFCHECK(value));
}

void xmenu_setsstp(BYTE value) {

	value &= 1;
	np2oscfg.sstp = value;
	CheckMenuItem(np2class_gethmenu(hWndMain), IDM_SSTP, MFCHECK(value));
}

#if defined(SUPPORT_PC88VA)
void xmenu_setmousedevice(UINT8 value) {
	HMENU	hmenu;

	mouseifvacfg.device = value;
	hmenu = np2class_gethmenu(hWndMain);
	CheckMenuItem(hmenu, IDM_PORTJOY, MFCHECK(value == 0x00));
	CheckMenuItem(hmenu, IDM_PORTMOUSE, MFCHECK(value == 0x01));
}

void xmenu_setva91(BYTE value) {
	value &= 1;
	va91cfg.enabled = value;
	CheckMenuItem(np2class_gethmenu(hWndMain), IDM_VA91, MFCHECK(value));
}
#endif
