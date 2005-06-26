/*
 *	GVAMVA.C: PC-88VA GVRAM
 */

#include	"compiler.h"
#include	"cpucore.h"
#include	"gactrlva.h"

#if defined(SUPPORT_PC88VA)

enum {
	MEMWAITVA_VRAM_SINGLE_R	=	4,
	MEMWAITVA_VRAM_MULTI_R	=	7,
	MEMWAITVA_VRAM_SINGLE_W	=	1,		// 1.5くらいにしたい・・・
	MEMWAITVA_VRAM_MULTI_W	=	4,
};

		BYTE		grphmem[0x40000];
		_GACTRLVA	gactrlva;


static UINT16 lu(UINT8 rop, UINT16 pat, UINT16 cpu, UINT16 mem) {
	UINT16 out;

	out = 0;
	if (rop & 0x01) {
		out |= ~pat & ~cpu & ~mem;
	}
	if (rop & 0x02) {
		out |= ~pat & ~cpu & mem;
	}
	if (rop & 0x04) {
		out |= ~pat & cpu & ~mem;
	}
	if (rop & 0x08) {
		out |= ~pat & cpu & mem;
	}
	if (rop & 0x10) {
		out |= pat & ~cpu & ~mem;
	}
	if (rop & 0x20) {
		out |= pat & ~cpu & mem;
	}
	if (rop & 0x40) {
		out |= pat & cpu & ~mem;
	}
	if (rop & 0x80) {
		out |= pat & cpu & mem;
	}
	return out;
}

static int addr2page(UINT32 address) {
	return (address >> 17) & 1;
}

static void memwait_r(void) {
	int wait;
	if (gactrlva.gmsp) {
		// シングル
		wait = MEMWAITVA_VRAM_SINGLE_R;
	}
	else {
		// マルチ
		wait = MEMWAITVA_VRAM_MULTI_R;
	}
	CPU_REMCLOCK -= wait;
}

static void memwait_w(void) {
	int wait;
	if (gactrlva.gmsp) {
		// シングル
		wait = MEMWAITVA_VRAM_SINGLE_W;
	}
	else {
		// マルチ
		wait = MEMWAITVA_VRAM_MULTI_W;
	}
	CPU_REMCLOCK -= wait;
}

/*
GVRAMへwriteするデータを求める
	入力:
		address		アドレス(偶数)
		value		データ
*/
static UINT16 writevalue(UINT32 address, REG16 value) {
	UINT16 mem;
	UINT16 pat;
	int page;

	page = addr2page(address);
	mem = *(UINT16 *)(grphmem + address);
	pat = gactrlva.s.pattern[page];
	if (gactrlva.gmsp) {
		// シングルプレーン
		switch ((gactrlva.s.writemode >> 3) & 3) {
		case 0:	// LU出力
			return lu(gactrlva.s.rop[page], pat, value, mem);
		case 1:	// パターンレジスタ
			return pat;
		case 2:	// CPUライトデータ
			return value;
		case 3:	// ノーオペレーション
		default:
			return mem;
		}
	}
	else {
		// マルチプレーン
		// ToDo:
		return value;
	}
}


// ---- write byte
/*
void MEMCALL _gvram_wt(UINT32 address, REG8 value) {
	grphmem[address] = (BYTE)value;
}
*/
void MEMCALL gvram_wt(UINT32 address, REG8 value) {

	//CPU_REMCLOCK -= MEMWAIT_VRAM;
	//_gvram_wt(address, value);
	UINT16 out;

	memwait_w();
	if (address & 1) {
		out = writevalue(address, (REG16)(((REG16)value) << 8));
		grphmem[address] = out >> 8;
	}
	else {
		out = writevalue(address, (REG16)value);
		grphmem[address] = (BYTE)out;
	}
}

// ---- read byte

REG8 MEMCALL _gvram_rd(UINT32 address) {	
	return(grphmem[address]);
}

REG8 MEMCALL gvram_rd(UINT32 address) {	

	memwait_r();
	return _gvram_rd(address);
}

// ---- write word

/*
	入力:
		address		アドレス(偶数)
*/
void MEMCALL _gvramw_wt(UINT32 address, REG16 value) {
	*(UINT16 *)(grphmem + address) = value;
}

void MEMCALL gvramw_wt(UINT32 address, REG16 value) {
	UINT16 out;

/*
	//CPU_REMCLOCK -= MEMWAIT_VRAM;
	_gvram_wt(address, (REG8) (value & 0x00ff));
	_gvram_wt(address + 1, (REG8) (value >> 8));
*/
	if (address & 1) {
		gvram_wt(address, (REG8) (value & 0x00ff));
		gvram_wt(address + 1, (REG8) (value >> 8));
	}
	else {
		memwait_w();
		out = writevalue(address, value);
		*(UINT16 *)(grphmem + address) = out;
	}
}

// ---- read word

REG16 MEMCALL gvramw_rd(UINT32 address) {
	memwait_r();
	return _gvram_rd(address) | (_gvram_rd(address + 1) << 8);
}

#endif