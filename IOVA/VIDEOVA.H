/*
 * VIDEOVA.H: PC-88VA Video Control
 */

enum {
	VIDEOVA_PALETTES = 32, 
};

typedef struct {
	WORD	grmode;			// 表示画面制御レジスタ
	WORD	palette[VIDEOVA_PALETTES];
} _VIDEOVA, *VIDEOVA;


#ifdef __cplusplus
extern "C" {
#endif

extern	_VIDEOVA videova;

void videova_reset(void);
void videova_bind(void);

#ifdef __cplusplus
}
#endif

