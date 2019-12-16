// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019-2021 NXP
 *
 * Jianchao Wang <jianchao.wang@nxp.com>
 */

#include <string.h>
#include <common.h>
#include <command.h>
#include <dm.h>
#include <fdtdec.h>
#include <i2s.h>
#include <sound.h>
#include "wavplayer.h"
#include "demo_mav.c"

DECLARE_GLOBAL_DATA_PTR;

struct wavctrl wav_ctrl;
u8 wavtransferend;
u8 wavwitchbuf;
struct udevice *dev;

u8 wav_decode_init(u8 *mav_file, struct wavctrl *wavx)
{
	u8 *buf = mav_file;
	struct chunk_riff *riff;
	struct chunk_fmt *fmt;
	struct chunk_data *data;

	riff = (struct chunk_riff *)buf;
	/* MAV file */
	if (riff->chunk_id == RIFF_CHUNKID && riff->format == RIFF_FORMAT) {
		fmt = (struct chunk_fmt *)(buf + sizeof(struct chunk_riff));
		wavx->datastart = sizeof(struct chunk_riff) +
				  sizeof(struct chunk_fmt);
		if (fmt->audio_format != FMT_AUDIOFORMAT_PCM)
			wavx->datastart += sizeof(struct chunk_fact);
		data = (struct chunk_data *)(buf + wavx->datastart);
		if (data->chunk_id == DATA_CHUNKID) {
			wavx->audioformat = fmt->audio_format;
			wavx->nchannels   = fmt->num_of_channels;
			wavx->samplerate  = fmt->sample_rate;
			wavx->bitrate     = fmt->byte_rate * 8;
			wavx->blockalign  = fmt->block_align;
			wavx->bps         = fmt->bits_per_sample;
			wavx->datasize    = data->chunk_size;
			wavx->datastart   = wavx->datastart + 8;
			printf("audioformat: %s\n", wavx->audioformat ? "PCM" :
						    "IMA ADPCM");
			printf("nchannels: %d\n", wavx->nchannels);
			printf("samplerate: %d\n", wavx->samplerate);
			printf("bitrate: %d\n", wavx->bitrate);
			printf("blockalign: %d\n", wavx->blockalign);
			printf("bps: %d\n", wavx->bps);
			printf("datasize: %d\n", wavx->datasize);
			printf("datastart: %d\n", wavx->datastart);
		} else {
			return 2;
		}
	} else {
		return 1; /* Not a wav file */
	}

	return 0;
}

/* Initilaise sound subsystem */
static int do_demo(u8 *mav_file)
{
	struct sound_uc_priv *uc_priv;
	struct i2s_uc_priv *i2s_priv;
	int ret = 0;

	printf("***********************************************************\n");
	wav_decode_init(mav_file, &wav_ctrl);
	printf("***********************************************************\n");

	if (!dev)
		ret = uclass_first_device_err(UCLASS_SOUND, &dev);
	if (!ret) {
		uc_priv = dev_get_uclass_priv(dev);
		i2s_priv = dev_get_uclass_priv(uc_priv->i2s);

		i2s_priv->channels = wav_ctrl.nchannels;
		i2s_priv->bitspersample = wav_ctrl.bps;
		i2s_priv->samplingrate = wav_ctrl.samplerate;

		sound_setup(dev); /* Setup the audio codec chip */
		i2s_transfer_tx_data(i2s_priv, &mav_file[wav_ctrl.datastart],
				     wav_ctrl.datasize);
	} else {
		printf("Initialise Audio driver failed (ret=%d)\n", ret);
		return CMD_RET_FAILURE;
	}
	printf("***********************************************************\n");

	return 0;
}

/* play sound from buffer */
static int do_play(int argc, char *const argv[])
{
	u32 addr;

	addr = simple_strtoul(argv[1], NULL, 16);
	printf("[WAV file address is 0x%08X]\n", addr);

	return do_demo((u8 *)addr);
}

/* process sound command */
static int do_sound(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	if (argc == 1)
		return do_demo(mav_file);
	else if (argc == 2)
		return do_play(argc, argv);

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	wavplayer, 2, 1, do_sound,
	"MAV player sub-system",
	"demo   - play a demo file\n"
	"mavplayer [addr] - play a mav file\n"
);
