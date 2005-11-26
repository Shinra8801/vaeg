/*
 * GACTRLVA.H: PC-88VA GVRAM access control
 *
 */


#ifdef __cplusplus
extern "C" {
#endif

// Single Plane
typedef struct {
	UINT8	writemode;		// 0580h ライトモードレジスタ
	UINT16	pattern[2];		// 0590h, 0592h パターンレジスタ
	UINT8	rop[2];			// 05a0h, 05a2h ROPコードレジスタ
} _GACTRLVA_SINGLE, *GACTRLVA_SINGLE;

typedef struct {
	UINT8	gmsp;				// 描画モード 0x10..シングルプレーン 0x00..マルチプレーン
	_GACTRLVA_SINGLE	s;		// シングルプレーン
} _GACTRLVA, *GACTRLVA;

extern	_GACTRLVA	gactrlva;


void gactrlva_reset(void);
void gactrlva_bind(void);


#ifdef __cplusplus
}
#endif
