#ifdef __cplusplus
extern "C" {
#endif

extern	WORD	vabitmap[];
extern	BYTE	colorlevel5[];
extern	BYTE	colorlevel6[];

#if 0
extern	BYTE	np2_tbitmap[SURFACE_SIZE];
#endif

void scrndrawva_compose_begin(void);
void scrndrawva_compose_raster(void);


void scrndrawva_draw_sub(const SCRNSURF	*surf);

#ifdef __cplusplus
}
#endif
