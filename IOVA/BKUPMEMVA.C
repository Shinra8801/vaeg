/*
 *	BKMEMVA.C: PC-88VA Backup memory
 *	ToDo:
 *		ファイル読み書き失敗時のエラー通知
 */


#include	"compiler.h"
#include	"dosio.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"memoryva.h"
#include	"bkupmemva.h"

#if defined(SUPPORT_PC88VA)


#define VABKUPMEM "vabkupmem.dat"

void bkupmemva_load(void) {
	char	path[MAX_PATH];
	FILEH	fh;
	BOOL	success;

	getbiospath(path, VABKUPMEM, sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, backupmem, 0x04000) == 0x04000);
		file_close(fh);
	}

}

void bkupmemva_save(void) {
	char	path[MAX_PATH];
	FILEH	fh;
	BOOL	success;

	getbiospath(path, VABKUPMEM, sizeof(path));
	fh = file_create(path);
	if (fh != FILEH_INVALID) {
		success = (file_write(fh, backupmem, 0x04000) == 0x04000);
		file_close(fh);
	}

}

#endif
