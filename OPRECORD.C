/*
 * OPRECORD.C: User Operation Recorder / Player
 */

#include	"compiler.h"
#include	"dosio.h"
#include	"cpucore.h"
#include	"nevent.h"
#include	"serial.h"

#include	"oprecord.h"

#if defined(SUPPORT_OPRECORD)

typedef struct {
	UINT32	past;
	UINT16	type;
} _HEADER, *HEADER;

typedef struct {
	_HEADER	h;
	UINT8	drv;
	UINT8	readonly;
	char	filename[MAX_PATH];
} _E_FDD, *E_FDD;

typedef struct {
	_HEADER	h;
	UINT8	id;			// �W���C�p�b�h�̔ԍ� 0 or 1
	UINT8	arrow;
	UINT8	button;
	UINT8	dmy;
} _E_JOYPAD, *E_JOYPAD;

typedef struct {
	_HEADER	h;
	UINT8	code;
	UINT8	dmy;
} _E_KEY, *E_KEY;

enum {
	TYPE_END		= 0,
	TYPE_FDD,
	TYPE_JOYPAD,
	TYPE_KEY,
	TYPE_INVALID,
	STATE_UNKNOWN,

	BUFFER_SIZE		= 1024,
};

		_OPRECORD oprecord = {0};

static	OPRECORD_SETFDD setfdd;
static	OPRECORD_RESTARTPLAY restartplay;

static	BYTE buffer[BUFFER_SIZE];
static	BYTE *bufferp;
static	UINT32 lasttime;
static	FILEH fh;
static	BOOL recording = FALSE;
static	BOOL playing = FALSE;

// �f�o�C�X�̌��݂̏��
static _E_JOYPAD s_joypad;

// ---- ����

static UINT32 now(void) {
	return CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
}

// ---- ���� API

void oprecord_set_setfdd(OPRECORD_SETFDD _setfdd) {
	setfdd = _setfdd;
}

void oprecord_set_restartplay(OPRECORD_RESTARTPLAY _restartplay) {
	restartplay = _restartplay;
}

// ---- �L�^

static void save(void) {
	if (bufferp == buffer) return;

	//buffer����bufferp�܂ł�ۑ�����B
	file_write(fh, buffer, bufferp - buffer);

	bufferp = buffer;
}

static HEADER new_event(UINT16 type, int size) {
	HEADER ret;
	if (buffer + BUFFER_SIZE - bufferp < size) {
		save();
	}

	ret = (HEADER)bufferp;
	bufferp += size;

	ret->type = type;
	ret->past = now() - lasttime;
	lasttime = now();

	return ret;
}

// ---- �L�^ API

void oprecord_record_fdd(UINT8 drv, const char *fname, UINT8 readonly) {
	E_FDD e;

	if (!recording) return;

	e = (E_FDD)new_event(TYPE_FDD, sizeof(_E_FDD));
	e->drv = drv;
	e->readonly = readonly;
	ZeroMemory(e->filename, sizeof(e->filename));
	if (fname) {
		file_cpyname(e->filename, fname, sizeof(e->filename));
	}
}

void oprecord_record_joypad(UINT8 id, UINT8 arrow, UINT8 button) {
	E_JOYPAD e;

	if (!recording) return;

	// ���݂̏�ԂƔ�r
	if (s_joypad.h.type != STATE_UNKNOWN) {
		if (s_joypad.arrow == arrow &&
			s_joypad.button == button) {

			return;
		}
	}

	// ��ԂɕύX������
	e = (E_JOYPAD)new_event(TYPE_JOYPAD, sizeof(_E_JOYPAD));
	e->id = id;
	e->arrow = arrow;
	e->button = button;

	s_joypad = *e;
}

void oprecord_record_key(UINT8 code) {
	E_KEY e;

	if (!recording) return;

	e = (E_KEY)new_event(TYPE_KEY, sizeof(_E_KEY));
	e->code = code;
}

int oprecord_start_record(const char *filename) {
	if (recording) return -1;
	if (playing) {
		oprecord_stop_play();
	}

	// �t�@�C�����J��
	fh = file_create(filename);
	if (fh == FILEH_INVALID) {
		// �쐬���s
		return -1;
	}

	// �ϐ�������
	bufferp = buffer;
	lasttime = now();

	s_joypad.h.type = STATE_UNKNOWN;

	recording = TRUE;
	return 0;
}

void oprecord_stop_record(void) {
	if (!recording) return;

	new_event(TYPE_END, sizeof(_HEADER));
	save();

	// �t�@�C�������
	file_close(fh);

	recording = FALSE;
}



// ---- �Đ�

static void load(void) {
	BYTE *dest;

	if (bufferp == buffer) return;

	// bufferp ����o�b�t�@�̍Ō�܂ł��A�o�b�t�@�̐擪�Ɉړ�
	dest = buffer;
	while (bufferp < buffer + BUFFER_SIZE) {
		*dest++ = *bufferp++;
	}

	// �����Ă���o�b�t�@�ɓǂݍ���
	file_read(fh, dest, BUFFER_SIZE - (dest - buffer));

	bufferp = buffer;
}

static HEADER peek_next_event(void) {
	if (buffer + BUFFER_SIZE - bufferp < sizeof(_HEADER)) {
		load();
	}
	return (HEADER)bufferp;
}

static HEADER next_event(int size) {
	HEADER ret;

	if (buffer + BUFFER_SIZE - bufferp < size) {
		load();
	}

	ret = (HEADER)bufferp;
	bufferp += size;

	lasttime += ret->past;

	return ret;
}

static void update_fdd(void) {
	E_FDD e;
	const char *filename = NULL;

	e = (E_FDD)next_event(sizeof(_E_FDD));
	if (e->filename[0] != '\0') filename = e->filename;
	if (setfdd) {
		setfdd(e->drv, filename, e->readonly);
	}
}

static void update_joypad(void) {
	E_JOYPAD e;

	e = (E_JOYPAD)next_event(sizeof(_E_JOYPAD));
	s_joypad.arrow = e->arrow;
	s_joypad.button = e->button;
}

static void update_key(void) {
	E_KEY e;

	e = (E_KEY)next_event(sizeof(_E_KEY));
	keyboard_send(e->code);
}

// ---- �Đ� API

void oprecord_play_update(void) {
	HEADER e;
	UINT32 n;

	if (!playing) return;

	n = now();
	e = peek_next_event();
	while (e->past <= n - lasttime) {
		switch(e->type) {
		case TYPE_END:
		default:
			oprecord_stop_play();
			if (oprecord.repeat) restartplay();
			goto exit;
			break;
		case TYPE_FDD:
			update_fdd();
			break;
		case TYPE_JOYPAD:
			update_joypad();
			break;
		case TYPE_KEY:
			update_key();
			break;
		}
		e = peek_next_event();
	}
exit:;
}

int oprecord_start_play(const char *filename) {
	if (playing) return -1;
	if (recording) {
		oprecord_stop_record();
	}

	// �t�@�C�����J��
	fh = file_open_rb(filename);
	if (fh == FILEH_INVALID) {
		return -1;
	}
	
	bufferp = buffer + BUFFER_SIZE;
	load();
	lasttime = now();

	playing = TRUE;
	return 0;
}

void oprecord_stop_play(void) {
	if (!playing) return;

	// �t�@�C�������
	file_close(fh);

	playing = FALSE;
}

int oprecord_play_joypad(UINT8 id, UINT8 *arrow, UINT8 *button) {
	if (!playing) return -1;

	oprecord_play_update();

	*arrow = s_joypad.arrow;
	*button = s_joypad.button;
	return 0;
}

#endif