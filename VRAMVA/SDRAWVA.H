#if defined(SUPPORT_PC88VA)

typedef struct {
	BYTE	*dst;
	int		width;
	int		xbytes;
	int		y;
	int		xalign;
	int		yalign;
	BYTE	dirty[SURFACE_WIDTH];
} _SDRAWVA, *SDRAWVA;

typedef void (SCRNCALL * SDRAWFNVA)(SDRAWVA sdraw, int maxy);


#ifdef __cplusplus
extern "C" {
#endif

const SDRAWFNVA sdrawva_getproctbl(const SCRNSURF *surf);

#ifdef __cplusplus
}
#endif

#endif