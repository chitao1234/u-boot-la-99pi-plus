#include <common.h>
#include <handoff.h>
#include <asm/global_data.h>

#if CONFIG_IS_ENABLED(HANDOFF)

DECLARE_GLOBAL_DATA_PTR;

int handoff_arch_save(struct spl_handoff *ho)
{
	ho->arch.board_type = gd->board_type;
	return 0;
}
#endif