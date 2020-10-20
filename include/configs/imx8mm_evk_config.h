// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2018-2020 NXP
 *
 */

#define CONFIG_SLAVE_FIRST_CORE                 1

/* set Master core size to 512M, slave core size to 256M, share mem to 256M */

/* Number of cores in each cluster */
#define CORE_NUM_PER_CLUSTER    4
#define CONFIG_MAX_CPUS  4

/* I2C */
//#define CONFIG_SYS_I2C_MXC_I2C1	/* enable I2C bus 0 */
//#define CONFIG_SYS_I2C_MXC_I2C2	/* enable I2C bus 1 */
//#define CONFIG_SYS_I2C_MXC_I2C3	/* enable I2C bus 2 */
//#define CONFIG_SYS_I2C_MXC_I2C4	/* enable I2C bus 3 */
//#define	CONFIG_I2C_BUS_CORE_ID_SET
//#define CONFIG_SYS_I2C_MXC_I2C0_COREID  1
//#define CONFIG_SYS_I2C_MXC_I2C1_COREID  2
//#define CONFIG_SYS_I2C_MXC_I2C2_COREID  3
//#define CONFIG_SYS_I2C_MXC_I2C3_COREID  1
