/*
 * SYSPORTVA.C: PC-88VA System port
 */

typedef struct {
	UINT8	c;
	UINT8	port010;
	UINT8	port040;
} _SYSPORTVA, *SYSPORTVA;


#ifdef __cplusplus
extern "C" {
#endif

void systemportva_reset(void);
void systemportva_bind(void);

#ifdef __cplusplus
}
#endif

