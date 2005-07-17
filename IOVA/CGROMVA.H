/*
 * CGROMVA.H: PC-88VA Character generator
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

BYTE *cgromva_font(UINT16 hccode);
int cgromva_width(UINT16 hccode);

void cgromva_reset(void);
void cgromva_bind(void);

#ifdef __cplusplus
}
#endif

