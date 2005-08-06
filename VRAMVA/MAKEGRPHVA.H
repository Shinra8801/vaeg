/*
 * MAKEGRPHVA.H: PC-88VA Graphics
 */

#ifdef __cplusplus
extern "C" {
#endif

extern	WORD	grph0_raster[];
extern	WORD	grph1_raster[];

void makegrphva_initialize(void);

void makegrphva_begin(void);
void makegrphva_raster(void);


#ifdef __cplusplus
}
#endif
