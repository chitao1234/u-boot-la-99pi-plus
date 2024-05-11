#ifndef __GENERAL_LOAD_H__
#define __GENERAL_LOAD_H__

#include <net.h>
#include <fs.h>

enum gl_device_e {
	GL_DEVICE_NET,
	GL_DEVICE_BLK,
};

typedef struct gl_device_s {
	enum gl_device_e type;
	void* desc;
	union {
		char interface[16];
		char ip[16];
	};
	union {
		char part[8];
		char port[8];
	};
} gl_device_t;

typedef union gl_format_s {
		int fstype;
		enum proto_t proto;
} gl_format_t;

typedef struct gl_target_s {
	gl_device_t gl_device;
	gl_format_t gl_format;
	char symbol[32];
} gl_target_t;

//|0		  | 1 ...
//| DECOMPRESSION | SECURE 
enum gl_extra_e {
	GL_EXTRA_DECOMPRESS = 0x01,
	GL_EXTRA_SECURECHECK = 0x02,
};

// load-from <src-target> burn-to <dest-target>
// burn-to decoration: <decompress/securecheck>
// target decoration:
// 	device <blk/net>,
//	format <fs/netproto>
//	symbol <filename>
// target: <symbol> in <device> with <format>
//
// load-from <src> burn-to <dest> and <extra>
// e.g.
// load {/update/uImage in usb0:1 with fs-ext4} to {/boot/uImage in mmc0:1 with fs-ext4}
// load {/a.gz in usb0:1 with fs-ext4} to {/root/a in mmc0:1 with fs-ext4} and {decompression}
// load {disk.img.gz in net:192.168.1.2 with netproto-tftp} to {mmc0} and {decompression}
// load {uboot in net:192.168.1.2 with netproto-tftp} to {sf:0} and {securecheck}
// load {mmc0} to {scsi0}

int general_load(gl_target_t* src, gl_target_t* dest, enum gl_extra_e extra);

#endif

