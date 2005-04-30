#ifdef __cplusplus
extern "C" {
#endif

extern	WORD	vabitmap[];
//extern	BYTE	colorlevel5[];
//extern	BYTE	colorlevel6[];
#if defined(SUPPORT_24BPP) || defined(SUPPORT_32BPP)
extern	RGB32	drawcolor32[];
#endif
#if defined(SUPPORT_16BPP)
extern	RGB16	drawcolor16[];
#endif


RGB32 scrndrawva_drawcolor32(WORD colorva16);

BYTE scrndrawva_draw(BYTE redraw);
BYTE scrndrawva_redraw(void);

void scrndrawva_compose_begin(void);
void scrndrawva_compose_raster(void);


//void scrndrawva_draw_sub(const SCRNSURF	*surf);

#ifdef __cplusplus
}
#endif
