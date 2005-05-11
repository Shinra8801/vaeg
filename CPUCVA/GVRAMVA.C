/*
 *	GVAMVA.C: PC-88VA GVRAM
 */

#include	"compiler.h"
#include	"cpucore.h"

		BYTE	grphmem[0x40000];

// ---- write byte

void MEMCALL _gvram_wt(UINT32 address, REG8 value) {
	grphmem[address] = (BYTE)value;
}

void MEMCALL gvram_wt(UINT32 address, REG8 value) {

	//CPU_REMCLOCK -= MEMWAIT_VRAM;
	_gvram_wt(address, value);
}

// ---- read byte

REG8 MEMCALL _gvram_rd(UINT32 address) {	
	return(grphmem[address]);
}

REG8 MEMCALL gvram_rd(UINT32 address) {	

	//CPU_REMCLOCK -= MEMWAIT_VRAM;
	return _gvram_rd(address);
}

// ---- write word

void MEMCALL gvramw_wt(UINT32 address, REG16 value) {

	//CPU_REMCLOCK -= MEMWAIT_VRAM;
	_gvram_wt(address, (REG8) (value & 0x00ff));
	_gvram_wt(address + 1, (REG8) (value >> 8));
}

// ---- read word

REG16 MEMCALL gvramw_rd(UINT32 address) {
	//CPU_REMCLOCK -= MEMWAIT_VRAM;
	return _gvram_rd(address) | (_gvram_rd(address + 1) << 8);
}
