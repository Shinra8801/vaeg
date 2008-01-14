#include	"compiler.h"
#include	"strres.h"
#include	"scrnmng.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"sound.h"
#include	"fmboard.h"
#include	"np2info.h"

#if defined(SUPPORT_PC88VA)
#include	"memoryva.h"
#include	"subsystem.h"
#include	"va91.h"
#endif

static const char str_comma[] = ", ";
static const char str_2halfMHz[] = "2.5MHz";
#define str_5MHz	(str_2halfMHz + 2)
static const char str_8MHz[] = "8MHz";
static const char str_notexist[] = "not exist";
static const char str_disable[] = "disable";

#if defined(VAEG_EXT)
static const char str_na[] = "N/A";
static const char str_blank[] = " ";
static const char str_exist[] = "exist";
static const char str_ok[] = "OK";
static const char str_ng[] = "NG";

static const char str_model[] =
						"PC-9801VF\0"				\
						"PC-9801VM\0"				\
						"PC-9801VX\0"				\
						"";

static const char str_model_9821[] =
						"PC-9821";

static const char str_model_epson[] =
						"PC-286";

#if defined(SUPPORT_PC88VA)
static const char str_model_88va[] =
						"\0"						\
						"PC-88VA1\0"				\
						"PC-88VA2";

static const char str_romtype_88va[] =
						"PC-88VA1\0"				\
						"PC-88VA2/3\0"				\
						"Unknown";

static const char str_88va_rom00[] = "00";
static const char str_88va_rom08[] = "08";
static const char str_88va_rom1[]  = "1";
static const char str_88va_dic[]   = "DIC";
static const char str_88va_font[]  = "FONT";
static const char str_88va_subsys[]= "SUBSYS";
#endif

#endif

static const char str_cpu[] =
						"8086-2\0"					\
						"70116\0"					\
						"80286\0"					\
						"80386\0"					\
						"80486\0"					\
						"Pentium\0"					\
						"PentiumPro";
static const char str_winclr[] =
						"256-colors\0"				\
						"65536-colors\0"			\
						"full color\0"				\
						"true color";
static const char str_winmode[] =
						" (window)\0"				\
						" (fullscreen)";
static const char str_grcgchip[] =
						"\0"						\
						"GRCG \0"					\
						"GRCG CG-Window \0"			\
						"EGC CG-Window ";
static const char str_vrammode[] =
						"Digital\0"					\
						"Analog\0"					\
						"256colors";
static const char str_vrampage[] =
						" page-0\0"					\
						" page-1\0"					\
						" page-all";
static const char str_chpan[] =
						"none\0"					\
						"Mono-R\0"					\
						"Mono-L\0"					\
						"Stereo";
#if defined(SUPPORT_PC88VA)
static const char str_fmboard[] =
						"none\0"					\
						"PC-9801-14\0"				\
						"PC-9801-26\0"				\
						"PC-9801-86\0"				\
						"PC-9801-26 + 86\0"			\
						"PC-9801-118\0"				\
						"PC-9801-86 + Chibi-oto\0"	\
						"Speak board\0"				\
						"Spark board\0"				\
						"AMD-98\0"					\
						"Sound Board 1\0"			\
						"Sound Board 2";
#else
static const char str_fmboard[] =
						"none\0"					\
						"PC-9801-14\0"				\
						"PC-9801-26\0"				\
						"PC-9801-86\0"				\
						"PC-9801-26 + 86\0"			\
						"PC-9801-118\0"				\
						"PC-9801-86 + Chibi-oto\0"	\
						"Speak board\0"				\
						"Spark board\0"				\
						"AMD-98";
#endif

static const char str_clockfmt[] = "%d.%1dMHz";
static const char str_memfmt[] = "%3uKB";
static const char str_memfmt2[] = "%3uKB + %uKB";
static const char str_memfmt3[] = "%d.%1dMB";
static const char str_width[] = "width-%u";
static const char str_dispclock[] = "%u.%.2ukHz / %u.%uHz";

static const char str_pcm86a[] = "   PCM: %dHz %dbit %s";
static const char str_pcm86b[] = "        %d / %d / 32768";
static const char str_rhythm[] = "BSCHTR";


// ---- common

static void info_ver(char *str, int maxlen, NP2INFOEX *ex) {

	milstr_ncpy(str, np2version, maxlen);
	(void)ex;
}

#if defined(VAEG_EXT)
static void info_model(char *str, int maxlen, NP2INFOEX *ex) {
#if defined(SUPPORT_PC88VA)
	if (pccore.model_va == PCMODEL_NOTVA) {
#endif
		if (pccore.model & PCMODEL_PC9821) {
			milstr_ncpy(str, str_model_9821, maxlen);
		}
		else if (pccore.model & PCMODEL_EPSON) {
			milstr_ncpy(str, str_model_epson, maxlen);
		}
		else {
			milstr_ncpy(str, milstr_list(str_model, pccore.model), maxlen);
		}
#if defined(SUPPORT_PC88VA)
	}
	else {
		milstr_ncpy(str, milstr_list(str_model_88va, pccore.model_va), maxlen);
	}
#endif
	(void)ex;
}

#endif


static void info_cpu(char *str, int maxlen, NP2INFOEX *ex) {

	UINT	family;

#if defined(CPU_FAMILY)
	family = min(CPU_FAMILY, 6);
#else
	family = (CPU_TYPE & CPUTYPE_V30)?1:2;
#endif
	milstr_ncpy(str, milstr_list(str_cpu, family), maxlen);
	(void)ex;
}

static void info_clock(char *str, int maxlen, NP2INFOEX *ex) {

	UINT32	clock;
	char	clockstr[16];

	clock = (pccore.realclock + 50000) / 100000;
	SPRINTF(clockstr, str_clockfmt, clock/10, clock % 10);
	milstr_ncpy(str, clockstr, maxlen);
	(void)ex;
}

static void info_base(char *str, int maxlen, NP2INFOEX *ex) {

	milstr_ncpy(str,
				(pccore.cpumode & CPUMODE_8MHZ)?str_8MHz:str_5MHz, maxlen);
	(void)ex;
}

static void info_mem1(char *str, int maxlen, NP2INFOEX *ex) {

	UINT	memsize;
	char	memstr[32];

	memsize = np2cfg.memsw[2] & 7;
	if (memsize < 6) {
		memsize = (memsize + 1) * 128;
	}
	else {
		memsize = 640;
	}
	if (pccore.extmem) {
		SPRINTF(memstr, str_memfmt2, memsize, pccore.extmem * 1024);
	}
	else {
		SPRINTF(memstr, str_memfmt, memsize);
	}
	milstr_ncpy(str, memstr, maxlen);
	(void)ex;
}

static void info_mem2(char *str, int maxlen, NP2INFOEX *ex) {

	UINT	memsize;
	char	memstr[16];

	memsize = np2cfg.memsw[2] & 7;
	if (memsize < 6) {
		memsize = (memsize + 1) * 128;
	}
	else {
		memsize = 640;
	}
	memsize += pccore.extmem * 1024;
	SPRINTF(memstr, str_memfmt, memsize);
	milstr_ncpy(str, memstr, maxlen);
	(void)ex;
}

static void info_mem3(char *str, int maxlen, NP2INFOEX *ex) {

	UINT	memsize;
	char	memstr[16];

	memsize = np2cfg.memsw[2] & 7;
	if (memsize < 6) {
		memsize = (memsize + 1) * 128;
	}
	else {
		memsize = 640;
	}
	if (pccore.extmem > 1) {
		SPRINTF(memstr, str_memfmt3, pccore.extmem, memsize / 100);
	}
	else {
		SPRINTF(memstr, str_memfmt, memsize);
	}
	milstr_ncpy(str, memstr, maxlen);
	(void)ex;
}

static void info_gdc(char *str, int maxlen, NP2INFOEX *ex) {

	milstr_ncpy(str, milstr_list(str_grcgchip, grcg.chip & 3), maxlen);
	milstr_ncat(str, str_2halfMHz + ((gdc.clock & 0x80)?2:0), maxlen);
	(void)ex;
}

static void info_gdc2(char *str, int maxlen, NP2INFOEX *ex) {

	char	textstr[32];

	SPRINTF(textstr, str_dispclock,
						gdc.hclock / 1000, (gdc.hclock / 10) % 100,
						gdc.vclock / 10, gdc.vclock % 10);
	milstr_ncpy(str, textstr, maxlen);
	(void)ex;
}

static void info_text(char *str, int maxlen, NP2INFOEX *ex) {

const char	*p;
	char	textstr[64];

	if (!(gdcs.textdisp & GDCSCRN_ENABLE)) {
		p = str_disable;
	}
	else {
		SPRINTF(textstr, str_width, ((gdc.mode1 & 0x4)?40:80));
		p = textstr;
	}
	milstr_ncpy(str, p, maxlen);
	(void)ex;
}

static void info_grph(char *str, int maxlen, NP2INFOEX *ex) {

const char	*p;
	UINT	md;
	UINT	pg;
	char	grphstr[32];

	if (!(gdcs.grphdisp & GDCSCRN_ENABLE)) {
		p = str_disable;
	}
	else {
		md = (gdc.analog & (1 << GDCANALOG_16))?1:0;
		pg = gdcs.access;
#if defined(SUPPORT_PC9821)
		if (gdc.analog & (1 << (GDCANALOG_256))) {
			md = 2;
			if (gdc.analog & (1 << (GDCANALOG_256E))) {
				pg = 2;
			}
		}
#endif
		milstr_ncpy(grphstr, milstr_list(str_vrammode, md), sizeof(grphstr));
		milstr_ncat(grphstr, milstr_list(str_vrampage, pg), sizeof(grphstr));
		p = grphstr;
	}
	milstr_ncpy(str, p, maxlen);
	(void)ex;
}

static void info_sound(char *str, int maxlen, NP2INFOEX *ex) {

	UINT	type;

	type = 0;
	switch(usesound) {
		case 0x01:
			type = 1;
			break;

		case 0x02:
			type = 2;
			break;

		case 0x04:
			type = 3;
			break;

		case 0x06:
			type = 4;
			break;

		case 0x08:
			type = 5;
			break;

		case 0x14:
			type = 6;
			break;

		case 0x20:
			type = 7;
			break;

		case 0x40:
			type = 8;
			break;

		case 0x80:
			type = 9;
			break;
#if defined(SUPPORT_PC88VA)
		case 0x200:
			type = 11;
			break;
#endif
	}
	milstr_ncpy(str, milstr_list(str_fmboard, type), maxlen);
	(void)ex;
}

static void info_extsnd(char *str, int maxlen, NP2INFOEX *ex) {

	char	buf[64];

	info_sound(str, maxlen, ex);
	if (usesound & 4) {
		milstr_ncat(str, ex->cr, maxlen);
		SPRINTF(buf, str_pcm86a,
							pcm86rate8[pcm86.fifo & 7] >> 3,
							(16 - ((pcm86.dactrl >> 3) & 8)),
							milstr_list(str_chpan, (pcm86.dactrl >> 4) & 3));
		milstr_ncat(str, buf, maxlen);
		milstr_ncat(str, ex->cr, maxlen);
		SPRINTF(buf, str_pcm86b, pcm86.virbuf, pcm86.fifosize);
		milstr_ncat(str, buf, maxlen);
	}
}

static void info_bios(char *str, int maxlen, NP2INFOEX *ex) {

	str[0] = '\0';
	if (pccore.rom & PCROM_BIOS) {
		milstr_ncat(str, str_biosrom, maxlen);
	}
	if (soundrom.name[0]) {
		if (str[0]) {
			milstr_ncat(str, str_comma, maxlen);
		}
		milstr_ncat(str, soundrom.name, maxlen);
	}
	if (str[0] == '\0') {
		milstr_ncat(str, str_notexist, maxlen);
	}
	(void)ex;
}

#if defined(SUPPORT_PC88VA)
static void info_romtype_88va(char *str, int maxlen, NP2INFOEX *ex) {
	if (pccore.model_va == PCMODEL_NOTVA) {
		milstr_ncpy(str, str_na, maxlen);
	}
	else {
		int romtype;

		romtype = 0xffff - (rom1mem[0xffff] * 256 + rom1mem[0xfffe]);
		if (romtype > 2) romtype = 2;
		milstr_ncpy(str, milstr_list(str_romtype_88va, romtype), maxlen);
	}
}

static void info_bios_88va(char *str, int maxlen, NP2INFOEX *ex) {
	milstr_ncpy(str, str_88va_rom00, maxlen);
	milstr_ncat(str, str_blank, maxlen);
	milstr_ncat(str, (((memoryva.rom0exist & 0x00ff) == 0x00ff) ? str_ok : str_ng), maxlen);
	milstr_ncat(str, str_comma, maxlen);

	milstr_ncat(str, str_88va_rom08, maxlen);
	milstr_ncat(str, str_blank, maxlen);
	milstr_ncat(str, (((memoryva.rom0exist & 0x0300) == 0x0300) ? str_ok : str_ng), maxlen);
	milstr_ncat(str, str_comma, maxlen);

	milstr_ncat(str, str_88va_rom1, maxlen);
	milstr_ncat(str, str_blank, maxlen);
	milstr_ncat(str, (((memoryva.rom1exist & 0x0003) == 0x0003) ? str_ok : str_ng), maxlen);
	milstr_ncat(str, str_comma, maxlen);

	milstr_ncat(str, str_88va_font, maxlen);
	milstr_ncat(str, str_blank, maxlen);
	milstr_ncat(str, (((memoryva.sysmromexist & 0x0300) == 0x0300) ? str_ok : str_ng), maxlen);
	milstr_ncat(str, str_comma, maxlen);

	milstr_ncat(str, str_88va_dic, maxlen);
	milstr_ncat(str, str_blank, maxlen);
	milstr_ncat(str, (((memoryva.sysmromexist & 0x3000) == 0x3000) ? str_ok : str_ng), maxlen);
}

static void info_bios_88va91(char *str, int maxlen, NP2INFOEX *ex) {
	if (va91.cfg.enabled) {
		milstr_ncpy(str, str_88va_rom00, maxlen);
		milstr_ncat(str, str_blank, maxlen);
		milstr_ncat(str, (((va91cfg.rom0exist & 0x00ff) == 0x00ff) ? str_ok : str_ng), maxlen);
		milstr_ncat(str, str_comma, maxlen);

		milstr_ncat(str, str_88va_rom08, maxlen);
		milstr_ncat(str, str_blank, maxlen);
		milstr_ncat(str, (((va91cfg.rom0exist & 0x0300) == 0x0300) ? str_ok : str_ng), maxlen);
		milstr_ncat(str, str_comma, maxlen);

		milstr_ncat(str, str_88va_rom1, maxlen);
		milstr_ncat(str, str_blank, maxlen);
		milstr_ncat(str, (((va91cfg.rom1exist & 0x0003) == 0x0003) ? str_ok : str_ng), maxlen);
		milstr_ncat(str, str_comma, maxlen);

		milstr_ncat(str, str_88va_dic, maxlen);
		milstr_ncat(str, str_blank, maxlen);
		milstr_ncat(str, (((va91cfg.sysmromexist & 0x3000) == 0x3000) ? str_ok : str_ng), maxlen);
	}
	else {
		milstr_ncpy(str, str_disable, maxlen);
	}
}

static void info_bios_88vasubsys(char *str, int maxlen, NP2INFOEX *ex) {
	if (subsystem.romexist) {
		milstr_ncpy(str, str_exist, maxlen);
	}
	else {
		milstr_ncpy(str, str_notexist, maxlen);
	}
}
#endif

static void info_rhythm(char *str, int maxlen, NP2INFOEX *ex) {

	char	rhythmstr[8];
	UINT	exist;
	UINT	i;

	exist = rhythm_getcaps();
	milstr_ncpy(rhythmstr, str_rhythm, sizeof(rhythmstr));
	for (i=0; i<6; i++) {
		if (!(exist & (1 << i))) {
			rhythmstr[i] = '_';
		}
	}
	milstr_ncpy(str, rhythmstr, maxlen);
	(void)ex;
}

static void info_display(char *str, int maxlen, NP2INFOEX *ex) {

	UINT	bpp;

	bpp = scrnmng_getbpp();
	milstr_ncpy(str, milstr_list(str_winclr, ((bpp >> 3) - 1) & 3), maxlen);
	milstr_ncat(str, milstr_list(str_winmode, (scrnmng_isfullscreen())?1:0),
																	maxlen);
	(void)ex;
}


// ---- make string

typedef struct {
	char	key[8];
	void	(*proc)(char *str, int maxlen, NP2INFOEX *ex);
} INFOPROC;

static const INFOPROC infoproc[] = {
#if defined(VAEG_EXT)
			{"MODEL",		info_model},
			{"ROMTPVA",		info_romtype_88va},
			{"BIOSVA",		info_bios_88va},
			{"BIOS91",		info_bios_88va91},
			{"BIOSSUB",		info_bios_88vasubsys},
#endif
			{"VER",			info_ver},
			{"CPU",			info_cpu},
			{"CLOCK",		info_clock},
			{"BASE",		info_base},
			{"MEM1",		info_mem1},
			{"MEM2",		info_mem2},
			{"MEM3",		info_mem3},
			{"GDC",			info_gdc},
			{"GDC2",		info_gdc2},
			{"TEXT",		info_text},
			{"GRPH",		info_grph},
			{"SND",			info_sound},
			{"EXSND",		info_extsnd},
			{"BIOS",		info_bios},
			{"RHYTHM",		info_rhythm},
			{"DISP",		info_display}};


static BOOL defext(char *dst, const char *key, int maxlen, NP2INFOEX *ex) {

	milstr_ncpy(dst, key, maxlen);
	(void)ex;
	return(TRUE);
}

void np2info(char *dst, const char *src, int maxlen, const NP2INFOEX *ex) {

	NP2INFOEX	statex;
	char		c;
	int			leng;
	char		infwork[12];
const INFOPROC	*inf;
const INFOPROC	*infterm;

	if ((dst == NULL) || (maxlen <= 0) || (src == NULL)) {
		return;
	}
	if (ex == NULL) {
		milstr_ncpy(statex.cr, str_oscr, sizeof(statex.cr));
		statex.ext = NULL;
	}
	else {
		statex = *ex;
	}
	if (statex.ext == NULL) {
		statex.ext = defext;
	}
	while(maxlen > 0) {
		c = *src++;
		if (c == '\0') {
			break;
		}
		else if (c == '\n') {
			milstr_ncpy(dst, statex.cr, maxlen);
		}
		else if (c != '%') {
			*dst++ = c;
			maxlen--;
			continue;
		}
		else if (*src == '%') {
			src++;
			*dst++ = c;
			maxlen--;
			continue;
		}
		else {
			leng = 0;
			while(1) {
				c = *src;
				if (c == '\0') {
					break;
				}
				src++;
				if (c == '%') {
					break;
				}
				if (leng < (int)(sizeof(infwork) - 1)) {
					infwork[leng++] = c;
				}
			}
			infwork[leng] = '\0';
			inf = infoproc;
			infterm = infoproc + (sizeof(infoproc) / sizeof(INFOPROC));
			while(inf < infterm) {
				if (!milstr_cmp(infwork, inf->key)) {
					inf->proc(dst, maxlen, &statex);
					break;
				}
				inf++;
			}
			if (inf >= infterm) {
				if (!(*statex.ext)(dst, infwork, maxlen, &statex)) {
					continue;
				}
			}
		}
		leng = strlen(dst);
		dst += leng;
		maxlen -= leng;
	}
	*dst = '\0';
}

