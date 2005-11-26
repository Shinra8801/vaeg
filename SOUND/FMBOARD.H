
#if !defined(DISABLE_SOUND)

#include	"soundrom.h"
#include	"tms3631.h"
#include	"fmtimer.h"
#include	"opngen.h"
#include	"psggen.h"
#include	"rhythm.h"
#include	"adpcm.h"
#include	"pcm86.h"
#include	"cs4231.h"


typedef struct {
	BYTE	reg[0x400];
	BYTE	opnreg;
	BYTE	extreg;
	BYTE	opn2reg;
	BYTE	ext2reg;
	BYTE	adpcmmask;
	BYTE	channels;
	BYTE	extend;
	BYTE	padding;
	UINT16	base;
} OPN_T;

typedef struct {
	UINT16	port;
	BYTE	psg3reg;
	BYTE	rhythm;
} AMD98;

typedef struct {
	BYTE	porta;
	BYTE	portb;
	BYTE	portc;
	BYTE	mask;
	BYTE	key[8];
	int		sync;
	int		ch;
} MUSICGEN;

#if defined(SUPPORT_PC88VA)
typedef struct {
	BYTE	sintm;			// bit7  0..äÑÇËçûÇ›ãñâ¬  1..äÑÇËçûÇ›ã÷é~
} _FMBOARDVA;
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern	UINT32		usesound;
extern	OPN_T		opn;
extern	AMD98		amd98;
extern	MUSICGEN	musicgen;

extern	_TMS3631	tms3631;
extern	_FMTIMER	fmtimer;
extern	_OPNGEN		opngen;
extern	OPNCH		opnch[OPNCH_MAX];
extern	_PSGGEN		psg1;
extern	_PSGGEN		psg2;
extern	_PSGGEN		psg3;
extern	_RHYTHM		rhythm;
extern	_ADPCM		adpcm;
extern	_PCM86		pcm86;
extern	_CS4231		cs4231;

#if defined(SUPPORT_PC88VA)
extern	_FMBOARDVA	fmboardva;
#endif

REG8 fmboard_getjoy(PSGGEN psg);

void fmboard_extreg(void (*ext)(REG8 enable));
void fmboard_extenable(REG8 enable);

void fmboard_reset(UINT32 type);
void fmboard_bind(void);

void fmboard_fmrestore(REG8 chbase, UINT bank);
void fmboard_rhyrestore(RHYTHM rhy, UINT bank);

#if defined(SUPPORT_PC88VA)
void fmboard_setintmask(BYTE mask);
BYTE fmboard_getintmask(void);
#endif


#ifdef __cplusplus
}
#endif

#else

#define	fmboard_reset(t)
#define	fmboard_bind()

#endif

