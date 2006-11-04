/*
 *	BOARDSB2.H: PC-88VA Sound board 2
 */

typedef	struct {
	UINT32	addrwritewait;		// レジスタアドレスライト時のウェイト時間(クロック)
	UINT32	datawritewait;		// データライト時のウェイト時間(クロック)

	UINT32	lastaccess;			// 最後にwriteした時刻
	UINT32	wait;				// lastaccessからwait時間の間に発生したアクセスは
								// ウェイトする
								// 0の場合ウェイトしない

	BOOL	waitenabled;		// ウェイト有効
} _BOARDSB2;

#ifdef __cplusplus
extern "C" {
#endif

extern	_BOARDSB2	boardsb2;

void boardsb2_reset(void);
void boardsb2_bind(void);

#ifdef __cplusplus
}
#endif

