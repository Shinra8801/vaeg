/*
 *	BOARDSB2.C: PC-88VA Sound board 2
 */

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"
#include	"cbuscore.h"
#include	"boardsb2.h"
#include	"sound.h"
#include	"fmboard.h"
#include	"s98.h"

#if defined(SUPPORT_PC88VA)

static unsigned int wait = 160;		// = 20μs ToDo: ウェイト入れすぎ ? 
//static unsigned int wait = 0;			// 

static void IOOUTCALL sb2_o044(UINT port, REG8 dat) {

//	CPU_REMCLOCK -= wait;

	opn.opnreg = dat;
	//TRACEOUT(("sb2: o044 <- %02x %.4x:%.4x", dat, CPU_CS, CPU_IP));
}

static void IOOUTCALL sb2_o045(UINT port, REG8 dat) {

	//TRACEOUT(("sb2: o045 (%02x) <- %02x %.4x:%.4x", opn.opnreg, dat, CPU_CS, CPU_IP));
	S98_put(NORMAL2608, opn.opnreg, dat);
	if (opn.opnreg < 0x10) {
		if (opn.opnreg != 0x0e) {
			psggen_setreg(&psg1, opn.opnreg, dat);
		}
	}
	else {
		if (opn.opnreg < 0x20) {
			rhythm_setreg(&rhythm, opn.opnreg, dat);
		}
		else if (opn.opnreg < 0x30) {
			if (opn.opnreg == 0x28) {
				if ((dat & 0x0f) < 3) {
					opngen_keyon(dat & 0x0f, dat);
				}
				else if (((dat & 0x0f) != 3) &&
						((dat & 0x0f) < 7)) {
					opngen_keyon((dat & 0x0f) - 1, dat);
				}
			}
			else {
				fmtimer_setreg(opn.opnreg, dat);
				if (opn.opnreg == 0x27) {
					opnch[2].extop = dat & 0xc0;
				}
			}
		}
		else if (opn.opnreg < 0xc0) {
			opngen_setreg(0, opn.opnreg, dat);
		}
		opn.reg[opn.opnreg] = dat;
	}
	(void)port;

	CPU_REMCLOCK -= wait;
}

static void IOOUTCALL sb2_o046(UINT port, REG8 dat) {

//	CPU_REMCLOCK -= wait;

	opn.extreg = dat;
	(void)port;
}

static void IOOUTCALL sb2_o047(UINT port, REG8 dat) {

	//TRACEOUT(("sb2: o047 (%02x) <- %02x %.4x:%.4x", opn.extreg, dat, CPU_CS, CPU_IP));
	S98_put(EXTEND2608, opn.extreg, dat);
	opn.reg[opn.extreg + 0x100] = dat;
	if (opn.extreg >= 0x30) {
		opngen_setreg(3, opn.extreg, dat);
	}
	else if (opn.extreg < 0x12) {
		adpcm_setreg(&adpcm, opn.extreg, dat);
	}
	(void)port;

	CPU_REMCLOCK -= wait;
}

static REG8 IOINPCALL sb2_i044(UINT port) {

	(void)port;
	return((fmtimer.status & 3) | adpcm_status(&adpcm));
}

static REG8 IOINPCALL sb2_i045(UINT port) {
	REG8 dat;

	if (opn.opnreg == 0x0e) {
		dat = fmboard_getjoy(&psg1) | 0xf0;
	}
	else if (opn.opnreg == 0x0f) {						// 88VA固有
		dat = (fmboard_getjoy(&psg1) >> 4) | 0xfc;
	}
	else if (opn.opnreg < 0x10) {
		dat = psggen_getreg(&psg1, opn.opnreg);
	}
	else {
		dat = opn.reg[opn.opnreg];
	}
	//TRACEOUT(("sb2: i045 (%02x) -> %02x %.4x:%.4x", opn.opnreg, dat, CPU_CS, CPU_IP));
	return(dat);
}

static REG8 IOINPCALL sb2_i047(UINT port) {

	if (opn.extreg == 0x08) {
		return(adpcm_readsample(&adpcm));
	}
	(void)port;
	return(opn.reg[opn.opnreg]);
}



// ----


void boardsb2_reset(void) {

	psggen_setreg(&psg1, 0x07, 0);			
											/* 88VA固有
											   psggen_reset(fmboard_resetから呼ばれる)
											   で初期値を0xbfにしている。
											   VAの場合、0のようだ 
											*/
	fmtimer_reset(0xc0);
	opn.channels = 6;
	opngen_setcfg(6, OPN_STEREO | 0x03f);
}

void boardsb2_bind(void) {

	fmboard_fmrestore(0, 0);
	fmboard_fmrestore(3, 1);
	psggen_restore(&psg1);
	fmboard_rhyrestore(&rhythm, 0);
	sound_streamregist(&opngen, (SOUNDCB)opngen_getpcmvr);
	sound_streamregist(&psg1, (SOUNDCB)psggen_getpcm);
	rhythm_bind(&rhythm);
	sound_streamregist(&adpcm, (SOUNDCB)adpcm_getpcm);

	iocoreva_attachinp(0x044, sb2_i044);
	iocoreva_attachinp(0x045, sb2_i045);
	iocoreva_attachinp(0x046, sb2_i044);
	iocoreva_attachinp(0x047, sb2_i047);
	iocoreva_attachout(0x044, sb2_o044);
	iocoreva_attachout(0x045, sb2_o045);
	iocoreva_attachout(0x046, sb2_o046);
	iocoreva_attachout(0x047, sb2_o047);
}




#endif
