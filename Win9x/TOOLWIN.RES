#if defined(SUPPORT_PC88VA)
static const char np2toolclass[] = "VAEG-toolwin";
static const char np2tooltitle[] = "VA-EG tool";
#else
static const char np2toolclass[] = "np2-toolwin";
static const char np2tooltitle[] = "NP2 tool";
#endif

static const char str_deffont[] = "‚l‚r ‚oƒSƒVƒbƒN";
static const char str_browse[] = "...";
static const char str_eject[] = "Eject";
static const char str_reset[] = "Reset";
static const char str_power[] = "Power";

static const char str_static[] = "STATIC";
static const char str_combobox[] = "COMBOBOX";
static const char str_button[] = "BUTTON";

#if defined(SUPPORT_PC88VA)
static const SUBITEM defsubitem[IDC_MAXITEMS] = {
		{TCTL_STATIC,	NULL,		  8, 66,  12,   4, 0, 0},
		{TCTL_STATIC,	NULL,		246, 10,  12,   4, 0, 0},
		{TCTL_DDLIST,	NULL,		234, 26, 144, 160, 2, 0},
		{TCTL_BUTTON,	str_browse,	378, 28,  16,  16, 2, 0},
		{TCTL_BUTTON,	str_eject,	341,  6,  41,  42, 1, 0},
		{TCTL_STATIC,	NULL,		246, 62,  12,   4, 0, 0},
		{TCTL_DDLIST,	NULL,		234, 78, 144, 160, 2, 0},
		{TCTL_BUTTON,	str_browse,	378, 80,  16,  16, 2, 0},
		{TCTL_BUTTON,	str_eject,	341, 58,  41,  42, 1, 0},
		{TCTL_BUTTON,	str_reset,   88,110,  22,   8, 1, 0},
		{TCTL_BUTTON,	str_power,	425, 81,  28,  20, 1, 0},

		{TCTL_STATIC,	NULL,		424, 13,   8,   4, 0, 0},
		{TCTL_STATIC,	NULL,		424, 24,   8,   4, 0, 0},
		{TCTL_STATIC,	NULL,		424, 35,   8,   4, 0, 0},
#if defined(VAEG_EXT)
		{TCTL_STATIC,	NULL,		234,  4, 160,  44, 0, 0},
		{TCTL_STATIC,	NULL,		234, 56, 160,  44, 0, 0},
#endif
};
#else

static const SUBITEM defsubitem[IDC_MAXITEMS] = {
		{TCTL_STATIC,	NULL,		 49, 44,   8,   3, 0, 0},
		{TCTL_STATIC,	NULL,		 93, 19,   8,   3, 0, 0},
		{TCTL_DDLIST,	NULL,		104,  6, 248, 160, 0, 0},
		{TCTL_BUTTON,	str_browse,	352,  7,  18,  17, 0, 0},
		{TCTL_BUTTON,	str_eject,	370,  7,  34,  17, 0, 0},
		{TCTL_STATIC,	NULL,		 93, 41,   8,   3, 0, 0},
		{TCTL_DDLIST,	NULL,		104, 28, 248, 160, 0, 0},
		{TCTL_BUTTON,	str_browse,	352, 29,  18,  17, 0, 0},
		{TCTL_BUTTON,	str_eject,	370, 29,  34,  17, 0, 0},
		{TCTL_BUTTON,	str_reset,    0,  0,   0,   0, 0, 0},
		{TCTL_BUTTON,	str_power,	  0,  0,   0,   0, 0, 0},
#if defined(VAEG_EXT)
		{TCTL_STATIC,	NULL,	      0,  0,   0,   0, 0, 0},
		{TCTL_STATIC,	NULL,	      0,  0,   0,   0, 0, 0},
#endif
};

#endif

// ----

static const char skintitle[] = "ToolWindow";

static const INITBL skinini1[] = {
	{"MAIN",		INITYPE_STR,	toolskin.main,	sizeof(toolskin.main)},
	{"FONT",		INITYPE_STR,	toolskin.font,	sizeof(toolskin.font)},
	{"FONTSIZE",	INITYPE_SINT32,	&toolskin.fontsize,					0},
	{"COLOR1",		INITYPE_HEX32,	&toolskin.color1,					0},
	{"COLOR2",		INITYPE_HEX32,	&toolskin.color2,					0},
#if defined(VAEG_EXT)
	{"XPARENT",		INITYPE_HEX32,	&toolskin.transparentcolor,			0},
#endif
	};

static const INITBL skinini2[] = {
	{"HDDACC",		INITYPE_ARGS16,	&subitem[IDC_TOOLHDDACC].posx,		5},
	{"FD1ACC",		INITYPE_ARGS16,	&subitem[IDC_TOOLFDD1ACC].posx,		5},
	{"FD1LIST",		INITYPE_ARGS16,	&subitem[IDC_TOOLFDD1LIST].posx,	5},
	{"FD1BROWSE",	INITYPE_ARGS16,	&subitem[IDC_TOOLFDD1BROWSE].posx,	5},
	{"FD1EJECT",	INITYPE_ARGS16,	&subitem[IDC_TOOLFDD1EJECT].posx,	5},
	{"FD2ACC",		INITYPE_ARGS16,	&subitem[IDC_TOOLFDD2ACC].posx,		5},
	{"FD2LIST",		INITYPE_ARGS16,	&subitem[IDC_TOOLFDD2LIST].posx,	5},
	{"FD2BROWSE",	INITYPE_ARGS16,	&subitem[IDC_TOOLFDD2BROWSE].posx,	5},
	{"FD2EJECT",	INITYPE_ARGS16,	&subitem[IDC_TOOLFDD2EJECT].posx,	5},
	{"RESETBTN",	INITYPE_ARGS16,	&subitem[IDC_TOOLRESET].posx,		5},
	{"POWERBTN",	INITYPE_ARGS16,	&subitem[IDC_TOOLPOWER].posx,		5},
#if defined(SUPPORT_PC88VA)
	{"V1MODE",		INITYPE_ARGS16,	&subitem[IDC_TOOLV1MODE].posx,		5},
	{"V2MODE",		INITYPE_ARGS16,	&subitem[IDC_TOOLV2MODE].posx,		5},
	{"V3MODE",		INITYPE_ARGS16,	&subitem[IDC_TOOLV3MODE].posx,		5},
#endif
	};


// static const DWORD mvccol[MVC_MAXCOLOR] = {
//						0xc0e8f8, 0xd8ecf4, 0x48a8c8, 0x000000};


// ----

static const char str_skindef[] = "<&Base Skin>";
static const char str_skinsel[] = "&Select Skin...";
static const char str_toolskin[] = "&Skins";
static const char str_toolclose[] = "&Close";

static const char skinui_title[] = "Select skin file";
static const char skinui_filter[] =								\
								"ini files (*.ini)\0*.ini\0"	\
								"text files (*.txt)\0*.txt\0"	\
								"All files (*.*)\0*.*\0";
static const char skinui_ext[] = "ini";
static const FILESEL skinui = {skinui_title, skinui_ext, skinui_filter, 1};

