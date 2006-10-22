#include	"compiler.h"
#include	"resource.h"
#include	"viewer.h"
#include	"viewcmn.h"
#include	"viewmenu.h"

#define	MFCHECK(a) ((a)?MF_CHECKED:MF_UNCHECKED)


void viewmenu_mode(NP2VIEW_T *view) {

	HMENU	hmenu;

	hmenu = GetMenu(view->hwnd);
	CheckMenuItem(hmenu, IDM_VIEWMODEREG, MFCHECK(view->type == VIEWMODE_REG));
	CheckMenuItem(hmenu, IDM_VIEWMODESEG, MFCHECK(view->type == VIEWMODE_SEG));
	CheckMenuItem(hmenu, IDM_VIEWMODE1MB, MFCHECK(view->type == VIEWMODE_1MB));
	CheckMenuItem(hmenu, IDM_VIEWMODEASM, MFCHECK(view->type == VIEWMODE_ASM));
	CheckMenuItem(hmenu, IDM_VIEWMODESND, MFCHECK(view->type == VIEWMODE_SND));
#if defined(SUPPORT_PC88VA)
	CheckMenuItem(hmenu, IDM_VIEWMODEVABANK, MFCHECK(view->type == VIEWMODE_VABANK));
	CheckMenuItem(hmenu, IDM_VIEWMODEVIDEOVA, MFCHECK(view->type == VIEWMODE_VIDEOVA));
	CheckMenuItem(hmenu, IDM_VIEWMODEGACTRLVA, MFCHECK(view->type == VIEWMODE_GACTRLVA));
	CheckMenuItem(hmenu, IDM_VIEWMODESUBMEM, MFCHECK(view->type == VIEWMODE_SUBMEM));
	CheckMenuItem(hmenu, IDM_VIEWMODESUBREG, MFCHECK(view->type == VIEWMODE_SUBREG));
	CheckMenuItem(hmenu, IDM_VIEWMODESUBASM, MFCHECK(view->type == VIEWMODE_SUBASM));
#endif
}


void viewmenu_lock(NP2VIEW_T *view) {

	HMENU	hmenu;

	hmenu = GetMenu(view->hwnd);
	CheckMenuItem(hmenu, IDM_VIEWMODELOCK, MFCHECK(view->lock));
}
