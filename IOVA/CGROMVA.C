/*
 * CGROMVA.C: PC-88VA Character generator
 *
 * ToDo:
 *   ハードウェア文字コードが不正の場合の動作を実機にあわせる。
 *	 8x8フォントへの対応
 */

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"

#include	"memoryva.h"

#if defined(SUPPORT_PC88VA)

#define SETLOWBYTE(x, y) (x) = ( (x) & 0xff00 | (y) )
#define SETHIGHBYTE(x, y) (x) = ( (x) & 0x00ff | ((WORD)(y) << 8) )


		_CGROMVA	cgromva;

static BYTE tofu[32] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

/*
フォントを取得する
  IN:	hccode		ハードウェア文字コード
					bit15 文字の右側のとき1
					bit14-bit8 JIS第二バイト
					bit7  0
					bit6-bit0  JIS第一バイト-0x20
*/
BYTE *cgromva_font(UINT16 hccode) {
	int		lr;
	UINT16	jis1;
	UINT16	jis2;
	BYTE	*base;
	unsigned int font;

	lr = hccode>>15;
	jis1 = (hccode & 0x7f) + 0x20;
	jis2 = (hccode >> 8) & 0x7f;

	if (jis2 == 0 && lr == 0) {
		// ANK (lr == 1の場合はANKとして扱わない)
		base = fontmem;
		if (videova.txtmode & 0x04) {
			// 8ドット
			font = 0x41000 + ((hccode & 0xff) << 3);
					/* テクマニに従えば 0x42000 + ((hccode & 0xff) << 4) となるが、
					   間違っているっぽい */
		}
		else {
			// 16ドット
			font = 0x40000 + ((hccode & 0xff) << 4);
		}
	}
	else {
		if (jis1 < 0x28) {
			// JIS非漢字
			base = fontmem;
			font = lr + 
					((jis2 & 0x60) << 8) +
					((jis1 & 0x07) << 10) +
					((jis2 & 0x1f) << 5);
		}
		else if (jis1 < 0x30) {
			// NEC非漢字
			base = fontmem;
			font = lr + 0x40000 + 
					((jis2 & 0x60) << 8) +
					((jis1 & 0x07) << 10) +
					((jis2 & 0x1f) << 5);
		}
		else if (jis1 < 0x40) {
			// JIS 第一水準 (3xxx)
			base = fontmem;
			font = lr + 
					(((UINT32)jis2 & 0x60) << 10) +
					((jis1 & 0x0f) << 10) +
					((jis2 & 0x1f) << 5);
		}
		else if (jis1 < 0x50) {
			// JIS 第一水準 (4xxx)
			base = fontmem;
			font = lr + 0x4000 +
					(((UINT32)jis2 & 0x60) << 10) +
					((jis1 & 0x0f) << 10) +
					((jis2 & 0x1f) << 5);
		}
		else if (jis1 < 0x60) {
			// JIS 第二水準 (5xxx)
			base = fontmem;
			font = lr + 0x20000 + 
					(((UINT32)jis2 & 0x60) << 10) +
					((jis1 & 0x0f) << 10) +
					((jis2 & 0x1f) << 5);
		}
		else if (jis1 < 0x70) {
			// JIS 第二水準 (6xxx)
			base = fontmem;
			font = lr + 0x20000 + 0x4000 + 
					(((UINT32)jis2 & 0x60) << 10) +
					((jis1 & 0x0f) << 10) +
					((jis2 & 0x1f) << 5);
		}
		else if (jis1 < 0x76) {
			// JIS 第二水準 (7xxx)
			base = fontmem;
			font = lr + 0x20000 + 
					((jis2 & 0x60) << 8) +
					((jis1 & 0x07) << 10) +
					((jis2 & 0x1f) << 5);
		}
		else if (jis1 < 0x78) {
			// 外字
			if (jis1 == 0x77 && (jis2 == 0x7e || jis2 == 0x7f)) {
				// 777e, 777f は豆腐が表示される
				base = tofu;
				font = lr;
			}
			else {
				base = backupmem;
				font = lr + 
						((jis2 & 0x60) << 6) +
						((jis1 & 0x01) << 10) +
						((jis2 & 0x1f) << 5);
			}
		}
		else {
			// 未定義
			base = fontmem;
			font = 0;
		}
	}
	return base + font;
}

/*
フォントの幅を取得(漢字ROM内で1ラスタ分のデータのバイト数)
  IN:	hccode		ハードウェア文字コード
					bit15 文字の右側のとき1
					bit14-bit8 JIS第二バイト
					bit7  0
					bit6-bit0  JIS第一バイト-0x20
*/
int cgromva_width(UINT16 hccode) {
	return (hccode & 0x7f00) == 0 ? 1 : 2;
}

/*
CGROMアクセスポートにセットされたハードウェア文字コードを、
TVRAMに格納するためのハードウェア文字コードに変換する。
*/
static UINT16 curhccode(void) {
	UINT16 hccode;

	/*
	if (cgromva.cgaddr & 0x7f00) {
		hccode = cgromva.cgaddr & 0x7fff | ((cgromva.cgrow & 0x20) ? 0x0000 : 0x8000);
	}
	else {
		// ANK
		hccode = cgromva.cgaddr;
	}
	*/
	hccode = cgromva.cgaddr & 0x7fff | ((cgromva.cgrow & 0x20) ? 0x0000 : 0x8000);
	return hccode;
}

// ---- I/O

static void IOOUTCALL cgromva_o14c(UINT port, REG8 dat) {
	SETLOWBYTE(cgromva.cgaddr, dat);
}

static void IOOUTCALL cgromva_o14d(UINT port, REG8 dat) {
	// 最上位ビットはマスクしておくことにする
	SETHIGHBYTE(cgromva.cgaddr, dat & 0x7f);
}

static REG8 IOINPCALL cgromva_i14e(UINT port) {
	BYTE *font;
	UINT16 hccode;
	int row;

	hccode = curhccode();
	font = cgromva_font(hccode);

	if (hccode < 0x100 && videova.txtmode & 0x04) {
		// ANK 8ドット
		row = cgromva.cgrow & 0x07;
	}
	else {
		row = cgromva.cgrow & 0x0f;
	}
	font += cgromva_width(hccode) * row;

	return *font;

		/*
			厳密には、JIS 777E, 777Fに対して返却する値は不定
		*/
}

static void IOOUTCALL cgromva_o14e(UINT port, REG8 dat) {
	int		lr;
	UINT16	jis1;
	UINT16	jis2;
	UINT16	hccode;
	int		writable = FALSE;
	BYTE	*font;

	hccode = curhccode();
	lr = hccode>>15;
	jis1 = (hccode & 0x7f) + 0x20;
	jis2 = (hccode >> 8) & 0x7f;

	if (jis2 == 0 && lr == 0) {
		// ANK (lr == 1の場合はANKとして扱わない)
		writable = FALSE;
	}
	else {
		if (jis1 < 0x76) {
			writable = FALSE;
		}
		else if (jis1 < 0x77) {
			// 外字 (76xx)
			writable = TRUE;
		}
		else if (jis1 < 0x78) {
			// 外字 (77xx)
			if (jis2 == 0x7e || jis2 == 0x7f) {
				// 777e, 777f は変更不可
				writable = FALSE;
			}
			else {
				writable = TRUE;
			}
		}
		else {
			writable = FALSE;
		}
	}
	
	if (writable) {
		font = cgromva_font(hccode);
		font += cgromva_width(hccode) * (cgromva.cgrow & 0x0f);
		*font = dat;
	}
}

static void IOOUTCALL cgromva_o14f(UINT port, REG8 dat) {
	cgromva.cgrow = dat;
}


// ---- I/F

void cgromva_reset(void) {
	ZeroMemory(&cgromva, sizeof(cgromva));
}

void cgromva_bind(void) {
	iocoreva_attachout(0x14c, cgromva_o14c);
	iocoreva_attachout(0x14d, cgromva_o14d);
	iocoreva_attachout(0x14e, cgromva_o14e);
	iocoreva_attachout(0x14f, cgromva_o14f);

	iocoreva_attachinp(0x14e, cgromva_i14e);
}

#endif
