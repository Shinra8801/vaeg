/*
 * MEMCTRLVA.C: PC-88VA memory control/memory switch
 */

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"
#include	"memoryva.h"
#include	"sgp.h"
#include	"va91.h"

#if defined(SUPPORT_PC88VA)

// ---- I/O

static void IOOUTCALL memctrlva_o152(UINT port, REG8 dat) {
	if (pccore.model_va == PCMODEL_VA1) {
		memoryva.rom0_bank = (dat & 0x0f);
		memoryva.rom1_bank = ((dat & 0xf0) >> 4);
	}
	else {
		memoryva.rom0_bank = ((dat & 0x40) >> 2) | (dat & 0x0f);
		memoryva.rom1_bank = ((dat & 0xb0) >> 4);
	}
	(void)port;
}

static void IOOUTCALL memctrlva_o153(UINT port, REG8 dat) {
	if ((dat & 0x0f) == 0x0f)
	TRACEOUT(("memctrlva: out %x %x %.4x:%.4x", port, dat, CPU_CS, CPU_IP));
	memoryva.sysm_bank = dat & 0x0f;
	if ((dat ^ gactrlva.gmsp) & 0x10) {
		// シングルプレーン⇔マルチプレーン 切り替え
		gactrlva_reset();
		if (dat & 0x10) {
			sgp_reset();
		}
	}
	gactrlva.gmsp = dat & 0x10;
	(void)port;
}


static REG8 IOINPCALL memctrlva_i152(UINT port) {
	(void)port;
	if (pccore.model_va == PCMODEL_VA1) {
		return (memoryva.rom0_bank & 0x0f) |
			   ((memoryva.rom1_bank & 0x0f) << 4);
	}
	else {
		return (memoryva.rom0_bank & 0x0f) |
			   ((memoryva.rom0_bank & 0x10) << 2) |
			   ((memoryva.rom1_bank & 0x0b) << 4);
	}
}

static REG8 IOINPCALL memctrlva_i153(UINT port) {
	(void)port;
	return (memoryva.sysm_bank & 0x0f) |
		   gactrlva.gmsp |
		   0x40;
}

static REG8 IOINPCALL memctrlva_i156(UINT port) {
	// ROMバンクステータス
	REG8 dat = 0xff;
	dat = ~(~dat | ~va91_rombankstatus());
	return dat;
}

static void IOOUTCALL memctrlva_o180(UINT port, REG8 dat) {
	memoryva.dma_sysm_bank = dat & 0x8f;
}

static REG8 IOINPCALL memctrlva_i180(UINT port) {
	return memoryva.dma_sysm_bank;
}

static void IOOUTCALL memctrlva_o198(UINT port, REG8 dat) {
	memoryva.backupmem_wp = 1;
	(void)port;
}

static void IOOUTCALL memctrlva_o19a(UINT port, REG8 dat) {
	memoryva.backupmem_wp = 0;
	(void)port;
}

static REG8 IOINPCALL memctrlva_i030(UINT port) {
	return backupmem[0x1fc2];
}

static REG8 IOINPCALL memctrlva_i031(UINT port) {
	return backupmem[0x1fc6];
}


// ---- I/F

void memctrlva_reset(void) {
	memctrlva_o152(0, 0);
	memctrlva_o153(0, 0x41);
	memctrlva_o198(0, 0);
}

void memctrlva_bind(void) {
	iocoreva_attachout(0x152, memctrlva_o152);
	iocoreva_attachout(0x153, memctrlva_o153);
	iocoreva_attachout(0x180, memctrlva_o180);
	iocoreva_attachout(0x198, memctrlva_o198);
	iocoreva_attachout(0x19a, memctrlva_o19a);

	iocoreva_attachinp(0x152, memctrlva_i152);
	iocoreva_attachinp(0x153, memctrlva_i153);
	iocoreva_attachinp(0x156, memctrlva_i156);
	iocoreva_attachinp(0x180, memctrlva_i180);

	iocoreva_attachinp(0x030, memctrlva_i030);
	iocoreva_attachinp(0x031, memctrlva_i031);
}

#endif
