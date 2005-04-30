/*
 * MAKEGRPHVA.C: PC-88VA Graphics
 */

#include	"compiler.h"
#include	"scrndraw.h"
#include	"gvramva.h"
#include	"videova.h"
#include	"makegrphva.h"

enum {
	GRPHVA_SCREENS	= 2,
};
typedef struct {
	UINT16	y;						// ���ݏ������̃��X�^(�O���t�B�b�N��ʂ̍��W�n��)
	BOOL	r200lines;				// �𑜓x200/204���C���Ȃ�TRUE
	BOOL	r320dots;				// �𑜓x320�h�b�g�Ȃ�TRUE
	BYTE	*rasterbuf;
	UINT32	addrmask;
	UINT32	addrofs;

	UINT32	lineaddr;				// ����\��GVRAM�A�h���X
	UINT32	wrappedaddr;			// ���b�v�A���E���h���GVRAM�A�h���X
	UINT16	wrapcount;
	FRAMEBUFFER	framebuffer;		// ���ݕ\�����Ă���t���[���o�b�t�@
	int		nextframebuffer;		// ���Ɏg�p����t���[���o�b�t�@�̔ԍ��B�����ꍇ��-1
} _SCREEN, *SCREEN;

typedef struct {
	UINT16		screeny;			// ���ݏ������̃��X�^(��ʋ��ʂ̍��W�n��)
	_SCREEN		screen[GRPHVA_SCREENS];
} _GRPHVAWORK;

static	_GRPHVAWORK	work;
		BYTE grph0p_raster[SURFACE_WIDTH + 32];
		BYTE grph1p_raster[SURFACE_WIDTH + 32];
											// 1���X�^���̃s�N�Z���f�[�^
											// �e�s�N�Z���̓p���b�g�ԍ�(0�`15)
											// ��ʉ��� + 
											// �ő�4�o�C�g(ex.1bit/pixel�Ȃ�32dot)��
											// �g�p����

#define addr18(scrn, x) ( (x) & ((scrn)->addrmask) | ((scrn)->addrofs) )

static void selectframe(SCREEN screen, int no) {
	FRAMEBUFFER	f;

	if (no < 0 || no >= VIDEOVA_FRAMEBUFFERS) {
		screen->framebuffer = NULL;
		return;
	}

	f = &videova.framebuffer[no];
	screen->framebuffer = f;
	screen->lineaddr = addr18(screen, f->dsa);
	screen->wrappedaddr = addr18(screen, f->dsa - f->ofx);
	if (no == 0) {
		screen->nextframebuffer = 2;
	}
	else if (no == 2) {
		screen->nextframebuffer = 3;
	}
	else {
		screen->nextframebuffer = -1;
	}
}

static void drawraster(SCREEN screen) {
	UINT16		xp;
	UINT16		wrapcount;
	UINT32		addr;
	BYTE		*b;

	WORD		d, d2;


	if (!screen->r200lines || (work.screeny & 1) == 0) {
		if (screen->framebuffer != NULL) {
			if (screen->framebuffer->dsp + screen->framebuffer->dsh == screen->y) {
				screen->framebuffer = NULL;
			}
		}

		// �K�v�Ȃ�A���̃t���[���o�b�t�@�ɐ؂�ւ���
		if (screen->framebuffer == NULL && screen->nextframebuffer >= 0) {
			if (videova.framebuffer[screen->nextframebuffer].dsp < screen->y) {
				// �������̃t���[���o�b�t�@�͕\�����Ȃ�
				screen->nextframebuffer = -1;
			}
			else if (videova.framebuffer[screen->nextframebuffer].dsp == screen->y) {
				selectframe(screen, screen->nextframebuffer);
			}
		}

		b = screen->rasterbuf;
		if (screen->framebuffer == NULL) {
			// �����\�����Ȃ�
			for (xp = 0; xp < 640; xp++) {
				*b++ = 0;
			}
		}
		else {
			addr = screen->lineaddr;
			if (screen->framebuffer->ofx == 0xffff) {
				// screen 1 (���b�v�A���E���h�Ȃ�)
				wrapcount = 0;
			}
			else {
				wrapcount = screen->framebuffer->fbw - screen->framebuffer->ofx;
			}

			if (screen->r320dots) {

				d = LOADINTELWORD(grphmem + addr);
				d2 = LOADINTELWORD(grphmem + addr + 2);
				addr = addr18(screen, addr + 4);

				switch (screen->framebuffer->dot & 0x13) {
				case 0:
					b[0] = b[1] = (d >>  4) & 0x0f;
					b += 2;
				case 1:
					b[0] = b[1] = (d      ) & 0x0f;
					b += 2;
				case 2:
					b[0] = b[1] = (d >> 12) & 0x0f;
					b += 2;
				case 3:
					b[0] = b[1] = (d >>  8) & 0x0f;
					b += 2;
				case 0x10:
					b[0] = b[1] = (d2 >>  4) & 0x0f;
					b += 2;
				case 0x11:
					b[0] = b[1] = (d2      ) & 0x0f;
					b += 2;
				case 0x12:
					b[0] = b[1] = (d2 >> 12) & 0x0f;
					b += 2;
				case 0x13:
					b[0] = b[1] = (d2 >>  8) & 0x0f;
					b += 2;
				}
				for (xp = 0; xp < 320/8; xp++) {
					wrapcount -= 4;
					if (wrapcount == 0) {
						addr = screen->wrappedaddr;
					}

					d = LOADINTELWORD(grphmem + addr);
					d2 = LOADINTELWORD(grphmem + addr + 2);
					addr = addr18(screen, addr + 4);

					b[0] = b[1] = (d >>  4) & 0x0f;
					b += 2;
					b[0] = b[1] = (d      ) & 0x0f;
					b += 2;
					b[0] = b[1] = (d >> 12) & 0x0f;
					b += 2;
					b[0] = b[1] = (d >>  8) & 0x0f;
					b += 2;

					b[0] = b[1] = (d2 >>  4) & 0x0f;
					b += 2;
					b[0] = b[1] = (d2      ) & 0x0f;
					b += 2;
					b[0] = b[1] = (d2 >> 12) & 0x0f;
					b += 2;
					b[0] = b[1] = (d2 >>  8) & 0x0f;
					b += 2;
				}
			}
			screen->lineaddr = addr18(screen, screen->lineaddr + screen->framebuffer->fbw);
			screen->wrappedaddr = addr18(screen, screen->wrappedaddr + screen->framebuffer->fbw);
		}
		screen->y++;

	}
}
		
void makegrphva_initialize(void) {
}

void makegrphva_begin(void) {

	work.screeny = 0;
	work.screen[0].rasterbuf = grph0p_raster;
	work.screen[1].rasterbuf = grph1p_raster;
	work.screen[0].r320dots = videova.grres & 0x0010;
	work.screen[1].r320dots = videova.grres & 0x1000;
	work.screen[0].r200lines = videova.grmode & 0x0002;
	work.screen[1].r200lines = videova.grmode & 0x0002;
	work.screen[0].addrmask = 0x0001ffffL;
	work.screen[0].addrofs  = 0x00000000L;
	work.screen[1].addrmask = 0x0001ffffL;
	work.screen[1].addrofs  = 0x00020000L;
	work.screen[0].nextframebuffer = 0;
	work.screen[1].nextframebuffer = 1;
	work.screen[0].y = 0;
	work.screen[1].y = 0;
	selectframe(&work.screen[0], -1);
	selectframe(&work.screen[1], -1);
}

void makegrphva_raster(void) {

	drawraster(&work.screen[0]);
	drawraster(&work.screen[1]);
	
	work.screeny++;
}