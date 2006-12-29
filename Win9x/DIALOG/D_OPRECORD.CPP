#include	"compiler.h"

#if defined(SUPPORT_OPRECORD)

#include	"dosio.h"

#include	"np2.h"
#include	"dialog.h"
#include	"dialogs.h"
#include	"oprecord.h"

#include	"statsave.h"
#include	"sysmng.h"
#include	"soundmng.h"
#include	"toolwin.h"
#include	"fddfile.h"

static const char *oprfilter =
 		"Operation record files (*.OPR)\0"
			"*.opr\0"
		"All files (*.*)\0"
			"*.*\0";
static const char *oprext = "opr";

static const FILESEL oprrecui = {
	"Enter operation record file name",
	oprext,
	oprfilter,
	1
};

static const FILESEL oprplayui = {
	"Select operation record file",
	oprext,
	oprfilter,
	1
};


		char oprfolder[MAX_PATH];

/*
void dialog_recoprecord(HWND hWnd) {
	char	path[MAX_PATH];

	file_cpyname(path, modulefile, sizeof(path));
	file_cutname(path);
	file_catname(path, "newopr", sizeof(path));
	if (dlgs_selectwritefile(hWnd, &oprrecui, path, sizeof(path))) {
		int		ret;
		char	statpath[MAX_PATH];

		file_cpyname(oprfolder, path, sizeof(oprfolder));

		file_cpyname(statpath, path, sizeof(statpath));
		file_cutext(statpath);
		file_catname(statpath, ".ops", sizeof(statpath));
		soundmng_stop();
		ret = statsave_save(statpath);
		soundmng_play();
		if (ret) {
			file_delete(path);
		}
		else {
			oprecord_start_record(path);
		}
	}
}
*/

BOOL dialog_oprec(HWND hWnd, char *path, UINT size) {
	file_cpyname(path, oprfolder, size);
	file_cutname(path);
	file_catname(path, "newopr", size);
	if (dlgs_selectwritefile(hWnd, &oprrecui, path, size)) {
		file_cpyname(oprfolder, path, sizeof(oprfolder));
		return TRUE;
	}
	return FALSE;
}


/*
void dialog_playoprecord(HWND hWnd) {
	char	path[MAX_PATH];
	int		ret;
	int		id;
	char	buf[1024];
	BOOL	force = TRUE;
	const char	*title = "Status Load";

	file_cpyname(path, oprfolder, sizeof(path));
	if (dlgs_selectfile(hWnd, &oprplayui, path, sizeof(path), NULL)) {
		char	statpath[MAX_PATH];

		file_cpyname(oprfolder, path, sizeof(oprfolder));

		file_cpyname(statpath, path, sizeof(statpath));
		file_cutext(statpath);
		file_catname(statpath, ".ops", sizeof(statpath));

		id = IDYES;
		ret = statsave_check(statpath, buf, sizeof(buf));
		if (ret & (~STATFLAG_DISKCHG)) {
			MessageBox(hWnd, "Couldn't restart", title, MB_OK | MB_ICONSTOP);
			id = IDNO;
		}
		else if ((!force) && (ret & STATFLAG_DISKCHG)) {
			char buf2[1024 + 256];
			wsprintf(buf2, "Conflict!\n\n%s\nContinue?", buf);
			id = MessageBox(hWnd, buf2, title,
										MB_YESNOCANCEL | MB_ICONQUESTION);
		}
		if (id == IDYES) {
			statsave_load(statpath);
			toolwin_setfdd(0, fdd_diskname(0));
			toolwin_setfdd(1, fdd_diskname(1));
		}
		sysmng_workclockreset();
		sysmng_updatecaption(1);

		oprecord_start_play(path);	
	}
}
*/

BOOL dialog_opplay(HWND hWnd, char *path, UINT size) {
	file_cpyname(path, oprfolder, size);
	if (dlgs_selectfile(hWnd, &oprplayui, path, size, NULL)) {
		file_cpyname(oprfolder, path, sizeof(oprfolder));
		return TRUE;
	}
	return FALSE;
}
#endif
