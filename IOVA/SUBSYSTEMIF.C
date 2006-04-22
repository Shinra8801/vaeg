/*
 * SUBSYSTEMIF.C: PC-88VA FD Sub System Interface
 */

#include	"compiler.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"

#include	"i8255.h"
#include	"subsystem.h"
#include	"subsystemif.h"

#if defined(SUPPORT_PC88VA)

		_SUBSYSTEMIF subsystemif;
static	_I8255CFG i8255cfg;

// ---- 

static void IOOUTCALL subsystemif_o0fd(UINT port, REG8 dat) {
	i8255_outportb(&i8255cfg, dat);
}

static void IOOUTCALL subsystemif_o0fe(UINT port, REG8 dat) {
	i8255_outportc(&i8255cfg, dat);
}

static void IOOUTCALL subsystemif_o0ff(UINT port, REG8 dat) {
	i8255_outctrl(&i8255cfg, dat);
}

static REG8 IOINPCALL subsystemif_i0fc(UINT port) {
	return i8255_inporta(&i8255cfg);
}

static REG8 IOINPCALL subsystemif_i0fe(UINT port) {
	return i8255_inportc(&i8255cfg);
}

// ---- I/F (sub system)

void subsystemif_businporta(BYTE dat) {
	i8255_businporta(&i8255cfg, dat);
}

void subsystemif_businportc(BYTE dat) {
	i8255_businportc(&i8255cfg, (BYTE)(dat >> 4));
}


// ---- I/F

void subsystemif_initialize(void) {
	i8255_init(&i8255cfg, &subsystemif.i8255);
	i8255cfg.busoutportb = subsystem_businporta;
	i8255cfg.busoutportc = subsystem_businportc;
}

void subsystemif_reset(void) {
	i8255_reset(&i8255cfg);
}

void subsystemif_bind(void) {
	iocoreva_attachout(0x0fd, subsystemif_o0fd);
	iocoreva_attachout(0x0fe, subsystemif_o0fe);
	iocoreva_attachout(0x0ff, subsystemif_o0ff);

	iocoreva_attachinp(0x0fc, subsystemif_i0fc);
	iocoreva_attachinp(0x0fe, subsystemif_i0fe);
}

#endif
