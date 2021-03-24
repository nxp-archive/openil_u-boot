// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019-2021 NXP
 *
 */

#define CONFIG_SLAVE_FIRST_CORE                 1

/* set Master core size to 512M, slave core size to 256M, share mem to 256M */
#define CONFIG_SYS_DDR_SDRAM_SLAVE_SIZE	(256 * 1024 * 1024)
#define CONFIG_SYS_DDR_SDRAM_MASTER_SIZE	(512 * 1024 * 1024)
#define CONFIG_SYS_DDR_SDRAM_SHARE_RESERVE_SIZE (16 * 1024 * 1024)
#define CONFIG_SYS_DDR_SDRAM_SHARE_SIZE \
	((256 * 1024 * 1024) - CONFIG_SYS_DDR_SDRAM_SHARE_RESERVE_SIZE)

/*
 * Size of malloc() pool
 */
#define CONFIG_SYS_MALLOC_LEN	(CONFIG_ENV_SIZE + 128 * 1024 * 1024)

/* Number of cores in each cluster */
#define CORE_NUM_PER_CLUSTER	2

/* I2C */
#define CONFIG_SYS_I2C_MXC_I2C1	/* enable I2C bus 0 */
#define CONFIG_SYS_I2C_MXC_I2C2	/* enable I2C bus 1 */
#define CONFIG_SYS_I2C_MXC_I2C3	/* enable I2C bus 2 */
#define CONFIG_SYS_I2C_MXC_I2C4	/* enable I2C bus 3 */

#ifdef CONFIG_EXTRA_ENV_SETTINGS
#undef CONFIG_EXTRA_ENV_SETTINGS
#endif
