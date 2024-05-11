#include <config.h>
#include <linux/linkage.h>
#include <asm/regdef.h>


WEAK(lowlevel_init)
    // implement it in every machine. do some necessary things.
    // At lease
    // 1. setup system clk.
    // 2. early init the serial.
    jirl	zero, ra, 0
ENDPROC(lowlevel_init)

/**********************************************
 * initial memory and return(reg a0) the ram top addr will used by sp.
 **********************************************/
WEAK(ram_init)
    // implement it in every machine.
#ifdef CONFIG_SYS_INIT_SP_ADDR
    li.d    a0, CONFIG_SYS_INIT_SP_ADDR
#else
    li.d    a0, (-1)
#endif
    jirl	zero, ra, 0
ENDPROC(ram_init)

// for early debug, init the debug serial.
WEAK(init_serial)
    // implement it in every machine.
    jirl	zero, ra, 0
ENDPROC(init_serial)

// for early debug, print char.
WEAK(printchar)
    // implement it in every machine.
    jirl	zero, ra, 0
ENDPROC(printchar)

