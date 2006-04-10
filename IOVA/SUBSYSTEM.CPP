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


// ---- Subsystem

class Subsystem : public IMemoryAccess, public IIOAccess
{
public:
	Subsystem();
	~Subsystem();

	BYTE *GetRomBuf();

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

	BYTE intopcode;		// 割り込みオペコード

	BYTE rom[0x2000];
	BYTE ram[0x4000];

	_I8255    i8255;

	uint ram_rd(uint addr);
	uint rom_rd(uint addr);
	uint nonmem_rd(uint addr);
	void ram_wt(uint addr, uint data);
};

static	_I8255CFG i8255cfg;

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


// ---- Sybsystem implementation

Subsystem::Subsystem() {
	z80 = new Z80C(DEV_ID('C','P','U','2'));
	clockcounter = new ClockCounter();
	clock = new Clock();
}

Subsystem::~Subsystem() {
	if (z80) delete z80;
	if (clockcounter) delete clockcounter;
	if (clock) delete clock;
}

BYTE *Subsystem::GetRomBuf() {
	return rom;
}

void Subsystem::Initialize() {
	z80->Init(this, this, clock, clockcounter, piac2);
	//rom[0] = 0xf3;
	//rom[1] = 0x76;
	i8255_init(&i8255cfg, &i8255);
	i8255cfg.busoutportb = subsystemif_businporta;
	i8255cfg.busoutportc = subsystemif_businportc;
}

void Subsystem::Reset() {
	clockcounter->SetMultiple(pccore.multiple);
	z80->Reset();
	i8255_reset(&i8255cfg);
	intopcode = 0x7f;
}

void Subsystem::IRQ(BOOL irq) {
	z80->IRQ(0,irq);
}

void Subsystem::Exec() {
	z80->Exec();
}

uint Subsystem::ram_rd(uint addr) {
	return ram[addr-0x4000];
}

void Subsystem::ram_wt(uint addr, uint data) {
	ram[addr-0x4000] = data;
}

uint Subsystem::rom_rd(uint addr) {
	return rom[addr];
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


uint IFCALL Subsystem::In(uint port) {
	uint ret = 0xff;

	switch(port) {
	case 0xf8:
		fdcsubsys_otc();
		ret = 0xff;
		break;
	case 0xfa:
		ret = fdcsubsys_ifdc0();
		break;
	case 0xfb:
		ret = fdcsubsys_ifdc1();
		break;
	case 0xfc:
		ret = i8255_inporta(&i8255cfg);
		break;
	case 0xfd:
		ret = i8255_inportb(&i8255cfg);
		break;
	case 0xfe:
		ret = i8255_inportc(&i8255cfg);
		break;

	// 仮想ポート
	case piac2:
		// 割り込み受理
		ret = intopcode;
		//z80->IRQ(0,0);
		break;
	}
	if (port != 0xfe) TRACEOUT(("subsys: in : %02x -> %02x  [%04x]", port, ret, z80->reg.pc));
	return ret;
}

void IFCALL Subsystem::Out(uint port, uint dat) {
	TRACEOUT(("subsys: out: %02x <- %02x  [%04x]", port, dat, z80->reg.pc));

	switch(port) {
	case 0xf0:
		intopcode = dat;
		break;
	case 0xf4:
		fdcsubsys_odrvctrl(dat);
		break;
	case 0xf8:
		fdcsubsys_omotor(dat);
		break;
	case 0xfb:
		fdcsubsys_ofdc1(dat);
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


// ---- C - C++ bridge for Subsystem

	Subsystem subsystem;

void subsystem_initialize(void) {
	subsystem.Initialize();	
}

void subsystem_reset(void) {
	subsystem.Reset();
}

void subsystem_irq(BOOL irq) {
	subsystem.IRQ(irq);
}

void subsystem_exec(void) {
	subsystem.Exec();
}

void subsystem_businporta(BYTE dat) {
	i8255_businporta(&i8255cfg, dat);
}

void subsystem_businportc(BYTE dat) {
	i8255_businportc(&i8255cfg, (BYTE)(dat >> 4));
}

BYTE subsystem_readmem(WORD addr) {
	return subsystem.Read8(addr);
}

struct Z80Reg *subsystem_getcpuregs(void) {
	return &subsystem.z80->reg;
}

BYTE *subsystem_getrombuf(void) {
	return subsystem.GetRomBuf();
}

WORD subsystem_disassemble(WORD pc, char *str) {
	return subsystem.z80->GetDiag()->Disassemble(pc,str);
}
#endif
