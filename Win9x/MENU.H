
void sysmenu_initialize(void);
void sysmenu_settoolwin(BYTE value);
void sysmenu_setkeydisp(BYTE value);
void sysmenu_setwinsnap(BYTE value);
void sysmenu_setbackground(BYTE value);
void sysmenu_setbgsound(BYTE value);
void sysmenu_setscrnmul(BYTE value);

void menu_addmenubar(HMENU popup, HMENU menubar);

void xmenu_initialize(void);
void xmenu_disablewindow(void);
#if defined(VAEG_EXT)
void xmenu_update(void);
#endif
void xmenu_setroltate(BYTE value);
void xmenu_setdispmode(BYTE value);
void xmenu_setraster(BYTE value);
void xmenu_setwaitflg(BYTE value);
void xmenu_setframe(BYTE value);
void xmenu_setkey(BYTE value);
void xmenu_setxshift(BYTE value);
void xmenu_setf12copy(BYTE value);
#if defined(VAEG_EXT)
void xmenu_setaltrkey(BYTE value);
#endif
void xmenu_setbeepvol(BYTE value);
#if defined(SUPPORT_PC88VA)
void xmenu_setsound(WORD value);
#else
void xmenu_setsound(BYTE value);
#endif
void xmenu_setjastsound(BYTE value);
void xmenu_setmotorflg(BYTE value);
void xmenu_setextmem(BYTE value);
void xmenu_setmouse(BYTE value);
void xmenu_sets98logging(BYTE value);
void xmenu_setwaverec(BYTE value);
void xmenu_setshortcut(BYTE value);
void xmenu_setdispclk(BYTE value);
void xmenu_setbtnmode(BYTE value);
void xmenu_setbtnrapid(BYTE value);
void xmenu_setmsrapid(BYTE value);
void xmenu_setsstp(BYTE value);

#if defined(SUPPORT_PC88VA)
void xmenu_setmousedevice(UINT8 value);
void xmenu_setva91(UINT8 value);
#endif

#if defined(SUPPORT_OPRECORD)
void xmenu_setrepeat(int value);
void xmenu_setoprecord(void);
#endif
