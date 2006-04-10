#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"sound.h"
#include	"cs4231.h"
#include	"sasiio.h"

#if defined(SUPPORT_PC88VA)
#include	"iocoreva.h"
#endif

// TRACEOUTを有効にする場合は、以下の1を0にする
#if 1
#undef TRACEOUT
#define TRACEOUT(arg)
#endif

void DMACCALL dma_dummyout(REG8 data) {

	(void)data;
}

REG8 DMACCALL dma_dummyin(void) {

	return(0xff);
}

REG8 DMACCALL dma_dummyproc(REG8 func) {

	(void)func;
	return(0);
}

static const DMAPROC dmaproc[] = {
		{dma_dummyout,		dma_dummyin,		dma_dummyproc},		// NONE
		{fdc_datawrite,		fdc_dataread,		fdc_dmafunc},		// 2HD
		{fdc_datawrite,		fdc_dataread,		fdc_dmafunc},		// 2DD
#if defined(SUPPORT_SASI)
		{sasi_datawrite,	sasi_dataread,		sasi_dmafunc},		// SASI
#else
		{dma_dummyout,		dma_dummyin,		dma_dummyproc},		// SASI
#endif
		{dma_dummyout,		dma_dummyin,		dma_dummyproc},		// SCSI
#if !defined(DISABLE_SOUND)
		{dma_dummyout,		dma_dummyin,		cs4231dmafunc},		// CS4231
#else
		{dma_dummyout,		dma_dummyin,		dma_dummyproc},		// SASI
#endif
};


// ----

void dmac_check(void) {

	BOOL	workchg;
	DMACH	ch;
	REG8	bit;

	workchg = FALSE;
	ch = dmac.dmach;
	bit = 1;
	do {
		if ((!(dmac.mask & bit)) && (ch->ready)) {
			if (!(dmac.work & bit)) {
				dmac.work |= bit;
				if (ch->proc.extproc(DMAEXT_START)) {
					dmac.stat &= ~bit;
					dmac.working |= bit;
					workchg = TRUE;
				}
			}
		}
		else {
			if (dmac.work & bit) {
				dmac.work &= ~bit;
				dmac.working &= ~bit;
				ch->proc.extproc(DMAEXT_BREAK);
				workchg = TRUE;
			}
		}
		bit <<= 1;
		ch++;
	} while(bit & 0x0f);
	if (workchg) {
		nevent_forceexit();
	}
}

UINT dmac_getdatas(DMACH dmach, BYTE *buf, UINT size) {

	UINT	leng;
	UINT32	addr;
	UINT	i;

	leng = min(dmach->leng.w, size);
	if (leng) {
		addr = dmach->adrs.d;					// + mask
		if (!(dmach->mode & 0x20)) {			// dir +
			for (i=0; i<leng; i++) {
				buf[i] = MEMP_READ8(addr + i);
			}
			dmach->adrs.d += leng;
		}
		else {									// dir -
			for (i=0; i<leng; i++) {
				buf[i] = MEMP_READ8(addr - i);
			}
			dmach->adrs.d -= leng;
		}
		dmach->leng.w -= leng;
		if (dmach->leng.w == 0) {
			dmach->proc.extproc(DMAEXT_END);
		}
	}
	return(leng);
}


// ---- I/O

static void IOOUTCALL dmac_o01(UINT port, REG8 dat) {

	DMACH	dmach;
	int		lh;

	dmach = dmac.dmach + ((port >> 2) & 3);
	lh = dmac.lh;
	dmac.lh = (UINT8)(lh ^ 1);
	dmach->adrs.b[lh + DMA32_LOW] = dat;
#if defined(SUPPORT_PC88VA)
	dmach->adrsorg.xb[lh + DMA32_LOW] = dat;
#else
	dmach->adrsorg.b[lh] = dat;
#endif
}

static void IOOUTCALL dmac_o03(UINT port, REG8 dat) {

	int		ch;
	DMACH	dmach;
	int		lh;

	ch = (port >> 2) & 3;
	dmach = dmac.dmach + ch;
	lh = dmac.lh;
	dmac.lh = lh ^ 1;
	dmach->leng.b[lh] = dat;
	dmach->lengorg.b[lh] = dat;
	dmac.stat &= ~(1 << ch);
}

static void IOOUTCALL dmac_o13(UINT port, REG8 dat) {

	dmac.dmach[dat & 3].sreq = dat;
	(void)port;
}

static void IOOUTCALL dmac_o15(UINT port, REG8 dat) {

	if (dat & 4) {
		dmac.mask |= (1 << (dat & 3));
	}
	else {
		dmac.mask &= ~(1 << (dat & 3));
	}
	dmac_check();
	(void)port;
}

static void IOOUTCALL dmac_o17(UINT port, REG8 dat) {

	dmac.dmach[dat & 3].mode = dat;
	(void)port;
}

static void IOOUTCALL dmac_o19(UINT port, REG8 dat) {

	dmac.lh = DMA16_LOW;
	(void)port;
	(void)dat;
}

static void IOOUTCALL dmac_o1b(UINT port, REG8 dat) {

	dmac.mask = 0x0f;
	(void)port;
	(void)dat;
}

static void IOOUTCALL dmac_o1f(UINT port, REG8 dat) {

	dmac.mask = dat;
	dmac_check();
	(void)port;
}

static void IOOUTCALL dmac_o21(UINT port, REG8 dat) {

	DMACH	dmach;

	dmach = dmac.dmach + (((port >> 1) + 1) & 3);
#if defined(CPUCORE_IA32)
	dmach->adrs.b[DMA32_HIGH + DMA16_LOW] = dat;
#else
	// IA16では ver0.75で無効、ver0.76で修正
	dmach->adrs.b[DMA32_HIGH + DMA16_LOW] = dat & 0x0f;
#endif
}

static void IOOUTCALL dmac_o29(UINT port, REG8 dat) {

	DMACH	dmach;

	dmach = dmac.dmach + (dat & 3);
	dmach->bound = dat;
	(void)port;
}

static REG8 IOINPCALL dmac_i01(UINT port) {

	DMACH	dmach;
	int		lh;

	dmach = dmac.dmach + ((port >> 2) & 3);
	lh = dmac.lh;
	dmac.lh = lh ^ 1;
	return(dmach->adrs.b[lh + DMA32_LOW]);
}

static REG8 IOINPCALL dmac_i03(UINT port) {

	DMACH	dmach;
	int		lh;

	dmach = dmac.dmach + ((port >> 2) & 3);
	lh = dmac.lh;
	dmac.lh = lh ^ 1;
	return(dmach->leng.b[lh]);
}

static REG8 IOINPCALL dmac_i11(UINT port) {

	(void)port;
	return(dmac.stat);												// ToDo!!
}


#if defined(SUPPORT_PC88VA)

/*
	bit0..reset
*/
static void IOOUTCALL dmacva_o160(UINT port, REG8 dat) {
	TRACEOUT(("dmac: o160: 0x%02x", dat));
	if (dat & 0x01) {
		TRACEOUT(("dmac: reset"));
		dmac.mask = 0x0f;		// DMA要求禁止 全チャンネル
		dmac.selch = 0;			// 根拠なし
		dmac.base = 0;			// 根拠なし
	}
}

/*
	bit2  ..base
	bit1-0..selch
*/
static void IOOUTCALL dmacva_o161(UINT port, REG8 dat) {
	TRACEOUT(("dmac: o161: 0x%02x", dat));
	dmac.selch = dat & 0x03;
	dmac.base = dat & 0x04;
	TRACEOUT(("dmac: selch=%d base=%d", dmac.selch, dmac.base));
}

/*
	bit4  ..base
	bit3-0..sel3-0
*/
static REG8 IOINPCALL dmacva_i161(UINT port) {
	BYTE ret;
	
	ret = (0x01 << dmac.selch) | (dmac.base ? 0x10 : 0x00);
	TRACEOUT(("dmac: i161: 0x%02x", ret));
	return ret;
}

/*
カウントレジスタ(L)
*/
static void IOOUTCALL dmacva_o162(UINT port, REG8 dat) {
	DMACH ch;

	TRACEOUT(("dmac: o162: 0x%02x", dat));

	ch = &dmac.dmach[dmac.selch];
	ch->lengorg.b[DMA16_LOW] = dat;			// ベース設定
	TRACEOUT(("dmac: set count reg L (base): ch=%d count L=0x%02x", dmac.selch, dat));
	//if (!dmac.base) {	//現状 ベースからカレントへのコピーは実装していないため、
						//常にカレントを設定する必要がある。
		ch->leng.b[DMA16_LOW] = dat;		// カレント設定
		dmac.stat &= ~(1 << dmac.selch);
		TRACEOUT(("dmac: set count reg L (current): ch=%d count L=0x%02x", dmac.selch, dat));
	//}
}

/*
カウントレジスタ(H)
*/
static void IOOUTCALL dmacva_o163(UINT port, REG8 dat) {
	DMACH ch;

	TRACEOUT(("dmac: o163: 0x%02x", dat));

	ch = &dmac.dmach[dmac.selch];
	ch->lengorg.b[DMA16_HIGH] = dat;		// ベース設定
	TRACEOUT(("dmac: set count reg H (base): ch=%d count=0x%04x", dmac.selch, ch->lengorg.w));
	//if (!dmac.base) {	//現状 ベースからカレントへのコピーは実装していないため、
						//常にカレントを設定する必要がある。
		ch->leng.b[DMA16_HIGH] = dat;		// カレント設定
		dmac.stat &= ~(1 << dmac.selch);
		TRACEOUT(("dmac: set count reg H (current): ch=%d count=0x%04x", dmac.selch, ch->leng.w));
	//}
}

/*
カウントレジスタ(L)
*/
static REG8 IOINPCALL dmacva_i162(UINT port) {
	BYTE ret;
	if (dmac.base) {
		// ベース
		ret = dmac.dmach[dmac.selch].lengorg.b[DMA16_LOW];
	}
	else {
		// カレント
		ret = dmac.dmach[dmac.selch].leng.b[DMA16_LOW];
	}
	TRACEOUT(("dmac: i162: ch=%d base=%d count L=0x%02x ", dmac.selch, dmac.base, ret));
	return ret;
}


/*
カウントレジスタ(H)
*/
static REG8 IOINPCALL dmacva_i163(UINT port) {
	BYTE ret;
	if (dmac.base) {
		// ベース
		ret = dmac.dmach[dmac.selch].lengorg.b[DMA16_HIGH];
	}
	else {
		// カレント
		ret = dmac.dmach[dmac.selch].leng.b[DMA16_HIGH];
	}
	TRACEOUT(("dmac: i163: ch=%d base=%d count H=0x%02x ", dmac.selch, dmac.base, ret));
	return ret;
}

static const int intel2idx[3] = {
	DMA32_LOW  + DMA16_LOW,
	DMA32_LOW  + DMA16_HIGH,
	DMA32_HIGH + DMA16_LOW,
};

/*
アドレスレジスタ
*/
static void IOOUTCALL dmacva_o164(UINT port, REG8 dat) {
	DMACH ch;
	int idx;
	int intelidx;

	TRACEOUT(("dmac: o%03x: 0x%02x", port, dat));

	intelidx = port - 0x164;
	idx = intel2idx[intelidx];
	ch = &dmac.dmach[dmac.selch];
	ch->adrsorg.xb[idx] = dat;		// ベース設定
	TRACEOUT(("dmac: set adrs reg (base): ch=%d adrs[%d]=0x%02x", dmac.selch, intelidx, dat));
	//if (!dmac.base) {	//現状 ベースからカレントへのコピーは実装していないため、
						//常にカレントを設定する必要がある。
		ch->adrs.b[idx] = dat;		// カレント設定
		TRACEOUT(("dmac: set adrs reg (current): ch=%d adrs[%d]=0x%02x", dmac.selch, intelidx, dat));
	//}
}

/*
アドレスレジスタ
*/
static REG8 IOINPCALL dmacva_i164(UINT port) {
	int idx;
	int intelidx;
	BYTE ret;

	intelidx = port - 0x164;
	idx = intel2idx[intelidx];

	if (dmac.base) {
		// ベース
		ret = dmac.dmach[dmac.selch].adrsorg.xb[idx];
	}
	else {
		// カレント
		ret = dmac.dmach[dmac.selch].adrs.b[idx];
	}
	TRACEOUT(("dmac: i%03x: ch=%d base=%d count[%d]=0x%02x ", port, dmac.selch, dmac.base, intelidx, ret));
	return ret;
}

#if 0
/*
アドレスレジスタ(bit7-0)
*/
static void IOOUTCALL dmacva_o164(UINT port, REG8 dat) {
	DMACH ch;

	TRACEOUT(("dmac: o164: 0x%02x", dat));

	ch = &dmac.dmach[dmac.selch];
	ch->adrsorg.xb[DMA32_LOW + DMA16_LOW] = dat;		// ベース設定
	TRACEOUT(("dmac: set adrs reg bit7-0 (base): ch=%d adrs(7-0)=0x%02x", dmac.selch, dat));
	//if (!dmac.base) {	//現状 ベースからカレントへのコピーは実装していないため、
						//常にカレントを設定する必要がある。
		ch->adrs.b[DMA32_LOW + DMA16_LOW] = dat;		// カレント設定
		TRACEOUT(("dmac: set adrs reg bit7-0 (current): ch=%d adrs(7-0)=0x%02x", dmac.selch, dat));
	//}
}

/*
アドレスレジスタ(bit15-8)
*/
static void IOOUTCALL dmacva_o165(UINT port, REG8 dat) {
	DMACH ch;

	TRACEOUT(("dmac: o165: 0x%02x", dat));

	ch = &dmac.dmach[dmac.selch];
	ch->adrsorg.xb[DMA32_LOW + DMA16_HIGH] = dat;		// ベース設定
	TRACEOUT(("dmac: set adrs reg bit15-8 (base): ch=%d adrs(15-8)=0x%02x", dmac.selch, dat));
	//if (!dmac.base) {	//現状 ベースからカレントへのコピーは実装していないため、
						//常にカレントを設定する必要がある。
		ch->adrs.b[DMA32_LOW + DMA16_HIGH] = dat;		// カレント設定
		TRACEOUT(("dmac: set adrs reg bit15-8 (current): ch=%d adrs(15-8)=0x%02x", dmac.selch, dat));
	//}
}

/*
アドレスレジスタ(bit19-16)
*/
static void IOOUTCALL dmacva_o166(UINT port, REG8 dat) {
	DMACH ch;

	TRACEOUT(("dmac: o166: 0x%02x", dat));

	dat &= 0x0f;
	ch = &dmac.dmach[dmac.selch];
	ch->adrsorg.xb[DMA32_HIGH + DMA16_LOW] = dat;		// ベース設定
	TRACEOUT(("dmac: set adrs reg bit19-16 (base): ch=%d adrs(19-16)=0x%02x", dmac.selch, dat));
	//if (!dmac.base) {	//現状 ベースからカレントへのコピーは実装していないため、
						//常にカレントを設定する必要がある。
		ch->adrs.b[DMA32_HIGH + DMA16_LOW] = dat;		// カレント設定
		TRACEOUT(("dmac: set adrs reg bit19-16 (current): ch=%d adrs(19-16)=0x%02x", dmac.selch, dat));
	//}
}
#endif

/*
デバイスコントロールレジスタ 未実装
	0x0168 I/O
	0x0169 I/O
*/

/*
モードコントロールレジスタ
*/
static void IOOUTCALL dmacva_o16a(UINT port, REG8 dat) {

	TRACEOUT(("dmac: o16a: 0x%02x", dat));
	TRACEOUT(("dmac: set mode: ch=%d mode=0x%02x", dmac.selch, dat));
	dmac.dmach[dmac.selch].mode = dat;
		/*
			bit1-0の意味が98とは違うが
			  98: SELCH
			  VA: bit1: 未使用
			      bit0: ワードバイトモード (VAはバイトモード(0)固定)
			参照されていないので問題なし。
		*/
}

/*
モードコントロールレジスタ
*/
static REG8 IOINPCALL dmacva_i16a(UINT port) {
	BYTE ret = dmac.dmach[dmac.selch].mode;
	TRACEOUT(("dmac: i16a: ch=%d mode=0x%02x", dmac.selch, ret));
	return ret;
}

/*
ステータスレジスタ
*/
static REG8 IOINPCALL dmacva_i16b(UINT port) {
	BYTE ret = dmac.stat;												// ToDo!!
							// オリジナルnp2のdmac_i11でToDoとなっている。
							// 何が必要なのか？
	TRACEOUT(("dmac: i16b: stat=0x%02x", ret));
	return ret;
}

/*
マスクレジスタ
*/
static void IOOUTCALL dmacva_o16f(UINT port, REG8 dat) {
	TRACEOUT(("dmac: o16f: 0x%02x", dat));
	TRACEOUT(("dmac: set mask: mask=0x%02x", dat));
	dmac.mask = dat;
	dmac_check();
}

static REG8 IOINPCALL dmacva_i16f(UINT port) {
	BYTE ret = dmac.mask;
	TRACEOUT(("dmac: i16f: mask=0x%02x", ret));
	return ret;
}


#endif


// ---- I/F

static const IOOUT dmaco00[16] = {
					dmac_o01,	dmac_o03,	dmac_o01,	dmac_o03,
					dmac_o01,	dmac_o03,	dmac_o01,	dmac_o03,
					NULL,		dmac_o13,	dmac_o15,	dmac_o17,
					dmac_o19,	dmac_o1b,	NULL,		dmac_o1f};

static const IOINP dmaci00[16] = {
					dmac_i01,	dmac_i03,	dmac_i01,	dmac_i03,
					dmac_i01,	dmac_i03,	dmac_i01,	dmac_i03,
					dmac_i11,	NULL,		NULL,		NULL,
					NULL,		NULL,		NULL,		NULL};

static const IOOUT dmaco21[8] = {
					dmac_o21,	dmac_o21,	dmac_o21,	dmac_o21,
					dmac_o29,	NULL,		NULL,		NULL};

void dmac_reset(void) {

	ZeroMemory(&dmac, sizeof(dmac));
	dmac.lh = DMA16_LOW;
	dmac.mask = 0xf;		// DMA要求禁止 全チャンネル
#if defined(SUPPORT_PC88VA)
	dmac.selch = 0;			// 根拠なし
	dmac.base = 0;			// 根拠なし
#endif
	dmac_procset();
//	TRACEOUT(("sizeof(_DMACH) = %d", sizeof(_DMACH)));
}

void dmac_bind(void) {

	iocore_attachsysoutex(0x0001, 0x0ce1, dmaco00, 16);
	iocore_attachsysinpex(0x0001, 0x0ce1, dmaci00, 16);
	iocore_attachsysoutex(0x0021, 0x0cf1, dmaco21, 8);

#if defined(SUPPORT_PC88VA)
	iocoreva_attachout(0x160, dmacva_o160);
	iocoreva_attachout(0x161, dmacva_o161);
	iocoreva_attachinp(0x161, dmacva_i161);

	iocoreva_attachout(0x162, dmacva_o162);
	iocoreva_attachinp(0x162, dmacva_i162);
	iocoreva_attachout(0x163, dmacva_o163);
	iocoreva_attachinp(0x163, dmacva_i163);

	iocoreva_attachout(0x164, dmacva_o164);
	iocoreva_attachinp(0x164, dmacva_i164);
	iocoreva_attachout(0x165, dmacva_o164);
	iocoreva_attachinp(0x165, dmacva_i164);
	iocoreva_attachout(0x166, dmacva_o164);
	iocoreva_attachinp(0x166, dmacva_i164);

	iocoreva_attachout(0x16a, dmacva_o16a);
	iocoreva_attachinp(0x16a, dmacva_i16a);
	iocoreva_attachinp(0x16b, dmacva_i16b);
	iocoreva_attachout(0x16f, dmacva_o16f);
	iocoreva_attachinp(0x16f, dmacva_i16f);

#endif
}


// ----

static void dmacset(REG8 channel) {

	DMADEV		*dev;
	DMADEV		*devterm;
	UINT		dmadev;

	dev = dmac.device;
	devterm = dev + dmac.devices;
	dmadev = DMADEV_NONE;
	while(dev < devterm) {
		if (dev->channel == channel) {
			dmadev = dev->device;
		}
		dev++;
	}
	if (dmadev >= sizeof(dmaproc) / sizeof(DMAPROC)) {
		dmadev = 0;
	}
//	TRACEOUT(("dmac set %d - %d", channel, dmadev));
	dmac.dmach[channel].proc = dmaproc[dmadev];
}

void dmac_procset(void) {

	REG8	i;

	for (i=0; i<4; i++) {
		dmacset(i);
	}
}

void dmac_attach(REG8 device, REG8 channel) {

	dmac_detach(device);

	if (dmac.devices < (sizeof(dmac.device) / sizeof(DMADEV))) {
		dmac.device[dmac.devices].device = device;
		dmac.device[dmac.devices].channel = channel;
		dmac.devices++;
		dmacset(channel);
	}
}

void dmac_detach(REG8 device) {

	DMADEV	*dev;
	DMADEV	*devterm;
	REG8	ch;

	dev = dmac.device;
	devterm = dev + dmac.devices;
	while(dev < devterm) {
		if (dev->device == device) {
			break;
		}
		dev++;
	}
	if (dev < devterm) {
		ch = dev->channel;
		dev++;
		while(dev < devterm) {
			*(dev - 1) = *dev;
			dev++;
		}
		dmac.devices--;
		dmacset(ch);
	}
}

