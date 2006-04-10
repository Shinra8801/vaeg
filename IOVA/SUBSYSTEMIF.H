/*
 * SUBSYSTEMIF.H: PC-88VA FD Sub System Interface
 */


#ifdef __cplusplus
extern "C" {
#endif

void subsystemif_businporta(BYTE dat);
void subsystemif_businportc(BYTE dat);

void subsystemif_initialize(void);
void subsystemif_reset(void);
void subsystemif_bind(void);

#ifdef __cplusplus
}
#endif

