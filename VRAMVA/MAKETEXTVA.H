
#ifdef __cplusplus
extern "C" {
#endif

extern	BYTE	textraster[];

void maketextva_initialize(void);
void maketextva(void);

void maketextva_begin(BOOL *scrn200);
void maketextva_raster(void);
void maketextva_blankraster(void);


#ifdef __cplusplus
}
#endif
