#include	"compiler.h"
#include	"cpucore.h"
#include	"commng.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"keystat.h"

#if defined(SUPPORT_OPRECORD)
#include	"oprecord.h"
#endif

#if defined(SUPPORT_PC88VA)
/*
  ToDo: 
	Port 197h (RESETだけは実装済み)
*/

#include	"iocoreva.h"

/*
スキャンコード→ I/Oポート番号(上位4ビット), ビット番号(下位4ビット)
*/
static UINT8 scantomap[0x80]={
	// ESC,   1,   2,   3,   4,   5,   6,   7,   8,   9,   0,   -,   ^,   \,  BS, TAB,
	  0x97,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x70,0x71,0x60,0x57,0x56,0x54,0xc5,0xa0,
	//   Q,   W,   E,   R,   T,   Y,   U,   I,   O,   P,   @,   [, RET,   A,   S,   D,
	  0x41,0x47,0x25,0x42,0x44,0x51,0x45,0x31,0x37,0x40,0x20,0x53,0xe0,0x21,0x43,0x24,
	//   F,   G,   H,   J,   K,   L,   ;,   :,   ],   Z,   X,   C,   V,   B,   N,   M,
	  0x26,0x27,0x30,0x32,0x33,0x34,0x73,0x72,0x55,0x52,0x50,0x23,0x46,0x22,0x36,0x35,
	//   <,   >,   /,   _, SPC,変換, RUP,RDOWN,INS, DEL,  ↑,  ←,  →,  ↓, CLR,HELP,
      0x74,0x75,0x76,0x77,0xf6,0xd0,0xb0,0xb1,0xc6,0xc7,0x81,0xa2,0x82,0xa1,0x80,0xa3,
	//   -,   /,   7,   8,   9,   *,   4,   5,   6,   +,   1,   2,   3,   =,   0,   ,,
      0xa5,0xa6,0x07,0x10,0x11,0x12,0x04,0x05,0x06,0x13,0x01,0x02,0x03,0x14,0x00,0x15,
	//   .,決定,
      0x16,0xd1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	//STOP,COPY,  F1,  F2,  F3,  F4,  F5,  F6,  F7,  F8,  F9, F10
	  0x90,0xa4,0xf1,0xf2,0xf3,0xf4,0xf5,0xc0,0xc1,0xc2,0xc3,0xc4,0xff,0xff,0xff,0xff,
	//LSFT,CAPS,カナ,GRPH,CTRL,    ,    ,    ,RSFT, RET,  PC,全角
	  0xe2,0xa7,0x85,0x84,0x87,0xff,0xff,0xff,0xe3,0xe1,0xd2,0xd3,0xff,0xff,0xff,0xff,
};

/*
	以下はエミュレータ内部で使用する(I/Oからは読み出せない)
	0xf1-0xf5: F1-F5
	0xf6     : SPACE
*/

static void setmapbit(UINT8 pos, BOOL brkflag) {
	UINT8 bit;
	UINT8 port;

	if (pos == 0xff) return;

	bit = (1 << (pos & 0x07));
	port = pos >> 4;
	if (brkflag) {
		// break
		keybrd.keymap[port] |= bit;
	}
	else {
		// make
		keybrd.keymap[port] &= ~bit;
	}
}

static void updatekeymap(UINT8 scancode) {
	UINT8	pos;
	BOOL	brkflg;

	pos = scantomap[scancode & 0x7f];
	brkflg = scancode & 0x80;
	setmapbit(pos, brkflg);

	// SHIFT
	setmapbit(0x86, !(
	 (~keybrd.keymap[0x0e] & 0x0c) |	// SFT RT, SFT LT
	 (~keybrd.keymap[0x0c] & 0x5f)		// INS, F10-F6
	));

	// INSDEL
	setmapbit(0x83, !(~keybrd.keymap[0x0c] & 0xe0));	// INS,DEL,BS

	// RETURN
	setmapbit(0x17, !(~keybrd.keymap[0x0e] & 0x03));	// RET 10, RET FK

	// SPACES
	setmapbit(0x96, !(
	 (~keybrd.keymap[0x0f] & 0x40) |	// SPACE
	 (~keybrd.keymap[0x0d] & 0x03)		// 決定, 変換
	));

	// F1-F5
	keybrd.keymap[0x09] = (keybrd.keymap[0x09] & 0xc1) | 
						  ((keybrd.keymap[0x0c]<<1) & keybrd.keymap[0x0f] & 0x3e); 
}

static REG8 convertmodeldependent(REG8 data) {
	REG8	code;

	code = data & 0x7f;
	if (pccore.model_va != PCMODEL_NOTVA) {
		// VA
		if (code >= 0x52) {
			if (code < 0x58) {
				return 0xff;
			}
			else if (code < 0x5c) {
				code += 0x20;		// 右SFT, tenkey RET, PC, 全角
			}
			else if (code < 0x60) {
				return 0xff;
			}
			else if (code < 0x75) {
			}
			else {
				return 0xff;
			}
			data = (data & 0x80) | code; 
		}
	}
	else {
		// 98
		// SHIFT右、tenkey RETを、SHIFT, RETに変換
		if (code >= 0x58 && code <= 0x5b) {
			static REG8 repl[4] = {0x70, 0x1c, 0xff, 0xff};
			code = repl[code - 0x58];
			data = (data & 0x80) | code;
		}
	}
	return data;
}
#endif


// ---- Keyboard

void keyboard_callback(NEVENTITEM item) {

	if (item->flag & NEVENT_SETEVENT) {
		if ((keybrd.ctrls) || (keybrd.buffers)) {
			if (!(keybrd.status & 2)) {
				keybrd.status |= 2;
				if (keybrd.ctrls) {
					keybrd.ctrls--;
					keybrd.data = keybrd.ctr[keybrd.ctrpos];
					keybrd.ctrpos = (keybrd.ctrpos + 1) & KB_CTRMASK;
				}
				else if (keybrd.buffers) {
					keybrd.buffers--;
					keybrd.data = keybrd.buf[keybrd.bufpos];
					keybrd.bufpos = (keybrd.bufpos + 1) & KB_BUFMASK;
#if defined(SUPPORT_PC88VA)
					//updatekeymap(keybrd.data);
#endif
				}
				//TRACEOUT(("recv -> %02x", keybrd.data));
			}
			pic_setirq(1);
			nevent_set(NEVENT_KEYBOARD, keybrd.xferclock,
										keyboard_callback, NEVENT_RELATIVE);
		}
	}
}

static void IOOUTCALL keyboard_o41(UINT port, REG8 dat) {

	if (keybrd.cmd & 1) {
//		TRACEOUT(("send -> %02x", dat));
		keystat_ctrlsend(dat);
	}
	else {
		keybrd.mode = dat;
	}
	(void)port;
}

static void IOOUTCALL keyboard_o43(UINT port, REG8 dat) {

//	TRACEOUT(("out43 -> %02x %.4x:%.8x", dat, CPU_CS, CPU_EIP));
	if ((!(dat & 0x08)) && (keybrd.cmd & 0x08)) {
		keyboard_resetsignal();
	}
	if (dat & 0x10) {
		keybrd.status &= ~(0x38);
	}
	keybrd.cmd = dat;
	(void)port;
}

static REG8 IOINPCALL keyboard_i41(UINT port) {

	(void)port;
	keybrd.status &= ~2;
	pic_resetirq(1);
//	TRACEOUT(("in41 -> %02x %.4x:%.8x", keybrd.data, CPU_CS, CPU_IP));
	return(keybrd.data);
}

static REG8 IOINPCALL keyboard_i43(UINT port) {

	(void)port;
//	TRACEOUT(("in43 -> %02x %.4x:%.8x", keybrd.status, CPU_CS, CPU_IP));
	return(keybrd.status | 0x85);
}

#if defined(SUPPORT_PC88VA)

static REG8 IOINPCALL keyboardva_i000(UINT port) {
	(void)port;
	return(keybrd.keymap[port]);
}

static void IOOUTCALL keyboardva_o197(UINT port, REG8 dat) {

	TRACEOUT(("keyboard: o197 -> %02x %.4x:%.4x", dat, CPU_CS, CPU_IP));
	if ((dat & 0x40) == 0x40) {		// テクマニではbit7,6=1,1 or 1,0となっているが、
									// VAのROMは0,1または1,0で本ポートに出力している。
									// このため、bit7は無視
		// モードコマンド
	}
	else {
		// オペレーションコマンド
		if (dat & 0x01) {
			// RESET
			keyboard_resetsignal();
		}
	}
	
	(void)port;
}

#endif

// ----

static const IOOUT keybrdo41[2] = {
					keyboard_o41,	keyboard_o43};

static const IOINP keybrdi41[2] = {
					keyboard_i41,	keyboard_i43};


void keyboard_reset(void) {
#if defined(SUPPORT_PC88VA)
	UINT8	mapbkup[KB_MAP];

	// リセット時はkeymapをクリアしない
	CopyMemory(mapbkup, keybrd.keymap, sizeof(mapbkup));
#endif

	ZeroMemory(&keybrd, sizeof(keybrd));
	keybrd.data = 0xff;
	keybrd.mode = 0x5e;

#if defined(SUPPORT_PC88VA)
	CopyMemory(keybrd.keymap, mapbkup, sizeof(keybrd.keymap));
	/*
	{
		int i;

		for (i = 0; i < KB_MAP; i++) {
			keybrd.keymap[i] = 0xff;
		}
	}
	*/
#endif

}

void keyboard_bind(void) {

	keystat_ctrlreset();
	keybrd.xferclock = pccore.realclock / 1920;
	iocore_attachsysoutex(0x0041, 0x0cf1, keybrdo41, 2);
	iocore_attachsysinpex(0x0041, 0x0cf1, keybrdi41, 2);

#if defined(SUPPORT_PC88VA)
	{
		int i;
		for (i = 0; i < 0x0f; i++) {
			iocoreva_attachinp(i, keyboardva_i000);
		}
		iocoreva_attachinp(0x1c1, keyboard_i41);
		iocoreva_attachout(0x197, keyboardva_o197);
	}
#endif
}

void keyboard_resetsignal(void) {

#if defined(SUPPORT_PC88VA)
	int i;

	for (i = 0; i < KB_MAP; i++) {
		keybrd.keymap[i] = 0xff;
	}
#endif

	nevent_reset(NEVENT_KEYBOARD);
	keybrd.cmd = 0;
	keybrd.status = 0;
	keybrd.ctrls = 0;
	keybrd.buffers = 0;
	keystat_ctrlreset();
	keystat_resendstat();
}

void keyboard_ctrl(REG8 data) {

	if ((data == 0xfa) || (data == 0xfc)) {
		keybrd.ctrls = 0;
	}
	if (keybrd.ctrls < KB_CTR) {
		keybrd.ctr[(keybrd.ctrpos + keybrd.ctrls) & KB_CTRMASK] = data;
		keybrd.ctrls++;
		if (!nevent_iswork(NEVENT_KEYBOARD)) {
			nevent_set(NEVENT_KEYBOARD, keybrd.xferclock,
										keyboard_callback, NEVENT_ABSOLUTE);
		}
	}
}

void keyboard_send(REG8 data) {

#if defined(SUPPORT_OPRECORD)
	oprecord_record_key(data);
#endif

#if defined(SUPPORT_PC88VA)
	data = convertmodeldependent(data);
	if (data == 0xff) return;
	updatekeymap(data);
#endif

	if (keybrd.buffers < KB_BUF) {
		keybrd.buf[(keybrd.bufpos + keybrd.buffers) & KB_BUFMASK] = data;
		keybrd.buffers++;
		if (!nevent_iswork(NEVENT_KEYBOARD)) {
			nevent_set(NEVENT_KEYBOARD, keybrd.xferclock,
										keyboard_callback, NEVENT_ABSOLUTE);
		}
	}
	else {
		keybrd.status |= 0x10;
	}
}



// ---- RS-232C

	COMMNG	cm_rs232c;

void rs232c_construct(void) {

	cm_rs232c = NULL;
}

void rs232c_destruct(void) {

	commng_destroy(cm_rs232c);
	cm_rs232c = NULL;
}

void rs232c_open(void) {

	if (cm_rs232c == NULL) {
		cm_rs232c = commng_create(COMCREATE_SERIAL);
#if defined(VAEG_FIX)
		cm_rs232c->msg(cm_rs232c, COMMSG_SETRSFLAG, rs232c.cmd & 0x22); /* RTS, DTR */
#endif
	}
}

void rs232c_callback(void) {

	BOOL	interrupt;

	interrupt = FALSE;
	if ((cm_rs232c) && (cm_rs232c->read(cm_rs232c, &rs232c.data))) {
		rs232c.result |= 2;
		if (sysport.c & 1) {
			interrupt = TRUE;
		}
	}
	else {
		rs232c.result &= (BYTE)~2;
	}
	if (sysport.c & 4) {
		if (rs232c.send) {
			rs232c.send = 0;
			interrupt = TRUE;
		}
	}
	if (interrupt) {
		pic_setirq(4);
	}
}

BYTE rs232c_stat(void) {

	if (cm_rs232c == NULL) {
#if defined(VAEG_FIX)
		rs232c_open();
#else
		cm_rs232c = commng_create(COMCREATE_SERIAL);
#endif
	}
	return(cm_rs232c->getstat(cm_rs232c));
}

void rs232c_midipanic(void) {

	if (cm_rs232c) {
		cm_rs232c->msg(cm_rs232c, COMMSG_MIDIRESET, 0);
	}
}


// ----

static void IOOUTCALL rs232c_o30(UINT port, REG8 dat) {

	if (cm_rs232c) {
		cm_rs232c->write(cm_rs232c, (UINT8)dat);
	}
	if (sysport.c & 4) {
		rs232c.send = 0;
		pic_setirq(4);
	}
	else {
		rs232c.send = 1;
	}
	(void)port;
}

static void IOOUTCALL rs232c_o32(UINT port, REG8 dat) {

	if (!(dat & 0xfd)) {
		rs232c.dummyinst++;
	}
	else {
		if ((rs232c.dummyinst >= 3) && (dat == 0x40)) {
			rs232c.pos = 0;
		}
		rs232c.dummyinst = 0;
	}
	switch(rs232c.pos) {
		case 0x00:			// reset
			rs232c.pos++;
			break;

		case 0x01:			// mode
			if (!(dat & 0x03)) {
				rs232c.mul = 10 * 16;
			}
			else {
				rs232c.mul = ((dat >> 1) & 6) + 10;
				if (dat & 0x10) {
					rs232c.mul += 2;
				}
				switch(dat & 0xc0) {
					case 0x80:
						rs232c.mul += 3;
						break;
					case 0xc0:
						rs232c.mul += 4;
						break;
					default:
						rs232c.mul += 2;
						break;
				}
				switch(dat & 0x03) {
					case 0x01:
						rs232c.mul >>= 1;
						break;
					case 0x03:
						rs232c.mul *= 32;
						break;
					default:
						rs232c.mul *= 8;
						break;
				}
			}
			rs232c.pos++;
			break;

		case 0x02:			// cmd
#if defined(VAEG_FIX)
			rs232c.cmd = dat;
			if (cm_rs232c) {
				cm_rs232c->msg(cm_rs232c, COMMSG_SETRSFLAG, dat & 0x22); /* RTS, DTR */
			}
#else
									// sysport.c下位3bitはRS-232Cの割り込みマスクで
									// あって、cmdでは変化しないはずなのだが・・・
									// また、rs232c.pos++ により、次回以降のcmdを
									// 受け付けなくなる。
									// (Shinra)
			sysport.c &= ~7;
			sysport.c |= (dat & 7);
			rs232c.pos++;
#endif
			break;
	}
	(void)port;
}

static REG8 IOINPCALL rs232c_i30(UINT port) {

	(void)port;
	return(rs232c.data);
}

static REG8 IOINPCALL rs232c_i32(UINT port) {

	if (!(rs232c_stat() & 0x20)) {
		return(rs232c.result | 0x80);
	}
	else {
		(void)port;
		return(rs232c.result);
	}
}


// ----

static const IOOUT rs232co30[2] = {
					rs232c_o30,	rs232c_o32};

static const IOINP rs232ci30[2] = {
					rs232c_i30,	rs232c_i32};

void rs232c_reset(void) {

	commng_destroy(cm_rs232c);
	cm_rs232c = NULL;
	rs232c.result = 0x05;
	rs232c.data = 0xff;
	rs232c.send = 1;
	rs232c.pos = 0;
	rs232c.dummyinst = 0;
	rs232c.mul = 10 * 16;
#if defined(VAEG_FIX)
	rs232c.cmd = 0;
#endif
}

void rs232c_bind(void) {

	iocore_attachsysoutex(0x0030, 0x0cf1, rs232co30, 2);
	iocore_attachsysinpex(0x0030, 0x0cf1, rs232ci30, 2);

#if defined(SUPPORT_PC88VA)
	iocoreva_attachout(0x020, rs232c_o30);
	iocoreva_attachout(0x021, rs232c_o32);
	iocoreva_attachinp(0x020, rs232c_i30);
	iocoreva_attachinp(0x021, rs232c_i32);
#endif
}

