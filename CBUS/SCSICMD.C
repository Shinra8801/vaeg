#include	"compiler.h"

#if defined(SUPPORT_SCSI)

#include	"dosio.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"cbuscore.h"
#include	"scsiio.h"
#include	"scsiio.tbl"
#include	"scsicmd.h"
#include	"sxsi.h"

#if defined(_WIN32) && defined(TRACE)
extern void iptrace_out(void);
#define	SCSICMD_ERR		MessageBox(NULL, "SCSI error", "?", MB_OK);	\
						exit(1);
#else
#define	SCSICMD_ERR
#endif


static const BYTE hdd_inquiry[0x20] = {
			0x00,0x00,0x02,0x02,0x1c,0x00,0x00,0x18,
			'N', 'E', 'C', 0x20,0x20,0x20,0x20,0x20,
			'N', 'P', '2', '-', 'H', 'D', 'D', 0x20,
			0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};


static UINT scsicmd_datain(SXSIDEV sxsi, BYTE *cdb) {

	UINT	length;

	switch(cdb[0]) {
		case 0x12:				// Inquiry
			TRACEOUT(("Inquiry"));
			// Logical unit number = cdb[1] >> 5;
			// EVPD = cdb[1] & 1;
			// Page code = cdb[2];
			length = cdb[4];
			if (length) {
				CopyMemory(scsiio.data, hdd_inquiry, min(length, 0x20));
			}
			break;

		default:
			length = 0;
	}
	(void)sxsi;
	return(length);
}






// ----

REG8 scsicmd_negate(REG8 id) {

	scsiio.phase = 0;
	(void)id;
	return(0x85);			// disconnect
}

REG8 scsicmd_select(REG8 id) {

	SXSIDEV	sxsi;

	TRACEOUT(("scsicmd_select"));
	if (scsiio.reg[SCSICTR_TARGETLUN] & 7) {
		TRACEOUT(("LUN = %d", scsiio.reg[SCSICTR_TARGETLUN] & 7));
		return(0x42);
	}
	sxsi = sxsi_getptr((REG8)(0x20 + id));
	if ((sxsi) && (sxsi->type)) {
		scsiio.phase = SCSIPH_COMMAND;
		return(0x8a);			// Transfer Command要求
	}
	return(0x42);				// Timeout
}

REG8 scsicmd_transfer(REG8 id, BYTE *cdb) {

	SXSIDEV	sxsi;
	UINT	leng;

	if (scsiio.reg[SCSICTR_TARGETLUN] & 7) {
		return(0x42);
	}

	sxsi = sxsi_getptr((REG8)(0x20 + id));
	if ((sxsi == NULL) || (sxsi->type == 0)) {
		return(0x42);
	}

	TRACEOUT(("sel ope code = %.2x", cdb[0]));
	switch(cdb[0]) {
		case 0x00:				// Test Unit Ready
			return(0x16);		// Succeed

		case 0x12:				// Inquiry
			leng = scsicmd_datain(sxsi, cdb);
#if 0
			if (leng > scsiio.transfer) {
				return(0x2b);		// Abort
			}
			else if (leng < scsiio.transfer) {
				return(0x20);		// Pause
			}
#endif
			return(0x16);			// Succeed
	}

	SCSICMD_ERR
	return(0xff);
}

#if defined(VAEG_EXT)
static const BYTE mode_sense_header[0x04] = {
	0x00, 0x00, 0x00, 0x08
};

static REG8 scsicmd_transferinfo_mode_sense_cmd(SXSIDEV sxsi, REG8 id, BYTE *cdb) {
	int pagecode = cdb[2] & 0x3f;
	BYTE *data;

	TRACEOUT(("scsi: mode sense command: page=%.2x", pagecode));

	data = scsiio.data;

	// mode sense header
	CopyMemory(data, mode_sense_header, sizeof(mode_sense_header));
	data += sizeof(mode_sense_header);

	// mode sense block descriptor
	*data++ = 0x00;	// dencity code
	*data++ = (BYTE)((sxsi->totals & 0x00ff0000L) >> 16);		// block number
	*data++ = (BYTE)((sxsi->totals & 0x0000ff00L) >>  8);
	*data++ = (BYTE)((sxsi->totals & 0x000000ffL));
	*data++ = 0;												// zero
	*data++ = 0;												// block length
	*data++ = (sxsi->size & 0xff00) >> 8;
	*data++ = (sxsi->size & 0x00ff);

	scsiio.phase = SCSIPH_DATAIN;
	switch(pagecode) {
	case 0x04:	// Rigid Disk Drive Geometry Parameters
		*data++ = 0x04;	// page code
		*data++ = 0x12;	// page length
		*data++ = 0x00; // cylinder number (3 bytes)
		*data++ = (BYTE)((sxsi->cylinders & 0xff00) >> 8);
		*data++ = (BYTE)((sxsi->cylinders & 0x00ff));
		*data++ = (BYTE)sxsi->surfaces;
		ZeroMemory(data, 14);
		data += 14;
		break;

	default:	// サポートしていないページコードの場合
				// どう応答を返したら良いか不明・・・
		scsiio.phase = SCSIPH_STATUS;
		break;
	}
	scsiio.data[0] = (data - scsiio.data) - 1;
	return 0x19;		// Succeed
}

static REG8 scsicmd_transferinfo_cmd(REG8 id, BYTE *cdb) {
	REG8 drv;
	SXSIDEV	sxsi;

	if (scsiio.reg[SCSICTR_TARGETLUN] & 7) {
		return(0x42); // このコードはselect/reselect用なので、ここで使うのは正しくないかも
	}

	drv = 0x20 + id;
	sxsi = sxsi_getptr(drv);
	if ((sxsi == NULL) || (sxsi->type == 0)) {
		return(0x42); // このコードはselect/reselect用なので、ここで使うのは正しくないかも
	}

	TRACEOUT(("scsi: command: %s (%.2x)", (cdb[0] < 0x30 ? scsicmdname[cdb[0]] : "unknown"), cdb[0]));
	switch(cdb[0]) {
		case 0x00:				// Test Unit Ready
			scsiio.phase = SCSIPH_STATUS;
			return(0x1b);		// Succeed
/*
		case 0x03:				// Request Sense
			scsiio.phase = SCSIPH_DATAIN;
			return(0x19);		// Succeed
*/
		case 0x12:				// Inquiry
			scsicmd_datain(sxsi, cdb);
			scsiio.phase = SCSIPH_DATAIN;
			return(0x19);		// Succeed

		case 0x1a:				// Sense Mode
			return(scsicmd_transferinfo_mode_sense_cmd(sxsi, id, cdb));

		case 0x25:				// Read Capacity
			scsiio.data[0] = (BYTE)((sxsi->totals & 0xff000000L) >> 24);	// total blocks
			scsiio.data[1] = (BYTE)((sxsi->totals & 0x00ff0000L) >> 16);
			scsiio.data[2] = (BYTE)((sxsi->totals & 0x0000ff00L) >>  8);
			scsiio.data[3] = (BYTE)((sxsi->totals & 0x000000ffL));
			scsiio.data[4] = 0;												// block size
			scsiio.data[5] = 0;
			scsiio.data[6] = (sxsi->size & 0xff00) >> 8;
			scsiio.data[7] = (sxsi->size & 0x00ff);
			scsiio.phase = SCSIPH_DATAIN;
			return(0x19);		// Succeed

		case 0x28:				// Read Extended
			{
				UINT32 pos;
				UINT16 blocks;

				pos = (((UINT32)cdb[2]) << 24) + 
					  (((UINT32)cdb[3]) << 16) + 
					  (((UINT32)cdb[4]) <<  8) + 
					  (((UINT32)cdb[5]));
				blocks = (((UINT16)cdb[7]) << 8) + ((UINT16)cdb[8]);
				if (pos > 0x7fffffffL ||
					blocks > 0x7fff ||
					pos + blocks > 0x7fffffffL ||
					pos + blocks >= (UINT32)sxsi->totals) {
					// pos(論理ブロックアドレス)またはblocks(転送長)が不正
					// どのような応答を返すべき？？
					scsiio.phase = SCSIPH_STATUS;
					return(0x19);		// Succeed
				}
				if ((long)blocks * sxsi->size > sizeof(scsiio.data)) {
					// blocks(転送長)が大きすぎてscsiio.dataに入らない
					// エラーの扱いとする。どのような応答を返すべき？？
					scsiio.phase = SCSIPH_STATUS;
					return(0x19);		// Succeed
				}
				if (sxsi_read(drv, pos, scsiio.data, blocks * sxsi->size)) {
					// 読み取りエラー
					// どのような応答を返すべき？？
					scsiio.phase = SCSIPH_STATUS;
					return(0x19);		// Succeed
				}
				scsiio.phase = SCSIPH_DATAIN;
				return(0x19);		// Succeed
			}

	}

	SCSICMD_ERR
	return(0xff);
}

REG8 scsicmd_transferinfo_out(REG8 id, BYTE *data) {
	switch(scsiio.phase) {
		case SCSIPH_COMMAND:
			return scsicmd_transferinfo_cmd(id, data);
		default:
			return 0xff;
	}
}

void scsicmd_start_transferinfo_in(REG8 id) {
	switch(scsiio.phase) {
		case SCSIPH_DATAIN:
			// データは、scsicmd_transferinfo_cmdの処理で、scsiio.dataに格納済み。
			scsiio.nextstatus = 0x1b;		// Transferコマンド正常終了(ステータスフェーズ)
			scsiio.nextphase = SCSIPH_STATUS;
			break;
		case SCSIPH_STATUS:
			scsiio.data[0] = 0; // Status
			scsiio.nextstatus = 0x1f;		// Transferコマンド正常終了(メッセージインフェーズ)
			scsiio.nextphase = SCSIPH_MSGIN;
			break;
		case SCSIPH_MSGIN:
			scsiio.data[0] = 0; // Message
			scsiio.nextstatus = 0x20;		// Transferコマンド(メッセージインフェーズ)がACKのアサート状態でポーズ
			scsiio.nextphase = SCSIPH_MSGIN;
			break;
	}
}

REG8 scsicmd_end_transferinfo_in(void) {
	REG8 ret = 0xff;

	if (scsiio.nextstatus != 0xff) {
		ret = scsiio.nextstatus;
		scsiio.nextstatus = 0xff;
		scsiio.phase = scsiio.nextphase;
	}
	return ret;
}

/*
REG8 scsicmd_transferinfo(REG8 id, BYTE *cdb) {

	SXSIDEV	sxsi;
	UINT	leng;

	if (scsiio.reg[SCSICTR_TARGETLUN] & 7) {
		return(0x42); // このコードはselect/reselect用なので、ここで使うのは正しくないかも
	}

	sxsi = sxsi_getptr((REG8)(0x20 + id));
	if ((sxsi == NULL) || (sxsi->type == 0)) {
		return(0x42); // このコードはselect/reselect用なので、ここで使うのは正しくないかも
	}

	TRACEOUT(("scsi: transfer info: scsi command code = %.2x", cdb[0]));
	switch(cdb[0]) {
		case 0x00:				// Test Unit Ready
			scsiio.data[0] = 0;	// Status Byte 
			scsiio.phase = SCSIPH_STATUS;
			return(0x1b);		// Succeed

		case 0x12:				// Inquiry
			leng = scsicmd_datain(sxsi, cdb);
			scsiio.phase = SCSIPH_DATAIN;
			return(0x19);			// Succeed

		case 0x03:				// Request Sense
			scsiio.phase = SCSIPH_DATAIN;
			return(0x19);		// Succeed
	}

	SCSICMD_ERR
	return(0xff);
}
*/
#endif

static REG8 scsicmd_cmd(REG8 id) {

	SXSIDEV	sxsi;

	TRACEOUT(("scsicmd_cmd = %.2x", scsiio.cmd[0]));
	if (scsiio.reg[SCSICTR_TARGETLUN] & 7) {
		return(0x42);
	}
	sxsi = sxsi_getptr((REG8)(0x20 + id));
	if ((sxsi == NULL) || (sxsi->type == 0)) {
		return(0x42);
	}
	switch(scsiio.cmd[0]) {
		case 0x00:
			return(0x8b);		// Transfer Status要求

		case 0x12:				// inquiry
			scsicmd_datain(sxsi, scsiio.cmd);
			scsiio.phase = SCSIPH_DATAIN;
			return(0x89);		// Transfer Data要求
	}

	SCSICMD_ERR
	return(0xff);
}

BOOL scsicmd_send(void) {

	switch(scsiio.phase) {
		case SCSIPH_COMMAND:
			scsiio.cmdpos = 0;
			return(SUCCESS);
	}
	return(FAILURE);
}


// ---- BIOS から

static const UINT8 stat2ret[16] = {
				0x40, 0x00, 0x10, 0x00,
				0x20, 0x00, 0x10, 0x00,
				0x30, 0x00, 0x10, 0x00,
				0x20, 0x00, 0x10, 0x00};

static REG8 bios1bc_seltrans(REG8 id) {

	BYTE	cdb[16];
	REG8	ret;

	MEML_READSTR(CPU_DS, CPU_DX, cdb, 16);
	scsiio.reg[SCSICTR_TARGETLUN] = cdb[0];
	if ((cdb[1] & 0x0c) == 0x08) {			// OUT
		MEML_READSTR(CPU_ES, CPU_BX, scsiio.data, CPU_CX);
	}
	ret = scsicmd_transfer(id, cdb + 4);
	if ((cdb[1] & 0x0c) == 0x04) {			// IN
		MEML_WRITESTR(CPU_ES, CPU_BX, scsiio.data, CPU_CX);
	}
	return(ret);
}

void scsicmd_bios(void) {

	UINT8	flag;
	UINT8	ret;
	REG8	stat;
	UINT	cmd;
	REG8	dstid;

	TRACEOUT(("BIOS 1B-C* CPU_AH %.2x", CPU_AH));

	if (CPU_AH & 0x80) {		// エラーぽ
		return;
	}

	flag = MEML_READ8(CPU_SS, CPU_SP+4) & 0xbe;
	ret = mem[0x0483];
	cmd = CPU_AH & 0x1f;
	dstid = CPU_AL & 7;
	if (ret & 0x80) {
		mem[0x0483] &= 0x7f;
	}
	else if (cmd < 0x18) {
		switch(cmd) {
			case 0x00:		// reset
				stat = 0x00;
				break;

			case 0x03:		// Negate ACK
				stat = scsicmd_negate(dstid);
				break;

			case 0x07:		// Select Without AMN
				stat = scsicmd_select(dstid);
				break;

			case 0x09:		// Select Without AMN and Transfer
				stat = bios1bc_seltrans(dstid);
				break;

			default:
				TRACEOUT(("cmd = %.2x", CPU_AH));
				SCSICMD_ERR
				stat = 0x42;
				break;
		}
		ret = stat2ret[stat >> 4] + (stat & 0x0f);
		TRACEOUT(("BIOS 1B-C* CPU_AH %.2x ret = %.2x", CPU_AH, ret));
		mem[0x0483] = ret;
	}
	else {
		if ((ret ^ cmd) & 0x0f) {
			ret = cmd | 0x80;
		}
		else {
			switch(cmd) {
				case 0x19:		// Data In
					MEML_WRITESTR(CPU_ES, CPU_BX, scsiio.data, CPU_CX);
					scsiio.phase = SCSIPH_STATUS;
					stat = 0x8b;
					break;

				case 0x1a:		// Transfer command
					MEML_READSTR(CPU_ES, CPU_BX, scsiio.cmd, 12);
					stat = scsicmd_cmd(dstid);
					break;

				case 0x1b:		// Status In
					scsiio.phase = SCSIPH_MSGIN;
					stat = 0x8f;
					break;

				case 0x1f:		// Message In
					scsiio.phase = 0;
					stat = 0x80;
					break;

				default:
					TRACEOUT(("cmd = %.2x", CPU_AH));
					SCSICMD_ERR
					stat = 0x42;
					break;
			}
			ret = stat2ret[stat >> 4] + (stat & 0x0f);
		}
		TRACEOUT(("BIOS 1B-C* CPU_AH %.2x ret = %.2x", CPU_AH, ret));
		mem[0x0483] = ret;
	}
	flag |= ret & Z_FLAG;
	if (ret & 0x80) {
		flag |= C_FLAG;
		ret &= 0x7f;
	}
	CPU_AH = ret;
	MEML_WRITE8(CPU_SS, CPU_SP + 4, flag);
}
#endif

