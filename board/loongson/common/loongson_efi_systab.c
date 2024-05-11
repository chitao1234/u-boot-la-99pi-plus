// SPDX-License-Identifier: GPL-2.0+
/*
 * EFI application boot time services
 *
 * Copyright (C) 2020 Loongson Technology Corporation Limited
 */

#include <common.h>
#include <efi_api.h>
#include <efi_loader.h>
#include <malloc.h>
#include <u-boot/crc.h>

#define FW_VERSION 0x1
#define FW_PATCHLEVEL 0x0

enum efi_table_id {
	FDT_TABLE_ID = 0,
	MAX_EFI_TABLES,
};

static struct efi_boot_services efi_boot_services = {
	.hdr = {
		.signature = EFI_BOOT_SERVICES_SIGNATURE,
		.revision = EFI_SPECIFICATION_VERSION,
		.headersize = sizeof(struct efi_boot_services),
	}
};

struct efi_runtime_services efi_runtime_services = {
	.hdr = {
		.signature = EFI_RUNTIME_SERVICES_SIGNATURE,
		.revision = EFI_SPECIFICATION_VERSION,
		.headersize = sizeof(struct efi_runtime_services),
	}
};

static u16 firmware_vendor[] = L"LoongSon";

struct efi_system_table systab = {
	.hdr = {
		.signature = EFI_SYSTEM_TABLE_SIGNATURE,
		.revision = EFI_SPECIFICATION_VERSION,
		.headersize = sizeof(struct efi_system_table),
	},
	.fw_vendor = firmware_vendor,
	.fw_revision = FW_VERSION << 16 | FW_PATCHLEVEL << 8,
	.runtime = 0,
	.nr_tables = 0,
	.tables = NULL,
};

/**
 * efi_crc32() - Update crc32 in table header
 *
 * @table:  EFI table
 */
void efi_crc32(struct efi_table_hdr *table)
{
	table->crc32 = 0;
	table->crc32 = crc32(0, (const unsigned char *)table, table->headersize);
}

/**
 * init_systab() - Initialize system table
 *
 * Return:	status code
 */
efi_status_t efi_init_systab(void)
{
	systab.boottime = &efi_boot_services;

	/* Set CRC32 field in table headers */
	efi_crc32(&systab.hdr);
	efi_crc32(&efi_runtime_services.hdr);
	efi_crc32(&efi_boot_services.hdr);

	return EFI_SUCCESS;
}

/**
 * efi_install_configuration_table() - adds, updates, or removes a
 *                                     configuration table
 * @guid:  GUID of the installed table
 * @table: table to be installed
 *
 * This function is used for internal calls. For the API implementation of the
 * InstallConfigurationTable service see efi_install_configuration_table_ext.
 *
 * Return: status code
 */
efi_status_t efi_install_configuration_table(const efi_guid_t *guid,
		void *table)
{
	int i = 0;

	if (!guid)
		return EFI_INVALID_PARAMETER;

	/* Check for GUID override */
	for (i = 0; i < systab.nr_tables; i++) {
		if (!guidcmp(guid, &systab.tables[i].guid)) {
			if (table)
				systab.tables[i].table = table;
			goto out;
		}
	}

	if (!table)
		return EFI_NOT_FOUND;

	/* No override, check for overflow */
	if (i >= EFI_MAX_CONFIGURATION_TABLES)
		return EFI_OUT_OF_RESOURCES;

	/* Add a new entry */
	guidcpy(&systab.tables[i].guid, guid);
	systab.tables[i].table = table;
	systab.nr_tables = i + 1;

out:
	/* systab.nr_tables may have changed. So we need to update the CRC32 */
	efi_crc32(&systab.hdr);

	return EFI_SUCCESS;
}
