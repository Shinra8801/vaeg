#define	NP2VIEW_MAX	8

#define	EXT	1		// Shinra

typedef struct {
	BYTE	vram;
	BYTE	itf;
	BYTE	A20;
#if defined(SUPPORT_BMS)
	BYTE	bmsio_bank;
#endif
#if defined(SUPPORT_PC88VA)
	BYTE	sysm_bank;
	BYTE	rom1_bank;
	BYTE	rom0_bank;
	BYTE	va91rom0_bank;
	BYTE	va91rom1_bank;
	BYTE	va91sysm_bank;
	BYTE	lock;
#endif
} VIEWMEM_T;

enum {
	VIEWMODE_REG = 0,
	VIEWMODE_SEG,
	VIEWMODE_1MB,
	VIEWMODE_ASM,
	VIEWMODE_SND,
#if defined(SUPPORT_PC88VA)
	VIEWMODE_VABANK,
	VIEWMODE_VIDEOVA,
	VIEWMODE_GACTRLVA,
	VIEWMODE_SUBMEM,
	VIEWMODE_SUBREG,
	VIEWMODE_SUBASM,
#endif
};

enum {
	ALLOCTYPE_NONE = 0,
	ALLOCTYPE_REG,
	ALLOCTYPE_SEG,
	ALLOCTYPE_1MB,
	ALLOCTYPE_ASM,
	ALLOCTYPE_SND,
#if defined(SUPPORT_PC88VA)
	ALLOCTYPE_VABANK,
	ALLOCTYPE_VIDEOVA,
	ALLOCTYPE_GACTRLVA,
	ALLOCTYPE_SUBMEM,
	ALLOCTYPE_SUBREG,
	ALLOCTYPE_SUBASM,
#endif

	ALLOCTYPE_ERROR = 0xffffffff
};

typedef struct {
	DWORD	type;
	DWORD	arg;
	DWORD	size;
	void	*ptr;
} VIEWMEMBUF;

// 各ウィンドウごとに持つデータ
typedef struct {
	HWND		hwnd;
	VIEWMEMBUF	buf1;
	VIEWMEMBUF	buf2;
	DWORD		pos;
	DWORD		maxline;
	WORD		step;
	WORD		mul;
	BYTE		alive;
	BYTE		type;
	BYTE		lock;
	BYTE		active;
	WORD		seg;
	WORD		off;
	VIEWMEM_T	dmem;
	SCROLLINFO	si;
#if EXT
	HWND		hDialogWnd;		// ダイアログのウィンドウ
#endif
} NP2VIEW_T;

extern	NP2VIEW_T	np2view[NP2VIEW_MAX];


BOOL viewer_init(HINSTANCE hPreInst);
void viewer_term(void);

void viewer_open(void);
void viewer_allclose(void);

void viewer_allreload(BOOL force);

