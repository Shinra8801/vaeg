/*
 * MOUSEIFVA.C: PC-88VA Mouse interface
 */

#include	"compiler.h"
#include	"pccore.h"
#include	"iocore.h"

#include	"iocoreva.h"

// ---- I/O

static void IOOUTCALL mouseifva_o1a8(UINT port, REG8 dat) {
	UINT8	xminten;

	mouseif.timing = dat & 3;
	xminten = (~dat >> 3) & 0x10;			// Š„‚è‚İ‹–‰Âƒtƒ‰ƒO
											// 0.‹–‰Â 1.‹Ö~

	if ((xminten ^ mouseif.upd8255.portc) & 0x10) {
		// •ÏX‚ ‚è
		if (!(xminten & 0x10)) {
			if (!nevent_iswork(NEVENT_MOUSE)) {
				nevent_set(NEVENT_MOUSE, mouseif.intrclock << mouseif.timing,
												mouseint, NEVENT_ABSOLUTE);
			}
		}
		mouseif.upd8255.portc = mouseif.upd8255.portc & ~0x10 | xminten;
	}
}

// ---- I/F

void mouseifva_reset(void) {

}

void mouseifva_bind(void) {

	iocoreva_attachout(0x1a8, mouseifva_o1a8);
}
