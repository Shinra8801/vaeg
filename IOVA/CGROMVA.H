/*
 * CGROMVA.H: PC-88VA Character generator
 *
 */

typedef	struct {
	WORD	cgaddr;				// 14Ch ハードウェア文字コード
	BYTE	cgrow;				// 14Fh ラスタ番号/フォント左右
} _CGROMVA;

#ifdef __cplusplus
extern "C" {
#endif

extern	_CGROMVA	cgromva;

BYTE *cgromva_font(UINT16 hccode);
int cgromva_width(UINT16 hccode);

void cgromva_reset(void);
void cgromva_bind(void);

#ifdef __cplusplus
}
#endif

