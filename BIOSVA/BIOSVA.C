/*
 *	BIOSVA.C: PC-88VA ROM control
 *	ToDo:
 *		ファイル読み込み失敗時のエラー通知
 */

#include	"compiler.h"
#include	"dosio.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"memoryva.h"
#include	"biosva.h"
#include	"subsystem.h"

#if defined(SUPPORT_PC88VA)

#define VAFONTROM "VAFONT.ROM"
#define VADICROM  "VADIC.ROM"
#define VAROM00ROM "VAROM00.ROM"
#define VAROM08ROM "VAROM08.ROM"
#define VAROM1ROM  "VAROM1.ROM"
#define VASUBSYSROM "VASUBSYS.ROM"

void biosva_initialize(void) {
	char	path[MAX_PATH];
	FILEH	fh;
	BOOL	success;

	memoryva.rom0exist = 0;
	memoryva.rom1exist = 0;
	memoryva.sysmromexist = 0;
	subsystem.romexist = FALSE;

	getbiospath(path, VAFONTROM, sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, fontmem, 0x50000) == 0x50000);
		if (success) memoryva.sysmromexist |= 0x300;	// bank 8,9
		file_close(fh);
	}

	getbiospath(path, VADICROM, sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, dicmem, 0x80000) == 0x80000);
		if (success) memoryva.sysmromexist |= 0x3000;	// bank C,D
		file_close(fh);
	}

	getbiospath(path, VAROM00ROM, sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, rom0mem, 0x80000) == 0x80000);
		if (success) memoryva.rom0exist |= 0xff;		// bank 0-7
		file_close(fh);
	}

	getbiospath(path, VAROM08ROM, sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, rom0mem + 0x80000, 0x20000) == 0x20000);
		if (success) memoryva.rom0exist |= 0x300;		// bank 8,9
		file_close(fh);
	}

	getbiospath(path, VAROM1ROM, sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, rom1mem, 0x20000) == 0x20000);
		if (success) memoryva.rom1exist |= 0x03;		// bank 0,1
		file_close(fh);
	}

	getbiospath(path, VASUBSYSROM, sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, subsystem.rom, 0x2000) == 0x2000);
		if (success) subsystem.romexist = TRUE;
		file_close(fh);
	}


}

#endif
