#include	"compiler.h"
#include	"pccore.h"
#include	"diskdrv.h"
#include	"fdd_mtr.h"
#include	"timing.h"


#define	MSSHIFT		16

typedef struct {
	UINT32	tick;			// 前回timing_getcount実行時のGETTICK()の値
	UINT32	msstep;			// 1msecあたりの画面表示サイクル数 << MSSHIFT
	UINT	cnt;			// 経過時間を画面表示サイクル数であらわしたもの(整数部)
	UINT32	fraction;		// 経過時間を画面表示サイクル数であらわしたもの(小数点以下MSSHIFTビット)
} TIMING;

static	TIMING	timing;


void timing_reset(void) {

	timing.tick = GETTICK();
	timing.cnt = 0;
	timing.fraction = 0;
}

/*
表示周期を設定する
	IN:		lines		1画面あたりのライン数(非表示区間、垂直同期区間を含む)
			crthz		1秒当り描画ライン数
*/
void timing_setrate(UINT lines, UINT crthz) {

	timing.msstep = (crthz << (MSSHIFT - 3)) / lines / (1000 >> 3);
}

void timing_setcount(UINT value) {

	timing.cnt = value;
}

/*
経過時間を画面表示サイクル数で返却する
	この値はtiming_setcountでリセットできる。
*/
UINT timing_getcount(void) {

	UINT32	ticknow;
	UINT32	span;
	UINT32	fraction;

	ticknow = GETTICK();
	span = ticknow - timing.tick;
	if (span) {
		timing.tick = ticknow;
		fddmtr_callback(ticknow);

		if (span >= 1000) {
			span = 1000;
		}
		fraction = timing.fraction + (span * timing.msstep);
		timing.cnt += fraction >> MSSHIFT;
		timing.fraction = fraction & ((1 << MSSHIFT) - 1);
	}
	return(timing.cnt);
}

