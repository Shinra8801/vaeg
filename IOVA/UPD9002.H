/*
 * UPD9002.H: PC-88VA CPU port
 */

typedef struct {
	BYTE	tcks;			// タイマ、カウンタの入力クロック指定
							// bit4-2 タイマ2-0への供給クロック 0..内部 1..外部
							// bit1-0 分周比　00..2 01..4 10..8 11..16
	BYTE	dmy[15];
} _UPD9002, *UPD9002;


#ifdef __cplusplus
extern "C" {
#endif

extern	_UPD9002		upd9002;

void upd9002_reset(void);
void upd9002_bind(void);

#ifdef __cplusplus
}
#endif

