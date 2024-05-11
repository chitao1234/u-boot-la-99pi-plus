#ifndef __BOARD_KERNEL_PRODUCT_H__
#define __BOARD_KERNEL_PRODUCT_H__

// 更新 dtb 的 smbios baseboard product 的 值 (根据bdinfo)
int update_board_kernel_product(void);

// 设置 bdinfo 中 关于 smbios baseboard product 的 值的 下标
int set_board_kernel_product_id(char* id);

// 获取 可设置的 product 的 描述 语句 (用于菜单显示)
char* get_board_kernel_product_menu_option(int n);

#endif
