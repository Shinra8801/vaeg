/*
 * CGROMVA.C: PC-88VA Character generator
 *
 * ToDo:
 *   I/Oポートからの読み出し
 *   ハードウェア文字コードが不正の場合の動作を実機にあわせる。
 *	 8x8フォントへの対応
 */

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"

#include	"memoryva.h"

#if defined(SUPPORT_PC88VA)

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

	if (jis2 == 0) {
		// ANK
		base = fontmem;
		font = 0x40000 + ((hccode & 0xff) << 4);
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
			base = backupmem;
			font = lr + 
					((jis2 & 0x60) << 6) +
					((jis1 & 0x01) << 10) +
					((jis2 & 0x1f) << 5);
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
	return (hccode & 0xff00) == 0 ? 1 : 2;
}

#endif
