/*
 * GACTRLVA.C: PC-88VA GVRAM access control
 */

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"

#if defined(SUPPORT_PC88VA)

#define SETLOWBYTE(x, y) (x) = ( (x) & 0xff00 | (y) )
#define SETHIGHBYTE(x, y) (x) = ( (x) & 0x00ff | ((WORD)(y) << 8) )
#define LOWBYTE(x)  ( (x) & 0xff )
#define HIGHBYTE(x) ( (x) >> 8 )

// ---- I/O

static void IOOUTCALL gactrlva_o580(UINT port, REG8 dat) {
	gactrlva.s.writemode = dat & 0x98;
	TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}

static void IOOUTCALL gactrlva_o590(UINT port, REG8 dat) {
	int	i;

	i = (port >> 1) & 1;
	SETLOWBYTE(gactrlva.s.pattern[i], dat);
	TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}

static void IOOUTCALL gactrlva_o591(UINT port, REG8 dat) {
	int	i;

	i = (port >> 1) & 1;
	SETHIGHBYTE(gactrlva.s.pattern[i], dat);
	TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}

static void IOOUTCALL gactrlva_o5a0(UINT port, REG8 dat) {
	int	i;

	i = (port >> 1) & 1;
	SETLOWBYTE(gactrlva.s.rop[i], dat);
	TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}

/*
static void IOOUTCALL gactrlva_o5a1(UINT port, REG8 dat) {
	int	i;

	i = (port >> 1) & 1;
	SETHIGHBYTE(gactrlva.s.rop[i], dat);
	TRACEOUT(("gactrlva - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}
*/

// ---- I/F

void gactrlva_reset(void) {
	ZeroMemory(&gactrlva, sizeof(gactrlva));
}

void gactrlva_bind(void) {
	int i;

	iocoreva_attachout(0x580, gactrlva_o580);
	for (i = 0; i < 2; i++) {
		iocoreva_attachout(0x590 + i * 2, gactrlva_o590);
		iocoreva_attachout(0x591 + i * 2, gactrlva_o591);
		iocoreva_attachout(0x5a0 + i * 2, gactrlva_o5a0);
		//iocoreva_attachout(0x5a1 + i * 2, gactrlva_o5a1);
	}
}

#endif
