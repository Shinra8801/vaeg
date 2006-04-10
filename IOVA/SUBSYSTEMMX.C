/*
 * SUBSYSTEMMX.C: PC-88VA FD Sub System (multiplexer)
 *
 */

#include	"compiler.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"
#include	"subsystemif.h"
#include	"fdsubsys.h"
#include	"subsystemmx.h"

#if defined(SUPPORT_PC88VA)

	_SUBSYSTEMMXCFG subsystemmxcfg = {1};

// ---- I/F

void subsystemmx_initialize(void) {
	if (subsystemmxcfg.mockup) {
	}
	else {
		subsystemif_initialize();
	}
}

void subsystemmx_reset(void) {
	if (subsystemmxcfg.mockup) {
		fdsubsys_reset();
	}
	else {
		subsystemif_reset();
	}
}

void subsystemmx_bind(void) {
	if (subsystemmxcfg.mockup) {
		fdsubsys_bind();
	}
	else {
		subsystemif_bind();
	}
}

#endif
