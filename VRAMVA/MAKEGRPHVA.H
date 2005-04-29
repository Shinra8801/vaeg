/*
 * MAKEGRPHVA.H: PC-88VA Graphics
 */

#ifdef __cplusplus
extern "C" {
#endif

extern	BYTE	grph0p_raster[];
extern	BYTE	grph1p_raster[];

void makegrphva_initialize(void);

void makegrphva_begin(void);
void makegrphva_raster(void);


#ifdef __cplusplus
}
#endif
