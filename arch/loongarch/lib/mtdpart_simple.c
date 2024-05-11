#include <common.h>
#include <jffs2/load_kernel.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

#define MTD_SIZE_REMAINING		(~0LLU)
#define MTD_OFFSET_NOT_SPECIFIED	(~0LLU)

#define MTD_NAME_LEN			32

static int get_mtdids_name(int type, char *out)
{
	char *mtdids = CONFIG_MTDIDS_DEFAULT;
	const char *type_name = MTD_DEV_TYPE(type);
	char *p, *name;
	int namelen = -ENOENT;

	do {
		name = mtdids;
		p = strchr(mtdids, '=');
		if (!p || *(p + 1) == '\0')
			continue;

		// walk to next mtdid
		mtdids = strchr(mtdids, ',');

		if (!strncmp(name, type_name, strlen(type_name))) {
			name = ++p;
			if (!mtdids || *(mtdids + 1) == '\0')
				namelen = strlen(name);
			else
				namelen = mtdids - name;

			if (namelen > MTD_NAME_LEN)
				namelen = MTD_NAME_LEN;

			strncpy(out, name, namelen);
			out[namelen] = '\0';
			break;
		}
	} while (mtdids && *(++mtdids) != '\0');

	return namelen;
}

/**
 * mtd_parse_partition - Parse @mtdparts partition definition, fill @partition
 *                       with it and update the @mtdparts string pointer.
 *
 * The partition name is allocated and must be freed by the caller.
 *
 * This function is widely inspired from part_parse (mtdparts.c).
 *
 * @mtdparts: String describing the partition with mtdparts command syntax
 * @partition: MTD partition structure to fill
 *
 * Return: 0 on success, an error otherwise.
 */
static int mtd_parse_partition(const char **_mtdparts,
			       struct mtd_partition *partition)
{
	const char *mtdparts = *_mtdparts;
	const char *name = NULL;
	int name_len;
	char *buf;

	/* Ensure the partition structure is empty */
	memset(partition, 0, sizeof(struct mtd_partition));

	/* Fetch the partition size */
	if (*mtdparts == '-') {
		/* Assign all remaining space to this partition */
		partition->size = MTD_SIZE_REMAINING;
		mtdparts++;
	} else {
		partition->size = ustrtoull(mtdparts, (char **)&mtdparts, 0);
		if (partition->size < SZ_4K) {
			printf("Minimum partition size 4kiB, %lldB requested\n",
			       partition->size);
			return -EINVAL;
		}
	}

	/* Check for the offset */
	partition->offset = MTD_OFFSET_NOT_SPECIFIED;
	if (*mtdparts == '@') {
		mtdparts++;
		partition->offset = ustrtoull(mtdparts, (char **)&mtdparts, 0);
	}

	/* Now look for the name */
	if (*mtdparts == '(') {
		name = ++mtdparts;
		mtdparts = strchr(name, ')');
		if (!mtdparts) {
			printf("No closing ')' found in partition name\n");
			return -EINVAL;
		}
		name_len = mtdparts - name + 1;
		if ((name_len - 1) == 0) {
			printf("Empty partition name\n");
			return -EINVAL;
		}
		mtdparts++;
	} else {
		/* Name will be of the form size@offset */
		name_len = 22;
	}

	/* Check if the partition is read-only */
	if (strncmp(mtdparts, "ro", 2) == 0) {
		partition->mask_flags |= MTD_WRITEABLE;
		mtdparts += 2;
	}

	/* Check for a potential next partition definition */
	if (*mtdparts == ',') {
		if (partition->size == MTD_SIZE_REMAINING) {
			printf("No partitions allowed after a fill-up\n");
			return -EINVAL;
		}
		++mtdparts;
	} else if ((*mtdparts == ';') || (*mtdparts == '\0')) {
		/* NOP */
	} else {
		printf("Unexpected character '%c' in mtdparts\n", *mtdparts);
		return -EINVAL;
	}

	/*
	 * Allocate a buffer for the name and either copy the provided name or
	 * auto-generate it with the form 'size@offset'.
	 */
	buf = malloc(name_len);
	if (!buf)
		return -ENOMEM;

	if (name)
		strncpy(buf, name, name_len - 1);
	else
		snprintf(buf, name_len, "0x%08llx@0x%08llx",
			 partition->size, partition->offset);

	buf[name_len - 1] = '\0';
	partition->name = buf;

	*_mtdparts = mtdparts;

	return 0;
}

static int mtd_parse_partitions_simple(const char **_mtdparts,
			 struct mtd_partition **_parts, int *_nparts)
{
	struct mtd_partition partition = {}, *parts;
	const char *mtdparts = *_mtdparts;
	uint64_t cur_off = 0;
	int nparts = 0;
	int ret, idx;

	/* First, iterate over the partitions until we know their number */
	while (mtdparts[0] != '\0' && mtdparts[0] != ';') {
		ret = mtd_parse_partition(&mtdparts, &partition);
		if (ret)
			return ret;

		free((char *)partition.name);
		nparts++;
	}

	/* Allocate an array of partitions to give back to the caller */
	parts = malloc(sizeof(*parts) * nparts);
	if (!parts) {
		printf("Not enough space to save partitions meta-data\n");
		return -ENOMEM;
	}

	/* Iterate again over each partition to save the data in our array */
	for (idx = 0; idx < nparts; idx++) {
		ret = mtd_parse_partition(_mtdparts, &parts[idx]);
		if (ret)
			return ret;

		if (parts[idx].offset == MTD_OFFSET_NOT_SPECIFIED)
			parts[idx].offset = cur_off;
		
		// '-' just allow in the last partition
		if (parts[idx].size == MTD_SIZE_REMAINING)
			break;

		cur_off += parts[idx].size;
	}

	/* Offset by one mtdparts to point to the next device if any */
	if (*_mtdparts[0] == ';')
		(*_mtdparts)++;

	*_parts = parts;
	*_nparts = nparts;

	return 0;
}

int mtd_get_partition(int type, const char *partname,
				struct mtd_partition *part)
{
	const char *mtdparts = CONFIG_MTDPARTS_DEFAULT;
	struct mtd_partition *parts;
	char mtd_name[MTD_NAME_LEN];
	const char *name;
	int nparts, namelen;
	int i, ret;

	memset(mtd_name, 0, MTD_NAME_LEN);
	ret = get_mtdids_name(type, mtd_name);
	if (ret < 0)
		return ret;

	while (*mtdparts != '\0') {
		name = mtdparts;
		mtdparts = strchr(mtdparts, ':');
		if (!mtdparts)
			return -ENOENT;

		namelen = mtdparts - name;
		if (strncmp(name, mtd_name, namelen)) {
			// walk to next mtd device
			mtdparts = strchr(mtdparts, ';');
			if (!mtdparts)
				break;

			mtdparts++;
			continue;
		}

		mtdparts++;
		ret = mtd_parse_partitions_simple(&mtdparts, &parts, &nparts);
		if (ret)
			return -ENOENT;

		for (i = 0; i < nparts; i++) {
			if (!strcmp(parts[i].name, partname)) {
				memcpy(part, &parts[i], sizeof(*part));
				free(parts);
				return 0;
			}
		}
	}

	free(parts);
	return -ENOENT;
}
