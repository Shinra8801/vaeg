/*
 * TSP.C: PC-88VA Text Sprite Processor
 */

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"
#include	"memoryva.h"
#include	"timing.h"

#if defined(SUPPORT_PC88VA)

enum {
	// TSPコマンド
	CMD_DSPON	= 0x12,
	CMD_DSPOFF	= 0x13,
	CMD_DSPDEF	= 0x14,
	CMD_CURDEF	= 0x15,
	CMD_SPRON	= 0x82,
	CMD_SPROFF	= 0x83,
	CMD_SPRDEF	= 0x84,
	CMD_EXIT	= 0x88,

	// ステータス
	STATUS_BUSY	= 0x04,
};


		_TSP	tsp;

static BYTE *getsprinfo(int no) {
	return textmem + tsp.sprtable + no * 8;
}

/*
スプライトの表示状態を変更する
*/
static void sprsw(int no, BOOL sw) {
	BYTE *sprinfo;
	WORD d;

	sprinfo = getsprinfo(no);
	d = LOADINTELWORD(sprinfo + 0);
	if (sw) {
		d |= 0x0200;
	}
	else {
		d &= ~0x0200;
	}
	STOREINTELWORD(sprinfo + 0, d);
}

/*
DSPON (TSP表示開始)
*/
static void exec_dspon(void) {
	TRACEOUT(("tsp: dspon: param=0x%.2x, 0x%.2x, 0x%.2x",
		tsp.parambuf[0], tsp.parambuf[1], tsp.parambuf[2]));

	tsp.texttable = tsp.parambuf[0] << 8;
	tsp.dspon = TRUE;

	tsp.status &= ~STATUS_BUSY;
}

/*
DSPDEF (画面構成と表示形態)
*/
static void exec_dspdef(void) {
	TRACEOUT(("tsp: dspdef: param=0x%.2x, 0x%.2x, 0x%.2x, 0x%.2x, 0x%.2x, 0x%.2x",
		tsp.parambuf[0], tsp.parambuf[1], tsp.parambuf[2], 
		tsp.parambuf[3], tsp.parambuf[4], tsp.parambuf[5])); 

	tsp.attroffset = tsp.parambuf[0] + (WORD)tsp.parambuf[1] * 0x100;
	//tsp.pitch = tsp.parambuf[2] >> 4;
	tsp.lineheight = tsp.parambuf[3] + 1;
	tsp.hlinepos = tsp.parambuf[4];
	tsp.blink = tsp.parambuf[5] >> 3;		// ToDo: そのまま採用すると周期が長すぎる
											// テクマニの誤植？
	tsp.blinkcnt = tsp.blink;

	tsp.status &= ~STATUS_BUSY;
}

/*
CURDEF (カーソル形態の指定)
*/
static void exec_curdef(void) {
	TRACEOUT(("tsp: curdef: param=0x%.2x",tsp.parambuf[0])); 

	tsp.curn = tsp.parambuf[0] >> 3;
	tsp.be = tsp.parambuf[0] & 0x01;
	sprsw(tsp.curn, tsp.parambuf[0] & 0x02);

	tsp.status &= ~STATUS_BUSY;
}

/*
SPRON (スプライトの表示開始)
*/
static void exec_spron(void) {
	TRACEOUT(("tsp: spron: param=0x%.2x, 0x%.2x, 0x%.2x",tsp.parambuf[0], tsp.parambuf[1], tsp.parambuf[2])); 

	tsp.sprtable = tsp.parambuf[0] << 8;
	tsp.hspn = tsp.parambuf[2] >> 3;
	tsp.mg = tsp.parambuf[2] & 0x02;
	tsp.gr = tsp.parambuf[2] & 0x01;
	tsp.spron = TRUE;

	tsp.status &= ~STATUS_BUSY;
}

/*
SPRDEF (スプライト制御テーブルへの書き込み)
*/
// パラメータ2バイト目以降
static void paramfunc_sprdef(REG8 dat) {
	BYTE *mem;

	mem = textmem + tsp.sprtable + tsp.sprdef_offset;
	*mem = dat;
	tsp.sprdef_offset++;
}

// パラメータ1バイト目
static void paramfunc_sprdef_begin(REG8 dat) {
	TRACEOUT(("tsp: sprdef: offset=0x%.2x", dat)); 

	tsp.sprdef_offset = dat;
	tsp.paramfunc = paramfunc_sprdef;
}

/*
EXIT (処理中止)
*/
static void exec_exit(void) {
	TRACEOUT(("tsp: exit")); 

	tsp.status &= ~STATUS_BUSY;
}

/*
未定義コマンド
*/
static void exec_unknown(void) {
	TRACEOUT(("tsp: unknown cmd: 0x%.2x", tsp.cmd));
	tsp.status &= ~STATUS_BUSY;
}

/*
static void execcmd(void) {
	switch(tsp.cmd) {
	case CMD_DSPON:
		exec_dspon();
		break;
	case CMD_DSPDEF:
		exec_dspdef();
		break;
	case CMD_CURDEF:
		exec_curdef();
		break;
	case CMD_SPRON:
		exec_spron();
		break;
	default:
		TRACEOUT(("tsp: unknown cmd: 0x%.2x", tsp.cmd));
	}
}
*/

static void paramfunc_nop(REG8 dat) {
}

static void paramfunc_generic(REG8 dat) {
	if (tsp.recvdatacnt) {
		*(tsp.datap++) = dat;
		if (--tsp.recvdatacnt == 0) {
			tsp.paramfunc = paramfunc_nop;
			tsp.endparamfunc();
		}
	}
}

// ---- I/O


/*
ステータス読み出し
*/
static REG8 IOINPCALL tsp_i142(UINT port) {
	return tsp.status;
}

/*
コマンド書き込み
*/
static void IOOUTCALL tsp_o142(UINT port, REG8 dat) {
	TRACEOUT(("tsp: command: 0x%.2x", dat));

	tsp.cmd = dat;
	tsp.datap = tsp.parambuf;
	tsp.status |= STATUS_BUSY;

	switch(dat) {
	case CMD_DSPON:
		tsp.recvdatacnt = 3;
		tsp.endparamfunc = exec_dspon;
		tsp.paramfunc = paramfunc_generic;
		break;
	case CMD_DSPDEF:
		tsp.recvdatacnt = 6;
		tsp.endparamfunc = exec_dspdef;
		tsp.paramfunc = paramfunc_generic;
		break;
	case CMD_CURDEF:
		tsp.recvdatacnt = 1;
		tsp.endparamfunc = exec_curdef;
		tsp.paramfunc = paramfunc_generic;
		break;
	case CMD_SPRON:
		tsp.recvdatacnt = 3;
		tsp.endparamfunc = exec_spron;
		tsp.paramfunc = paramfunc_generic;
		break;
	case CMD_SPRDEF:
		tsp.paramfunc = paramfunc_sprdef_begin;
		break;
	case CMD_EXIT:
		tsp.paramfunc = paramfunc_nop;
		exec_exit();
		break;
	default:
		tsp.paramfunc = paramfunc_nop;
		exec_unknown();
		break;
	}

	//if (tsp.recvdatacnt == 0) execcmd();
}

/*
パラメータ書き込み
*/
static void IOOUTCALL tsp_o146(UINT port, REG8 dat) {
	TRACEOUT(("tsp: parameter: 0x%.2x", dat));
/*
	if (tsp.recvdatacnt) {
		*(tsp.datap++) = dat;
		if (--tsp.recvdatacnt == 0) execcmd();
	}
*/
	tsp.paramfunc(dat);
}

// ---- I/F

void tsp_reset(void) {
	ZeroMemory(&tsp, sizeof(tsp));
	tsp.paramfunc = paramfunc_nop;
}

void tsp_bind(void) {
	/*
	iocoreva_attachout(0x152, memctrlva_o152);
	iocoreva_attachout(0x153, memctrlva_o153);
	iocoreva_attachout(0x198, memctrlva_o198);
	iocoreva_attachout(0x19a, memctrlva_o19a);
	*/
	iocoreva_attachinp(0x142, tsp_i142);
	iocoreva_attachout(0x142, tsp_o142);
	iocoreva_attachout(0x146, tsp_o146);
}

// ---- 

void tsp_updateclock(void) {
	/*
	TSPの仕様はわからないので、代わりに、
	np2のGDCの処理と同じ計算を実施する。
	*/
	UINT hs = 7;		// 水平同期信号の幅(文字)
	UINT hfp = 9;		// 右方向の非表示区間(文字)
	UINT hbp = 7;		// 左方向の非表示区間(文字)
	UINT vs = 8;		// 垂直同期信号の幅(ライン)
	UINT vfp = 7;		// 下方向の非表示区間(ライン)
	UINT vbp = 0x19;	// 上方向の非表示区間(ライン)
	UINT lf = 400;		// 1画面あたり表示ライン数
	UINT cr = 80;		// 1行あたり表示文字数
	UINT32 clock = 21052600 / 8;	// 動作クロック=1秒あたり表示ブロック数
						// (ブロック=1文字の1ライン分)
	UINT x;
	UINT y;
	UINT cnt;
	UINT32 hclock;

	x = hfp + hbp + hs + cr + 3;	// 1行あたり総表示文字数
	y = vfp + vbp + vs + lf;		// 1画面あたり総表示ライン数

	hclock = clock / x;				// 1秒あたり表示ライン数
	cnt = (pccore.baseclock * y) / hclock;	// 1画面あたり時間(ベースクロック数)
	cnt *= pccore.multiple;			// 1画面あたり時間(CPUクロック数)
	tsp.rasterclock = cnt / y;
//	tsp.hsyncclock = (tsp.rasterclock * cr) / x;
	tsp.dispclock = tsp.rasterclock * lf;
	tsp.vsyncclock = cnt - tsp.dispclock;
	timing_setrate(y, hclock);


}

#endif
