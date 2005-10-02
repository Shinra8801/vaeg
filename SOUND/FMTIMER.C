#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"sound.h"
#include	"fmboard.h"

static const UINT8 irqtable[4] = {0x03, 0x0d, 0x0a, 0x0c};

#if defined(VAEG_FIX)
static void set_fmtimeraevent(BOOL absolute);
static void set_fmtimerbevent(BOOL absolute);
#endif

void fmport_a(NEVENTITEM item) {

	BOOL	intreq = FALSE;

	if (item->flag & NEVENT_SETEVENT) {
		intreq = pcm86gen_intrq();
		//if (intreq) TRACEOUT(("fmtimer: int-A (pcm86gen)"));
		if (fmtimer.reg & 0x04) {
			fmtimer.status |= 0x01;
			intreq = TRUE;
		}
		if (intreq) {
//			pcm86.write = 1;
#if defined(SUPPORT_PC88VA)
			if (!fmboard_getintmask()) {
				//TRACEOUT(("fmtimer: int-A"));
				pic_setirq(fmtimer.irq);
			}
			else {
				//TRACEOUT(("fmtimer: int-A (masked)"));
			}
#else
			pic_setirq(fmtimer.irq);
#endif
			//TRACEOUT(("fm int-A"));
		}
//		TRACE_("A: fifo = ", pcm86.fifo);
//		TRACE_("A: virbuf = ", pcm86.virbuf);
//		TRACE_("A: fifosize = ", pcm86.fifosize);

#if defined(VAEG_FIX)
		set_fmtimeraevent(FALSE);
			// 直前のイベント発生を基点に次のイベント発生時刻を指定するため、
			// absoluteにはFALSEを指定
#endif
	
	}
}

void fmport_b(NEVENTITEM item) {

	BOOL	intreq = FALSE;

	if (item->flag & NEVENT_SETEVENT) {
		intreq = pcm86gen_intrq();
		//if (intreq) TRACEOUT(("fmtimer: int-B (pcm86gen)"));
		if (fmtimer.reg & 0x08) {
			fmtimer.status |= 0x02;
			intreq = TRUE;
		}
#if 0
		if (pcm86.fifo & 0x20) {
			sound_sync();
			if (pcm86.virbuf <= pcm86.fifosize) {
				intreq = TRUE;
			}
		}
#endif
		if (intreq) {
//			pcm86.write = 1;
#if defined(SUPPORT_PC88VA)
			if (!fmboard_getintmask()) {
				pic_setirq(fmtimer.irq);
				//TRACEOUT(("fmtimer: int-B"));
			}
			else {
				//TRACEOUT(("fmtimer: int-B (masked)"));
			}
#else
			pic_setirq(fmtimer.irq);
#endif
			//TRACEOUT(("fm int-B"));
		}
//		TRACE_("B: fifo = ", pcm86.fifo);
//		TRACE_("B: virbuf = ", pcm86.virbuf);
//		TRACE_("B: fifosize = ", pcm86.fifosize);

#if defined(VAEG_FIX)
		set_fmtimerbevent(FALSE);
			// 直前のイベント発生を基点に次のイベント発生時刻を指定するため、
			// absoluteにはFALSEを指定
#endif

	}
}

static void set_fmtimeraevent(BOOL absolute) {

	SINT32	l;

	l = 18 * (1024 - fmtimer.timera);
#if defined(SUPPORT_PC88VA)
	if (pccore.cpumode & CPUMODE_BASE4MHZ) {	// ベースクロック4MHz
		l = (l * 1248 * 2 / 625) * pccore.multiple;
	}
	else
#endif
	if (pccore.cpumode & CPUMODE_8MHZ) {		// 4MHz
		l = (l * 1248 / 625) * pccore.multiple;
	}
	else {										// 5MHz
		l = (l * 1536 / 625) * pccore.multiple;
	}
	nevent_set(NEVENT_FMTIMERA, l, fmport_a, absolute);
	//TRACEOUT(("fmtimer: start timer-A"));
}

static void set_fmtimerbevent(BOOL absolute) {

	SINT32	l;

	l = 288 * (256 - fmtimer.timerb);
#if defined(SUPPORT_PC88VA)
	if (pccore.cpumode & CPUMODE_BASE4MHZ) {	// ベースクロック4MHz
		l = (l * 1248 * 2 / 625) * pccore.multiple;
	}
	else
#endif
	if (pccore.cpumode & CPUMODE_8MHZ) {		// 4MHz
		l = (l * 1248 / 625) * pccore.multiple;
	}
	else {										// 5MHz
		l = (l * 1536 / 625) * pccore.multiple;
	}
	nevent_set(NEVENT_FMTIMERB, l, fmport_b, absolute);
	//TRACEOUT(("fmtimer: start timer-B"));
}

void fmtimer_reset(UINT irq) {

	ZeroMemory(&fmtimer, sizeof(fmtimer));
	fmtimer.intr = irq & 0xc0;
	fmtimer.intdisabel = irq & 0x10;
	fmtimer.irq = irqtable[irq >> 6];
//	pic_registext(fmtimer.irq);
}

void fmtimer_setreg(REG8 reg, REG8 value) {

//	TRACEOUT(("fm %x %x [%.4x:%.4x]", reg, value, CPU_CS, CPU_IP));

	switch(reg) {
		case 0x24:
			fmtimer.timera = (value << 2) + (fmtimer.timera & 3);
			//TRACEOUT(("fmtimer: o24 <- %02x: set timera(high): %.4x:%.4x", value, CPU_CS, CPU_IP));
			break;

		case 0x25:
			fmtimer.timera = (fmtimer.timera & 0x3fc) + (value & 3);
			//TRACEOUT(("fmtimer: o25 <- %02x: set timera(low): %.4x:%.4x", value, CPU_CS, CPU_IP));
			break;

		case 0x26:
			fmtimer.timerb = value;
			//TRACEOUT(("fmtimer: o26 <- %02x: set timerb: %.4x:%.4x", value, CPU_CS, CPU_IP));
			break;

		case 0x27:
			//TRACEOUT(("fmtimer: o27 <- %02x: set ctrl: %.4x:%.4x", value, CPU_CS, CPU_IP));
			fmtimer.reg = value;
			//TRACEOUT(("fmtimer: o27: fmtimer.status(old) = %02x", fmtimer.status));
			fmtimer.status &= ~((value & 0x30) >> 4);
			//TRACEOUT(("fmtimer: o27: fmtimer.status(new) = %02x", fmtimer.status));
#if 0	// Shinra 挿入してみたが効果がなさそうなのでやっぱり削除
			if (!(fmtimer.status & 0x03)) {
				pic_resetirq(fmtimer.irq);
			}
#endif
			if (value & 0x01) {
				if (!nevent_iswork(NEVENT_FMTIMERA)) {
					set_fmtimeraevent(NEVENT_ABSOLUTE);
				}
			}
			else {
				nevent_reset(NEVENT_FMTIMERA);
				//TRACEOUT(("fmtimer: stop timerA"));
			}
			if (value & 0x02) {
				if (!nevent_iswork(NEVENT_FMTIMERB)) {
					set_fmtimerbevent(NEVENT_ABSOLUTE);
				}
			}
			else {
				nevent_reset(NEVENT_FMTIMERB);
				//TRACEOUT(("fmtimer: stop timerB"));
			}
			if (!(value & 0x03)) {
				pic_resetirq(fmtimer.irq);
				//TRACEOUT(("fmtimer: o27: reset irq"));
			}
			break;
	}
}


#if defined(VAEG_FIX)
/*
	・タイマーが、オーバーフロー後停止していたのを修正し、
	  オーバーフローと同時にプリセット値をロードして動作再開するように変更。
*/
#endif
