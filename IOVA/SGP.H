/*
 * SGP.H: PC-88VA Super Graphic Processor
 *
 */

typedef struct {
	UINT16	dmy;
} _SGP, *SGP;


#ifdef __cplusplus
extern "C" {
#endif


void sgp_reset(void);
void sgp_bind(void);


extern	_SGP	sgp;

#ifdef __cplusplus
}
#endif
