#if defined(VAEG_EXT)

typedef struct {
	int		type;
	union {
		UINT32		uint32;
		int			reg;
	} p;
} BREAKVAL;

typedef struct STRUCT_BREAKCOND {
	int		type;
	union {
		struct {
			struct STRUCT_BREAKCOND *cond1;
			struct STRUCT_BREAKCOND *cond2;
		} logexp;
		struct {
			BREAKVAL val1;
			BREAKVAL val2;
		} priexp;
	} p;
} BREAKCOND;


typedef struct {
	BOOL		enabled;
	int			type;
	BREAKCOND	*cond;
	union {
		UINT8		intnum;
		UINT16		port;			// BREAKPOINT_IOIN
		UINT32		addr;			// BREAKPOINT_MEMWRITE
	} p;
} BREAKPOINT;

enum {
	BREAKCOND_EQUAL			= 0,
};

enum {
	BREAKPOINT_INVALID		= 0,	// 無効なブレークポイント
	BREAKPOINT_STEP			= 0x0001,
	BREAKPOINT_INTERRUPT	= 0x0002,
	BREAKPOINT_IOIN			= 0x0004,
	BREAKPOINT_MEMWRITE		= 0x0009,
};

enum {
	BREAKVAL_UINT32			= 0,
	BREAKVAL_REG			= 1,
};

enum {
	BREAKREG_BYTEREG_BASE	= 0,
	BREAKREG_AL				= 0,
	BREAKREG_AH				= 1,
	BREAKREG_CL				= 2,
	BREAKREG_CH				= 3,
	BREAKREG_DL				= 4,
	BREAKREG_DH				= 5,
	BREAKREG_BL				= 6,
	BREAKREG_BH				= 7,
	BREAKREG_BYTEREG_MAX	= BREAKREG_BH,

	BREAKREG_WORDREG_BASE	= 8,
	BREAKREG_AX				= 8,
	BREAKREG_CX				= 9,
	BREAKREG_DX				= 10,
	BREAKREG_BX				= 11,
	BREAKREG_SP				= 12,
	BREAKREG_BP				= 13,
	BREAKREG_SI				= 14,
	BREAKREG_DI				= 15,
	BREAKREG_ES				= 16,
	BREAKREG_CS				= 17,
	BREAKREG_SS				= 18,
	BREAKREG_DS				= 19,
	BREAKREG_FLAG			= 20,
	BREAKREG_IP				= 21,
	BREAKREG_WORDREG_MAX	= BREAKREG_IP,

	BREAKREG_CSIP			= 22,
};

#ifdef __cplusplus
extern "C" {
#endif

BREAKPOINT *breakpoint_get(int index);
void breakpoint_clear(int index);

void breakpoint_check_setenabled(BOOL enabled);
BOOL breakpoint_check_step(void);
BOOL breakpoint_check_ioin(UINT port);
BOOL breakpoint_check_memwrite(UINT32 addr);
int breakpoint_parse(const char *str, BREAKPOINT *bp);

#ifdef __cplusplus
}
#endif



#endif
