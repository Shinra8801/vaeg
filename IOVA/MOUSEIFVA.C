/*
 * MOUSEIFVA.C: PC-88VA Mouse interface
 */

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"joymng.h"
#include	"keystat.h"
#include	"oprecord.h"

#include	"iocoreva.h"


#if defined(SUPPORT_PC88VA)

enum {
	XH	= 0,
	XL	= 1,
	YH	= 2,
	YL	= 3,
	INITSTATE	= XH,
	LASTSTATE	= YL,
};


	_MOUSEIFVACFG	mouseifvacfg = {MOUSEIFVA_JOYPAD};

	_MOUSEIFVA		mouseifva;

// ---- Mouse

static void latch(void) {

	calc_mousexy();
	mouseif.latch_x = mouseif.x;
	mouseif.x = 0;
	mouseif.latch_y = mouseif.y;
	mouseif.y = 0;
	if (mouseif.latch_x > 127) {
		mouseif.latch_x = 127;
	}
	else if (mouseif.latch_x < -128) {
		mouseif.latch_x = -128;
	}
	if (mouseif.latch_y > 127) {
		mouseif.latch_y = 127;
	}
	else if (mouseif.latch_y < -128) {
		mouseif.latch_y = -128;
	}
}


/*
マウスへの出力
	IN:
		strobe	ストローブ
*/
void mouseva_outstrobe(UINT8 strobe) {
	if (strobe ^ mouseifva.laststrobe) {
		mouseifva.laststrobe = strobe;
		mouseifva.state++;
		if (mouseifva.state > LASTSTATE) {
			mouseifva.state = INITSTATE;
			latch();
		}
	}
}

/*
マウスからの入力
	IN:
		data4	入力データの格納先
		button	ボタンの格納先
				ボタン: bit5=右, bit7=左  0で押下
*/
void mouseva_indata(UINT8 *data, UINT8 *button) {
	UINT8 x,y,b;

	x = -mouseif.latch_x;
	y = -mouseif.latch_y;
	switch(mouseifva.state) {
	case XL:
		*data = x & 0x0f;
		break;
	case XH:
		*data = (x >> 4) & 0x0f;
		break;
	case YL:
		*data = y & 0x0f;
		break;
	case YH:
		*data = (y >> 4) & 0x0f;
		break;
	}

	b = mouseif.b;
	if (np2cfg.MOUSERAPID) {
		b |= mouseif.rapid;
	}
	*button = 0;
	if (b & 0x20) *button |= 0x02;				// 左
	if (b & 0x80) *button |= 0x01;				// 右
}


// ---- Joy pad

/*
ジョイパッドからの入力
	IN:
		data4	入力データの格納先
		button	ボタンの格納先
				ボタン: bit0=A, bit1=B  0で押下
*/
static void joypad_indata(UINT8 *data, UINT8 *button) {
	static	REG8	rapids = 0;
	static	SINT32	lastc = 0;
	REG8	ret;
	UINT32	clock;
	SINT32	diff;

#if defined(SUPPORT_OPRECORD)
	if (!oprecord_play_joypad(0, data, button)) return;
#endif

	clock = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
	diff = clock - lastc;

	if (diff > 200000 || diff < 0) {	// 8MHzで20打/秒 くらい
										// lastcが凄く昔の場合にdiff < 0となることがある
		rapids ^= 0xf0;
		lastc = clock;
	}
	ret = 0xff;

	ret &= (joymng_getstat() | (rapids & 0x30));
	if (np2cfg.KEY_MODE == 1) {
		ret &= keystat_getjoy();
	}
	if (np2cfg.BTN_RAPID) {
		ret |= rapids;
	}

	// rapidと非rapidを合成
	ret &= ((ret >> 2) | (~0x30));

	if (np2cfg.BTN_MODE) {
		BYTE bit1 = (ret & 0x20) >> 1;
		BYTE bit2 = (ret & 0x10) << 1;
		ret = (ret & (~0x30)) | bit1 | bit2;
	}

	*data = ret & 0x0f;
	*button = (ret >> 4) & 0x03;

#if defined(SUPPORT_OPRECORD)
	oprecord_record_joypad(0, *data, *button);
#endif
}


// ---- Mouse port

/*
マウスポートへSTROBEを出力
*/
void mouseifva_outstrobe(UINT8 strobe) {
	switch(mouseifvacfg.device) {
	case MOUSEIFVA_MOUSE:
		mouseva_outstrobe(strobe);
		break;
	}
}

/*
マウスポートからデータ入力
*/
void mouseifva_indata(UINT8 *data4, UINT8 *data2) { 
	switch(mouseifvacfg.device) {
	case MOUSEIFVA_MOUSE:
		mouseva_indata(data4, data2);
		break;
	default:
		joypad_indata(data4, data2);
		break;
	}
}

// ---- I/O

static void IOOUTCALL mouseifva_o1a8(UINT port, REG8 dat) {
	UINT8	xminten;

	mouseif.timing = dat & 3;
	xminten = (~dat >> 3) & 0x10;			// 割り込み許可フラグ
											// 0.許可 1.禁止

	if ((xminten ^ mouseif.upd8255.portc) & 0x10) {
		// 変更あり
		if (!(xminten & 0x10)) {
			if (!nevent_iswork(NEVENT_MOUSE)) {
				nevent_set(NEVENT_MOUSE, mouseif.intrclock << mouseif.timing,
												mouseint, NEVENT_ABSOLUTE);
			}
		}
		mouseif.upd8255.portc = mouseif.upd8255.portc & ~0x10 | xminten;
	}
}

// ---- I/F

void mouseifva_reset(void) {
	ZeroMemory(&mouseifva, sizeof(mouseifva));
	mouseifva.state = LASTSTATE;
}

void mouseifva_bind(void) {

	iocoreva_attachout(0x1a8, mouseifva_o1a8);
}

#endif
