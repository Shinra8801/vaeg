/*
 * TSP.H: PC-88VA Text Sprite Processor
 *
 */

typedef struct {
	BOOL	dspon;				// TSPの表示ON
	BOOL	spron;				// スプライトの表示ON

	UINT16	texttable;			// テキスト画面制御テーブル(TVRAM先頭からのオフセット)
	UINT16	attroffset;			// 文字コード領域とアトリビュート領域の差
	UINT8	lineheight;			// 1行のラスタ数
	UINT8	hlinepos;			// ホリゾンタルラインを表示するラスタ
	UINT8	blink;				// ブリンク文字とカーソルの点滅速度

	UINT8	curn;				// カーソルとして使用するスプライトの番号
	BOOL	be;					// カーソルブリンク

	UINT16	sprtable;			// スプライト制御テーブル(TVRAM先頭からのオフセット)
	BOOL	mg;					// 縦2倍拡大
	UINT8	hspn;				// 1ラスタ中に同時に表示できるスプライトの最大数
	BOOL	gr;					// グルーピングモード

	UINT8	blinkcnt;			// ブリンク用カウンタ。画面表示1サイクルごとにデクリメント
	UINT8	blinkcnt2;			// ブリンク用カウンタ。blinkcnt==0になるたびにインクリメント

	BYTE	cmd;				// コマンド
	BYTE	recvdatacnt;		// パラメータ残り受信バイト数
	BYTE	senddatacnt;		// パラメータ残り送信バイト数
	BYTE	*datap;				// 次に送信する/受信するデータ
	BYTE	parambuf[16];		// 送受信バッファ
} _TSP, *TSP;


#ifdef __cplusplus
extern "C" {
#endif


void tsp_reset(void);
void tsp_bind(void);

extern	_TSP	tsp;

#ifdef __cplusplus
}
#endif
