/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _ASM_LOONGARCH_UNALIGNED_H
#define _ASM_LOONGARCH_UNALIGNED_H

#include <linux/compiler.h>
#define get_unaligned	__get_unaligned_le
#define put_unaligned	__put_unaligned_le

#include <linux/unaligned/le_byteshift.h>
#include <linux/unaligned/be_byteshift.h>
#include <linux/unaligned/generic.h>

#endif /* _ASM_LOONGARCH_UNALIGNED_H */
