#include	"compiler.h"
#include	"resource.h"
#include	"np2.h"
#include	"viewer.h"
#include	"viewmem.h"
#include	"cpucore.h"

#if defined(SUPPORT_PC88VA)
#include	"pccore.h"
#include	"bmsio.h"
#include	"memoryva.h"
#include	"gvramva.h"

//extern	_BMSIO		bmsio;

/*
CPUメモリアドレス*adrsから*sizeバイトのうち、
CPUメモリアドレスsrcadrsからlimit-1までの範囲に重なる部分を、bufにコピーする。
メモリのデータはsrcからコピーする。srcがCPUメモリアドレスsrcadrsに対応する。
コピー後、コピーした範囲の直後の位置を表すように、*buf, *adrs, *sizeを更新する。

*adrs>=srcadrsでなければならない。
*/
static void cpmem(DWORD *adrs, BYTE **buf, DWORD *size, DWORD srcadrs, DWORD limit, const BYTE *src) {
	DWORD len;

	if (!*size) return;
	if (*adrs < limit) {
		len = *size;
		if ((*adrs + len) > limit) {
			len = limit - *adrs;
		}
		if (src) {
			CopyMemory(*buf, src + *adrs - srcadrs, len);
		}
		else {
			FillMemory(*buf, len, 0xff);
		}
		*buf += len;
		*size -= len;
		*adrs += len;
	}
}


void viewmemva_read(VIEWMEM_T *cfg, DWORD adrs, BYTE *buf, DWORD size) {
	const BYTE *src;

	if (!size) {
		return;
	}

	// Main Memory
	cpmem(&adrs, &buf, &size, 0x00000, 0x80000, mem);
	// ToDo: I-Oバンクメモリ
//	cpmem(&adrs, &buf, &size, 0x80000, 0xa0000, bmsio.addr);
	cpmem(&adrs, &buf, &size, 0x80000, 0xa0000, NULL);
	// システムメモリエリア
	switch(sysm_bank) {
	case 1:
		cpmem(&adrs, &buf, &size, 0xa0000, 0xe0000, textmem);
		break;
	case 4:
		cpmem(&adrs, &buf, &size, 0xa0000, 0xe0000, grphmem);
		break;
	case 8:
		cpmem(&adrs, &buf, &size, 0xa0000, 0xe0000, fontmem);
		break;
//	ToDo:
//	case 9:
//		break;
	case 0x0c:
		cpmem(&adrs, &buf, &size, 0xa0000, 0xe0000, dicmem);
		break;
	case 0x0d:
		cpmem(&adrs, &buf, &size, 0xa0000, 0xe0000, dicmem + 0x40000);
		break;
	default:
		cpmem(&adrs, &buf, &size, 0xa0000, 0xe0000, NULL);
		break;
	}
	// ROMエリア0
	src = NULL;
	if (rom0_bank < 0x0a) {
		src = rom0mem + 0x10000 * rom0_bank;
	}
	cpmem(&adrs, &buf, &size, 0xe0000, 0xf0000, src);
	// ROMエリア1
	src = NULL;
	if (rom1_bank < 0x02) {
		src = rom1mem + 0x10000 * rom1_bank;
	}
	cpmem(&adrs, &buf, &size, 0xf0000, 0x100000, src);

	// 0x100000以降
	cpmem(&adrs, &buf, &size, 0x100000, 0xffffff, NULL);
}
#endif


void viewmem_read(VIEWMEM_T *cfg, DWORD adrs, BYTE *buf, DWORD size) {

#if defined(SUPPORT_PC88VA)
	if (pccore.model_va != PCMODEL_NOTVA) {
		viewmemva_read(cfg, adrs, buf, size);
		return;
	}
#endif

	if (!size) {
		return;
	}

	// Main Memory
	if (adrs < 0xa4000) {
		if ((adrs + size) <= 0xa4000) {
			CopyMemory(buf, mem + adrs, size);
			return;
		}
		else {
			DWORD len;
			len = 0xa4000 - adrs;
			CopyMemory(buf, mem + adrs, len);
			buf += len;
			size -= len;
			adrs = 0xa4000;
		}
	}

	// CG-Windowは無視
	if (adrs < 0xa5000) {
		if ((adrs + size) <= 0xa5000) {
			ZeroMemory(buf, size);
			return;
		}
		else {
			DWORD len;
			len = 0xa5000 - adrs;
			ZeroMemory(buf, len);
			buf += len;
			size -= len;
			adrs = 0xa5000;
		}
	}

	// Main Memory
	if (adrs < 0xa8000) {
		if ((adrs + size) <= 0xa8000) {
			CopyMemory(buf, mem + adrs, size);
			return;
		}
		else {
			DWORD len;
			len = 0xa8000 - adrs;
			CopyMemory(buf, mem + adrs, len);
			buf += len;
			size -= len;
			adrs = 0xa8000;
		}
	}

	// Video Memory
	if (adrs < 0xc0000) {
		DWORD page;
		page = ((cfg->vram)?VRAM_STEP:0);
		if ((adrs + size) <= 0xc0000) {
			CopyMemory(buf, mem + page + adrs, size);
			return;
		}
		else {
			DWORD len;
			len = 0xc0000 - adrs;
			CopyMemory(buf, mem + page + adrs, len);
			buf += len;
			size -= len;
			adrs = 0xc0000;
		}
	}

	// Main Memory
	if (adrs < 0xe0000) {
		if ((adrs + size) <= 0xe0000) {
			CopyMemory(buf, mem + adrs, size);
			return;
		}
		else {
			DWORD len;
			len = 0xe0000 - adrs;
			CopyMemory(buf, mem + adrs, len);
			buf += len;
			size -= len;
			adrs = 0xe0000;
		}
	}

	// Video Memory
	if (adrs < 0xe8000) {
		DWORD page;
		page = ((cfg->vram)?VRAM_STEP:0);
		if ((adrs + size) <= 0xe8000) {
			CopyMemory(buf, mem + page + adrs, size);
			return;
		}
		else {
			DWORD len;
			len = 0xe8000 - adrs;
			CopyMemory(buf, mem + page + adrs, len);
			buf += len;
			size -= len;
			adrs = 0xe8000;
		}
	}

	// BIOS
	if (adrs < 0x0f8000) {
		if ((adrs + size) <= 0x0f8000) {
			CopyMemory(buf, mem + adrs, size);
			return;
		}
		else {
			DWORD len;
			len = 0x0f8000 - adrs;
			CopyMemory(buf, mem + adrs, len);
			buf += len;
			size -= len;
			adrs = 0x0f8000;
		}
	}

	// BIOS/ITF
	if (adrs < 0x100000) {
		DWORD page;
		page = ((cfg->itf)?VRAM_STEP:0);
		if ((adrs + size) <= 0x100000) {
			CopyMemory(buf, mem + page + adrs, size);
			return;
		}
		else {
			DWORD len;
			len = 0x100000 - adrs;
			CopyMemory(buf, mem + page + adrs, len);
			buf += len;
			size -= len;
			adrs = 0x100000;
		}
	}

	// HMA
	if (adrs < 0x10fff0) {
		DWORD adrs2;
		adrs2 = adrs & 0xffff;
		adrs2 += ((cfg->A20)?VRAM_STEP:0);
		if ((adrs + size) <= 0x10fff0) {
			CopyMemory(buf, mem + adrs2, size);
			return;
		}
		else {
			DWORD len;
			len = 0x10fff0 - adrs;
			CopyMemory(buf, mem + adrs2, len);
			buf += len;
			size -= len;
			adrs = 0x10fff0;
		}
	}
}

