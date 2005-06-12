/*
 * SGP.C: PC-88VA Super Graphic Processor
 */

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"
#include	"memoryva.h"
#include	"gvramva.h"

#if defined(SUPPORT_PC88VA)

#define		SWAPWORD(x)	( ((x) << 8) | ((x) >> 8) )


		_SGP	sgp;


// ---- 

static REG16 MEMCALL mainw_rd(UINT32 address) {
	return *(REG16 *)(mem + address);
}

static void MEMCALL mainw_wt(UINT32 address, REG16 value) {
	*(REG16 *)(mem + address) = value;
}

static REG16 MEMCALL bmsw_rd(UINT32 address) {
	return 0;
}

static void MEMCALL bmsw_wt(UINT32 address, REG16 value) {
}

static REG16 MEMCALL tvramw_rd(UINT32 address) {
	return 0;
}

static void MEMCALL tvramw_wt(UINT32 address, REG16 value) {
}

static REG16 MEMCALL gvramw_rd(UINT32 address) {
	address -= 0x200000L;
	return _gvram_rd(address) | ((REG16)_gvram_rd(address + 1) << 8);
}

static void MEMCALL gvramw_wt(UINT32 address, REG16 value) {
	address -= 0x200000L;
/*
	_gvram_wt(address, (REG8)(value & 0xff));
	_gvram_wt(address + 1, (REG8)(value >> 8));
*/
	//_gvramw_wt(address, value);
	*(UINT16 *)(grphmem + address) = value;
}

static REG16 MEMCALL knj1w_rd(UINT32 address) {
	return 0;
}

static REG16 MEMCALL knj2w_rd(UINT32 address) {
	return 0;
}

static void MEMCALL knj2w_wt(UINT32 address, REG16 value) {
}

static REG16 MEMCALL nonw_rd(UINT32 address) {
	return 0xffff;
}

static void MEMCALL nonw_wt(UINT32 address, REG16 value) {
}


typedef void (MEMCALL * MEM16WRITE)(UINT32 address, REG16 value);
typedef REG16 (MEMCALL * MEM16READ)(UINT32 address);

static MEM16READ rd16[0x40] = {
	mainw_rd,	// 00xxxx
	mainw_rd,	// 01xxxx
	mainw_rd,	// 02xxxx
	mainw_rd,	// 03xxxx
	mainw_rd,	// 04xxxx
	mainw_rd,	// 05xxxx
	mainw_rd,	// 06xxxx
	mainw_rd,	// 07xxxx
	bmsw_rd,	// 08xxxx
	bmsw_rd,	// 09xxxx
	nonw_rd,	// 0axxxx	ToDo
	nonw_rd,	// 0bxxxx	ToDo
	nonw_rd,	// 0cxxxx	ToDo
	nonw_rd,	// 0dxxxx	ToDo
	nonw_rd,	// 0exxxx	ToDo
	nonw_rd,	// 0fxxxx	ToDo

	knj1w_rd,	// 10xxxx
	knj1w_rd,	// 11xxxx
	knj1w_rd,	// 12xxxx
	knj1w_rd,	// 13xxxx
	knj2w_rd,	// 14xxxx
	knj2w_rd,	// 15xxxx
	knj2w_rd,	// 16xxxx
	knj2w_rd,	// 17xxxx
	tvramw_rd,	// 18xxxx
	tvramw_rd,	// 19xxxx	ToDo 本当？
	tvramw_rd,	// 1axxxx	ToDo 本当？
	tvramw_rd,	// 1bxxxx	ToDo 本当？
	nonw_rd,	// 1cxxxx	ToDo
	nonw_rd,	// 1dxxxx	ToDo
	nonw_rd,	// 1exxxx	ToDo
	nonw_rd,	// 1fxxxx	ToDo

	gvramw_rd,	// 20xxxx
	gvramw_rd,	// 21xxxx
	gvramw_rd,	// 22xxxx
	gvramw_rd,	// 23xxxx
	nonw_rd,	// 24xxxx
	nonw_rd,	// 25xxxx
	nonw_rd,	// 26xxxx
	nonw_rd,	// 27xxxx
	nonw_rd,	// 28xxxx
	nonw_rd,	// 29xxxx
	nonw_rd,	// 2axxxx
	nonw_rd,	// 2bxxxx
	nonw_rd,	// 2cxxxx
	nonw_rd,	// 2dxxxx
	nonw_rd,	// 2exxxx
	nonw_rd,	// 2fxxxx

	nonw_rd,	// 30xxxx
	nonw_rd,	// 31xxxx
	nonw_rd,	// 32xxxx
	nonw_rd,	// 33xxxx
	nonw_rd,	// 34xxxx
	nonw_rd,	// 35xxxx
	nonw_rd,	// 36xxxx
	nonw_rd,	// 37xxxx
	nonw_rd,	// 38xxxx
	nonw_rd,	// 39xxxx
	nonw_rd,	// 3axxxx
	nonw_rd,	// 3bxxxx
	nonw_rd,	// 3cxxxx
	nonw_rd,	// 3dxxxx
	nonw_rd,	// 3exxxx
	nonw_rd,	// 3fxxxx

};


static MEM16WRITE wt16[0x40] = {
	mainw_wt,	// 00xxxx
	mainw_wt,	// 01xxxx
	mainw_wt,	// 02xxxx
	mainw_wt,	// 03xxxx
	mainw_wt,	// 04xxxx
	mainw_wt,	// 05xxxx
	mainw_wt,	// 06xxxx
	mainw_wt,	// 07xxxx
	bmsw_wt,	// 08xxxx
	bmsw_wt,	// 09xxxx
	nonw_wt,	// 0axxxx	ToDo
	nonw_wt,	// 0bxxxx	ToDo
	nonw_wt,	// 0cxxxx	ToDo
	nonw_wt,	// 0dxxxx	ToDo
	nonw_wt,	// 0exxxx	ToDo
	nonw_wt,	// 0fxxxx	ToDo

	nonw_wt,	// 10xxxx
	nonw_wt,	// 11xxxx
	nonw_wt,	// 12xxxx
	nonw_wt,	// 13xxxx
	knj2w_wt,	// 14xxxx
	knj2w_wt,	// 15xxxx
	knj2w_wt,	// 16xxxx
	knj2w_wt,	// 17xxxx
	tvramw_wt,	// 18xxxx
	tvramw_wt,	// 19xxxx	ToDo 本当？
	tvramw_wt,	// 1axxxx	ToDo 本当？
	tvramw_wt,	// 1bxxxx	ToDo 本当？
	nonw_wt,	// 1cxxxx	ToDo
	nonw_wt,	// 1dxxxx	ToDo
	nonw_wt,	// 1exxxx	ToDo
	nonw_wt,	// 1fxxxx	ToDo

	gvramw_wt,	// 20xxxx
	gvramw_wt,	// 21xxxx
	gvramw_wt,	// 22xxxx
	gvramw_wt,	// 23xxxx
	nonw_wt,	// 24xxxx
	nonw_wt,	// 25xxxx
	nonw_wt,	// 26xxxx
	nonw_wt,	// 27xxxx
	nonw_wt,	// 28xxxx
	nonw_wt,	// 29xxxx
	nonw_wt,	// 2axxxx
	nonw_wt,	// 2bxxxx
	nonw_wt,	// 2cxxxx
	nonw_wt,	// 2dxxxx
	nonw_wt,	// 2exxxx
	nonw_wt,	// 2fxxxx

	nonw_wt,	// 30xxxx
	nonw_wt,	// 31xxxx
	nonw_wt,	// 32xxxx
	nonw_wt,	// 33xxxx
	nonw_wt,	// 34xxxx
	nonw_wt,	// 35xxxx
	nonw_wt,	// 36xxxx
	nonw_wt,	// 37xxxx
	nonw_wt,	// 38xxxx
	nonw_wt,	// 39xxxx
	nonw_wt,	// 3axxxx
	nonw_wt,	// 3bxxxx
	nonw_wt,	// 3cxxxx
	nonw_wt,	// 3dxxxx
	nonw_wt,	// 3exxxx
	nonw_wt,	// 3fxxxx

};

/*
static BYTE sgp_memoryread(UINT32 address) {
//...
}
*/

/*
	入力:
		address		アドレス(偶数)
*/
static REG16 sgp_memoryread_w(UINT32 address) {
	return rd16[(address >> 16) & 0x3f](address);
}

/*
	入力:
		address		アドレス(偶数)
*/
static void sgp_memorywrite_w(UINT32 address, REG16 value) {
	wt16[(address >> 16) & 0x3f](address, value);
}

// ---- 
static int dotcountmax[4] = {0x10, 0x04, 0x02, 0x01};
static int bpp[4] = {1, 4, 8, 16};

static void fetch_command(void);

static void fetch_block(UINT32 address, SGP_BLOCK block) {
	UINT16 dat;

	dat = sgp_memoryread_w(address);
	block->scrnmode = dat & 0x03;
	block->dot = (dat >> 4) & (dotcountmax[block->scrnmode] -1);
	block->width = sgp_memoryread_w(address + 2) & 0x0fff;
	block->height = sgp_memoryread_w(address + 4) & 0x0fff;
	block->fbw = sgp_memoryread_w(address + 6) & 0xfffc;
	block->address = sgp_memoryread_w(address + 8) & 0xfffe | ((UINT32)sgp_memoryread_w(address + 10) << 16);

	// ToDo とりあえずのつじつまあわせ(R-TYPE)
	if (block->fbw < 0) block->fbw += 4;
}

static void init_block(SGP_BLOCK block) {
	block->lineaddress = block->address;
	block->nextaddress = block->address;
	block->ycount = block->height;
}

static void read_word(SGP_BLOCK block) {
	UINT16 dat;

	dat = sgp_memoryread_w(block->nextaddress);
//	block->buf = (dat << 8) | (dat >> 8);
	block->buf = SWAPWORD(dat);
	block->nextaddress += 2;
	block->dotcount = dotcountmax[block->scrnmode];
}

/*
static void write_word(SGP_BLOCK block) {
	UINT16 dat;
	UINT16 mask;

	dat = (block->buf << 8) | (block->buf >> 8);
	mask = (block->mask << 8) | (block->mask >> 8);
	dat = dat & mask | (sgp_memoryread_w(block->nextaddress) & ~mask);
	sgp_memorywrite_w(block->nextaddress, dat);
	block->nextaddress += 2;
	block->dotcount = dotcountmax[block->scrnmode];
}
*/

static void write_dest(void) {
	UINT16 dat;
	UINT16 mask;

	dat = SWAPWORD(sgp.newval);
	mask = SWAPWORD(sgp.newvalmask);
	dat = dat & mask | (sgp_memoryread_w(sgp.dest.nextaddress) & ~mask);
	sgp_memorywrite_w(sgp.dest.nextaddress, dat);
	sgp.dest.nextaddress += 2;
	sgp.dest.dotcount = dotcountmax[sgp.dest.scrnmode];
}

static void init_src_line(SGP_BLOCK block) {
	read_word(block);
	//block->dotcount = dotcountmax[block->scrnmode];
	//if (!sf) {
		block->dotcount -= block->dot;
	//}
	block->buf <<= (dotcountmax[block->scrnmode] - block->dotcount) * bpp[block->scrnmode];

	block->xcount = block->width;
}
/*
static void init_dest_line(SGP_BLOCK block) {
	block->buf = 0;
	block->mask = 0;
	//read_word(block);
	block->dotcount = dotcountmax[block->scrnmode];
	//if (!sf) {
		block->dotcount -= block->dot;
	//}
	//block->buf <<= (dotcountmax[block->scrnmode] - block->dotcount) * bpp[block->scrnmode];

	block->xcount = block->width;
}
*/
static void init_dest_line(SGP_BLOCK block) {
	sgp.newval = 0;
	sgp.newvalmask = 0;
	//read_word(block);
	sgp.dest.dotcount = dotcountmax[sgp.dest.scrnmode];
	//if (!sf) {
		sgp.dest.dotcount -= sgp.dest.dot;
	//}
	//block->buf <<= (dotcountmax[block->scrnmode] - block->dotcount) * bpp[block->scrnmode];

	sgp.dest.xcount = sgp.dest.width;
}

static void cmd_unknown(void) {
	// 無効な命令
	TRACEOUT(("SGP: cmd: unknown"));
}

static void cmd_nop(void) {
	// なにもしない
	TRACEOUT(("SGP: cmd: nop"));
}

static void cmd_end(void) {
	TRACEOUT(("SGP: cmd: end"));

	sgp.busy &= ~SGP_BUSY;
	// ToDo: 割り込みの発生

}

static void cmd_set_work(void) {
	TRACEOUT(("SGP: cmd: set work"));
	// 作業領域(58バイト)の設定
	sgp.workmem = sgp_memoryread_w(sgp.pc) & 0xfffe | ((UINT32)sgp_memoryread_w(sgp.pc + 2) << 16);
	sgp.pc += 4;
	sgp.remainclock -= 8;

}

static void cmd_set_source(void) {
	fetch_block(sgp.pc, &sgp.src);
	TRACEOUT(("SGP: cmd: set source     : dot=%d, mode=%d, w=%d, h=%d, fbw=%d, addr=%08lx", sgp.src.dot, sgp.src.scrnmode, sgp.src.width, sgp.src.height, sgp.src.fbw, sgp.src.address));
	sgp.pc += 12;
	sgp.remainclock -= 6*4;
}

static void cmd_set_destination(void) {
	fetch_block(sgp.pc, &sgp.dest);
	TRACEOUT(("SGP: cmd: set destination: dot=%d, mode=%d, w=%d, h=%d, fbw=%d, addr=%08lx", sgp.dest.dot, sgp.dest.scrnmode, sgp.dest.width, sgp.dest.height, sgp.dest.fbw, sgp.dest.address));
	sgp.pc += 12;
	sgp.remainclock -= 6*4;
}

static void cmd_set_color(void) {
	TRACEOUT(("SGP: cmd: set color"));
	sgp.pc += 2;
}

static void exec_bitblt(void) {
	UINT16 dat;
	UINT16 datmask;
	int BPP = 4;
	UINT16 PIXMASK = ~(0xffff << BPP);

	sgp.remainclock -= 24;

	if (sgp.src.dotcount == 0) {
		read_word(&sgp.src);
		sgp.remainclock -= 4;
	}
	dat = sgp.src.buf >> (16 - BPP);

	switch (sgp.bltmode & SGP_BLTMODE_TP) {
	case 0x0000:		// ソースをそのまま転送
		datmask = 0xffff;
		break;
	case 0x0100:		// ソースが0の部分は転送しない
		datmask = dat ? 0xffff : 0;
		break;
	case 0x0200:		// デスティネーションブロックが0の部分だけ転送する
		//ToDo:
		datmask = 0xffff;
		break;
	case 0x0300:		// 禁止
		//ToDo:
		datmask = 0xffff;
		break;
	}

	switch (sgp.bltmode & SGP_BLTMODE_OP) {
	case 0:		// 0
		dat = 0;
		break;
	case 3:		// NOP
		datmask = 0;
		break;
	case 5:		// S
	default:
		break;
	case 0x0a:	// NOT(S)
		dat = ~dat;
		break;
	case 0x0f:	// 1
		dat = 0xffff;
		break;
	}
/*
	sgp.dest.buf = (sgp.dest.buf << 4) | dat;
	sgp.dest.mask = (sgp.dest.mask << 4) | (dat ? 0x000f : 0);
*/
	sgp.newval = (sgp.newval << BPP) | (dat & PIXMASK);
	sgp.newvalmask = (sgp.newvalmask << BPP) | (datmask & PIXMASK);
	sgp.dest.dotcount--;

	sgp.src.buf <<= BPP;
	sgp.src.dotcount--;

	sgp.dest.xcount--;

	if (sgp.dest.dotcount == 0 || sgp.dest.xcount == 0) {
/*
		sgp.dest.buf <<= sgp.dest.dotcount * 4;
		sgp.dest.mask <<= sgp.dest.dotcount * 4;
*/
		sgp.newval <<= sgp.dest.dotcount * BPP;
		sgp.newvalmask <<= sgp.dest.dotcount * BPP;
//		write_word(&sgp.dest);
		write_dest();
		sgp.remainclock -= 4;
	}

	if (sgp.dest.xcount == 0) {
		sgp.dest.ycount--;
		if (sgp.dest.ycount == 0) {
			sgp.func = fetch_command;
		}
		else {
			if (sgp.bltmode & SGP_BLTMODE_VD) {
				sgp.src.lineaddress -= (SINT32)sgp.src.fbw;
				sgp.dest.lineaddress -= (SINT32)sgp.dest.fbw;
			}
			else {
				sgp.src.lineaddress += (SINT32)sgp.src.fbw;
				sgp.dest.lineaddress += (SINT32)sgp.dest.fbw;

				{	// ToDo とりあえずのつじつまあわせ(神羅万象)
					UINT16 width_w;

					// ブロックが横切るワード数
					width_w = (sgp.src.width + sgp.src.dot + dotcountmax[sgp.src.scrnmode] - 1) / dotcountmax[sgp.src.scrnmode];
					if (width_w * 2 > sgp.src.fbw) {
						sgp.src.lineaddress = sgp.src.nextaddress;
					}
					/*
					神羅万象では、ソースフレームバッファの幅として4バイトの倍数に合わない
					幅(4bbpで12dot,20dotなど)を使用している。この場合、fbwに、実際の幅を超えない
					4バイトの倍数(12dot->4, 20dot->8)を指定している。		

					lineが変わるときは、
					nextaddress += (fbw>>2 - width_w>>1) << 2 
					とするのが正しい？

					nextaddressを増やすタイミングはメモリからの読み取り直前に変更して、
					lineが変わるときは、
					nextaddres += fbw + 2(byte) - width 
					とするのが正しい？
					*/
				}
			}
			sgp.src.nextaddress = sgp.src.lineaddress;
			sgp.dest.nextaddress = sgp.dest.lineaddress;
			init_src_line(&sgp.src);
			init_dest_line(&sgp.dest);
		}
	}
}

static void cmd_bitblt(void) {

	sgp.bltmode = sgp_memoryread_w(sgp.pc);
	sgp.pc += 2;
	sgp.remainclock -= 4;

	TRACEOUT(("SGP: cmd: bitblt: %04x", sgp.bltmode));

	// ToDo: SF,HD,TP-MODEの実現

	init_block(&sgp.src);
	init_block(&sgp.dest);
	init_src_line(&sgp.src);
	init_dest_line(&sgp.dest);

	sgp.func = exec_bitblt;
}

static void cmd_patblt(void) {
	TRACEOUT(("SGP: cmd: patblt"));
	sgp.pc += 2;
}

static void cmd_line(void) {
	TRACEOUT(("SGP: cmd: line"));
	sgp.pc += 14;
}

static void cmd_cls(void) {
	TRACEOUT(("SGP: cmd: cls"));
	sgp.pc += 4;
}

static void cmd_scan_right(void) {
	TRACEOUT(("SGP: cmd: scan right"));
}

static void cmd_scan_left(void) {
	TRACEOUT(("SGP: cmd: scan left"));
}

// ---- 

typedef void (*COMMANDFUNC)();

static COMMANDFUNC commandtable[] = {
	cmd_unknown,			// 00
	cmd_end,				// 01
	cmd_nop,				// 02
	cmd_set_work,			// 03
	cmd_set_source,			// 04
	cmd_set_destination,	// 05
	cmd_set_color,			// 06
	cmd_bitblt,				// 07
	cmd_patblt,				// 08
	cmd_line,				// 09
	cmd_cls,				// 0a
	cmd_scan_right,			// 0b
	cmd_scan_left,			// 0c
//	cmd_unknown,			// 0d
//	cmd_unknown,			// 0e
//	cmd_unknown,			// 0f
};

// ---- 

static void fetch_command(void) {
	WORD cmd;
	
	cmd = sgp_memoryread_w(sgp.pc);
	sgp.pc += 2;
	sgp.remainclock -= 4;
	if (cmd >= 0x0d) {
		TRACEOUT(("SGP: cmd: unknown %04x", cmd));
	}
	else {
		commandtable[cmd]();
	}
}

void sgp_step(void) {
	UINT32 now;
	UINT32 past;

	now = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
	past = now - sgp.lastclock;
	sgp.remainclock += past;

	while (sgp.remainclock > 0) {
		if (!(sgp.busy & SGP_BUSY)) {
			sgp.remainclock = 0;
			break;
		}
		sgp.func();
	}
	sgp.lastclock = now;
}

// ---- I/O

/*
プログラムカウンタ
*/
static void IOOUTCALL sgp_o500(UINT port, REG8 dat) {
	UINT32 mask;
	int	bit;

	mask = 0x000000ffL;
	bit = (port - 0x500) * 8;
	mask <<= bit;
	sgp.initialpc = (sgp.initialpc & ~mask | ((UINT32)dat << bit)) & 0xfffffffeL;
}

/*
割り込み許可、中断要求
*/
static void IOOUTCALL sgp_o504(UINT port, REG8 dat) {
	dat &= SGP_INTF | SGP_ABORT;
	sgp.ctrl = dat;
}

static REG8 IOINPCALL sgp_i504(UINT port) {
	return sgp.ctrl;
}

/*
実行開始
*/
static void IOOUTCALL sgp_o506(UINT port, REG8 dat) {
	dat &= SGP_BUSY;
	if (!(sgp.busy & SGP_BUSY) && (dat & SGP_BUSY)) {
		// 実行開始
		sgp.func = fetch_command;
		sgp.pc = sgp.initialpc;
	}
	sgp.busy = dat;
}

/*
ステータス読み出し
*/
static REG8 IOINPCALL sgp_i506(UINT port) {
	return sgp.busy;
}

// ---- I/F

void sgp_reset(void) {
	ZeroMemory(&sgp, sizeof(sgp));
	sgp.lastclock = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
}

void sgp_bind(void) {
	int i;

	for (i = 0x500; i < 0x504; i++) {
		iocoreva_attachout(i, sgp_o500);
	}
	iocoreva_attachout(0x504, sgp_o504);
	iocoreva_attachinp(0x504, sgp_i504);

	iocoreva_attachout(0x506, sgp_o506);
	iocoreva_attachinp(0x506, sgp_i506);
}

#endif
