/*
 * SYSPORTVA.C: PC-88VA System port/Calendar/Printer/Dip switch
 */

typedef struct {
	BYTE	a;				// bit5: SPEED, bit4-1: sw5-2
	BYTE	c;
	BYTE	port010;
	BYTE	port032;
	BYTE	port040;
	WORD	modesw;			// bit0: X88V1, bit1: X88V2
} _SYSPORTVA, *SYSPORTVA;


#ifdef __cplusplus
extern "C" {
#endif

void systemportva_reset(void);
void systemportva_bind(void);

#ifdef __cplusplus
}
#endif

