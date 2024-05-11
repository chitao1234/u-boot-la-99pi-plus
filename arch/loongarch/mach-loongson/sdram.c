#include <common.h>
#include <dm.h>
#include <ram.h>
#include <fdtdec.h>

DECLARE_GLOBAL_DATA_PTR;

int dram_init(void)
{
	struct udevice *dev;
	int probed_ram_devices = 0;
	
	for (uclass_first_device(UCLASS_RAM, &dev);
			dev != NULL; uclass_next_device(&dev)) {
		probed_ram_devices++;
	}

	if (probed_ram_devices < 1) {
		panic("DDR init failed!\n");
	}

	if (fdtdec_setup_mem_size_base() != 0) {
		printf("setup mem failed\n");
		return -EINVAL;
	}

	return 0;
}

int dram_init_banksize(void)
{
	fdtdec_setup_memory_banksize();
	return 0;
}
