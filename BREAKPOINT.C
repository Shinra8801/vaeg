#include	"compiler.h"
#include	"cpucore.h"

#include	"breakpoint.h"

#if defined(VAEG_EXT)

enum {
	BREAKPOINT_MAX = 10,
};


		BOOL		breakpoint_check_enabled = FALSE;
		BREAKPOINT	breakpoint[BREAKPOINT_MAX];


BREAKPOINT *breakpoint_get(int index) {
	if (index >= BREAKPOINT_MAX) return NULL;
	return &breakpoint[index];
}

BREAKCOND *breakpoint_newcond(void) {
	BREAKCOND *cond;

	cond = malloc(sizeof(BREAKCOND));
	ZeroMemory(cond, sizeof(BREAKCOND));
	return cond;
}

static void breakpoint_clearcond(BREAKCOND *cond) {
	if (cond == NULL) return;
	switch (cond->type) {
	}
	free(cond);
}

void breakpoint_clear(int index) {
	BREAKPOINT	*bp;

	if (index >= BREAKPOINT_MAX) return;
	bp = &breakpoint[index];
	breakpoint_clearcond(bp->cond);
	bp->enabled = FALSE;
	bp->type = BREAKPOINT_INVALID;
}

UINT32 breakpoint_evalval(BREAKVAL *val) {
	UINT32 v;

	v = 0;

	switch(val->type) {
	case BREAKVAL_UINT32:
		v = val->p.uint32;
		break;
	case BREAKVAL_REG:
		if (val->p.reg <= BREAKREG_BYTEREG_MAX) {
#if defined(BYTESEX_LITTLE)
			v = ((BYTE *)&i286core.s.r)[val->p.reg - BREAKREG_BYTEREG_BASE];
#else
			v = ((BYTE *)&i286core.s.r)[(val->p.reg - BREAKREG_BYTEREG_BASE) ^ 1];
#endif
		}
		else if (val->p.reg <= BREAKREG_WORDREG_MAX) {
			v = ((WORD *)&i286core.s.r)[val->p.reg - BREAKREG_WORDREG_BASE];
		}
		else switch(val->p.reg) {
		case BREAKREG_CSIP:
			v = CPU_CS * 16 + CPU_IP;
			break;
		}
		break;
	}
	return v;
}

BOOL breakpoint_evalcond(BREAKCOND *cond) {
	UINT32 v1, v2;

	if (cond == NULL) return TRUE;
	switch (cond->type) {
	case BREAKCOND_EQUAL:
		v1 = breakpoint_evalval(&cond->p.priexp.val1);
		v2 = breakpoint_evalval(&cond->p.priexp.val2);
		return v1 == v2;
	}
	return FALSE;
}

#define CPUPREFETCH(index) (i286core.s.prefetchque[index])

static BOOL breakpoint_check(int type, UINT32 param1) {
	int	i;
	BREAKPOINT	*bp;
	BREAKCOND	*cond;

	if (!breakpoint_check_enabled) return FALSE;

	bp = breakpoint;
	for (i = 0; i < BREAKPOINT_MAX; i++, bp++) {
		if (bp->enabled && (bp->type & type)) {
			cond = bp->cond;
			switch (bp->type) {
			case BREAKPOINT_STEP:
				if (!breakpoint_evalcond(cond)) break;
				return TRUE;
			case BREAKPOINT_IOIN:
				if (bp->p.port != (UINT16)param1) break;
				if (!breakpoint_evalcond(cond)) break;
				return TRUE;
			case BREAKPOINT_MEMWRITE:
				if (bp->p.addr != param1) break;
				if (!breakpoint_evalcond(cond)) break;
				return TRUE;
			case BREAKPOINT_INTERRUPT:	// int data Ç‹ÇΩÇÕ int3
				if ((CPUPREFETCH(0) == 0xcd && CPUPREFETCH(1) == bp->p.intnum) ||
					(CPUPREFETCH(0) == 0xcc && bp->p.intnum == 3)) {
					if (breakpoint_evalcond(cond)) return TRUE;
				}
				break;
			}
		}
	}

	return FALSE;
}

void breakpoint_check_setenabled(BOOL enabled) {
	breakpoint_check_enabled = enabled;
}

BOOL breakpoint_check_step() {
	return breakpoint_check(BREAKPOINT_STEP | BREAKPOINT_INTERRUPT, 0);
}

BOOL breakpoint_check_ioin(UINT port) {
	return breakpoint_check(BREAKPOINT_IOIN, (UINT32)port);
}

BOOL breakpoint_check_memwrite(UINT32 addr) {
	return breakpoint_check(BREAKPOINT_MEMWRITE, addr);
}


/*
csip=4567:89ab
int 21 if csip=4567:89ab and ax=1234 and bl=ab12
in 506
out 180 if al=12

breakpoint :=
    action ['if' condition]?
  | condition

action :=
    'int' hex
  | 'in' hex
  | 'out' hex
  | 'w' segoff			// memory write

condition := log_expression

log_expression := 
     pri_expression
   | '(' log_expression ')'
   | 'not' log_expression
   | log_expression logop log_expreession

logop := 'and' | 'or'

pri_expression := 
    val op val

op := '=' | '<' | '>'

val :=
    hex
  | segoff
  | reg

segoff := hex ':' hex

reg := 'ax' | 'bx' | ... | 'cs' | 'ip' | 'csip'

*/

static BOOL is_blank(char c) {
	return strchr(" \r\n\t", c) != NULL;
}

static BOOL is_alnum(char c) {
	return strchr("0123456789_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", c) != NULL;
}

static void skipblank(const char **str) {
	while (**str != '\0' && is_blank(**str)) (*str)++;
}

static int parse_charseq(const char *word, const char **str) {
	int len;

	skipblank(str);
	len = strlen(word);
	if (strncmp(*str, word, len)==0) {
		*str += len;
		return 1;
	}
	return 0;
}

static int parse_keyword(const char *word, const char **str) {
	int len;

	skipblank(str);
	len = strlen(word);
	if (strncmp(*str, word, len)==0) {
		if (!is_alnum((*str)[len])) {
			*str += len;
			return 1;
		}
	}
	return 0;
}

static int parse_op(const char **str, int *op) {
	if (parse_charseq("=", str)) {
		*op = BREAKCOND_EQUAL;
		return 1;
	}
	return 0;
}

static int parse_reg(const char **str, BREAKVAL *val) {
	static const char *sym[]={
		"al","ah","cl","ch",
		"dl","dh","bl","bh",
		"ax","cx","dx","bx",
		"sp","bp","si","di",
		"es","cs","ss","ds",
		"flag","ip",
		"csip"};
	static const int reg[]={
		BREAKREG_AL, BREAKREG_AH, BREAKREG_CL, BREAKREG_CH,
		BREAKREG_DL, BREAKREG_DH, BREAKREG_BL, BREAKREG_BH,
		BREAKREG_AX, BREAKREG_CX, BREAKREG_DX, BREAKREG_BX,
		BREAKREG_SP, BREAKREG_BP, BREAKREG_SI, BREAKREG_DI,
		BREAKREG_ES, BREAKREG_CS, BREAKREG_SS, BREAKREG_DS,
		BREAKREG_FLAG, BREAKREG_IP,
		BREAKREG_CSIP};
	int i;

	for (i = 0; i < sizeof(reg)/sizeof(int); i++) {
		if (parse_keyword(sym[i], str)) {
			val->type = BREAKVAL_REG;
			val->p.reg = reg[i];
			return 1;
		}
	}
	return 0;
	
}

static int parse_hex(const char **str, UINT32 *val) {
	static const char *digits = "0123456789ABCDEF";
	const char *pt;
	const char *d;
	UINT32 v;

	char c;

	pt = *str;
	v = 0;
	for (;;) {
		c = *pt;
		if (is_blank(c) || !is_alnum(c)) {
			break;
		}
		pt++;
		if (c >= 'a') c -= 'a'-'A';
		d = strchr(digits, c);
		if (d == NULL) {
			return 0;
		}
		v *= 16;
		v += d - digits;
	}
	if (*str == pt) {
		// 1ï∂éöÇ‡è¡îÔÇµÇƒÇ¢Ç»Ç¢
		return 0;
	}
	*str = pt;
	*val = v;
	return 1;
}

static int parse_hexval(const char **str, BREAKVAL *val) {
	UINT32 v;

	if (parse_hex(str, &v)) {
		val->type = BREAKVAL_UINT32;
		val->p.uint32 = v;
		return 1;
	}
	return 0;
/*
	static const char *digits = "0123456789ABCDEF";
	const char *pt;
	const char *d;
	UINT32 v;

	char c;


	pt = *str;
	v = 0;
	for (;;) {
		c = *pt;
		if (is_blank(c) || !is_alnum(c)) {
			break;
		}
		pt++;
		if (c >= 'a') c -= 'a'-'A';
		d = strchr(digits, c);
		if (d == NULL) {
			return 0;
		}
		v *= 16;
		v += d - digits;
	}
	*str = pt;
	val->type = BREAKVAL_UINT32;
	val->p.uint32 = v;
	return 1;
*/
}


static int parse_segoff(const char **str, BREAKVAL *val) {
	const char *pt;
	BREAKVAL buf;

	pt = *str;
	if (parse_hexval(&pt, &buf)) {
		if (parse_charseq(":", &pt)) {
			if (parse_hexval(&pt, val)) {
				*str = pt;
				val->type = BREAKVAL_UINT32;
				val->p.uint32 += buf.p.uint32 * 16;
				return 1;
			}
		}
	}
	return 0;
}

static int parse_segoffaddr(const char **str, UINT32 *val) {
	const char *pt;
	UINT32 seg;

	pt = *str;
	if (parse_hex(&pt, &seg)) {
		if (parse_charseq(":", &pt)) {
			if (parse_hex(&pt, val)) {
				*str = pt;
				*val += seg * 16;
				return 1;
			}
		}
	}
	return 0;
}

static int parse_val(const char **str, BREAKVAL *val) {
	if (parse_reg(str, val)) {
		return 1;
	}
	else if (parse_segoff(str, val)) {
		return 1;
	}
	else if (parse_hexval(str, val)) {
		return 1;
	}
	return 0;
}

static int parse_pri_expression(const char **str, BREAKCOND **cond) {
	int op;
	BREAKVAL val1, val2;

	if (parse_val(str, &val1)) {
		if (parse_op(str, &op)) {
			if (parse_val(str, &val2)) {
				*cond = breakpoint_newcond();
				(*cond)->type = op;
				(*cond)->p.priexp.val1=val1;
				(*cond)->p.priexp.val2=val2;
				return 1;
			}
		}
	}
	return 0;
}

static int parse_log_expression(const char **str, BREAKCOND **cond) {
	if (parse_pri_expression(str, cond)) {
		return 1;
	}
	return 0;
}

static int parse_condition(const char **str, BREAKCOND **cond) {
	return parse_log_expression(str, cond);
}

static int parse_action(const char **str, BREAKPOINT *bp) {
	UINT32 v;

	if (parse_keyword("int", str)) {
		bp->type = BREAKPOINT_INTERRUPT;
		skipblank(str);
		if (parse_hex(str, &v)) {
			bp->p.intnum = v;
		}
		else {
			// TODO
			bp->p.intnum = 0;
		}
		return 1;
	}
	else if (parse_keyword("in", str)) {
		bp->type = BREAKPOINT_IOIN;
		skipblank(str);
		if (parse_hex(str, &v)) {
			bp->p.port = v;
		}
		else {
			// TODO
			bp->p.port = 0;
		}
		return 1;
	}
	else if (parse_keyword("w", str)) {
		bp->type = BREAKPOINT_MEMWRITE;
		skipblank(str);
		if (parse_segoffaddr(str, &v)) {
			bp->p.addr = v;
		}
		else {
			// TODO
			bp->p.addr = 0;
		}
		return 1;
	}
	return 0;
}

static int parse_endofstr(const char **str) {
	skipblank(str);
	if (**str == '\0') {
		return 1;
	}
	return 0;
}

int breakpoint_parse(const char *str, BREAKPOINT *bp) {
	if (parse_action(&str, bp)) {
		if (parse_keyword("if", &str)) {
			BREAKCOND *cond;
			if (parse_condition(&str, &cond)) {
				if (parse_endofstr(&str)) {
					// ê≥èÌ
					bp->cond = cond;
					// bp->type ÇÕ parse_actionÇ≈ê›íË
					return 1;
				}
			}
		}
		else {
			bp->cond = NULL;
			return 1;
		}
	}
	else {
		BREAKCOND *cond;
		if (parse_condition(&str, &cond)) {
			if (parse_endofstr(&str)) {
				// ê≥èÌ
				bp->cond = cond;
				bp->type = BREAKPOINT_STEP;
				return 1;
			}
		}
	}
	// ÉGÉâÅ[
	return 0;
}

#endif
