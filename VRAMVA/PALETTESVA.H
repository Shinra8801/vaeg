#if defined(SUPPORT_PC88VA)

enum {
	// np2_palva??[] のインデックス番号
	//NP2PALVA_SET1		= 0,	// パレットセット1
	//NP2PALVA_SET2		= 16,	// パレットセット2
	NP2PALVA_BACKDROP	= 32,	// バックドロップカラー
	NP2PALVA_TOTAL		= 33	// パレット数合計
};


#ifdef __cplusplus
extern "C" {
#endif

extern	RGB32		np2_palva32[];
#if defined(SUPPORT_16BPP)
extern	RGB16		np2_palva16[];
#endif

void palva_maketable(void);
void palva_make(RGB32 *pal);

#ifdef __cplusplus
}
#endif


#endif