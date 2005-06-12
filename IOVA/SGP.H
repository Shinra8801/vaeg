/*
 * SGP.H: PC-88VA Super Graphic Processor
 *
 */

typedef struct {
	int		scrnmode;	// ピクセルサイズ
	int		dot;		// 開始ドット位置
	UINT16	width;		// ブロックの幅(ピクセル, 12bit)
	UINT16	height;		// ブロックの高さ(ピクセル, 12bit)
	SINT16	fbw;		// フレームバッファの横幅(バイト, 下位2bit=0)
	UINT32	address;	// 開始アドレス(偶数)

	UINT32	lineaddress;
	UINT32	nextaddress;
	int		dotcount;
	UINT16	buf;
	UINT16	xcount;
	UINT16	ycount;
} _SGP_BLOCK, *SGP_BLOCK;

typedef struct {
	UINT32	initialpc;
	UINT32	pc;			// プログラムカウンタ
	UINT32	workmem;
	UINT8	ctrl;		// bit2:割り込み許可 bit1=中断要求
	UINT8	busy;		// bit0:ビジー
	UINT32	lastclock;
	SINT32	remainclock;

	void (*func)();

	_SGP_BLOCK	src;
	_SGP_BLOCK	dest;
	UINT16	newval;
	UINT16	newvalmask;
	UINT16	bltmode;
} _SGP, *SGP;

enum {
	SGP_INTF	= 0x04,	// 割り込み許可
	SGP_ABORT	= 0x02, // 中断要求

	SGP_BUSY	= 0x01,	// ビジー

	SGP_BLTMODE_SF	= 0x1000,
	SGP_BLTMODE_VD	= 0x0800,
	SGP_BLTMODE_HD	= 0x0400,
	SGP_BLTMODE_TP	= 0x0300,
	SGP_BLTMODE_OP	= 0x000f,
};

#ifdef __cplusplus
extern "C" {
#endif

void sgp_step(void);

void sgp_reset(void);
void sgp_bind(void);


extern	_SGP	sgp;

#ifdef __cplusplus
}
#endif
