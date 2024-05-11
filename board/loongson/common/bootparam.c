#include <common.h>
#include <asm/addrspace.h>
#include <smbios.h>
#include <dm/device.h>
#include <dm/uclass.h>
#include <ram.h>
#include "bootparam.h"

#include "board_kernel_product.h"

#define BOOTPARAMS_ADDRESS			0x0f030400
#define BOOTPARAMS_SIZE				0x9c00
#define SMBIOS_PHYSICAL_ADDRESS 	0x0fffe000
#define SMBIOS_SIZE_LIMIT 			0x800
#define ACPI_TABLE_PHYSICAL_ADDRESS	0x0fefe000
#define ACPI_TABLE_SIZE_LIMIT 		0x100000

// static struct boot_params ls_boot_params;

#if defined(BOOT_PARAMS_BPI)

#ifdef ACPI_SUPPORT
const efi_guid_t smbios_guid = SMBIOS_TABLE_GUID;
#endif

extern struct efi_system_table systab;
extern efi_status_t efi_init_systab(void);
extern efi_status_t efi_install_configuration_table(const efi_guid_t *guid,
			void *table);
extern ulong write_smbios_table(ulong addr);

unsigned char checksum8(char *buff, int size)
{
	int sum, cnt;

	for (sum = 0, cnt = 0; cnt < size; cnt++) {
		sum = (char) (sum + *(buff + cnt));
	}

	return (char)(0x100 - sum);
}

void bp_checksum(char *buff, int size)
{
	int cksum_off;

	cksum_off = (int)OFFSET_OF(struct ext_list_hdr, checksum);
	buff[cksum_off] = 0;

	buff[cksum_off] = checksum8(buff, size);

	return;
}

void addlist(struct boot_params *bp, struct ext_list_hdr *header)
{
	struct ext_list_hdr *last;

	if (bp->elh == NULL) {
		 bp->elh = header;
	} else {
		for (last = bp->elh; last->next; last = last->next)
			;
		last->next = header;
		bp_checksum((u8 *)last, last->len);
	}
	header->next = NULL;
	bp_checksum((u8 *)header, header->len);
}

void map_entry_init(struct mem_info *mem_data, int type, u64 start, u64 size)
{
	mem_data->map[mem_data->map_num].type = type;
	mem_data->map[mem_data->map_num].start = start;
	mem_data->map[mem_data->map_num].size = size;
	mem_data->map_num++;
}

u64 init_mem(struct boot_params *bp, u64 offset)
{
	struct mem_info *mem_data = (struct mem_info *)((u64)bp + offset);
	char data[8] = {'M', 'E', 'M'};
	u64 totalsize = 0, size = 0;
	// int i;
	struct udevice *dev;
	struct ram_info info;
	const struct ram_ops *ops;

	memcpy(&mem_data->header.sign, data, sizeof(data));
	mem_data->header.rev = 0;
	mem_data->header.len = sizeof (*mem_data);
	mem_data->map_num = 0;
	/*
	 * 1. The lowest 2M region cannot record in MemMap, cause Linux ram region should begin with usable ram.
	 *    map_entry_init(mem_data, MEM_TYPE_RESERVED, 0x0, 0x200000);  // 0x0-0x200000
	 */

	/* 2. Available SYSTEM_RAM area. */
	map_entry_init(mem_data, MEM_TYPE_SYSRAM, 0x200000, 0xf000000 - 0x200000);  // 0x200000~0xf000000

	/* 3. Reserved low memory highest 16M. */
	map_entry_init(mem_data, MEM_TYPE_RESERVED, 0xf000000, 0x1000000);  // 0xf000000~0x10000000

	/* 4. Available SYSTEM_RAM area */
	// for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
	// 	totalsize += gd->bd->bi_dram[i].size;
	// }

	int ret = uclass_first_device_err(UCLASS_RAM, &dev);
	if (ret) {
		printf("Failed to find ram node. Check device tree!!!\n");
		return 0;
	}

	ops = dev->driver->ops;
	ops->get_info(dev, &info);
	totalsize = info.size;

	size = totalsize - 0x10000000;
	if (size > 0) {
		// (HIGH_MEM_WIN_BASE_ADDR + 0x10000000) ~ MAX
		map_entry_init(mem_data, MEM_TYPE_SYSRAM, HIGH_MEM_ENTRY_ADDR + SZ_256M, size);
	}

	addlist(bp, &mem_data->header);

	return sizeof(struct mem_info);
}

u64 init_vbios(struct boot_params *bp, u64 offset)
{
	return 0;
}

void loongson_smbios_init(void)
{
	write_smbios_table((ulong)SMBIOS_PHYSICAL_ADDRESS);

#ifdef ACPI_SUPPORT
	/* And expose them to our EFI payload */
	return efi_install_configuration_table(&smbios_guid,
			(void *)SMBIOS_PHYSICAL_ADDRESS);
#endif
}

#ifdef ACPI_SUPPORT
static const efi_guid_t acpi_guid = EFI_ACPI_TABLE_GUID;

void loongson_acpi_init(void)
{
	write_acpi_tables((void *)ACPI_TABLE_PHYSICAL_ADDRESS);

	/* And expose them to our EFI payload */
	return efi_install_configuration_table(&acpi_guid,
			(void *)v_to_p(ACPI_TABLE_PHYSICAL_ADDRESS));
}
#endif

int init_boot_param(struct boot_params *bp)
{
	u64 __maybe_unused tab_offset = sizeof(struct boot_params);
	char sign_bp[8] = {'B', 'P', 'I', '0', '1', '0', '0', '0'};

	bp->flags = 0;
#ifdef SOC_CPU
	sign_bp[7] = '1'; // BPI_VERSION_V2
	bp->flags |= BPI_FLAGS_SOC_CPU;
#endif
	memcpy(bp, sign_bp, 8);

	efi_init_systab();
	bp->efitab = &systab;
	bp->elh = NULL;

#ifdef MEM_TAB
	tab_offset += init_mem(bp, tab_offset);
#endif
#ifdef VBIOS_TAB
	tab_offset += init_vbios(bp, tab_offset);
#endif
#ifdef SMBIOS_SUPPORT
	loongson_smbios_init();
#endif
#ifdef ACPI_SUPPORT
	loongson_acpi_init();
#endif
	return 0;
}

#elif defined(BOOT_PARAMS_EFI)
extern void _machine_restart(void);
extern void _machine_poweroff(void);

#ifndef STR_FUNC_ADDR
#define STR_FUNC_ADDR (&_machine_restart)
#endif

struct board_devices *board_devices_info();
struct interface_info *init_interface_info();
struct irq_source_routing_table *init_irq_source();
struct system_loongson *init_system_loongson();
struct efi_cpuinfo_loongson *init_cpu_info();
struct efi_memory_map_loongson * init_memory_map();
void init_efi(struct efi_loongson *efi);
struct loongson_special_attribute *init_special_info();


void init_efi(struct efi_loongson *efi)
{
	init_smbios(&(efi->smbios));
}

void init_reset_system(struct efi_reset_system_t *reset)
{
	reset->Shutdown = &_machine_poweroff;
	reset->ResetWarm = &_machine_restart;
#ifdef LS_STR
	reset->ResetCold = STR_FUNC_ADDR;
#endif
}

void init_smbios(struct smbios_tables *smbios)
{
	smbios->vers = 0;
	smbios->vga_bios = init_vga_bios();
	init_loongson_params(&(smbios->lp));
}

void init_loongson_params(struct loongson_params *lp)
{
	lp->memory_offset = (unsigned long long)init_memory_map() - (unsigned long long)lp;
	lp->cpu_offset = (unsigned long long)init_cpu_info() - (unsigned long long)lp;
	lp->system_offset = (unsigned long long)init_system_loongson() - (unsigned long long)lp;
	lp->irq_offset = (unsigned long long)init_irq_source() - (unsigned long long)lp;
	lp->interface_offset = (unsigned long long)init_interface_info() - (unsigned long long)lp;
	lp->boarddev_table_offset = (unsigned long long)board_devices_info() - (unsigned long long)lp;
	lp->special_offset = (unsigned long long)init_special_info() - (unsigned long long)lp;

	printf("memory_offset = 0x%llx;cpu_offset = 0x%llx; system_offset = 0x%llx; irq_offset = 0x%llx; interface_offset = 0x%llx;\n", lp->memory_offset, lp->cpu_offset, lp->system_offset, lp->irq_offset, lp->interface_offset);
}

int init_boot_param(struct boot_params *bp)
{
	init_efi(&(bp->efi));
	init_reset_system(&(bp->reset_system));
}
#endif

void *build_boot_param(void)
{
	struct boot_params *bp = 
		(struct boot_params *)PHYS_TO_CACHED(BOOTPARAMS_ADDRESS);

	update_board_kernel_product();

	init_boot_param(bp);
	return bp;
}
