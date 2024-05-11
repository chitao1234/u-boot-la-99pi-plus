/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_LA_BYTEORDER_H
#define __ASM_LA_BYTEORDER_H

#include <asm/types.h>

#ifdef __GNUC__

#if !defined(__STRICT_ANSI__) || defined(__KERNEL__)
#define __BYTEORDER_HAS_U64__
#define __SWAB_64_THRU_32__
#endif

#endif /* __GNUC__ */

#include <linux/byteorder/little_endian.h>

#endif /* __ASM_LA_BYTEORDER_H */
