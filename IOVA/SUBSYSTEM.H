/*
 * SUBSYSTEM.H: PC-88VA FD Sub System
 */


#ifdef __cplusplus
extern "C" {
#endif

void subsystem_initialize(void);
void subsystem_exec(void);
BYTE subsystem_readmem(WORD addr);
struct Z80Reg *subsystem_getcpuregs(void);
BYTE *subsystem_getrombuf(void);
WORD subsystem_disassemble(WORD pc, char *str);

void subsystem_businporta(BYTE dat);
void subsystem_businportc(BYTE dat);


void subsystem_reset(void);
void subsystem_irq(BOOL irq);


#ifdef __cplusplus
}
#endif

