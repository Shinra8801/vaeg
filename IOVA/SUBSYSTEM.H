/*
 * SUBSYSTEM.H: PC-88VA FD Sub System
 */

#include "i8255.h"

typedef struct {
	BYTE	romexist;		// ROMをロードした
	BYTE	intopcode;		// 割り込みオペコード
	_I8255	i8255;
	BYTE	rom[0x2000];
	BYTE	ram[0x4000];
} _SUBSYSTEM, *SUBSYSTEM;


#ifdef __cplusplus
extern "C" {
#endif

extern	_SUBSYSTEM subsystem;

void subsystem_initialize(void);
void subsystem_exec(void);
BYTE subsystem_readmem(WORD addr);
const struct Z80Reg *subsystem_getcpureg(void);
WORD subsystem_disassemble(WORD pc, char *str);
UINT subsystem_getcpustatussize(void);
void subsystem_savecpustatus(UINT8 *buf);
void subsystem_loadcpustatus(const UINT8 *buf);

void subsystem_businporta(BYTE dat);
void subsystem_businportc(BYTE dat);


void subsystem_reset(void);
void subsystem_irq(BOOL irq);


#ifdef __cplusplus
}
#endif

