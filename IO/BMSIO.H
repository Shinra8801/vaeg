/*
 * BMSIO.H: I-O Bank Memory
 */

typedef struct {
	BYTE	*bmsmem;
	BYTE	*addr;
	UINT8	nomem;
	UINT8	bank;
	UINT8	numbanks;
} _BMSIO, *BMSIO;


#ifdef __cplusplus
extern "C" {
#endif

void bmsio_reset(void);
void bmsio_bind(void);

//extern	UINT32	bmsio_addr;
//extern	UINT8	bmsio_nomem;

#ifdef __cplusplus
}
#endif

