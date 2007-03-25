/*
 * VA91.C: PC-88VA-91 Version-up board
 */


typedef struct {		// see, memoryva.x86 va91cfg_t
	BOOL	enabled;
} _VA91CFG, *VA91CFG;

typedef struct {		// see, memoryva.x86 va91_t
	UINT8		sysm_bank;
	UINT8		rom0_bank;
	UINT8		dmy0;
	UINT8		dmy1;
	_VA91CFG	cfg;
} _VA91, *VA91;

#ifdef __cplusplus
extern "C" {
#endif

extern	BYTE	va91rom0mem[0xa0000];
extern	BYTE	va91rom1mem[0x10000];
extern	BYTE	va91dicmem[0x80000];

extern	_VA91CFG	va91cfg;
extern	_VA91		va91;


REG8 va91_rombankstatus(void);
void va91_reset(void);
void va91_bind(void);
void va91_initialize(void);

#ifdef __cplusplus
}
#endif
