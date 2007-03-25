/*
 * VA91.C: PC-88VA-91 Version-up board
 */

#include	"compiler.h"
#include	"dosio.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"

#include	"va91.h"

#if defined(SUPPORT_PC88VA)

#define VUROM00ROM "VUROM00.ROM"
#define VUROM08ROM "VUROM08.ROM"
#define VUROM1ROM  "VUROM1.ROM"
#define VUDICROM   "VUDIC.ROM"

// ---- I/O

static void IOOUTCALL va91_off2(UINT port, REG8 dat) {
	if (dat>=0x10)
	TRACEOUT(("va91: out %x %x %.4x:%.4x", port, dat, CPU_CS, CPU_IP));
	va91.rom0_bank = dat & 0x0f;
}

static void IOOUTCALL va91_off3(UINT port, REG8 dat) {
	if (dat!=9 && dat!=0x0c && dat!=0x0d)
	TRACEOUT(("va91: out %x %x %.4x:%.4x", port, dat, CPU_CS, CPU_IP));
	va91.sysm_bank = dat & 0x0f;
}

static REG8 IOINPCALL va91_iff2(UINT port) {
	return va91.rom0_bank & 0x0f;
}

static REG8 IOINPCALL va91_iff3(UINT port) {
	return va91.sysm_bank & 0x0f;
}

REG8 va91_rombankstatus(void) {
//	return 0xff;
	return (va91.cfg.enabled) ? 0x7f : 0xff;
}

// ---- I/F


void va91_reset(void) {
	// ダイアログで設定した内容を動作環境に反映する
	// VA-EGリセット時に呼ばれる(STATSAVEのロード時は呼ばれない)
	va91.cfg = va91cfg;

	// バンクをリセット
	va91_off2(0, 0);
	va91_off3(0, 0);
}

void va91_bind(void) {
	if (va91.cfg.enabled) {
		iocoreva_attachout(0xff2, va91_off2);
		iocoreva_attachout(0xff3, va91_off3);

		iocoreva_attachinp(0xff2, va91_iff2);
		iocoreva_attachinp(0xff3, va91_iff3);
	}
}

void va91_initialize(void) {
	char	path[MAX_PATH];
	FILEH	fh;
	BOOL	success;

	if (!va91.cfg.enabled) return;

	getbiospath(path, VUDICROM, sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, va91dicmem, 0x80000) == 0x80000);
		file_close(fh);
	}

	getbiospath(path, VUROM00ROM, sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, va91rom0mem, 0x80000) == 0x80000);
		file_close(fh);
	}

	getbiospath(path, VUROM08ROM, sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, va91rom0mem + 0x80000, 0x20000) == 0x20000);
		file_close(fh);
	}

	getbiospath(path, VUROM1ROM, sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, va91rom1mem, 0x10000) == 0x10000);
		file_close(fh);
	}
}

#endif
