/*
 * MEMCTRLVA.C: PC-88VA memory control/memory switch
 */

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"
#include	"memoryva.h"

#if defined(SUPPORT_PC88VA)

// ---- I/O

static void IOOUTCALL memctrlva_o152(UINT port, REG8 dat) {
	rom0_bank = ((dat & 0x40) >> 2) | dat & 0x0f;
	rom1_bank = (dat & 0xb0) >> 4;
	(void)port;
}

static void IOOUTCALL memctrlva_o153(UINT port, REG8 dat) {
	sysm_bank = dat & 0x0f;
	(void)port;
}


static REG8 IOINPCALL memctrlva_i152(UINT port) {
	(void)port;
	return (rom0_bank & 0x0f) |
		   ((rom0_bank & 0x10) << 2) |
		   ((rom1_bank & 0x0b) << 4);
}

static REG8 IOINPCALL memctrlva_i153(UINT port) {
	(void)port;
	return (sysm_bank & 0x0f) |
		   0x40;
}


static void IOOUTCALL memctrlva_o198(UINT port, REG8 dat) {
	backupmem_wp = 1;
	(void)port;
}

static void IOOUTCALL memctrlva_o19a(UINT port, REG8 dat) {
	backupmem_wp = 0;
	(void)port;
}

static REG8 IOINPCALL memctrlva_i030(UINT port) {
	return backupmem[0x1fc2];
}

static REG8 IOINPCALL memctrlva_i031(UINT port) {
	return backupmem[0x1fc6];
}

static REG8 IOINPCALL memctrlva_i150(UINT port) {
	// ToDo: VA2の場合、V1/V2切り替えスイッチがないため、
	//       この値が何で決まるのか定かでない。
	// ToDo: リセット時に取り込むのが正しい？
	if (backupmem[0x1fc6] & 0x80) {
		// V1
		return 0xfe;
	}
	else {
		// V2
		return 0xfd;
	}
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
	iocoreva_attachout(0x198, memctrlva_o198);
	iocoreva_attachout(0x19a, memctrlva_o19a);

	iocoreva_attachinp(0x152, memctrlva_i152);
	iocoreva_attachinp(0x153, memctrlva_i153);

	iocoreva_attachinp(0x030, memctrlva_i030);
	iocoreva_attachinp(0x031, memctrlva_i031);

	iocoreva_attachinp(0x150, memctrlva_i150);
}

#endif
