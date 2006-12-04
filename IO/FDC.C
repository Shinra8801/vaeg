//
// FDC μPD765A
//


#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"fddfile.h"

#if defined(VAEG_FIX)
#include	"sysmng.h"
#endif

#if defined(VAEG_EXT)
#include	"soundmng.h"
#endif

#if defined(SUPPORT_PC88VA)
#include	"iocoreva.h"
#include	"subsystem.h"
#endif

enum {
	FDC_DMACH2HD	= 2,
	FDC_DMACH2DD	= 3,

#if defined(VAEG_EXT)
	CLOCK80				= 7987200,		// 8MHz
	CLOCK48				= 4792320,		// 4.8MHz

	FDD_MOTORDELAY	= 505,	// FDDのモーターをONしてからreadyになるまでの時間(msec)

	FDD_MOTOR_STOPPED	= 0,
	FDD_MOTOR_STARTING	= 1,
	FDD_MOTOR_STABLE	= 2,

	FDD_HEAD_UNLOADED	= 0,
	FDD_HEAD_LOADING	= 1,
	FDD_HEAD_STABLE		= 2,
	FDD_HEAD_IDLE		= 3,

	FDD_HEADREACH_IDLE		  = 0,		// ヘッドがロードされていない
	FDD_HEADREACH_WAITINGLOAD = 1,		// ヘッドがロード完了するのを待っている
	FDD_HEADREACH_REACHING    = 2,		// ヘッドがロードされており、セクタに到達するのを待っている
	FDD_HEADREACH_REACHED     = 3,		// ヘッドがロードされており、目的のセクタに到達した
#endif

#if defined(SUPPORT_PC88VA)
	FDD_48TPI			= 0,
	FDD_96TPI			= 1,
#endif

};

static const UINT8 FDCCMD_TABLE[32] = {
						0, 0, 8, 2, 1, 8, 8, 1, 0, 8, 1, 0, 8, 5, 0, 2,
						0, 8, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 8, 0, 0};


#define FDC_FORCEREADY (1)
#define	FDC_DELAYERROR7

#if defined(VAEG_FIX) || defined(VAEG_EXT)
#define getnow() 	(CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK)

		// neventから呼び出されるコールバックルーチンで、
		// イベント発生時刻を知るにはgetnow_oneventを使う。
		// getnowはCPUの時刻なので、getnow_oneventより進んでいる。
#define getnow_onevent()	(CPU_CLOCK)
#endif

#if defined(VAEG_FIX)
static void start_executionphase(void);
static void stop_executionphase(void);
#endif


#if defined(VAEG_EXT)

static BOOL seek1sound = FALSE;	// 次にヘッドロード音を出すときは1シリンダシーク用の音を出す

// ----------------------------------------------------------------------
// head location management

/*
	直前のディスクアクセスがR==eot (これをそのトラックの最終セクタとみなす)で、
	次のFDCコマンドによるアクセス要求が、
	1. 同じシリンダのセクタ1の場合、
	   直前のディスクアクセス終了から
	   ポストアンプル+プリアンプル分の時間を待つ
	2. 次のシリンダのセクタ1の場合、
	   直前のディスクアクセス終了からポストアンプル+プリアンプル+1トラック分の時間を待つ
	3. その他のシリンダのセクタ1の場合、
	   直前のディスクアクセス終了から
	   ポストアンプル+プリアンプル+半トラック分の時間を待つ
	4. セクタ1以外の場合
	   半トラック分の時間を待つ	   

	直前のディスクアクセスから次のFDCコマンドによるアクセス要求までの間に
	ヘッドがunloadされた場合は、常に上記4の扱いにする。

	マルチトラックアクセス(1回のコマンドで最終セクタから次のトラックの先頭セクタをまたがって
	アクセス)は考慮しない。VAのFDD BIOSはこの機能を使っていないから。
*/

static void reached_sector(void) {
	fdc.reachlastus = fdc.us;
	fdc.reachlastC = fdc.C;
	fdc.reachlastR = fdc.R;
	fdc.reachlastN = fdc.N;
	fdc.reachlasteot = fdc.eot;
	fdc.reachlastclock = getnow();
}


static void update_headreach(void) {

	switch(fdc.reach) {
	case FDD_HEADREACH_REACHING:
		{
			UINT32 now;
			now = getnow();
			if (now - fdc.reachlastclock >= fdc.reachtime) {
				fdc.reach = FDD_HEADREACH_REACHED;
			}
		}
		break;
	case FDD_HEADREACH_WAITINGLOAD:
		if (fdc.head == FDD_HEAD_STABLE) {
			fdc.reach = FDD_HEADREACH_REACHING;
			fdc.reachlastclock = getnow();
		}
		break;
	}
}

static void want_sector(void) {
	if (fdc.reach == FDD_HEADREACH_REACHED &&
		fdc.us == fdc.reachlastus && 
		fdc.R == 1 && 
		fdc.reachlastR == fdc.reachlasteot) {
		// 前回からの連続アクセスで、
		// 前回と同じドライブで、
		// 前回はトラックの最後のセクタで(EOT==Rなら最後のセクタと仮定)、
		// 次にアクセスするのは最初のセクタで(セクタ1がトラック先頭と仮定)
		// ある場合。
		int datalen;
		if (fdc.reachlastN < 8) {
			datalen = 128 << fdc.reachlastN;
		}
		else {
			datalen = 128 << 8;
		}
		fdc.reachtime = fdc.rqminterval * datalen;
		if (fdc.C == fdc.reachlastC) {
			// 前回と同じシリンダ
			// プリアンプル、ポストアンプル分待つ
			fdc.reachtime += fdc.amptime;
		}
		else if (fdc.C == fdc.reachlastC + 1) {
			// 前回の次のシリンダ
			// 1トラック待つ
			fdc.reachtime += fdc.amptime + fdc.roundtime;
		}
		else {
			// 半トラック待つ
			fdc.reachtime += fdc.roundtime / 2;
		}
		fdc.reach = FDD_HEADREACH_REACHING;
	}
	else {
		// 無条件に半トラック待つ
		if (fdc.head == FDD_HEAD_STABLE) {
			// ヘッドロードが完了している
			fdc.reachlastclock = getnow();
			fdc.reach = FDD_HEADREACH_REACHING;
		}
		else {
			// ヘッドロードが完了していない
			fdc.reach = FDD_HEADREACH_WAITINGLOAD;
		}
		// 現在から、または、ヘッドロード完了から半トラック待つ
		fdc.reachtime = fdc.roundtime / 2;
	}
}


// ----------------------------------------------------------------------
// head status management

// TODO: FDC_ReadDataなどで、エラー検出時でも、hltの時間を待って
//       結果を返すようにする。

static void head_load_sound(void) {
	if (np2cfg.MOTOR) {
		// ヘッドロード音
		soundmng_pcmstop(SOUND_PCMHEADOFF);
		if (seek1sound) {
			soundmng_pcmstop(SOUND_PCMSEEK1);
			soundmng_pcmplay(SOUND_PCMSEEK1, FALSE);
			seek1sound = FALSE;
		}
		else {
			soundmng_pcmstop(SOUND_PCMHEADON);
			soundmng_pcmplay(SOUND_PCMHEADON, FALSE);
		}
	}
}

static void head_unload_sound(void) {
	if (np2cfg.MOTOR) {
		// ヘッドアンロード音
		soundmng_pcmstop(SOUND_PCMHEADOFF);
		soundmng_pcmplay(SOUND_PCMHEADOFF, FALSE);
	}
}

static void update_head(void) {
	SINT32 now;
	SINT32 d;

	now = getnow();

	switch (fdc.head) {
	case FDD_HEAD_LOADING:
		d = (SINT32)((UINT64)pccore.realclock * 16000 * fdc.hlt / fdc.clock);
		if (now - fdc.headlastclock > d) {
			fdc.head = FDD_HEAD_STABLE;
			fdc.headlastclock += d;
		}
		break;
	case FDD_HEAD_IDLE:
		d = (SINT32)((UINT64)pccore.realclock * 128000L * fdc.hut / fdc.clock);
		if (now - fdc.headlastclock > d) {
			fdc.head = FDD_HEAD_UNLOADED;
			fdc.headlastclock += d;

			fdc.reach = FDD_HEADREACH_IDLE;

			head_unload_sound();
		}
		break;
	}

}

static void activate_head(void) {
	SINT32 now;

	now = getnow();

	if (fdc.headlastactive == fdc.us) {
		switch (fdc.head) {
		case FDD_HEAD_UNLOADED:
			fdc.head = FDD_HEAD_LOADING;
			fdc.headlastclock = now;
			head_load_sound();
			break;
		case FDD_HEAD_IDLE:
			fdc.head = FDD_HEAD_STABLE;
			fdc.headlastclock = now;
			break;
		}
	}
	else {
		fdc.headlastactive = fdc.us;
		fdc.head = FDD_HEAD_LOADING;
		fdc.headlastclock = now;
		head_load_sound();
	}
}

static void deactivate_head(void) {
	SINT32 now;

	now = getnow();

	if (fdc.headlastactive == fdc.us) {
		switch (fdc.head) {
		case FDD_HEAD_STABLE:
			fdc.head = FDD_HEAD_IDLE;
			fdc.headlastclock = now;
			break;

		case FDD_HEAD_LOADING:
			fdc.head = FDD_HEAD_UNLOADED;
			fdc.headlastclock = now;

			fdc.reach = FDD_HEADREACH_IDLE;

			head_unload_sound();
			break;

		}
	}
	else {
		fdc.headlastactive = fdc.us;
		fdc.head = FDD_HEAD_UNLOADED;
		fdc.headlastclock = now;

		fdc.reach = FDD_HEADREACH_IDLE;

		head_unload_sound();
	}
}

static void unload_head_forcedly(void) {
	fdc.head = FDD_HEAD_UNLOADED;
	head_unload_sound();
}

// ----------------------------------------------------------------------
// seek management

static void fdc_stepwaitset(void) {
	UINT32 srttime;

	// SRTをFDCクロック数で表すと 8000 * (16 - fdc.srt);
	// それをCPUクロック数で表すと、8000 * (16 - fdc.srt) * (pccore.realclock/fdc.clock);
	srttime = (UINT32)((UINT64)pccore.realclock * 8000 * (16 - fdc.srt) / fdc.clock);

	nevent_set(NEVENT_FDCSTEPWAIT, srttime, fdc_stepwait, NEVENT_ABSOLUTE);
}

static void succeed_seek(int us) {
	fdc.us = us;
	fdc.ncn = fdc.headpcn[us];
	fdc.stat[fdc.us] = (fdc.hd << 2) | fdc.us;
	fdc.stat[fdc.us] |= FDCRLT_SE;
	fdd_seek();
	fdc_interrupt();
}

static void start_seek(int us, int ncn) {
	if (fdc.headpcn[us] == ncn) {
		succeed_seek(us);
	}
	else {
		fdc.headncn[us] = ncn;
		if (np2cfg.MOTOR) {
			if (fdc.head != FDD_HEAD_UNLOADED && 
				(ncn == fdc.headpcn[us] + 1 || ncn + 1 == fdc.headpcn[us])) {
				seek1sound = TRUE;
			}
			else {
				seek1sound = FALSE;
			}
			// シーク音
			soundmng_pcmstop(SOUND_PCMSEEK);
			soundmng_pcmplay(SOUND_PCMSEEK, TRUE);
		}
		unload_head_forcedly();
		fdc_stepwaitset();
	}
}

static UINT8 isseeking(void) {
	int us;
	UINT8 stat;

	stat = 0;
	for (us = 0; us < 4; us++) {
		if (fdc.headncn[us] != fdc.headpcn[us]) {
			stat |= (1 << us);
		}
	}
	return stat;
}

static UINT8 fdbusybits(void) {
	int us;
	UINT8 stat;

	stat = isseeking();
	for (us = 0; us < 4; us++) {
		if (fdc.stat[us] & FDCRLT_SE) {
			stat |= (1 << us);
		}
	}
	return stat;
}

void fdc_stepwait(NEVENTITEM item) {
	int us;

	for (us = 0; us < 4; us++) {
		if (fdc.headncn[us] != fdc.headpcn[us]) {
			if (fdc.headncn[us] < fdc.headpcn[us]) {
				fdc.headpcn[us]--;
			}
			else if (fdc.headncn[us] > fdc.headpcn[us]) {
				fdc.headpcn[us]++;
			}
			if (fdc.headncn[us] == fdc.headpcn[us]) {
				// 目的のシリンダに到達
				succeed_seek(us);

				if (np2cfg.MOTOR) {
					soundmng_pcmstop(SOUND_PCMSEEK);
				}
			}
		}
	}

	if (isseeking()) {
		fdc_stepwaitset();
	}
}


#endif

// ----------------------------------------------------------------------
// interrupt

#if defined(VAEG_FIX)
static void fdc_dointerrupt(void) {
		fdc.intreq = TRUE;
		TRACEOUT(("fdc: send interrupt request"));
#if defined(SUPPORT_PC88VA)
		if (fdc.fddifmode) {
			// DMA mode
			if (fdc.chgreg & 1) {
				pic_setirq(0x0b);
			}
			else {
				pic_setirq(0x0a);
			}
		}
		else {
			// Intelligent mode
			subsystem_irq(TRUE);
		}
#else
		if (fdc.chgreg & 1) {
			pic_setirq(0x0b);
		}
		else {
			pic_setirq(0x0a);
		}
#endif
}
#endif

void fdc_intwait(NEVENTITEM item) {

	if (item->flag & NEVENT_SETEVENT) {
#if defined(VAEG_FIX)
		fdc_dointerrupt();
#else
		fdc.intreq = TRUE;
		TRACEOUT(("fdc: send interrupt request"));
#if defined(SUPPORT_PC88VA)
		if (fdc.fddifmode) {
			// DMA mode
			if (fdc.chgreg & 1) {
				pic_setirq(0x0b);
			}
			else {
				pic_setirq(0x0a);
			}
		}
		else {
			// Intelligent mode
			subsystem_irq(TRUE);
		}
#else
		if (fdc.chgreg & 1) {
			pic_setirq(0x0b);
		}
		else {
			pic_setirq(0x0a);
		}
#endif	/* defined(SUPPORT_PC88VA) */
#endif	/* defined(VAEG_FIX) */
	}
}

void fdc_interrupt(void) {
	nevent_set(NEVENT_FDCINT, 512, fdc_intwait, NEVENT_ABSOLUTE);
}


static void fdc_interruptreset(void) {
	fdc.intreq = FALSE;
}

static BOOL fdc_isfdcinterrupt(void) {
	return(fdc.intreq);
}

#if defined(VAEG_FIX)
static void fdc_resetirq(void) {
	if (!fdc.fddifmode) {
		// Intelligent mode
		subsystem_irq(FALSE);
	}
}
#endif

// ----------------------------------------------------------------------
// DMA 

REG8 DMACCALL fdc_dmafunc(REG8 func) {
	//TRACEOUT(("fdc_dmafunc = %d", func));

	switch(func) {
		case DMAEXT_START:
			return(1);

		case DMAEXT_END:				// TC
#if defined(VAEG_FIX)
			fdc.tcreserved = TRUE;		// 次のread/writeが終わった時点でTC実行
#else
			fdc.tc = 1;
#endif
			break;

#if defined(VAEG_FIX)
		case DMAEXT_DRQ: 
			return fdc.rqm ? 0 : 1;
#endif
	}
	return(0);
}

static void fdc_dmaready(REG8 enable) {
#if defined(SUPPORT_PC88VA)
	if (!fdc.fddifmode) {
		// Intelligent
		return;
	}
#endif
	if (fdc.chgreg & 1) {
		dmac.dmach[FDC_DMACH2HD].ready = enable;
	}
	else {
		dmac.dmach[FDC_DMACH2DD].ready = enable;
	}
}


// ----------------------------------------------------------------------

void fdcsend_error7(void) {

#if defined(VAEG_FIX)
	stop_executionphase();
#else
	fdc.tc = 0;
#endif
	fdc.event = FDCEVENT_BUFSEND;
	fdc.bufp = 0;
	fdc.bufcnt = 7;
	fdc.buf[0] = (BYTE)(fdc.stat[fdc.us] >>  0);
	fdc.buf[1] = (BYTE)(fdc.stat[fdc.us] >>  8);
	fdc.buf[2] = (BYTE)(fdc.stat[fdc.us] >> 16);
	fdc.buf[3] = fdc.C;
	fdc.buf[4] = fdc.H;
	fdc.buf[5] = fdc.R;
	fdc.buf[6] = fdc.N;
	fdc.status = FDCSTAT_RQM | FDCSTAT_CB | FDCSTAT_DIO;
	fdc.stat[fdc.us] = 0;										// ver0.29

	TRACEOUT(("fdc: error7"));
	TRACEOUT(("fdc: ST0=0x%02x ST1=0x%02x ST2=0x%02x",fdc.buf[0],fdc.buf[1],fdc.buf[2]));
	TRACEOUT(("fdc: C=0x%02x H=0x%02x R=0x%02x N=0x%02x",fdc.buf[3],fdc.buf[4],fdc.buf[5],fdc.buf[6]));

	fdc_dmaready(0);
	dmac_check();

	fdc_interrupt();

#if defined(VAEG_EXT)
	deactivate_head();
#endif
}

void fdcsend_success7(void) {

#if defined(VAEG_FIX)
	stop_executionphase();
#else
	fdc.tc = 0;
#endif
	fdc.event = FDCEVENT_BUFSEND;
	fdc.bufp = 0;
	fdc.bufcnt = 7;
	fdc.buf[0] = (fdc.hd << 2) | fdc.us;
	fdc.buf[1] = 0;
	fdc.buf[2] = 0;
	fdc.buf[3] = fdc.C;
	fdc.buf[4] = fdc.H;
	fdc.buf[5] = fdc.R;
	fdc.buf[6] = fdc.N;
	fdc.status = FDCSTAT_RQM | FDCSTAT_CB | FDCSTAT_DIO;
	fdc.stat[fdc.us] = 0;										// ver0.29

	TRACEOUT(("fdc: success7"));
	TRACEOUT(("fdc: ST0=0x%02x ST1=0x%02x ST2=0x%02x",fdc.buf[0],fdc.buf[1],fdc.buf[2]));
	TRACEOUT(("fdc: C=0x%02x H=0x%02x R=0x%02x N=0x%02x",fdc.buf[3],fdc.buf[4],fdc.buf[5],fdc.buf[6]));

	fdc_dmaready(0);
	dmac_check();

	fdc_interrupt();

#if defined(VAEG_EXT)
	deactivate_head();
#endif
}

// ----------------------------------------------------------------------

#if 0
// FDCのタイムアウト			まぁ本当はこんなんじゃダメだけど…	ver0.29
void fdctimeoutproc(NEVENTITEM item) {

	if (item->flag & NEVENT_SETEVENT) {
		fdc.stat[fdc.us] = FDCRLT_IC0 | FDCRLT_EN | (fdc.hd << 2) | fdc.us;
		fdcsend_error7();
	}
}

static void fdc_timeoutset(void) {

	nevent_setbyms(NEVENT_FDCTIMEOUT, 166, fdctimeoutproc, NEVENT_ABSOLUTE);
}
#endif


// ----------------------------------------------------------------------
// Command subroutines

static BOOL FDC_DriveCheck(BOOL protectcheck) {

	if (!fddfile[fdc.us].fname[0]) {
		fdc.stat[fdc.us] = FDCRLT_IC0 | FDCRLT_NR | (fdc.hd << 2) | fdc.us;
		fdcsend_error7();
		return(FALSE);
	}
	else if ((protectcheck) && (fddfile[fdc.us].protect)) {
		fdc.stat[fdc.us] = FDCRLT_IC0 | FDCRLT_NW | (fdc.hd << 2) | fdc.us;
		fdcsend_error7();
		return(FALSE);
	}
	return(TRUE);
}

static void get_mtmfsk(void) {

	fdc.mt = (fdc.cmd >> 7) & 1;
	fdc.mf = fdc.cmd & 0x40;							// ver0.29
	fdc.sk = (fdc.cmd >> 5) & 1;
}

static void get_hdus(void) {

	fdc.hd = (fdc.cmds[0] >> 2) & 1;
	fdc.us = fdc.cmds[0] & 3;
}

static void get_chrn(void) {

	fdc.C = fdc.cmds[1];
	fdc.H = fdc.cmds[2];
	fdc.R = fdc.cmds[3];
	fdc.N = fdc.cmds[4];
}

static void get_eotgsldtl(void) {

	fdc.eot = fdc.cmds[5];
	fdc.gpl = fdc.cmds[6];
	fdc.dtl = fdc.cmds[7];
}

#if defined(VAEG_FIX)

static BOOL inc_fdcR(void) {
	BOOL	over;			// シリンダをまたがった

	over = FALSE;

	fdc.R++;
	if (fdc.R > fdc.eot) {
		if (fdc.cmd & 0x80) {
			// MT=1
			if (fdc.hd) over = TRUE;
			fdc.C += fdc.hd;
			fdc.H = fdc.hd ^ 1;
			fdc.R = 1;
		}
		else {
			over = TRUE;
			fdc.C++;
			fdc.R = 1;
		}
	}
	return over;
}
#endif

// --------------------------------------------------------------------------
// Commands

static void FDC_Invalid(void) {							// cmd: xx

	fdc.event = FDCEVENT_BUFSEND;
	fdc.bufcnt = 1;
	fdc.bufp = 0;
	fdc.buf[0] = 0x80;
	fdc.status = FDCSTAT_RQM | FDCSTAT_CB | FDCSTAT_DIO;
}

#if 0
static void FDC_ReadDiagnostic(void) {					// cmd: 02

	switch(fdc.event) {
		case FDCEVENT_CMDRECV:
			get_hdus();
			get_chrn();
			get_eotgsldtl();
			fdc.stat[fdc.us] = (fdc.hd << 2) | fdc.us;

			if (FDC_DriveCheck(FALSE)) {
				fdc.event = FDCEVENT_BUFSEND;
//				fdc.bufcnt = makedianosedata();
				fdc.bufp = 0;
			}
			break;

		default:
			fdc.event = FDCEVENT_NEUTRAL;
			break;
	}
}
#endif

static void FDC_Specify(void) {							// cmd: 03

	switch(fdc.event) {
		case FDCEVENT_CMDRECV:
			fdc.srt = fdc.cmds[0] >> 4;
			fdc.hut = fdc.cmds[0] & 0x0f;
			fdc.hlt = fdc.cmds[1] >> 1;
			fdc.nd = fdc.cmds[1] & 1;
			fdc.stat[fdc.us] = (fdc.hd << 2) | fdc.us;
			break;
	}
	fdc.event = FDCEVENT_NEUTRAL;
	fdc.status = FDCSTAT_RQM;
}

static void FDC_SenseDeviceStatus(void) {				// cmd: 04

	switch(fdc.event) {
		case FDCEVENT_CMDRECV:
			get_hdus();
			fdc.buf[0] = (fdc.hd << 2) | fdc.us;
			fdc.stat[fdc.us] = (fdc.hd << 2) | fdc.us;
			if (fdc.equip & (1 << fdc.us)) {
#if defined(VAEG_EXT)
				sysmng_fddaccess(fdc.us, CTRL_FDMEDIA[fdc.us] == DISKTYPE_2HD);
#endif
#if defined(SUPPORT_PC88VA)
				if (pccore.model_va == PCMODEL_NOTVA) {
					fdc.buf[0] |= 0x08;
				}
#else
				fdc.buf[0] |= 0x08;
#endif
				if (!fdc.treg[fdc.us]) {
					fdc.buf[0] |= 0x10;
				}
				if (fddfile[fdc.us].fname[0]) {
#if defined(VAEG_EXT)
					if (fdc.motor[fdc.us] == FDD_MOTOR_STABLE) {
#endif
					fdc.buf[0] |= 0x20;
#if defined(SUPPORT_PC88VA)
					if (pccore.model_va != PCMODEL_NOTVA) {
						fdc.buf[0] |= 0x08;
							/*
								VAの場合、Ready=0ならTwo Side=0のようだ。
							*/
					}
#endif
#if defined(VAEG_EXT)
					}
#endif
				}
				if (fddfile[fdc.us].protect) {
					fdc.buf[0] |= 0x40;
				}
			}
			else {
				fdc.buf[0] |= 0x80;
			}
			TRACEOUT(("fdc: FDC_SenseDeviceStatus %.2x", fdc.buf[0]));
			fdc.event = FDCEVENT_BUFSEND;
			fdc.bufcnt = 1;
			fdc.bufp = 0;
			fdc.status = FDCSTAT_RQM | FDCSTAT_CB | FDCSTAT_DIO;
			break;

		default:
			fdc.event = FDCEVENT_NEUTRAL;
			fdc.status = FDCSTAT_RQM;
			break;
	}
}

#if defined(VAEG_FIX)
static void start_writesector(void) {
	fdc.event = FDCEVENT_BUFRECV;
	fdc.bufcnt = 128 << fdc.N;
	fdc.bufp = 0;
	ZeroMemory(fdc.buf, fdc.bufcnt);

#if defined(VAEG_EXT)
	reached_sector();
#endif
}
#endif

static BOOL writesector(void) {

	fdc.stat[fdc.us] = (fdc.hd << 2) | fdc.us;
	if (!FDC_DriveCheck(TRUE)) {
		return(FAILURE);
	}
	if (fdd_write()) {
		fdc.stat[fdc.us] = fdc.us | (fdc.hd << 2) | FDCRLT_IC0 | FDCRLT_ND;
		fdcsend_error7();
		return(FAILURE);
	}
#if defined(VAEG_FIX)
	fdc.event = FDCEVENT_STARTBUFRECV;
	fdc.bufp = 0;				// TCの処理でチェックするので必要
/*
	if (fdc.R == fdc.eot) {
		// トラックの最後のセクタを書き終わった
		fdc.priampcnt = LENGTH_PRIAMP;
	}
	else {
		fdc.priampcnt = 0;
	}
*/
#else
	fdc.event = FDCEVENT_BUFRECV;
	fdc.bufcnt = 128 << fdc.N;
	fdc.bufp = 0;
	fdc.status = FDCSTAT_RQM | FDCSTAT_NDM | FDCSTAT_CB;
	fdc_dmaready(1);
	dmac_check();
#endif
	return(SUCCESS);
}

static void FDC_WriteData(void) {						// cmd: 05
														// cmd: 09
	switch(fdc.event) {
		case FDCEVENT_CMDRECV:
			get_hdus();
			get_chrn();
			get_eotgsldtl();
#if defined(VAEG_FIX)
			fdc.stat[fdc.us] = (fdc.hd << 2) | fdc.us;
			if (FDC_DriveCheck(TRUE)) {
				start_executionphase();
				fdc.event = FDCEVENT_FIRSTSTARTBUFRECV;
				fdc.status = FDCSTAT_NDM | FDCSTAT_CB;
				if (!fdc.nd) {
					// DMA
					fdc_dmaready(1);
					dmac_check();
				}
#if defined(VAEG_EXT)
				activate_head();
				want_sector();
#endif
			}
			break;
#else
			fdc.stat[fdc.us] = (fdc.hd << 2) | fdc.us;
			if (FDC_DriveCheck(TRUE)) {
#if defined(VAEG_EXT)
				activate_head();
#endif
				fdc.event = FDCEVENT_BUFRECV;
				fdc.bufcnt = 128 << fdc.N;
				fdc.bufp = 0;
#if 1															// ver0.27 ??
				fdc.status = FDCSTAT_NDM | FDCSTAT_CB;
				if (!(fdc.ctrlreg & 0x10)) {
					fdc.status |= FDCSTAT_RQM;
				}
#else
				fdc.status = FDCSTAT_RQM | FDCSTAT_NDM | FDCSTAT_CB;
#endif
				fdc_dmaready(1);
				dmac_check();
			}
			break;
#endif

#if defined(VAEG_FIX)
		case FDCEVENT_FIRSTSTARTBUFRECV:
			start_writesector();
			break;

		case FDCEVENT_STARTBUFRECV:
			if (inc_fdcR()) {
				fdc.stat[fdc.us] = fdc.us | (fdc.hd << 2) |
													FDCRLT_IC0 | FDCRLT_EN;
				fdcsend_error7();
			}
			else {
				start_writesector();
			}
			break;
#endif

		case FDCEVENT_BUFRECV:
			if (writesector()) {
				return;
			}
#if defined(VAEG_FIX)
#else
			if (fdc.tc) {
				fdcsend_success7();
				return;
			}
			if (fdc.R++ == fdc.eot) {
				fdc.stat[fdc.us] = fdc.us | (fdc.hd << 2) |
													FDCRLT_IC0 | FDCRLT_EN;
				fdcsend_error7();
				break;
			}
#endif
			break;

#if defined(VAEG_FIX)
		case FDCEVENT_TC:
			if (fdc.bufp) {
				// セクタの途中
				if (writesector()) {
					// エラーなのでfdc.event は更新済みのはず
					break;
				}
			}
			inc_fdcR();
			fdcsend_success7();
			break;
#endif
		default:
			fdc.event = FDCEVENT_NEUTRAL;
			fdc.status = FDCSTAT_RQM;
			break;
	}
}

static void readsector(void) {

	fdc.stat[fdc.us] = (fdc.hd << 2) | fdc.us;
	if (!FDC_DriveCheck(FALSE)) {
		return;
	}
	if (fdd_read()) {
		fdc.stat[fdc.us] = fdc.us | (fdc.hd << 2) | FDCRLT_IC0 | FDCRLT_ND;
		fdcsend_error7();
		return;
	}

	fdc.event = FDCEVENT_BUFSEND2;
	fdc.bufp = 0;

#if defined(VAEG_EXT)
	reached_sector();
#endif

#if defined(VAEG_FIX)
#else
#if 1															// ver0.27 ??
	fdc.status = FDCSTAT_NDM | FDCSTAT_CB;
	if (!(fdc.ctrlreg & 0x10)) {
		fdc.status |= FDCSTAT_RQM | FDCSTAT_DIO;
	}
#else
	fdc.status = FDCSTAT_RQM | FDCSTAT_DIO | FDCSTAT_NDM | FDCSTAT_CB;
#endif
	fdc_dmaready(1);
	dmac_check();
#endif	// defined(VAEG_FIX)
}

static void FDC_ReadData(void) {						// cmd: 06
														// cmd: 0c
	switch(fdc.event) {
		case FDCEVENT_CMDRECV:
			get_hdus();
			get_chrn();
			get_eotgsldtl();
#if defined(VAEG_FIX)
			fdc.stat[fdc.us] = (fdc.hd << 2) | fdc.us;
			if (FDC_DriveCheck(FALSE)) {
				start_executionphase();
				fdc.event = FDCEVENT_FIRSTSTARTBUFSEND2;
				fdc.status = FDCSTAT_NDM | FDCSTAT_CB;
				if (!(fdc.ctrlreg & 0x10)) {
					fdc.status |= FDCSTAT_DIO;
				}
				if (!fdc.nd) {
					// DMA
					fdc_dmaready(1);
					dmac_check();
				}
				fdc.bufcnt = 0;
#if defined(VAEG_EXT)
				activate_head();
				want_sector();
#endif
			}
			break;
#else
			readsector();
			break;
#endif

#if defined(VAEG_FIX)
		case FDCEVENT_FIRSTDATA:
			readsector();
			break;
#endif

		case FDCEVENT_NEXTDATA:
			fdc.bufcnt = 0;
#if defined(VAEG_FIX)
			if (inc_fdcR()) {
#else
			if (fdc.R++ == fdc.eot) {
#endif
				fdc.stat[fdc.us] = fdc.us | (fdc.hd << 2) |
													FDCRLT_IC0 | FDCRLT_EN;
				fdcsend_error7();
				break;
			}
			readsector();
			break;

#ifdef FDC_DELAYERROR7
		case FDCEVENT_BUSY:
			break;
#endif

#if defined(VAEG_FIX)
		case FDCEVENT_TC:
			inc_fdcR();
			fdcsend_success7();
			return;
#endif
		default:
			fdc.event = FDCEVENT_NEUTRAL;
			fdc.status = FDCSTAT_RQM;
			break;
	}
}

static void FDC_Recalibrate(void) {						// cmd: 07

	switch(fdc.event) {
		case FDCEVENT_CMDRECV:
#if defined(VAEG_EXT)
			get_hdus();
			if (!(fdc.equip & (1 << fdc.us))) {
				fdc.stat[fdc.us] = (fdc.hd << 2) | fdc.us;
				fdc.stat[fdc.us] |= FDCRLT_SE | FDCRLT_NR | FDCRLT_IC0;
				fdc_interrupt();
			}
			else if (!fddfile[fdc.us].fname[0]) {
				fdc.stat[fdc.us] = (fdc.hd << 2) | fdc.us;
				fdc.stat[fdc.us] |= FDCRLT_SE | FDCRLT_NR;
				fdc_interrupt();
			}
			else {
				start_seek(fdc.us, 0);
			}
			break;
#else
			get_hdus();
			fdc.ncn = 0;
			fdc.stat[fdc.us] = (fdc.hd << 2) | fdc.us;
			fdc.stat[fdc.us] |= FDCRLT_SE;
			if (!(fdc.equip & (1 << fdc.us))) {
				fdc.stat[fdc.us] |= FDCRLT_NR | FDCRLT_IC0;
			}
			else if (!fddfile[fdc.us].fname[0]) {
				fdc.stat[fdc.us] |= FDCRLT_NR;
			}
			else {
				fdd_seek();
			}
			fdc_interrupt();
			break;
#endif
	}
	fdc.event = FDCEVENT_NEUTRAL;
	fdc.status = FDCSTAT_RQM;
}

static void FDC_SenceintStatus(void) {					// cmd: 08

	int		i;

	fdc.event = FDCEVENT_BUFSEND;
	fdc.bufp = 0;
	fdc.bufcnt = 0;
	fdc.status = FDCSTAT_RQM | FDCSTAT_CB | FDCSTAT_DIO;

	if (fdc_isfdcinterrupt()) {
		i = 0;
		if (fdc.stat[fdc.us]) {
			fdc.buf[0] = (BYTE)fdc.stat[fdc.us];
			fdc.buf[1] = fdc.treg[fdc.us];
			fdc.bufcnt = 2;
			fdc.stat[fdc.us] = 0;
			TRACEOUT(("fdc: fdc stat - %d [%.2x]", fdc.us, fdc.buf[0]));
		}
		else {
			for (; i<4; i++) {
				if (fdc.stat[i]) {
					fdc.buf[0] = (BYTE)fdc.stat[i];
					fdc.buf[1] = fdc.treg[i];
					fdc.bufcnt = 2;
					fdc.stat[i] = 0;
					TRACEOUT(("fdc: fdc stat - %d [%.2x]", i, fdc.buf[0]));
					break;
				}
			}
		}
		for (; i<4; i++) {
			if (fdc.stat[i]) {
				break;
			}
		}
		if (i >= 4) {
			fdc_interruptreset();
		}
	}
	if (!fdc.bufcnt) {
		fdc.buf[0] = FDCRLT_IC1;
		fdc.bufcnt = 1;
		TRACEOUT(("fdc: fdc stat - [%.2x]", fdc.buf[0]));
	}
}

static void FDC_ReadID(void) {							// cmd: 0a

	switch(fdc.event) {
		case FDCEVENT_CMDRECV:
#if defined(VAEG_EXT)
					// ToDo: 他と同じようにstart_executionphaseに対応しなくていいの？
			activate_head();
#endif
			fdc.mf = fdc.cmd & 0x40;
			get_hdus();
			if (fdd_readid() == SUCCESS) {
				fdcsend_success7();
			}
			else {
				fdc.stat[fdc.us] = fdc.us | (fdc.hd << 2) |
													FDCRLT_IC0 | FDCRLT_MA;
				fdcsend_error7();
			}
			break;
	}
}

static void FDC_WriteID(void) {							// cmd: 0d

	switch(fdc.event) {
		case FDCEVENT_CMDRECV:
//			TRACE_("FDC_WriteID FDCEVENT_CMDRECV", 0);
			get_hdus();
			fdc.N = fdc.cmds[1];
			fdc.sc = fdc.cmds[2];
			fdc.gpl = fdc.cmds[3];
			fdc.d = fdc.cmds[4];
			if (FDC_DriveCheck(TRUE)) {
//				TRACE_("FDC_WriteID FDC_DriveCheck", 0);
				if (fdd_formatinit()) {
//					TRACE_("FDC_WriteID fdd_formatinit", 0);
					fdcsend_error7();
					break;
				}
#if defined(VAEG_FIX)

				start_executionphase();
				fdc.event = FDCEVENT_FIRSTSTARTBUFRECV;
				fdc.status = FDCSTAT_NDM | FDCSTAT_CB;
				if (!fdc.nd) {
					// DMA
					fdc_dmaready(1);
					dmac_check();
				}
#if defined(VAEG_EXT)
				activate_head();
				fdc.reach = FDD_HEADREACH_REACHED;
#endif

#else // defined(VAEG_FIX)
				
//				TRACE_("FDC_WriteID FDCEVENT_BUFRECV", 0);
				fdc.event = FDCEVENT_BUFRECV;
				fdc.bufcnt = 4;
				fdc.bufp = 0;
#if 1															// ver0.27 ??
				fdc.status = FDCSTAT_NDM | FDCSTAT_CB;
				if (!(fdc.ctrlreg & 0x10)) {
					fdc.status |= FDCSTAT_RQM;
				}
#else
				fdc.status = FDCSTAT_RQM | FDCSTAT_NDM | FDCSTAT_CB;
#endif
				fdc_dmaready(1);
				dmac_check();
#if defined(VAEG_EXT)
				activate_head();
#endif

#endif // defined(VAEG_FIX)
			}
			break;


#if defined(VAEG_FIX)
		case FDCEVENT_FIRSTSTARTBUFRECV:
		case FDCEVENT_STARTBUFRECV:
			fdc.event = FDCEVENT_BUFRECV;
			fdc.bufcnt = 4;
			fdc.bufp = 0;
			break;


#endif


		case FDCEVENT_BUFRECV:
			if (fdd_formating(fdc.buf)) {
				fdcsend_error7();
				break;
			}
#if defined(VAEG_FIX)
			if (!fdd_isformating()) {
				fdcsend_success7();
				break;
			}

			fdc.event = FDCEVENT_STARTBUFRECV;
			fdc.bufp = 0;
			break;

#else

			if ((fdc.tc) || (!fdd_isformating())) {
				fdcsend_success7();
				return;
			}
			fdc.event = FDCEVENT_BUFRECV;
			fdc.bufcnt = 4;
			fdc.bufp = 0;
#if 1															// ver0.27 ??
			fdc.status = FDCSTAT_NDM | FDCSTAT_CB;
			if (!(fdc.ctrlreg & 0x10)) {
				fdc.status |= FDCSTAT_RQM;
			}
#else
			fdc.status = FDCSTAT_RQM | FDCSTAT_NDM | FDCSTAT_CB;
#endif
			break;
#endif	/* VAEG_FIX */


#if defined(VAEG_FIX)
		case FDCEVENT_TC:
			fdcsend_success7();
			break;
#endif

		default:
			fdc.event = FDCEVENT_NEUTRAL;
			fdc.status = FDCSTAT_RQM;
			break;
	}
}

static void FDC_Seek(void) {							// cmd: 0f

	switch(fdc.event) {
		case FDCEVENT_CMDRECV:
#if defined(VAEG_EXT)
			get_hdus();
			if ((!(fdc.equip & (1 << fdc.us))) ||
				(!fddfile[fdc.us].fname[0])) {
				fdc.stat[fdc.us] = (fdc.hd << 2) | fdc.us;
				fdc.stat[fdc.us] |= FDCRLT_SE;
				fdc.stat[fdc.us] |= FDCRLT_NR | FDCRLT_IC0;
				fdc_interrupt();
			}
			else {
#if defined(SUPPORT_PC88VA)
				if (fdc.trackdensity[fdc.us] == FDD_48TPI) {
					start_seek(fdc.us, fdc.cmds[1] * 2);
				}
				else {
					start_seek(fdc.us, fdc.cmds[1]);
				}
#else
				start_seek(fdc.us, fdc.cmds[1]);
#endif
			}
			break;
#else
			get_hdus();
			fdc.ncn = fdc.cmds[1];
			fdc.stat[fdc.us] = (fdc.hd << 2) | fdc.us;
			fdc.stat[fdc.us] |= FDCRLT_SE;
			if ((!(fdc.equip & (1 << fdc.us))) ||
				(!fddfile[fdc.us].fname[0])) {
				fdc.stat[fdc.us] |= FDCRLT_NR | FDCRLT_IC0;
			}
			else {
				fdd_seek();
			}
			fdc_interrupt();
			break;
#endif
	}
	fdc.event = FDCEVENT_NEUTRAL;
	fdc.status = FDCSTAT_RQM;
}

#if 0
static void FDC_ScanEqual(void) {						// cmd: 11, 19, 1d

	switch(fdc.event) {
		case FDCEVENT_CMDRECV:
			get_hdus();
			get_chrn();
			fdc.eot = fdc.cmds[5];
			fdc.gpl = fdc.cmds[6];
			fdc.stp = fdc.cmds[7];
			break;
	}
}
#endif

// --------------------------------------------------------------------------

typedef void (*FDCOPE)(void);

static const FDCOPE FDC_Ope[0x20] = {
				FDC_Invalid,
				FDC_Invalid,
				FDC_ReadData,			// FDC_ReadDiagnostic,
				FDC_Specify,
				FDC_SenseDeviceStatus,
				FDC_WriteData,
				FDC_ReadData,
				FDC_Recalibrate,
				FDC_SenceintStatus,
				FDC_WriteData,
				FDC_ReadID,
				FDC_Invalid,
				FDC_ReadData,
				FDC_WriteID,
				FDC_Invalid,
				FDC_Seek,
				FDC_Invalid,					// 10
				FDC_Invalid,			// FDC_ScanEqual,
				FDC_Invalid,
				FDC_Invalid,
				FDC_Invalid,
				FDC_Invalid,
				FDC_Invalid,
				FDC_Invalid,
				FDC_Invalid,
				FDC_Invalid,			// FDC_ScanEqual,
				FDC_Invalid,
				FDC_Invalid,
				FDC_Invalid,
				FDC_Invalid,			// FDC_ScanEqual,
				FDC_Invalid,
				FDC_Invalid};


// --------------------------------------------------------------------------
// RQM management

#if defined(VAEG_FIX)

static void setrqm(void) {
	fdc.rqmlastclock += fdc.rqminterval;
	fdc.rqm = TRUE;
	if (!(fdc.ctrlreg & 0x10)) {
		fdc.status |= FDCSTAT_RQM;
	}
	if (fdc.nd) {
		fdc_dointerrupt();
	}
}

static void resetrqm(void) {
	if (fdc.rqm) {
		fdc.rqm = FALSE;
		fdc.status &= ~FDCSTAT_RQM;
		if (fdc.nd) {
			fdc_resetirq();
		}
	}
}

// エクスキュージョンフェーズに入ったらこれを呼ぶ				
static void start_executionphase(void) {
	int rpm = 360;
	int tracklen;
	int seclen;
	int sectors;
	int gap0,sync,iam=4,gap1,idam=4,chrn=4,crc=2,gap2,ddam=4,data,gap3,gap4;

	switch(CTRL_FDMEDIA[fdc.us]) {
	// TODO: 2HD 1.44Mの場合→VAでは不要なので考えないことにする。
	case DISKTYPE_2DD:		// 2D/2DD
		gap0 = 80;
		sync = 12;
		gap1 = 50;
		gap2 = 22;
		data = 512;		// DOS(メディアID F9h, 2DD 720K)のフォーマットで代表させる
		gap3 = 84;		// 同上
		gap4 = 182;		// 同上
		sectors = 9;	// 同上
						// 備考
						// 2DD 640K(512byte*8sector), 2D 320K(256byte*16sector)をこの設定で
						// アクセスすると、実機より短時間でアクセスが終了する。
		break;
	case DISKTYPE_2HD:		// 2HD
	default:
		gap0 = 80;
		sync = 12;
		gap1 = 50;
		gap2 = 22;
		data = 1024;	// DOS(メディアID FEh)のフォーマットで代表させる
		gap3 = 116;		// 同上
		gap4 = 654;		// 同上
		sectors = 8;	// 同上
		break;
	}
	seclen = sync + idam + chrn + crc + gap2 + sync + ddam + data + crc + gap3;
	tracklen = gap0 + sync + iam + gap1 + seclen * sectors + gap4;
//	fdc.rqminterval = pccore.realclock * 60 / (tracklen * rpm);
	fdc.rqminterval = (SINT32)((UINT64)pccore.realclock * 60 / rpm / tracklen * seclen / data);
						// 1トラックのデータを読み込む時間 = 
						// プリアンブル・ポストアンブルを除いた部分を読み込むのに要する時間
						// となるように設定する。

						// pccore.realclock * 60 / rpm = 1回転時間(clock)
						// 1回転時間 * seclen / tracklen = 1セクタリード時間(clock)
						// 1セクタリード時間 / data = 実データ1バイトあたりのリード時間
	
	fdc.rqmlastclock = getnow();
	resetrqm();

#if defined(VAEG_EXT)
	fdc.roundtime = (UINT32)((UINT64)pccore.realclock * 60 / rpm);
	fdc.amptime = fdc.roundtime - fdc.rqminterval * sectors * data;
#endif
}


// エクスキュージョンフェーズをぬけるときにこれを呼ぶ				
static void stop_executionphase(void) {
	resetrqm();
}

static void update_executionphase_read(void){
	SINT32 now;
	SINT32 d;

	now = getnow_onevent();
	d = now - fdc.rqmlastclock;
	if (!fdc.rqm && d >= fdc.rqminterval) {

#if defined(VAEG_EXT)
		if (fdc.head == FDD_HEAD_STABLE && fdc.reach == FDD_HEADREACH_REACHED) {
#endif
			switch(fdc.event) {
			case FDCEVENT_FIRSTSTARTBUFSEND2:
				fdc.event = FDCEVENT_FIRSTDATA;
				FDC_Ope[fdc.cmd & 0x1f]();
				break;
			case FDCEVENT_STARTBUFSEND2:
/*
				if (fdc.priampcnt) {
					fdc.priampcnt--;
					fdc.rqmlastclock += fdc.rqminterval;
				}
				else {
*/
					fdc.event = FDCEVENT_NEXTDATA;
					FDC_Ope[fdc.cmd & 0x1f]();
/*
				}
*/
				break;
			}
			if (fdc.event == FDCEVENT_BUFSEND2) {
				setrqm();
			}
#if defined(VAEG_EXT)
		}
		else {
			fdc.rqmlastclock = now;
		}
#endif
	}
}



static void update_executionphase_write(void){
	SINT32 now;
	SINT32 d;

	now = getnow_onevent();
	d = now - fdc.rqmlastclock;
	if (!fdc.rqm && d >= fdc.rqminterval) {
#if defined(VAEG_EXT)
		if (fdc.head == FDD_HEAD_STABLE && fdc.reach == FDD_HEADREACH_REACHED) {
#endif
			switch(fdc.event) {
			case FDCEVENT_FIRSTSTARTBUFRECV:
				FDC_Ope[fdc.cmd & 0x1f]();
				break;
			case FDCEVENT_STARTBUFRECV:
/*
				if (fdc.priampcnt) {
					fdc.priampcnt--;
					fdc.rqmlastclock += fdc.rqminterval;
				}
				else {
*/
					FDC_Ope[fdc.cmd & 0x1f]();
/*
				}
*/
				break;
			}
			if (fdc.event == FDCEVENT_BUFRECV) {
				setrqm();
			}

#if defined(VAEG_EXT)
		}
		else {
			fdc.rqmlastclock = now;
		}
#endif
	}
}

static void update_executionphase(void) {
	switch(fdc.event) {
	case FDCEVENT_BUFSEND2:
	case FDCEVENT_STARTBUFSEND2:
	case FDCEVENT_FIRSTSTARTBUFSEND2:
		update_executionphase_read();
		break;
	case FDCEVENT_BUFRECV:
	case FDCEVENT_STARTBUFRECV:
	case FDCEVENT_FIRSTSTARTBUFRECV:
		update_executionphase_write();
		break;
	}
}

#endif

// --------------------------------------------------------------------------
// TC

#if defined(VAEG_FIX)

void settc(void) {
	TRACEOUT(("fdc: tc"));
	switch (fdc.event) {
	case FDCEVENT_FIRSTSTARTBUFSEND2:
	case FDCEVENT_STARTBUFSEND2:
	case FDCEVENT_BUFSEND2:
		TRACEOUT(("fdc: valid tc"));
		fdc.event = FDCEVENT_TC;
		FDC_Ope[fdc.cmd & 0x1f]();
		return;
	case FDCEVENT_FIRSTSTARTBUFRECV:
	case FDCEVENT_STARTBUFRECV:
	case FDCEVENT_BUFRECV:
		TRACEOUT(("fdc: valid tc"));
		fdc.event = FDCEVENT_TC;
		FDC_Ope[fdc.cmd & 0x1f]();
		break;
	}
}

#endif

// --------------------------------------------------------------------------
// FDC data port read/write

static void fdcstatusreset(void) {

	fdc.event = FDCEVENT_NEUTRAL;
	fdc.status = FDCSTAT_RQM;
}

void DMACCALL fdc_datawrite(REG8 data) {

#if defined(VAEG_FIX)
		fdc_resetirq();
#endif

//	if ((fdc.status & (FDCSTAT_RQM | FDCSTAT_DIO)) == FDCSTAT_RQM) {
		switch(fdc.event) {
#if defined(VAEG_EXT)
			case FDCEVENT_FIRSTSTARTBUFRECV:
			case FDCEVENT_STARTBUFRECV:
				break;
#endif
			case FDCEVENT_BUFRECV:
#if defined(VAEG_FIX)
				if (fdc.rqm) {
					resetrqm();
					fdc.buf[fdc.bufp++] = data;
					if (!(--fdc.bufcnt)) {
						FDC_Ope[fdc.cmd & 0x1f]();
					}
				}
				break;
#else
//				TRACE_("write", fdc.bufp);
				fdc.buf[fdc.bufp++] = data;
				if ((!(--fdc.bufcnt)) || (fdc.tc)) {
					fdc.status &= ~FDCSTAT_RQM;
					FDC_Ope[fdc.cmd & 0x1f]();
				}
				break;
#endif

			case FDCEVENT_CMDRECV:
				fdc.cmds[fdc.cmdp++] = data;
				TRACEOUT(("fdc: cmd parameter=0x%02x", data));
				if (!(--fdc.cmdcnt)) {
					fdc.status &= ~FDCSTAT_RQM;
					FDC_Ope[fdc.cmd & 0x1f]();
				}
				break;

			default:
				fdc.cmd = data;
				TRACEOUT(("fdc: cmd=0x%02x (bit4-bit0=0x%02x)", fdc.cmd, fdc.cmd & 0x1f));
				get_mtmfsk();
				if (FDCCMD_TABLE[data & 0x1f]) {
					fdc.event = FDCEVENT_CMDRECV;
					fdc.cmdp = 0;
					fdc.cmdcnt = FDCCMD_TABLE[data & 0x1f];
					fdc.status = FDCSTAT_RQM | FDCSTAT_CB;
				}
				else {
					fdc.status &= ~FDCSTAT_RQM;
					FDC_Ope[fdc.cmd & 0x1f]();
				}
				break;
		}
//	}
#if defined(VAEG_FIX)
	if (fdc.tcreserved) {
		fdc.tcreserved = FALSE;
		settc();
	}
#endif
}

REG8 DMACCALL fdc_dataread(void) {

#if defined(VAEG_FIX)
		fdc_resetirq();
#endif

//	if ((fdc.status & (FDCSTAT_RQM | FDCSTAT_DIO))
//									== (FDCSTAT_RQM | FDCSTAT_DIO)) {
		switch(fdc.event) {
			case FDCEVENT_BUFSEND:
				fdc.lastdata = fdc.buf[fdc.bufp++];
				if (!(--fdc.bufcnt)) {
					fdc.event = FDCEVENT_NEUTRAL;
					fdc.status = FDCSTAT_RQM;
				}
				break;

#if defined(VAEG_FIX)
			case FDCEVENT_BUFSEND2:
				if (fdc.rqm) {
					resetrqm();
					fdc.lastdata = fdc.buf[fdc.bufp++];
					fdc.bufcnt--;
					if (!fdc.bufcnt) {
						fdc.event = FDCEVENT_STARTBUFSEND2;
/*
						if (fdc.R == fdc.eot) {
							// トラックの最後のセクタを読み終わった
							fdc.priampcnt = LENGTH_PRIAMP;
						}
						else {
							fdc.priampcnt = 0;
						}
*/
					}
				}
				break;
#else
			case FDCEVENT_BUFSEND2:
				if (fdc.bufcnt) {
					fdc.lastdata = fdc.buf[fdc.bufp++];
					fdc.bufcnt--;
				}
				if (fdc.tc) {
					if (!fdc.bufcnt) {						// ver0.26
						fdc.R++;
						if ((fdc.cmd & 0x80) && fdd_seeksector()) {
							fdc.C += fdc.hd;
							fdc.H = fdc.hd ^ 1;
							fdc.R = 1;
						}
					}
					fdcsend_success7();
				}
				if (!fdc.bufcnt) {
					fdc.event = FDCEVENT_NEXTDATA;
					fdc.status &= ~(FDCSTAT_RQM | FDCSTAT_NDM);
					FDC_Ope[fdc.cmd & 0x1f]();
				}
				break;
#endif /* VAEG_FIX */
		}
//	}
#if defined(VAEG_FIX)
	if (fdc.tcreserved) {
		fdc.tcreserved = FALSE;
		settc();
	}
#endif
	return(fdc.lastdata);
}

#if defined(VAEG_FIX)
// --------------------------------------------------------------------------
// State watch timer

static void start_statewatch(void);

void fdc_statewatch(NEVENTITEM item) {
#if defined(VAEG_EXT)
	update_head();
	update_headreach();
#endif
	update_executionphase();
	
	start_statewatch();
}

static void start_statewatch(void) {
	nevent_set(NEVENT_FDCSTATE, fdc.rqminterval, fdc_statewatch, NEVENT_RELATIVE);
}

static void stop_statewatch(void) {
	nevent_reset(NEVENT_FDCSTATE);
}

#endif

#if defined(VAEG_EXT)
// --------------------------------------------------------------------------
// FDC timer

void fdc_timer(NEVENTITEM item) {
	if (fdc.ctrlreg & 0x04) {
		// XTMASK (1で割り込み許可)
		if (fdc.chgreg & 1) {
			pic_setirq(0x0b);
		}
		else {
			pic_setirq(0x0a);
		}
	}
}

// --------------------------------------------------------------------------
// FDD motor management

void fdc_fddmotor(NEVENTITEM item) {
	if (fdc.motor[0] == FDD_MOTOR_STARTING) {
		fdc.motor[0] = 
		fdc.motor[1] = 
		fdc.motor[2] = 
		fdc.motor[3] = FDD_MOTOR_STABLE;
	}
}

#endif

// ---- I/O

static void IOOUTCALL fdc_o92(UINT port, REG8 dat) {

//	TRACEOUT(("fdc out %.2x %.2x [%.4x:%.4x]", port, dat, CPU_CS, CPU_IP));

	if (((port >> 4) ^ fdc.chgreg) & 1) {
		return;
	}
	if ((fdc.status & (FDCSTAT_RQM | FDCSTAT_DIO)) == FDCSTAT_RQM) {
		fdc_datawrite(dat);
	}
}

static void IOOUTCALL fdc_o94(UINT port, REG8 dat) {

//	TRACEOUT(("fdc out %.2x %.2x [%.4x:%.4x]", port, dat, CPU_CS, CPU_IP));

	if (((port >> 4) ^ fdc.chgreg) & 1) {
		return;
	}
	if ((fdc.ctrlreg ^ dat) & 0x10) {
		fdcstatusreset();
		fdc_dmaready(0);
		dmac_check();
	}
	fdc.ctrlreg = dat;
}

#if defined(SUPPORT_PC88VA)

static void IOOUTCALL fdcva_o_fdc1(UINT port, REG8 dat) {

//	TRACEOUT(("fdc out %.2x %.2x [%.4x:%.4x]", port, dat, CPU_CS, CPU_IP));

	if ((fdc.status & (FDCSTAT_RQM | FDCSTAT_DIO)) == FDCSTAT_RQM) {
		fdc_datawrite(dat);
	}
}

static void IOOUTCALL fdcva_o_dskmisc(UINT port, REG8 dat) {

//	TRACEOUT(("fdc out %.2x %.2x [%.4x:%.4x]", port, dat, CPU_CS, CPU_IP));

	if ((fdc.ctrlreg ^ dat) & 0x10) {
		fdcstatusreset();
		fdc_dmaready(0);
		dmac_check();
	}
	if (dat & 0x01) {
		// TTRG
		nevent_setbyms(NEVENT_FDCTIMER, 100, fdc_timer, NEVENT_ABSOLUTE);

	}
	fdc.ctrlreg = dat & 0xf5;		// 実際には、参照しているのはbit4(DMAE)のみのようだ
}



#endif


static REG8 IOINPCALL fdc_i90(UINT port) {
#if defined(VAEG_EXT)
	fdc.status = fdc.status & 0xf0 | fdbusybits();
#endif
//	TRACEOUT(("fdc in %.2x %.2x [%.4x:%.4x]", port, fdc.status,
//															CPU_CS, CPU_IP));

	if (((port >> 4) ^ fdc.chgreg) & 1) {
		return(0xff);
	}
	return(fdc.status);
}

static REG8 IOINPCALL fdc_i92(UINT port) {

	REG8	ret;

	if (((port >> 4) ^ fdc.chgreg) & 1) {
		return(0xff);
	}
	if ((fdc.status & (FDCSTAT_RQM | FDCSTAT_DIO))
										== (FDCSTAT_RQM | FDCSTAT_DIO)) {
		ret = fdc_dataread();
	}
	else {
		ret = fdc.lastdata;
	}
//	TRACEOUT(("fdc in %.2x %.2x [%.4x:%.4x]", port, ret, CPU_CS, CPU_IP));
	return(ret);
}

static REG8 IOINPCALL fdc_i94(UINT port) {

	if (((port >> 4) ^ fdc.chgreg) & 1) {
		return(0xff);
	}
	if (port & 0x10) {		// 94
		return(0x40);
	}
	else {					// CC
		return(0x70);		// readyを立てるるる
	}
}

#if defined(SUPPORT_PC88VA)

static REG8 IOINPCALL fdcva_i_fdc0(UINT port) {
#if defined(VAEG_EXT)
	fdc.status = fdc.status & 0xf0 | fdbusybits();
#endif

//	TRACEOUT(("fdcva: in %.2x %.2x [%.4x:%.4x]", port, fdc.status,
//															CPU_CS, CPU_IP));

	return(fdc.status);
}

static REG8 IOINPCALL fdcva_i_fdc1(UINT port) {

	REG8	ret;

	if ((fdc.status & (FDCSTAT_RQM | FDCSTAT_DIO))
										== (FDCSTAT_RQM | FDCSTAT_DIO)) {
		ret = fdc_dataread();
	}
	else {
		ret = fdc.lastdata;
	}
//	TRACEOUT(("fdcva: in %.2x %.2x [%.4x:%.4x]", port, ret, CPU_CS, CPU_IP));
	return(ret);
}

static REG8 IOINPCALL fdcva_i_dskmisc(UINT port) {
	REG8	ret;

	ret = 0xa6 | 0x10;			// 常にready
								// VA2のmonでi1b6 したら a6 が帰ってくる
								// ToDo: bit4以外も意味があるのか？？

//	TRACEOUT(("fdcva: in %.2x %.2x [%.4x:%.4x]", port, ret, CPU_CS, CPU_IP));
	return ret;
}

#endif



static void IOOUTCALL fdc_obe(UINT port, REG8 dat) {

#if defined(SUPPORT_PC88VA)
	int	i;

	fdc.chgreg = dat;
	if (fdc.chgreg & 2) {
		for (i = 0; i < 4; i++) CTRL_FDMEDIA[i] = DISKTYPE_2HD;
#if defined(VAEG_FIX)
		fdc.clock = CLOCK80;
#endif
	}
	else {
		for (i = 0; i < 4; i++) CTRL_FDMEDIA[i] = DISKTYPE_2DD;
#if defined(VAEG_FIX)
		fdc.clock = CLOCK48;
#endif
	}
#else
	fdc.chgreg = dat;
	if (fdc.chgreg & 2) {
		CTRL_FDMEDIA = DISKTYPE_2HD;
	}
	else {
		CTRL_FDMEDIA = DISKTYPE_2DD;
	}
#endif
	(void)port;
}

static REG8 IOINPCALL fdc_ibe(UINT port) {

	(void)port;
	return((fdc.chgreg & 3) | 8);
}

#if defined(SUPPORT_PC88VA)

static void IOOUTCALL fdcva_o_dskctl(UINT port, REG8 dat) {
/*
	ToDo:
	ドライブのモードにあわせて CTRL_FDMEDIA を切り替える必要があるが、
	98は2ドライブ共通なのに対し、VAは独立に指定できるため、
	困った。→対応済み
	また、2Dの扱いは・・・？→対応済み
*/
	int i;

	TRACEOUT(("fdcva: %.4x %.2x", port, dat));
	for (i = 0; i < 2; i++) {
		if (dat & (1 << i)) {
			CTRL_FDMEDIA[i] = DISKTYPE_2HD;
		}
		else {
			CTRL_FDMEDIA[i] = DISKTYPE_2DD;
		}
	}
	for (i = 0; i < 2; i++) {
		if (dat & (4 << i)) {
			fdc.trackdensity[i] = FDD_96TPI;
		}
		else {
			fdc.trackdensity[i] = FDD_48TPI;
		}
	}
#if defined(VAEG_FIX)
	if (dat & 0x20) {
		// 8MHz
		fdc.clock = CLOCK80;
	}
	else {
		// 4.8MHz
		fdc.clock = CLOCK48;
	}
#endif
	(void)port;
}

#endif

static void IOOUTCALL fdc_o4be(UINT port, REG8 dat) {

	fdc.reg144 = dat;
	if (dat & 0x10) {
		fdc.rpm[(dat >> 5) & 3] = dat & 1;
	}
	(void)port;
}

static REG8 IOINPCALL fdc_i4be(UINT port) {

	(void)port;
	return(fdc.rpm[(fdc.reg144 >> 5) & 3] | 0xf0);
}

#if defined(SUPPORT_PC88VA)

static void IOOUTCALL fdcva_o_mtrctl(UINT port, REG8 dat) {
//	TRACEOUT(("fdcva: out %.2x %.2x [%.4x:%.4x]", port, dat, CPU_CS, CPU_IP));

	// TODO: ドライブ1と2のモーターを区別せず駆動している。
	//       正確にはわける必要がある。
	if (dat & 0x03) {
		if (fdc.motor[0] == FDD_MOTOR_STOPPED) {
			fdc.motor[0] = fdc.motor[1] = FDD_MOTOR_STARTING;
			nevent_setbyms(NEVENT_FDDMOTOR, FDD_MOTORDELAY, fdc_fddmotor, NEVENT_ABSOLUTE);
#if defined(VAEG_FIX)
			start_statewatch();
#endif
		}
	}
	else {
		if (fdc.motor[0] != FDD_MOTOR_STOPPED) {
			fdc.motor[0] = fdc.motor[1] = FDD_MOTOR_STOPPED;
#if defined(VAEG_FIX)
			stop_statewatch();
#endif
		}
	}
}
#endif

// ---- for PC-88VA Main System

#if defined(SUPPORT_PC88VA)

static void IOOUTCALL fdcva_o1b0(UINT port, REG8 dat) {
	fdc.fddifmode = dat & 1;
}

static void IOOUTCALL fdcva_o1b2(UINT port, REG8 dat) {
	fdcva_o_dskctl(port, dat);
}

static void IOOUTCALL fdcva_o1b4(UINT port, REG8 dat) {
	fdcva_o_mtrctl(port, dat);
}

static void IOOUTCALL fdcva_o1b6(UINT port, REG8 dat) {
	fdcva_o_dskmisc(port, dat);
}

static void IOOUTCALL fdcva_o1ba(UINT port, REG8 dat) {
	fdcva_o_fdc1(port, dat);
}

static REG8 IOINPCALL fdcva_i1b6(UINT port) {
	return fdcva_i_dskmisc(port);
}

static REG8 IOINPCALL fdcva_i1b8(UINT port) {
	return fdcva_i_fdc0(port);
}

static REG8 IOINPCALL fdcva_i1ba(UINT port) {
	return fdcva_i_fdc1(port);
}

#endif

// ---- for FD Sub System

#if defined(SUPPORT_PC88VA)

BYTE fdcsubsys_i_fdc0(void) {
	return fdcva_i_fdc0(0);
}

BYTE fdcsubsys_i_fdc1(void) {
	return fdcva_i_fdc1(0);
}

void fdcsubsys_o_fdc1(BYTE dat) {
	fdcva_o_fdc1(0, dat);
}

void fdcsubsys_o_mtrctl(BYTE dat) {
	fdcva_o_mtrctl(0, dat);
}

void fdcsubsys_o_dskctl(BYTE dat) {
	fdcva_o_dskctl(0, dat);
}

void fdcsubsys_o_tc(void) {
	settc();
}

#endif

// ---- I/F

static const IOOUT fdco90[4] = {
					NULL,		fdc_o92,	fdc_o94,	NULL};
static const IOINP fdci90[4] = {
					fdc_i90,	fdc_i92,	fdc_i94,	NULL};
static const IOOUT fdcobe[1] = {fdc_obe};
static const IOINP fdcibe[1] = {fdc_ibe};

void fdc_reset(void) {

	ZeroMemory(&fdc, sizeof(fdc));
	fdc.equip = np2cfg.fddequip;
#if defined(SUPPORT_PC9821)
	fdc.support144 = 1;
#else
	fdc.support144 = np2cfg.usefd144;
#endif
	fdcstatusreset();
	dmac_attach(DMADEV_2HD, FDC_DMACH2HD);
	dmac_attach(DMADEV_2DD, FDC_DMACH2DD);
#if defined(SUPPORT_PC88VA)
	{
		UINT8 trackdensity;
		UINT8 ctrlfd;
		if (pccore.model_va == PCMODEL_NOTVA) {
			// 98
			ctrlfd = DISKTYPE_2HD;
			trackdensity = FDD_96TPI; 
		}
		else {
			// VA
			ctrlfd = DISKTYPE_2DD;
			trackdensity = FDD_48TPI; 
		}
		CTRL_FDMEDIA[0] = CTRL_FDMEDIA[1] = CTRL_FDMEDIA[2] = CTRL_FDMEDIA[3] = ctrlfd;
		fdc.trackdensity[0] = fdc.trackdensity[1] = fdc.trackdensity[2] = fdc.trackdensity[3] = trackdensity;
	}
#else
	CTRL_FDMEDIA = DISKTYPE_2HD;
#endif
	fdc.chgreg = 3;
#if defined(VAEG_EXT)
	fdc.headlastactive = -1;
	fdc.reach = FDD_HEADREACH_IDLE;
	fdc.clock = CLOCK48;

	soundmng_pcmstop(SOUND_PCMSEEK);
	seek1sound = FALSE;
#endif
#if defined(SUPPORT_PC88VA)
	if (pccore.model_va == PCMODEL_NOTVA) {
		// 98
		fdc.fddifmode = 1;	// DMA mode
	}
	else {
		// VA
		fdc.fddifmode = 0;	// Intelligent mode
	}
#endif
#if defined(VAEG_FIX)
	fdc.rqminterval = pccore.realclock / 100000;	// 10μsec
#endif
}

void fdc_bind(void) {

	iocore_attachcmnoutex(0x0090, 0x00f9, fdco90, 4);
	iocore_attachcmninpex(0x0090, 0x00f9, fdci90, 4);
	iocore_attachcmnoutex(0x00c8, 0x00f9, fdco90, 4);
	iocore_attachcmninpex(0x00c8, 0x00f9, fdci90, 4);

	if (fdc.support144) {
		iocore_attachout(0x04be, fdc_o4be);
		iocore_attachinp(0x04be, fdc_i4be);
	}
	iocore_attachsysoutex(0x00be, 0x0cff, fdcobe, 1);
	iocore_attachsysinpex(0x00be, 0x0cff, fdcibe, 1);

#if defined(SUPPORT_PC88VA)
	iocoreva_attachout(0x01b0, fdcva_o1b0);
	iocoreva_attachout(0x01b2, fdcva_o1b2);
	iocoreva_attachout(0x01b4, fdcva_o1b4);
	iocoreva_attachout(0x01b6, fdcva_o1b6);
	iocoreva_attachinp(0x01b6, fdcva_i1b6);
	iocoreva_attachinp(0x01b8, fdcva_i1b8);
	iocoreva_attachout(0x01ba, fdcva_o1ba);
	iocoreva_attachinp(0x01ba, fdcva_i1ba);
#endif
}

