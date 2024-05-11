#ifndef __LS_BDINFO_H__
#define __LS_BDINFO_H__

enum bdi_id {
	BDI_ID_NAME = 0,
	BDI_ID_SN,
	BDI_ID_MAC0,
	BDI_ID_MAC1,
	BDI_ID_SYSPART,
#ifdef LS_DOUBLE_SYSTEM
	BDI_ID_SYSPART_LAST,
#endif
	BDI_ID_BOARD_PRODUCT_ID,
	BDI_ID_MAX,
};

int bdinfo_init(void);
char* bdinfo_get(enum bdi_id id);
int bdinfo_set(enum bdi_id id, const char *val);

int bdinfo_save_in_nv(void);

//#ifndef CONFIG_SPL_BUILD
int bdinfo_save(void);
//#endif

#endif /* __LS_BDINFO_H__ */
