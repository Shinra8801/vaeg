#ifdef __cplusplus
extern "C" {
#endif

extern	BYTE	textmem[0x40000];
extern	BYTE	fontmem[0x50000];
extern	BYTE	backupmem[0x04000];
extern	BYTE	dicmem[0x80000];
extern	BYTE	rom0mem[0xa0000];
extern	BYTE	rom1mem[0x20000];

extern	BYTE	backupmem_wp;
extern	BYTE	sysm_bank;
extern	BYTE	rom1_bank;
extern	BYTE	rom0_bank;
extern	BYTE	dma_sysm_bank;

#ifdef __cplusplus
}
#endif

