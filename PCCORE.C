#include	"compiler.h"
#include	"strres.h"
#include	"dosio.h"
#include	"soundmng.h"
#include	"sysmng.h"
#include	"timemng.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"gdc_sub.h"
#include	"cbuscore.h"
#include	"pc9861k.h"
#include	"mpu98ii.h"
#include	"amd98.h"
#include	"bios.h"
#include	"biosva.h"
#include	"biosmem.h"
#include	"vram.h"
#include	"scrndraw.h"
#include	"dispsync.h"
#include	"palettes.h"
#include	"maketext.h"
#include	"maketgrp.h"
#include	"makegrph.h"
#include	"makegrex.h"
#include	"sound.h"
#include	"fmboard.h"
#include	"beep.h"
#include	"s98.h"
#include	"font.h"
#include	"diskdrv.h"
#include	"fddfile.h"
#include	"fdd_mtr.h"
#include	"sxsi.h"
#if defined(SUPPORT_HOSTDRV)
#include	"hostdrv.h"
#endif
#include	"np2ver.h"
#include	"calendar.h"
#include	"timing.h"
#include	"keystat.h"
#include	"debugsub.h"

#include	"bmsio.h"

#if defined(SUPPORT_PC88VA)
#include	"../vramva/maketextva.h"
#include	"../vramva/makesprva.h"
#include	"../vramva/makegrphva.h"
#include	"scrnmng.h"
#include	"../vramva/scrndrawva.h"
#include	"memoryva.h"
#include	"tsp.h"
#include	"sgp.h"
#include	"videova.h"
#include	"subsystemmx.h"
#include	"va91.h"
#endif

const OEMCHAR np2version[] = OEMTEXT(NP2VER_CORE);

#if defined(_WIN32_WCE)
#define	PCBASEMULTIPLE	2
#else
#define	PCBASEMULTIPLE	4
#endif


	NP2CFG	np2cfg = {
				0, 1, 0, 32, 0, 0, 0x40,
				0, 0, 0, 0,
				{0x3e, 0x73, 0x7b}, 0,
				0, 0, {1, 1, 6, 1, 8, 1},
#if defined(SUPPORT_PC88VA)
				OEMTEXT("88VA2"), PCBASECLOCK40, 2,
#else
				OEMTEXT("VX"), PCBASECLOCK25, PCBASEMULTIPLE,
#endif
				{0x48, 0x05, 0x04, 0x00, 0x01, 0x00, 0x00, 0x6e},
				1, 1, 2, 1, 0x000000, 0xffffff,
#if defined(SUPPORT_PC88VA)
				22050, 500, 0x200, 0,
#else
				22050, 500, 4, 0,
#endif
				{0, 0, 0}, 0xd1, 0x7f, 0xd1, 0, 0, 1,
				3, {0x0c, 0x0c, 0x08, 0x06, 0x03, 0x0c}, 64, 64, 64, 64, 64,
				1, 0x82,
				0, {0x17, 0x04, 0x1f}, {0x0c, 0x0c, 0x02, 0x10, 0x3f, 0x3f},
#if defined(VAEG_EXT)
				3, 1, 85, 0, 0,
#else
				3, 1, 80, 0, 0,
#endif
#if defined(SUPPORT_PC88VA)
				0,
#endif
				{OEMTEXT(""), OEMTEXT("")},
#if defined(SUPPORT_SCSI)
				{OEMTEXT(""), OEMTEXT(""), OEMTEXT(""), OEMTEXT("")},
#endif
				OEMTEXT(""), OEMTEXT(""), OEMTEXT("")};

	PCCORE	pccore = {	PCBASECLOCK25, PCBASEMULTIPLE,
						0, PCMODEL_VX, 0, 0, {0x3e, 0x73, 0x7b}, 0,
						0, 0,
						PCBASECLOCK25 * PCBASEMULTIPLE};

	UINT8	screenupdate = 3;			// bit0=1.画面の一部について描画が必要
										// bit1=1.画面全体の描画が必要
	int		screendispflag = 1;
	int		soundrenewal = 0;
	BOOL	drawframe;
	UINT	drawcount = 0;
	BOOL	hardwarereset = FALSE;


// ---------------------------------------------------------------------------

void getbiospath(OEMCHAR *path, const OEMCHAR *fname, int maxlen) {

const OEMCHAR	*p;

	p = np2cfg.biospath;
	if (p[0]) {
		file_cpyname(path, p, maxlen);
		file_setseparator(path, maxlen);
		file_catname(path, fname, maxlen);
	}
	else {
		file_cpyname(path, file_getcd(fname), maxlen);
	}
}


// ----

static void pccore_set(void) {

	UINT8	model;
	UINT32	multiple;
	UINT8	extsize;

	ZeroMemory(&pccore, sizeof(pccore));
	model = PCMODEL_VX;

#if defined(SUPPORT_PC88VA)
	pccore.model_va = PCMODEL_NOTVA;
	if (!milstr_cmp(np2cfg.model, str_VA1)) {
		model = PCMODEL_VM;
		pccore.model_va = PCMODEL_VA1;
	}
	else if (!milstr_cmp(np2cfg.model, str_VA2)) {
		model = PCMODEL_VM;
		pccore.model_va = PCMODEL_VA2;
	}
	else
#endif

	if (!milstr_cmp(np2cfg.model, str_VM)) {
		model = PCMODEL_VM;
	}
	else if (!milstr_cmp(np2cfg.model, str_EPSON)) {
		model = PCMODEL_EPSON | PCMODEL_VM;
	}
	pccore.model = model;

#if defined(SUPPORT_PC88VA)
	if (np2cfg.baseclock >= ((PCBASECLOCK40 + PCBASECLOCK25) / 2)) {
		pccore.baseclock = PCBASECLOCK40;			// 4.0MHz
		pccore.cpumode = CPUMODE_BASE4MHZ;
	}
	else 
#endif
	if (np2cfg.baseclock >= ((PCBASECLOCK25 + PCBASECLOCK20) / 2)) {
		pccore.baseclock = PCBASECLOCK25;			// 2.5MHz
		pccore.cpumode = 0;
	}
	else {
		pccore.baseclock = PCBASECLOCK20;			// 2.0MHz
		pccore.cpumode = CPUMODE_8MHZ;
	}
	multiple = np2cfg.multiple;
	if (multiple == 0) {
		multiple = 1;
	}
	else if (multiple > 32) {
		multiple = 32;
	}
	pccore.multiple = multiple;
	pccore.realclock = pccore.baseclock * multiple;

	// HDDの接続 (I/Oの使用状態が変わるので..
	if (np2cfg.dipsw[1] & 0x20) {
		pccore.hddif |= PCHDD_IDE;
	}

	// 拡張メモリ
	extsize = 0;
	if (!(np2cfg.dipsw[2] & 0x80)) {
		extsize = min(np2cfg.EXTMEM, 13);
	}
	pccore.extmem = extsize;
	CopyMemory(pccore.dipsw, np2cfg.dipsw, 3);

	// サウンドボードの接続
	pccore.sound = np2cfg.SOUND_SW;

	// その他CBUSの接続
	pccore.device = 0;
	if (np2cfg.pc9861enable) {
		pccore.device |= PCCBUS_PC9861K;
	}
	if (np2cfg.mpuenable) {
		pccore.device |= PCCBUS_MPU98;
	}
}


// --------------------------------------------------------------------------

#if !defined(DISABLE_SOUND)
static void sound_init(void) {

	UINT	rate;

	rate = np2cfg.samplingrate;
	if ((rate != 11025) && (rate != 22050) && (rate != 44100)) {
		rate = 0;
	}
	sound_create(rate, np2cfg.delayms);
	fddmtrsnd_initialize(rate);
	beep_initialize(rate);
	beep_setvol(np2cfg.BEEP_VOL);
	tms3631_initialize(rate);
	tms3631_setvol(np2cfg.vol14);
	opngen_initialize(rate);
	opngen_setvol(np2cfg.vol_fm);
	psggen_initialize(rate);
	psggen_setvol(np2cfg.vol_ssg);
	rhythm_initialize(rate);
	rhythm_setvol(np2cfg.vol_rhythm);
	adpcm_initialize(rate);
	adpcm_setvol(np2cfg.vol_adpcm);
	pcm86gen_initialize(rate);
	pcm86gen_setvol(np2cfg.vol_pcm);
	cs4231_initialize(rate);
	amd98_initialize(rate);
}

static void sound_term(void) {

	soundmng_stop();
	amd98_deinitialize();
	rhythm_deinitialize();
	beep_deinitialize();
	fddmtrsnd_deinitialize();
	sound_destroy();
}
#endif

void pccore_init(void) {

	CPU_INITIALIZE();

	pal_initlcdtable();
	pal_makelcdpal();
	pal_makeskiptable();
#if defined(SUPPORT_PC88VA)
//	palva_maketable();
	maketextva_initialize();
	makesprva_initialize();
	makegrphva_initialize();
	subsystemmx_initialize();
#endif
	dispsync_initialize();
	sxsi_initialize();

	font_initialize();
	font_load(np2cfg.fontfile, TRUE);
	maketext_initialize();
	makegrph_initialize();
	gdcsub_initialize();
	fddfile_initialize();

#if !defined(DISABLE_SOUND)
	sound_init();
#endif

	rs232c_construct();
	mpu98ii_construct();
	pc9861k_initialize();

	iocore_create();

#if defined(SUPPORT_HOSTDRV)
	hostdrv_initialize();
#endif
}

void pccore_term(void) {

#if defined(SUPPORT_HOSTDRV)
	hostdrv_deinitialize();
#endif

#if !defined(DISABLE_SOUND)
	sound_term();
#endif

	fdd_eject(0);
	fdd_eject(1);
	fdd_eject(2);
	fdd_eject(3);

	iocore_destroy();

	pc9861k_deinitialize();
	mpu98ii_destruct();
	rs232c_destruct();

	sxsi_trash();

	CPU_DEINITIALIZE();
}


void pccore_cfgupdate(void) {

	BOOL	renewal;
	int		i;

	renewal = FALSE;
	for (i=0; i<8; i++) {
		if (np2cfg.memsw[i] != mem[MEMX_MSW + i*4]) {
			np2cfg.memsw[i] = mem[MEMX_MSW + i*4];
			renewal = TRUE;
		}
	}
#if defined(SUPPORT_PC88VA)
	{
		UINT8 val;

		if (pccore.model_va != PCMODEL_NOTVA) {
			val = keystat_getlockedkey();
			if (np2cfg.lockedkey != val) {
				np2cfg.lockedkey = val;
				renewal = TRUE;
			}
		}
	}
#endif
	if (renewal) {
		sysmng_update(SYS_UPDATECFG);
	}
}

void pccore_reset(void) {

	int		i;

	soundmng_stop();
#if !defined(DISABLE_SOUND)
	if (soundrenewal) {
		soundrenewal = 0;
		sound_term();
		sound_init();
	}
#endif
	ZeroMemory(mem, 0x110000);
	ZeroMemory(mem + VRAM1_B, 0x18000);
	ZeroMemory(mem + VRAM1_E, 0x08000);
	ZeroMemory(mem + FONT_ADRS, 0x08000);

	//メモリスイッチ
	for (i=0; i<8; i++) {
		mem[0xa3fe2 + i*4] = np2cfg.memsw[i];
	}

	pccore_set();
#if defined(SUPPORT_BMS)
	bmsio_set();
#endif
#if defined(SUPPORT_PC88VA)
	if (pccore.model_va != PCMODEL_NOTVA) {
		keystat_setlockedkey(np2cfg.lockedkey);
	}
#endif
	nevent_allreset();

#if defined(VAEG_FIX)
	//後ろに移動
#else
	CPU_RESET();
	CPU_SETEXTSIZE((UINT32)pccore.extmem);
#endif

	CPU_TYPE = 0;
	if (np2cfg.dipsw[2] & 0x80) {
		CPU_TYPE = CPUTYPE_V30;
	}
#if defined(SUPPORT_PC88VA)
	if (pccore.model_va != PCMODEL_NOTVA) {
		CPU_TYPE = CPUTYPE_V30;
	}
#endif

#if defined(VAEG_FIX)
	CPU_RESET();
	CPU_SETEXTSIZE((UINT32)pccore.extmem);
#endif

	if (pccore.model & PCMODEL_EPSON) {			// RAM ctrl
		CPU_RAM_D000 = 0xffff;
	}

	// HDDセット
	sxsi_open();
#if defined(SUPPORT_SASI)
	if (sxsi_issasi()) {
		pccore.hddif &= ~PCHDD_IDE;
		pccore.hddif |= PCHDD_SASI;
		TRACEOUT(("supported SASI"));
	}
#endif
#if defined(SUPPORT_SCSI)
	if (sxsi_isscsi()) {
		pccore.hddif |= PCHDD_SCSI;
		TRACEOUT(("supported SCSI"));
	}
#endif

	sound_changeclock();
	beep_changeclock();
	sound_reset();
	fddmtrsnd_bind();

	fddfile_reset2dmode();
	bios0x18_16(0x20, 0xe1);

	iocore_reset();								// サウンドでpicを呼ぶので…
	cbuscore_reset();
	fmboard_reset(pccore.sound);

	i286_memorymap((pccore.model & PCMODEL_EPSON)?1:0);
#if defined(SUPPORT_PC88VA)
	i286_memorymap_va();
#endif
	iocore_build();
	iocore_bind();
	cbuscore_bind();
	fmboard_bind();

	fddmtr_initialize();
	calendar_initialize();
	vram_initialize();

	pal_change(1);

	bios_initialize();
#if defined(SUPPORT_PC88VA)
	if (pccore.model_va != PCMODEL_NOTVA) {
		biosva_initialize();
		va91_initialize();
	}
#endif
	CS_BASE = 0xf0000;
	CPU_CS = 0xf000;
	CPU_IP = 0xfff0;

	CPU_CLEARPREFETCH();
	sysmng_cpureset();

#if defined(SUPPORT_HOSTDRV)
	hostdrv_reset();
#endif

	timing_reset();
	soundmng_play();
}


static void drawscreen(void) {

	UINT8	timing;
	void	(VRAMCALL * grphfn)(int page, int alldraw);
	UINT8	bit;

	tramflag.timing++;
	timing = ((LOADINTELWORD(gdc.m.para + GDC_CSRFORM + 1)) >> 5) & 0x3e;
	if (!timing) {
		timing = 0x40;
	}
	if (tramflag.timing >= timing) {
		tramflag.timing = 0;
		tramflag.count++;
		tramflag.renewal |= (tramflag.count ^ 2) & 2;
		tramflag.renewal |= 1;
	}

	if (gdcs.textdisp & GDCSCRN_EXT) {
		gdc_updateclock();
	}

	if (!drawframe) {
		return;
	}
	if ((gdcs.textdisp & GDCSCRN_EXT) || (gdcs.grphdisp & GDCSCRN_EXT)) {
		if (dispsync_renewalvertical()) {
			gdcs.textdisp |= GDCSCRN_ALLDRAW2;
			gdcs.grphdisp |= GDCSCRN_ALLDRAW2;
		}
	}
	if (gdcs.textdisp & GDCSCRN_EXT) {
		gdcs.textdisp &= ~GDCSCRN_EXT;
		dispsync_renewalhorizontal();
		tramflag.renewal |= 1;
		if (dispsync_renewalmode()) {
			screenupdate |= 2;
		}
	}
	if (gdcs.palchange) {
		gdcs.palchange = 0;
		pal_change(0);
		screenupdate |= 1;
	}
	if (gdcs.grphdisp & GDCSCRN_EXT) {
		gdcs.grphdisp &= ~GDCSCRN_EXT;
		if (((gdc.clock & 0x80) && (gdc.clock != 0x83)) ||
			(gdc.clock == 0x03)) {
			gdc.clock ^= 0x80;
			gdcs.grphdisp |= GDCSCRN_ALLDRAW2;
		}
	}
	if (gdcs.grphdisp & GDCSCRN_ENABLE) {
		if (!(gdc.mode1 & 2)) {
			grphfn = makegrph;
			bit = GDCSCRN_MAKE;
			if (gdcs.disp) {
				bit <<= 1;
			}
#if defined(SUPPORT_PC9821)
			if (gdc.analog & 2) {
				grphfn = makegrphex;
				if (gdc.analog & 4) {
					bit = GDCSCRN_MAKE | (GDCSCRN_MAKE << 1);
				}
			}
#endif
			if (gdcs.grphdisp & bit) {
				(*grphfn)(gdcs.disp, gdcs.grphdisp & bit & GDCSCRN_ALLDRAW2);
				gdcs.grphdisp &= ~bit;
				screenupdate |= 1;
			}
		}
		else if (gdcs.textdisp & GDCSCRN_ENABLE) {
			if (!gdcs.disp) {
				if ((gdcs.grphdisp & GDCSCRN_MAKE) ||
					(gdcs.textdisp & GDCSCRN_MAKE)) {
					if (!(gdc.mode1 & 0x4)) {
						maketextgrph(0, gdcs.textdisp & GDCSCRN_ALLDRAW,
								gdcs.grphdisp & GDCSCRN_ALLDRAW);
					}
					else {
						maketextgrph40(0, gdcs.textdisp & GDCSCRN_ALLDRAW,
								gdcs.grphdisp & GDCSCRN_ALLDRAW);
					}
					gdcs.grphdisp &= ~GDCSCRN_MAKE;
					screenupdate |= 1;
				}
			}
			else {
				if ((gdcs.grphdisp & (GDCSCRN_MAKE << 1)) ||
					(gdcs.textdisp & GDCSCRN_MAKE)) {
					if (!(gdc.mode1 & 0x4)) {
						maketextgrph(1, gdcs.textdisp & GDCSCRN_ALLDRAW,
								gdcs.grphdisp & (GDCSCRN_ALLDRAW << 1));
					}
					else {
						maketextgrph40(1, gdcs.textdisp & GDCSCRN_ALLDRAW,
								gdcs.grphdisp & (GDCSCRN_ALLDRAW << 1));
					}
					gdcs.grphdisp &= ~(GDCSCRN_MAKE << 1);
					screenupdate |= 1;
				}
			}
		}
	}
	if (gdcs.textdisp & GDCSCRN_ENABLE) {
		if (tramflag.renewal) {
			gdcs.textdisp |= maketext_curblink();
		}
		if ((cgwindow.writable & 0x80) && (tramflag.gaiji)) {
			gdcs.textdisp |= GDCSCRN_ALLDRAW;
		}
		cgwindow.writable &= ~0x80;
		if (gdcs.textdisp & GDCSCRN_MAKE) {
			if (!(gdc.mode1 & 0x4)) {
				maketext(gdcs.textdisp & GDCSCRN_ALLDRAW);
			}
			else {
				maketext40(gdcs.textdisp & GDCSCRN_ALLDRAW);
			}
			gdcs.textdisp &= ~GDCSCRN_MAKE;
			screenupdate |= 1;
		}
	}
	if (screenupdate) {
		screenupdate = scrndraw_draw((BYTE)(screenupdate & 2));
		drawcount++;
	}
}


// 表示期間の開始
void screendisp(NEVENTITEM item) {

	PICITEM		pi;

	gdc_work(GDCWORK_SLAVE);
	gdc.vsync = 0;
	screendispflag = 0;
	if (!np2cfg.DISPSYNC) {
		drawscreen();
	}
	pi = &pic.pi[0];
	if (pi->irr & PIC_CRTV) {
		pi->irr &= ~PIC_CRTV;
		gdc.vsyncint = 1;
	}
	(void)item;
}

// VSYNC期間の開始
void screenvsync(NEVENTITEM item) {

	MEMWAIT_TRAM = np2cfg.wait[1];
	MEMWAIT_VRAM = np2cfg.wait[3];
	MEMWAIT_GRCG = np2cfg.wait[5];
	gdc_work(GDCWORK_MASTER);
	gdc.vsync = 0x20;
	if (gdc.vsyncint) {
		gdc.vsyncint = 0;
		pic_setirq(2);
	}
	nevent_set(NEVENT_FLAMES, gdc.vsyncclock, screendisp, NEVENT_RELATIVE);

	// drawscreenで pccore.vsyncclockが変更される可能性があります
	if (np2cfg.DISPSYNC) {
		drawscreen();
	}
	(void)item;
}

#if defined(SUPPORT_PC88VA)

static void drawscreenva(void) {

	int y;
	BOOL text200;
	BOOL grph200;
	UINT16 lines;

//	if (videova.grmode & 0x1000) {
//		// SYNCEN (水平同期信号出力)
		tsp_updateclock();
//	}

	if (!drawframe) {
		return;
	}

	lines = tsp.screenlines;
	if (videova_hsyncmode() != VIDEOVA_24_8KHZ) lines *= 2;
	if (lines > SURFACE_HEIGHT) lines = SURFACE_HEIGHT;

	if ((tsp.flag & TSP_F_LINESCHANGED) && (videova.grmode & 0x1000)) {
		// TSPの表示ライン数に変更あり、かつ、SYNCEN(水平同期信号出力)
		/* dispsync_renewalvertical() */
		scrnmng_setheight(0, lines);
		tsp.flag &= ~TSP_F_LINESCHANGED;
	}

	maketextva_begin(&text200);
	makesprva_begin();
	makegrphva_begin(&grph200);
	scrndrawva_compose_begin();

	if (videova_hsyncmode() != VIDEOVA_24_8KHZ) {
		// 15KHz
		switch(videova.grmode & 0x00c0) {
		case 0x00:	// ノンインターレースモード0
		case 0x40:	// ノンインターレースモード1
			for (y = 0; y < lines/*SURFACE_HEIGHT*/;) {
				// 偶数ライン
				maketextva_raster();
				makesprva_raster();
				makegrphva_raster();
				scrndrawva_compose_raster();
				y++;
				// 奇数ライン
				maketextva_blankraster();
				makesprva_blankraster();
				makegrphva_blankraster();
				scrndrawva_compose_raster();
				y++;
			}
			break;
		
		case 0x80:	// インターレースモード0
			for (y = 0; y < lines /*SURFACE_HEIGHT*/;) {
				// 偶数ライン
				maketextva_raster();
				makesprva_raster();
				makegrphva_raster();
				scrndrawva_compose_raster();
				y++;
				// 奇数ライン(直前のラインと同一内容)
				if (text200) {
					// 直前のラインと同一内容
				}
				else {
					maketextva_raster();
				}
				scrndrawva_compose_raster();
				y++;
			}
			break;
		case 0xc0:	// インターレースモード1
			for (y = 0; y < lines /*SURFACE_HEIGHT*/;) {
				// 偶数ライン
				maketextva_raster();
				makesprva_raster();
				makegrphva_raster();
				scrndrawva_compose_raster();
				y++;
				// 奇数ライン
				if (text200) {
					// 直前のラインと同一内容
				}
				else {
					maketextva_raster();
				}
					// スプライトは常に200ライン(直前のラインと同一内容)
				if (grph200) {
					// 直前のラインと同一内容
				}
				else {
					makegrphva_raster();
				}
				scrndrawva_compose_raster();
				y++;
			}
			break;
		}
	}
	else {
		// 24KHz
		switch(videova.grmode & 0x00c0) {
		case 0x00:	// ノンインターレースモード0
			for (y = 0; y < lines/*SURFACE_HEIGHT*/;) {
				// 偶数ライン
				maketextva_raster();
				makesprva_raster();
				makegrphva_raster();
				scrndrawva_compose_raster();
				y++;
				// 奇数ライン
				maketextva_raster();
				makesprva_raster();
				if (grph200) {
					makegrphva_blankraster();
				}
				else {
					makegrphva_raster();
				}
				scrndrawva_compose_raster();
				y++;
			}
			break;
		case 0x40:	// ノンインターレースモード1
			for (y = 0; y < lines/*SURFACE_HEIGHT*/;) {
				// 偶数ライン
				maketextva_raster();
				makesprva_raster();
				makegrphva_raster();
				scrndrawva_compose_raster();
				y++;
				// 奇数ライン
				maketextva_raster();
				makesprva_raster();
				if (grph200) {
					// 直前のラインと同一内容
				}
				else {
					makegrphva_raster();
				}
				scrndrawva_compose_raster();
				y++;
			}
			break;
		case 0x80:	// インターレースモード0
		case 0xc0:	// インターレースモード1
			// 禁止
			for (y = 0; y < lines /*SURFACE_HEIGHT*/;) {
				maketextva_blankraster();
				makesprva_blankraster();
				makegrphva_blankraster();
				scrndrawva_compose_raster();
				y++;
				scrndrawva_compose_raster();
				y++;
			}
			break;
		}
	}


	screenupdate |= 2;			// 今のところVA用描画ルーチンは全体描画しか実装していない

	if (screenupdate) {
		screenupdate = scrndrawva_draw((BYTE)(screenupdate & 2));
		drawcount++;
	}
}


static void screendispva_setnevent() {
	nevent_set(NEVENT_FLAMES, tsp.dispclock, screenvsyncva, NEVENT_RELATIVE);
	nevent_set(NEVENT_FLAMES2, tsp.sysp4vsyncextension, sysp4vsyncend, NEVENT_RELATIVE);
}

// 表示期間の開始
void screendispva(NEVENTITEM item) {
/*
	PICITEM		pi;
*/
//	gdc_work(GDCWORK_SLAVE);
	tsp.vsync = 0;
/*	sysp4vsyncstartに移動
	screendispflag = 0;
*/
	if (!np2cfg.DISPSYNC) {
		drawscreenva();
	}
/*
	pi = &pic.pi[0];
	if (pi->irr & PIC_CRTV) {
		pi->irr &= ~PIC_CRTV;
//		gdc.vsyncint = 1;
	}
*/
	screendispva_setnevent();
}

// VSYNC期間の開始
void screenvsyncva(NEVENTITEM item) {

//	MEMWAIT_TRAM = np2cfg.wait[1];
//	MEMWAIT_VRAM = np2cfg.wait[3];
//	MEMWAIT_GRCG = np2cfg.wait[5];
//	gdc_work(GDCWORK_MASTER);

//	tsp.vsync = 0x20;
	tsp.vsync = 0x40;

	videova.blinkcnt++;
	if (--tsp.blinkcnt == 0) {
		tsp.blinkcnt = tsp.blink;
		tsp.blinkcnt2++;
	}
#if 0
	if (/*gdc.vsyncint ||*/ pccore.model_va != PCMODEL_NOTVA) {
//		gdc.vsyncint = 0;
		pic_setirq(2);
	}
	nevent_set(NEVENT_FLAMES, tsp.vsyncclock, screendispva, NEVENT_RELATIVE);
#else
/*
	// 割り込みは6クロック遅れて発生させる。
	// (割り込みより前に IN AL,40hでVSYNC(bit5)=1が検出される場合がある
	// 「最終平気UPO」ハング対策)
	nevent_set(NEVENT_FLAMES, 6, screenvsyncva2, NEVENT_RELATIVE);
*/
	nevent_set(NEVENT_FLAMES, tsp.vsyncclock, screendispva, NEVENT_RELATIVE);
#endif
	// drawscreenで pccore.vsyncclockが変更される可能性があります
	if (np2cfg.DISPSYNC) {
		drawscreenva();
	}
}

#if 1
/*
void screenvsyncva2(NEVENTITEM item) {

	pic_setirq(2);
	nevent_set(NEVENT_FLAMES, tsp.vsyncclock - 6, screendispva, NEVENT_RELATIVE);

}
*/
#endif

void sysp4vsyncint(NEVENTITEM item) {
	pic_setirq(2);
}

void sysp4vsyncstart(NEVENTITEM item) {
	tsp.sysp4vsync = 0x20;

	// 割り込みは6クロック遅れて発生させる。
	// (割り込みより前に IN AL,40hでVSYNC(bit5)=1が検出される場合がある
	// 「最終平気UPO」ハング対策)
	nevent_set(NEVENT_FLAMES2, 6, sysp4vsyncint, NEVENT_RELATIVE);

	screendispflag = 0;
}

void sysp4vsyncend(NEVENTITEM item) {
	PICITEM		pi;

	tsp.sysp4vsync = 0;

	pi = &pic.pi[0];
	if (pi->irr & PIC_CRTV) {
		pi->irr &= ~PIC_CRTV;
	}

	nevent_set(NEVENT_FLAMES2, tsp.sysp4dispclock, sysp4vsyncstart, NEVENT_RELATIVE);

}

#endif


// ---------------------------------------------------------------------------

//@@@@@@
#if defined(VAEG_EXT)
#include	"breakpoint.h"
#endif

// ブレークポイント
typedef struct {
	BOOL	enabled;
	UINT16	seg;
	UINT16	off;
} BREAKADDR;


enum {
	BREAKADDR_MAX = 16,
};

		BOOL	stopexec = FALSE;					// 実行を停止する
		BOOL	singlestep = FALSE;					// シングルステップ実行
		BOOL	breakpointflag = TRUE;//FALSE;				// ブレークポイントを有効にする
		BREAKADDR	breakaddrx[BREAKADDR_MAX] = {	// ブレークポイント
			{FALSE, 0xe000, 0x9213},
			{FALSE, 0xe000, 0xb577},
		};

#if defined(VAEG_EXT)
		DEBUGCALLBACK	debugcallback;

void pccore_debugsetcallback(DEBUGCALLBACK *callback) {
	debugcallback = *callback;
}

void pccore_debugpause(BOOL pauseflag) {
	stopexec = pauseflag;
}

BOOL pccore_getdebugpause(void) {
	return stopexec;
}

void pccore_debugsinglestep(BOOL singlestepflag) {
	singlestep = singlestepflag;
}

void pccore_debugioin(BOOL word, UINT port) {
	if (breakpoint_check_ioin(port)) {
		stopexec = TRUE;
	}
	else if (word && breakpoint_check_ioin(port+1)) {
		stopexec = TRUE;
	}
}
#endif

void pccore_debugmem(UINT32 op, UINT32 addr, UINT16 data) {
	if (breakpoint_check_memwrite(addr)) {
		stopexec = TRUE;
	}
/*
	int	x = 0;

    if (addr == 0x2d830+0xb955) {
		x=op+addr+data;
	}
*/
}

void pccore_debugint(UINT32 no) {
	if (no != 0x82 && !(no == 0x83 && CPU_AX==0x2e00) && no != 0x96) {
		TRACEOUT(("cpu: int 0x%02x %04x:%04x rom0=%02x AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x BP=%04x SP=%04x DS=%04x ES=%04x SS=%04x",
		no, CPU_CS, CPU_IP,  memoryva.rom0_bank, CPU_AX, CPU_BX, CPU_CX, CPU_DX, CPU_SI, CPU_DI, CPU_BP, CPU_SP, CPU_DS, CPU_ES, CPU_SS));
	}
	/*
	if (no == 0x8b && CPU_AH == 0x17) {
		int i;
		TRACEOUT(("Music BIOS (17h): CH=%x, DL=%x, ES:BP=%.4x:%.4x",CPU_CH, CPU_DL, CPU_ES, CPU_BP));
		for (i = 0; i < CPU_DL; i++) {
			TRACEOUT(("  %.2x",mem[(CPU_ES << 4) + CPU_BP + i]));
		}
	}
	*/
}

//@@@@@@

#if defined(USEIPTRACE)					// Shinra
#define	IPTRACE			(1 << 12)
#endif

#if defined(TRACE) && IPTRACE
static	UINT	trpos = 0;
static	UINT32	treip[IPTRACE];
#if defined(SUPPORT_PC88VA)
static	BYTE	trerom0bank[IPTRACE];
static	WORD	tredata1[IPTRACE];

		int		treafter = 0;			// Shinra
#endif

void iptrace_out(void) {

	FILEH	fh;
	UINT	s;
	UINT32	eip;
	char	buf[32];

	s = trpos;
	if (s > IPTRACE) {
		s -= IPTRACE;
	}
	else {
		s = 0;
	}
	fh = file_create_c("his.txt");
	while(s < trpos) {
#if defined(SUPPORT_PC88VA)
		BYTE	bank = trerom0bank[s & (IPTRACE - 1)];
#endif
		eip = treip[s & (IPTRACE - 1)];
#if defined(SUPPORT_PC88VA)
//		SPRINTF(buf, "%.4x:%.4x (rom0=%.2x)\r\n", (eip >> 16), eip & 0xffff, bank);
		SPRINTF(buf, "%.4x:%.4x (rom0=%.2x) ES=%.4x\r\n", (eip >> 16), eip & 0xffff, bank, tredata1[s & (IPTRACE - 1)]);
#else
		SPRINTF(buf, "%.4x:%.4x\r\n", (eip >> 16), eip & 0xffff);
#endif
		s++;
		file_write(fh, buf, strlen(buf));
	}
	file_close(fh);
}
#endif


#if defined(TRACE)
static int resetcnt = 0;
static int execcnt = 0;
int piccnt = 0;
int tr = 0;
UINT	cflg;
#endif


void pccore_postevent(UINT32 event) {	// yet!

	(void)event;
}

void pccore_exec(BOOL draw) {

	drawframe = draw;
//	keystat_sync();
	soundmng_sync();
	mouseif_sync();
	pal_eventclear();

	gdc.vsync = 0;
	screendispflag = 1;
	MEMWAIT_TRAM = np2cfg.wait[0];
	MEMWAIT_VRAM = np2cfg.wait[2];
	MEMWAIT_GRCG = np2cfg.wait[4];
#if defined(SUPPORT_PC88VA)
/*	screendispvaに移動
	tsp.vsync = 0;
*/
	if (pccore.model_va == PCMODEL_NOTVA) {
		nevent_set(NEVENT_FLAMES, gdc.dispclock, screenvsync, NEVENT_RELATIVE);
	}
/*	screendispva_setneventに移動
	else {
		nevent_set(NEVENT_FLAMES, tsp.dispclock, screenvsyncva, NEVENT_RELATIVE);
		nevent_set(NEVENT_FLAMES2, tsp.sysp4vsyncextension, sysp4vsyncend, NEVENT_RELATIVE);
	}
*/
	else {
		if (!nevent_iswork(NEVENT_FLAMES)) {
			screendispva_setnevent();
		}
	}
#else
	nevent_set(NEVENT_FLAMES, gdc.dispclock, screenvsync, NEVENT_RELATIVE);
#endif

//	nevent_get1stevent();

	while(screendispflag) {
#if defined(TRACE)
		resetcnt++;
#endif
		pic_irq();
		if (CPU_RESETREQ) {
			CPU_RESETREQ = 0;
			CPU_SHUT();
		}

#define SINGLESTEPONLY
#if !defined(SINGLESTEPONLY)
		if (CPU_REMCLOCK > 0) {
			if (!(CPU_TYPE & CPUTYPE_V30)) {
				CPU_EXEC();
			}
			else {
				CPU_EXECV30();
			}
#if defined(SUPPORT_PC88VA)
			if (pccore.model_va != PCMODEL_NOTVA) {
				subsystemmx_exec();
				sgp_step();
			}
#endif
		}

#else	// SINGLESTEPONLY

		while(CPU_REMCLOCK > 0) {
#if defined(TRACE) && IPTRACE
			treip[trpos & (IPTRACE - 1)] = (CPU_CS << 16) + CPU_IP;
#if defined(SUPPORT_PC88VA)
			trerom0bank[trpos & (IPTRACE - 1)] = memoryva.rom0_bank;
			tredata1[trpos & (IPTRACE - 1)] =CPU_ES;
#endif
			trpos++;
#endif
//@@@@@@
			if (!stopexec) {
				if (singlestep) {
					stopexec = TRUE;
				}
/*
				else if (breakpointflag) {
					int	i;
					BREAKADDR *ba;

					ba = breakaddrx;
					for (i = 0; i < BREAKADDR_MAX; i++, ba++) {
						if (ba->enabled) {
							if (ba->seg == CPU_CS && ba->off == CPU_IP) {
								stopexec = TRUE;
							}
						}
					}
				}
*/
				else if (breakpoint_check_step()) {
					stopexec = TRUE;
				}
			}
			
			if (stopexec) {
				debugcallback.onpause();
				while (stopexec) {
					debugcallback.wait();
				}
			}
//@@@@@@

#if defined(TRACE) && defined(IPTRACE)	// Shinra
			if (treafter) {
				if (treafter < 0) {
					iptrace_out();
					treafter = 0;
				}
				else {
					if (--treafter == 0) {
						iptrace_out();
					}
				}
			}
#endif

			//TRACEOUT(("%.4x:%.4x", CPU_CS, CPU_IP));
			if (!(CPU_TYPE & CPUTYPE_V30)) {		// added by Shinra
				i286x_step();
//				i286c_step();
			}
			else {
				v30x_step();						// added by Shinra
			}
#if defined(SUPPORT_PC88VA)
			if (pccore.model_va != PCMODEL_NOTVA) {
				subsystemmx_exec();
				sgp_step();
			}
#endif
		}
#endif	// SINGLESTEPONLY

		nevent_progress();
	}
	artic_callback();
	mpu98ii_callback();
	diskdrv_callback();
	calendar_inc();
	S98_sync();
	sound_sync();													// happy!

	if (hardwarereset) {
		hardwarereset = FALSE;
		pccore_cfgupdate();
		pccore_reset();
	}

#if defined(TRACE)
	execcnt++;
	if (execcnt >= 60) {
//		TRACEOUT(("resetcnt = %d / pic %d", resetcnt, piccnt));
		execcnt = 0;
		resetcnt = 0;
		piccnt = 0;
	}
#endif
}

