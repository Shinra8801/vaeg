/*
 * MAKEGRPHVA.H: PC-88VA Graphics
 */

#ifdef __cplusplus
extern "C" {
#endif

extern	WORD	grph0_raster[];
extern	WORD	grph1_raster[];

extern	BOOL	grph0_noraster;
extern	BOOL	grph1_noraster;

void makegrphva_initialize(void);

void makegrphva_begin(BOOL *scrn200);
void makegrphva_raster(void);
void makegrphva_blankraster(void);

#ifdef __cplusplus
}
#endif
