// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright 2019-2020 NXP
 *
 */

#define CONFIG_ICC
#define CONFIG_MASTER_CORE	0
#define CONFIG_SLAVE_FIRST_CORE                 1

/* set Master core size to 512M, slave core size to 256M, share mem to 256M */
#define CONFIG_SYS_DDR_SDRAM_SLAVE_SIZE (256 * 1024 * 1024)
#define CONFIG_SYS_DDR_SDRAM_MASTER_SIZE    (512 * 1024 * 1024)
#define CONFIG_SYS_DDR_SDRAM_SHARE_RESERVE_SIZE (16 * 1024 * 1024)
#define CONFIG_SYS_DDR_SDRAM_SHARE_SIZE \
	((256 * 1024 * 1024) - CONFIG_SYS_DDR_SDRAM_SHARE_RESERVE_SIZE)

#define CONFIG_MASTER_CORE                     0

#define CONFIG_SYS_DDR_SDRAM_SHARE_BASE \
	(CONFIG_SYS_DDR_SDRAM_BASE + CONFIG_SYS_DDR_SDRAM_MASTER_SIZE \
	 + CONFIG_SYS_DDR_SDRAM_SLAVE_SIZE * (CONFIG_MAX_CPUS - 1))

#define CONFIG_SYS_DDR_SDRAM_SHARE_RESERVE_BASE \
	(CONFIG_SYS_DDR_SDRAM_SHARE_BASE + CONFIG_SYS_DDR_SDRAM_SHARE_SIZE)

/*
 * Size of malloc() pool
 */
#define CONFIG_SYS_MALLOC_LEN	(CONFIG_ENV_SIZE + 128 * 1024 * 1024)

#define CONFIG_PL011_SERIAL

#define CORE_NUM_PER_CLUSTER	2


/* I2C */
#define CONFIG_SYS_I2C_MXC_I2C1	/* enable I2C bus 0 */
#define CONFIG_SYS_I2C_MXC_I2C2	/* enable I2C bus 1 */
#define CONFIG_SYS_I2C_MXC_I2C3	/* enable I2C bus 2 */
#define CONFIG_SYS_I2C_MXC_I2C4	/* enable I2C bus 3 */
#define CONFIG_SYS_I2C_MXC_I2C5	/* enable I2C bus 4 */
#define CONFIG_SYS_I2C_MXC_I2C6	/* enable I2C bus 5 */
#define CONFIG_SYS_I2C_MXC_I2C7	/* enable I2C bus 6 */
#define CONFIG_SYS_I2C_MXC_I2C8	/* enable I2C bus 7 */
