/*
 * SUBSYSTEM.CPP: PC-88VA FD Sub System
 *
 */

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"

#include	"cpucva/Z80if.h"
#include	"cpucva/Z80c.h"
#include	"i8255.h"
#include	"subsystemif.h"
#include	"fdc.h"

#include	"subsystem.h"

#if defined(SUPPORT_PC88VA)

// TRACEOUTを有効にする場合は、以下の1を0にする
#if 1
#undef TRACEOUT
#define TRACEOUT(arg)
#endif

#define SLEEP_HACK		// メインからのコマンド待ち時にCPUを停止する機能を有効にする 


static	_I8255CFG i8255cfg;
		_SUBSYSTEM subsystem;


// ---- Clock

class Clock : public IClock
{
	uint32 IFCALL now() {
		return CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
	}
};


// ---- ClockCounter

class ClockCounter : public IClockCounter
{
public:
	void IFCALL ClockCounter::past(sint32 clock);
	sint32 IFCALL ClockCounter::GetRemainclock();
	void IFCALL ClockCounter::SetRemainclock(sint32 clock);
	void IFCALL SetMultiple(int multiple);
private:
	SINT32 remainclock;
	int mul;
};

// ---- ClockCounter implementation

void ClockCounter::SetMultiple(int _mul) {
	mul = _mul;
}

void IFCALL ClockCounter::past(sint32 clock) {
	remainclock -= clock * mul;
}
sint32 IFCALL ClockCounter::GetRemainclock() {
	return remainclock;
}
void IFCALL ClockCounter::SetRemainclock(sint32 clock) {
	remainclock = clock;
}


// ---- Subsystem

class Subsystem : public IMemoryAccess, public IIOAccess
{
public:
	Subsystem();
	~Subsystem();

	void Initialize();
	void Reset();
	void IRQ(BOOL irq);
	void Exec();

	uint IFCALL	Read8(uint addr);
	void IFCALL Write8(uint addr, uint data);

	uint IFCALL In(uint port);
	void IFCALL Out(uint port, uint data);



	enum SpecialPort2
	{
		pres2 = 0x100,	// たぶんつかわない
		pirq2,			// たぶんつかわない
		piac2,
		pfdstat,		// たぶんつかわない
						// FD の動作状況 (b0-1 = LAMP, b2-3 = MODE, b4=SEEK)
		portend2
	};

public:
	Z80C *z80;

private:
	ClockCounter *clockcounter;
	Clock *clock;

	uint _ram_rd(uint addr);
	uint ram_rd(uint addr);
	uint rom_rd(uint addr);
	uint nonmem_rd(uint addr);
	void ram_wt(uint addr, uint data);

#if defined(SLEEP_HACK)
	bool SleepCheck_VA(uint portc);
	bool SleepCheck_Sorcerian(uint portc);
#endif
};




// ---- Sybsystem implementation

Subsystem::Subsystem() {
	z80 = new Z80C();
	clockcounter = new ClockCounter();
	clock = new Clock();
}

Subsystem::~Subsystem() {
	if (z80) delete z80;
	if (clockcounter) delete clockcounter;
	if (clock) delete clock;
}

//BYTE *Subsystem::GetRomBuf() {
//	return rom;
//}

void Subsystem::Initialize() {
	z80->Init(this, this, clock, clockcounter, piac2);
	//rom[0] = 0xf3;
	//rom[1] = 0x76;
	i8255_init(&i8255cfg, &subsystem.i8255);
	i8255cfg.busoutportb = subsystemif_businporta;
	i8255cfg.busoutportc = subsystemif_businportc;
}

void Subsystem::Reset() {
	clockcounter->SetMultiple(pccore.multiple);
	z80->Reset();
	i8255_reset(&i8255cfg);
	subsystem.intopcode = 0x7f;
}

void Subsystem::IRQ(BOOL irq) {
	z80->IRQ(0,irq);
}

void Subsystem::Exec() {
	z80->Exec();
}

uint Subsystem::_ram_rd(uint addr) {
	return subsystem.ram[addr-0x4000];
}

uint Subsystem::ram_rd(uint addr) {
	return _ram_rd(addr);
}

void Subsystem::ram_wt(uint addr, uint data) {
	subsystem.ram[addr-0x4000] = data;
}

uint Subsystem::rom_rd(uint addr) {
	return subsystem.rom[addr];
}

uint Subsystem::nonmem_rd(uint addr) {
	return 0xff;
}


uint IFCALL Subsystem::Read8(uint addr) {
	switch(addr >> 13) {
	case 0: return rom_rd(addr);
	case 2: return ram_rd(addr);
	case 3: return ram_rd(addr);
	default: return nonmem_rd(addr);
	}
}

void IFCALL Subsystem::Write8(uint addr, uint data) {
	switch(addr >> 13) {
	case 2: ram_wt(addr, data);
	case 3: ram_wt(addr, data);
	}
}

#if defined(SLEEP_HACK)
// VA/2 の ROMの処理の場合
bool Subsystem::SleepCheck_VA(uint portc) {
	return !(portc & 0x08) && _ram_rd(0x7f67)==0xff && z80->GetReg()->pc == 0x1732;
	/*
	  7f67 は サブシステムスリープ時 ffh, アクティブ時 00h
      アクティブ時は適当な時間をまってFDDモータをOFFする処理が動いているので、
	  ATN=0の条件だけでZ80を停止してはならない。そのため、条件にスリープ時であること
	  を追加している。
      1732 のコードでポートfehからinしている
	*/
}

// ソーサリアンの場合
bool Subsystem::SleepCheck_Sorcerian(uint portc) {
	return !(portc & 0x08) && z80->GetReg()->pc == 0x700e;
	/*
	  700e のコードでポートfehからinしている
	*/
}
#endif

uint IFCALL Subsystem::In(uint port) {
	uint ret = 0xff;

	switch(port) {
	case 0xf8:
		fdcsubsys_o_tc();
		ret = 0xff;
		break;
	case 0xfa:
		ret = fdcsubsys_i_fdc0();
		break;
	case 0xfb:
		ret = fdcsubsys_i_fdc1();
		break;
	case 0xfc:
		ret = i8255_inporta(&i8255cfg);
		break;
	case 0xfd:
		ret = i8255_inportb(&i8255cfg);
		break;
	case 0xfe:
		ret = i8255_inportc(&i8255cfg);
#if defined(SLEEP_HACK)
		if (SleepCheck_VA(ret) || SleepCheck_Sorcerian(ret)) {
			// サブシステムZ80スリープ
			z80->Wait(true);
		}
#endif
		break;

	// 仮想ポート
	case piac2:
		// 割り込み受理
		ret = subsystem.intopcode;
		//z80->IRQ(0,0);
		break;
	}
	if (port != 0xfe) {
		TRACEOUT(("subsys: in : %02x -> %02x  [%04x]", port, ret, z80->GetReg()->pc));
	}
	return ret;
}

void IFCALL Subsystem::Out(uint port, uint dat) {
	TRACEOUT(("subsys: out: %02x <- %02x  [%04x]", port, dat, z80->GetReg()->pc));

	switch(port) {
	case 0xf0:
		subsystem.intopcode = dat;
		break;
	case 0xf4:
		fdcsubsys_o_dskctl(dat);
		break;
	case 0xf8:
		fdcsubsys_o_mtrctl(dat);
		break;
	case 0xfb:
		fdcsubsys_o_fdc1(dat);
		break;
	case 0xfc:
		i8255_outporta(&i8255cfg, dat);
		break;
	case 0xfd:
		i8255_outportb(&i8255cfg, dat);
		break;
	case 0xfe:
		i8255_outportc(&i8255cfg, dat);
		break;
	case 0xff:
		i8255_outctrl(&i8255cfg, dat);
		break;
	default:	// unknown
		{
			int a;
			a=0;
		}
		break;
	}
}

// ----

	Subsystem subsystemobj;

// ---- Main → Subsystem connection

void subsystem_businporta(BYTE dat) {
	i8255_businporta(&i8255cfg, dat);
}

void subsystem_businportc(BYTE dat) {
	i8255_businportc(&i8255cfg, (BYTE)(dat >> 4));
#if defined(SLEEP_HACK)
	if (dat & 0x80) {
		// ATN = 1
		subsystemobj.z80->Wait(false);
	}
#endif
}

// ---- C - C++ bridge for Subsystem

void subsystem_initialize(void) {
	subsystemobj.Initialize();	
}

void subsystem_reset(void) {
	subsystemobj.Reset();
}

void subsystem_irq(BOOL irq) {
	subsystemobj.IRQ(irq);
}

void subsystem_exec(void) {
	subsystemobj.Exec();
}

BYTE subsystem_readmem(WORD addr) {
	return subsystemobj.Read8(addr);
}

const struct Z80Reg *subsystem_getcpureg(void) {
	return subsystemobj.z80->GetReg();
}

WORD subsystem_disassemble(WORD pc, char *str) {
	return subsystemobj.z80->GetDiag()->Disassemble(pc,str);
}

UINT subsystem_getcpustatussize(void) {
	return subsystemobj.z80->GetStatusSize();
}

void subsystem_savecpustatus(UINT8 *buf) {
	subsystemobj.z80->SaveStatus(buf);
}

void subsystem_loadcpustatus(const UINT8 *buf) {
	subsystemobj.z80->LoadStatus(buf);
}

#endif
