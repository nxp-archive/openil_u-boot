/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/* #define	DEBUG	*/

#include <common.h>
#include <autoboot.h>
#include <cli.h>
#include <console.h>
#include <version.h>
#include <mmc.h>
#include <asm/io.h>

DECLARE_GLOBAL_DATA_PTR;

/*
 * Board-specific Platform code can reimplement show_boot_progress () if needed
 */
__weak void show_boot_progress(int val) {}

static void run_preboot_environment_command(void)
{
#ifdef CONFIG_PREBOOT
	char *p;

	p = env_get("preboot");
	if (p != NULL) {
# ifdef CONFIG_AUTOBOOT_KEYED
		int prev = disable_ctrlc(1);	/* disable Control C checking */
# endif

		run_command_list(p, -1, 0);

# ifdef CONFIG_AUTOBOOT_KEYED
		disable_ctrlc(prev);	/* restore Control C checking */
# endif
	}
#endif /* CONFIG_PREBOOT */
}

struct sdversion_t{
	unsigned char   updateflag;
	unsigned char   updatepart;
	unsigned char   data[0x200 - 2];
};

int check_SDenv_version(void)
{
	struct mmc *mmc;
	uint blk_start, blk_cnt;
	unsigned long offset, size;
	struct sdversion_t sdversion_env;
	int dev = 0;

	offset	= 0x1FE000;
	size	= 0x200;
	mmc = find_mmc_device(dev);
	struct blk_desc *desc = mmc_get_blk_desc(mmc);
	blk_start   = ALIGN(offset, mmc->read_bl_len) / mmc->read_bl_len;
	blk_cnt     = ALIGN(size, mmc->read_bl_len) / mmc->read_bl_len;
	blk_dread(desc, blk_start, blk_cnt, (uchar *)&sdversion_env);
	if(sdversion_env.updateflag == '1')
	{
		printf("system is updating\n");
		sdversion_env.updateflag = '2';
		blk_dwrite(desc, blk_start, blk_cnt, (uchar *)&sdversion_env);
		return 0;
	}
	else if(sdversion_env.updateflag == '2')
	{
		sdversion_env.updateflag = '3';
		blk_dwrite(desc, blk_start, blk_cnt, (uchar *)&sdversion_env);
		if(sdversion_env.updatepart == '3')
		{
			setenv("bootfile", getenv("bootfile_old"));
			return 0;
		}
		else
			return 1;
	}
	else
		return 0;
}

/* We come here after U-Boot is initialised and ready to process commands */
void main_loop(void)
{
	const char *s;

	bootstage_mark_name(BOOTSTAGE_ID_MAIN_LOOP, "main_loop");

#ifdef CONFIG_VERSION_VARIABLE
	env_set("ver", version_string);  /* set version variable */
#endif /* CONFIG_VERSION_VARIABLE */

	if(check_SDenv_version())
		setenv("bootcmd","run rollbackboot");
	cli_init();

	run_preboot_environment_command();

#if defined(CONFIG_UPDATE_TFTP)
	update_tftp(0UL, NULL, NULL);
#endif /* CONFIG_UPDATE_TFTP */

	s = bootdelay_process();
	if (cli_process_fdt(&s))
		cli_secure_boot_cmd(s);

	autoboot_command(s);

	cli_loop();
	panic("No CLI available");
}
