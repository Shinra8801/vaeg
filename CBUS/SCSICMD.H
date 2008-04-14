
#if defined(SUPPORT_SCSI)

#ifdef __cplusplus
extern "C" {
#endif

REG8 scsicmd_negate(REG8 id);
REG8 scsicmd_select(REG8 id);
REG8 scsicmd_transfer(REG8 id, BYTE *cdb);
BOOL scsicmd_send(void);

#if defined(VAEG_EXT)
REG8 scsicmd_transferinfo_out(REG8 id, BYTE *data);
void scsicmd_start_transferinfo_in(REG8 id);
REG8 scsicmd_end_transferinfo_in(void);
#endif

void scsicmd_bios(void);

#ifdef __cplusplus
}
#endif

#endif

