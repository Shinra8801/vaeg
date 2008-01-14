
#ifndef MEMCALL
#define	MEMCALL
#endif

typedef struct {		// see, memoryva.x86
		UINT8	sysm_bank;
		UINT8	rom0_bank;
		UINT8	rom1_bank;
		UINT8	dma_sysm_bank;
		UINT8	dma_access;
		UINT8	backupmem_wp;
		UINT8	dmy0;
		UINT8	dmy1;

		UINT32	rom0exist;
		UINT32	rom1exist;
		UINT32	sysmromexist;
} _MEMORYVA;

#ifdef __cplusplus
extern "C" {
#endif

extern	BYTE	textmem[0x40000];
extern	BYTE	fontmem[0x50000];
extern	BYTE	backupmem[0x04000];
extern	BYTE	dicmem[0x80000];
extern	BYTE	rom0mem[0xa0000];
extern	BYTE	rom1mem[0x20000];

//extern	BYTE	backupmem_wp;
//extern	BYTE	sysm_bank;
//extern	BYTE	rom1_bank;
//extern	BYTE	rom0_bank;
//extern	BYTE	dma_sysm_bank;

extern	_MEMORYVA	memoryva;
extern	BOOL	textmem_dirty;

void MEMCALL i286_memorymap_va(void);

#ifdef __cplusplus
}
#endif

