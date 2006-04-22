/*
 * Z80if.h: Z80 interfaces
 */

#if !defined(__Z80if_h__)
#define __Z80if_h__

#include "types.h"

#ifndef IFCALL
#define IFCALL __stdcall
#endif
#ifndef IOCALL
#define IOCALL __stdcall
#endif


// ----------------------------------------------------------------------------
//	メモリ空間にアクセスするためのインターフェース
//
struct IMemoryAccess
{
	virtual uint IFCALL	Read8(uint addr) = 0;
	virtual void IFCALL Write8(uint addr, uint data) = 0;
};

// ----------------------------------------------------------------------------
//	IO 空間にアクセスするためのインターフェース
//
struct IIOAccess
{
	virtual uint IFCALL In(uint port) = 0;
	virtual void IFCALL Out(uint port, uint data) = 0;
};


// ----------------------------------------------------------------------------
//  現在時刻(クロック)にアクセスするためのインタフェース
//

struct IClock
{
	virtual uint32 IFCALL now() = 0;
};

// ----------------------------------------------------------------------------
//  経過時間記録インタフェース
//
struct IClockCounter
{
	virtual void IFCALL past(sint32 clock) = 0;
	virtual sint32 IFCALL GetRemainclock() = 0;
	virtual void IFCALL SetRemainclock(sint32 clock) = 0;
};

#endif // __Z80_if_h__
