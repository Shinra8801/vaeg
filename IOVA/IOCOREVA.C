/*
 * IOCORE.C: PC-88VA I/O
 */

/*
	I/Oアドレスはすべて16bitデコード
*/

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"

#include	"iocoreva.h"

#if defined(SUPPORT_PC88VA)

enum {
//	IOFUNC_SYS	= 0x01,
//	IOFUNC_SND	= 0x02,
	IOFUNC_EXT	= 0x04
};

typedef struct {
	IOOUT	ioout[256];
	IOINP	ioinp[256];
	UINT	type;
	UINT	port;
} _IOFUNC, *IOFUNC;

typedef struct {
	IOFUNC		base[256];
	UINT		busclock;
	LISTARRAY	iotbl;
} _IOCORE, *IOCORE;

static	_IOCORE		iocore;

// ----

static void IOOUTCALL defout8(UINT port, REG8 dat) {

	TRACEOUT(("defout8 - %x %x %.4x:%.4x", port, dat, CPU_CS, CPU_IP));
}

static REG8 IOINPCALL definp8(UINT port) {

	TRACEOUT(("definp8 - %x %.4x:%.4x", port, CPU_CS, CPU_IP));
	return(0xff);
}


static void attachout(IOFUNC iof, UINT port, IOOUT func) {

	if (func) {
		iof->ioout[port] = func;
	}
}

static void attachinp(IOFUNC iof, UINT port, IOINP func) {

	if (func) {
		iof->ioinp[port] = func;
	}
}


// ----

static IOFUNC getextiofunc(UINT port) {

	IOFUNC	iof;

	iof = iocore.base[(port >> 8) & 0xff];
	if (!(iof->type & IOFUNC_EXT)) {
		iof = (IOFUNC)listarray_append(iocore.iotbl, iof);	//iofのコピーを作成し、それをリストに追加
															//作成したコピーを返す。
		if (iof != NULL) {
			iocore.base[(port >> 8) & 0xff] = iof;
			iof->type |= IOFUNC_EXT;
			iof->port = port & 0xff00;
		}
	}
	return(iof);
}

BOOL iocoreva_attachout(UINT port, IOOUT func) {

	IOFUNC	iof;

	iof = getextiofunc(port);
	if (iof) {
		attachout(iof, port & 0xff, func);
		return(SUCCESS);
	}
	else {
		return(FAILURE);
	}
}

BOOL iocoreva_attachinp(UINT port, IOINP func) {

	IOFUNC	iof;

	iof = getextiofunc(port);
	if (iof) {
		attachinp(iof, port & 0xff, func);
		return(SUCCESS);
	}
	else {
		return(FAILURE);
	}
}



// ----

void iocoreva_create(void) {
	ZeroMemory(&iocore, sizeof(iocore));
}

void iocoreva_destroy(void) {

	IOCORE		ioc;

	ioc = &iocore;
	listarray_destroy(ioc->iotbl);
	ioc->iotbl = NULL;
}


BOOL iocoreva_build(void) {

	IOCORE		ioc;
	IOFUNC		cmn;
	int			i;
	LISTARRAY	iotbl;

	ioc = &iocore;
	listarray_destroy(ioc->iotbl);
	iotbl = listarray_new(sizeof(_IOFUNC), 32);		//_IOFUNCのリストを作成。リストの要素はナシ。
	ioc->iotbl = iotbl;
	if (iotbl == NULL) {
		goto icbld_err;
	}
	cmn = (IOFUNC)listarray_append(iotbl, NULL);	//_IOFUNCに新しい要素を作成、その要素を返す。
	if (cmn == NULL) {
		goto icbld_err;
	}
	cmn->type = 0;
	for (i=0; i<256; i++) {
		cmn->ioout[i] = defout8;
		cmn->ioinp[i] = definp8;
	}
	for (i=0; i<256; i++) {
		ioc->base[i] = cmn;
	}
	return(SUCCESS);

icbld_err:
	return(FAILURE);
}


//IOCORE.Cを使用
//void iocore_cb(const IOCBFN *cbfn, UINT count)
//void iocore_reset(void)

void iocoreva_bind(void) {
	iocore.busclock = pccore.multiple;
	//iocore_cb(bindfn, sizeof(bindfn)/sizeof(IOCBFN));
	//↑iocore_bindに任す
}


void IOOUTCALL iocoreva_out8(UINT port, REG8 dat) {

	IOFUNC	iof;

//	TRACEOUT(("iocoreva_out8(%.4x, %.2x)", port, dat));
	CPU_REMCLOCK -= iocore.busclock;
	iof = iocore.base[(port >> 8) & 0xff];
	iof->ioout[port & 0xff](port, dat);
}

REG8 IOINPCALL iocoreva_inp8(UINT port) {

	IOFUNC	iof;
	REG8	ret;

#if defined(VAEG_EXT)
	pccore_debugioin(FALSE, port);
#endif

	CPU_REMCLOCK -= iocore.busclock;
	iof = iocore.base[(port >> 8) & 0xff];
	ret = iof->ioinp[port & 0xff](port);
//	TRACEOUT(("iocoreva_inp8(%.4x) -> %.2x", port, ret));
	return(ret);
}

void IOOUTCALL iocoreva_out16(UINT port, REG16 dat) {

	IOFUNC	iof;

//	TRACEOUT(("iocoreva_out16(%.4x, %.4x)", port, dat));
	CPU_REMCLOCK -= iocore.busclock;
	iof = iocore.base[(port >> 8) & 0xff];
	iof->ioout[port & 0xff](port, (UINT8)dat);
	port++;
	iof = iocore.base[(port >> 8) & 0xff];
	iof->ioout[port & 0xff](port, (UINT8)(dat >> 8));
}

REG16 IOINPCALL iocoreva_inp16(UINT port) {

	IOFUNC	iof;
	REG16	ret;

#if defined(VAEG_EXT)
	pccore_debugioin(TRUE, port);
#endif

	CPU_REMCLOCK -= iocore.busclock;

	iof = iocore.base[(port >> 8) & 0xff];
	ret = iof->ioinp[port & 0xff](port);
	port++;
	iof = iocore.base[(port >> 8) & 0xff];
	ret = ((UINT16)((iof->ioinp[port & 0xff](port) << 8) + ret));
//	TRACEOUT(("iocoreva_inp16(%.4x) -> %.4x", port-1, ret));
	return(ret);
}


#endif
