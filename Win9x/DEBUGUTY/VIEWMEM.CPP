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
#include	"va91.h"


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

	// I-Oバンクメモリ
	src = NULL;
	if (cfg->bmsio_bank < bmsio.cfg.numbanks) {
		src = bmsiowork.bmsmem + cfg->bmsio_bank * 0x20000;
	}
	cpmem(&adrs, &buf, &size, 0x80000, 0xa0000, src);

	// システムメモリエリア
	switch(cfg->sysm_bank) {
	case 1:
		cpmem(&adrs, &buf, &size, 0xa0000, 0xe0000, textmem);
		break;
	case 4:
		cpmem(&adrs, &buf, &size, 0xa0000, 0xe0000, grphmem);
		break;
	case 8:
		cpmem(&adrs, &buf, &size, 0xa0000, 0xe0000, fontmem);
		break;
	case 9:
		cpmem(&adrs, &buf, &size, 0xa0000, 0xb0000, fontmem + 0x40000);
		cpmem(&adrs, &buf, &size, 0xb0000, 0xb4000, backupmem);
		cpmem(&adrs, &buf, &size, 0xb4000, 0xe0000, NULL);
		break;
	case 0x0c:
		cpmem(&adrs, &buf, &size, 0xa0000, 0xe0000, dicmem);
		break;
	case 0x0d:
		cpmem(&adrs, &buf, &size, 0xa0000, 0xe0000, dicmem + 0x40000);
		break;
	case 0x0f:
		if (va91.cfg.enabled) {
			switch(cfg->va91sysm_bank) {
			case 9:
				cpmem(&adrs, &buf, &size, 0xa0000, 0xb2000, NULL);
				cpmem(&adrs, &buf, &size, 0xb2000, 0xb4000, backupmem + 0x2000);
				cpmem(&adrs, &buf, &size, 0xb4000, 0xe0000, NULL);
				break;
			case 0x0c:
				cpmem(&adrs, &buf, &size, 0xa0000, 0xe0000, va91dicmem);
				break;
			case 0x0d:
				cpmem(&adrs, &buf, &size, 0xa0000, 0xe0000, va91dicmem + 0x40000);
				break;
			default:
				cpmem(&adrs, &buf, &size, 0xa0000, 0xe0000, NULL);
				break;
			}
		}
		else {
			cpmem(&adrs, &buf, &size, 0xa0000, 0xe0000, NULL);
		}
		break;
	default:
		cpmem(&adrs, &buf, &size, 0xa0000, 0xe0000, NULL);
		break;
	}
	// ROMエリア0
	src = NULL;
	switch(cfg->rom0_bank) {
	case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
	case 8: case 9:
		src = rom0mem + 0x10000 * cfg->rom0_bank;
		break;
	case 0x0f:
		if (va91.cfg.enabled && cfg->va91rom0_bank < 0x0a) {
			src = va91rom0mem + 0x10000 * cfg->va91rom0_bank;
		}
		break;
	}
	/*
	if (cfg->rom0_bank < 0x0a) {
		src = rom0mem + 0x10000 * cfg->rom0_bank;
	}
	*/
	cpmem(&adrs, &buf, &size, 0xe0000, 0xf0000, src);
	// ROMエリア1
	src = NULL;
	switch(cfg->rom1_bank) {
	case 0:
	case 1:
		src = rom1mem + 0x10000 * cfg->rom1_bank;
		break;
	case 0x0f:
		if (va91.cfg.enabled && cfg->va91rom1_bank < 0x02) {
			src = va91rom1mem + 0x10000 * cfg->va91rom1_bank;
		}
		break;
	}
	/*
	if (cfg->rom1_bank < 0x02) {
		src = rom1mem + 0x10000 * cfg->rom1_bank;
	}
	*/
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

