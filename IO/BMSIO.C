/*
 * BMSIO.C: I-O Bank Memory
 *
 * ToDo:
 *	バンク数のカスタマイズ
 */

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"

static char bmsio_initialized = 0;

// ---- internal

void bmsio_setnumbanks(UINT8 num) {
	if (bmsio.numbanks != num) {
		if (bmsio.bmsmem) {
			_MFREE(bmsio.bmsmem);
			bmsio.bmsmem = NULL;
		}
	}
	if (bmsio.bmsmem == NULL) {
		if (num > 0) {
			bmsio.bmsmem = (BYTE *)_MALLOC(((UINT32)num) * 0x20000, "BMSMEM");
			if (bmsio.bmsmem == NULL) {
				num = 0;
			}
		}
	}
	bmsio.numbanks = num;
}

// ---- I/O

static void IOOUTCALL bmsio_o00ec(UINT port, REG8 dat) {
	UINT8 bank;

	bank=dat;
	bmsio.bank=bank;
	if (bank<bmsio.numbanks)  {
		bmsio.addr = bmsio.bmsmem + (((UINT32)bank) << 17);
		bmsio.nomem=0;
	}
	else {
		bmsio.nomem=1;
	}
}

static REG8 IOINPCALL bmsio_i00ec(UINT port) {
	return bmsio.bank;
}

// ---- I/F

void bmsio_reset(void) {
	if (!bmsio_initialized) {
		ZeroMemory(&bmsio, sizeof(bmsio));
		bmsio_setnumbanks(8);
		bmsio_initialized = 1;
	}
	bmsio_o00ec(0,0);
}

void bmsio_bind(void) {
	iocore_attachout(0x00ec, bmsio_o00ec);
	iocore_attachinp(0x00ec, bmsio_i00ec);
}

