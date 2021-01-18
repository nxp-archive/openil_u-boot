// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019-2021 NXP
 *
 * Jianchao Wang <jianchao.wang@nxp.com>
 */

#ifndef __WAVPLAYER_H
#define __WAVPLAYER_H

struct chunk_riff {
	u32 chunk_id;	/* "RIFF",0X46464952 */
	u32 chunk_size;
	u32 format;	/* WAVE,0X45564157 */
};

struct chunk_fmt {
	u32 chunk_id;		/* "fmt ",0X20746D66 */
	u32 chunk_size;
	u16 audio_format;	/* 0X01,PCM; 0X11,IMA ADPCM */
	u16 num_of_channels;	/* 1,single track; 2,dual track */
	u32 sample_rate;	/* 0X1F40,表示8Khz */
	u32 byte_rate;
	u16 block_align;
	u16 bits_per_sample;
};

struct chunk_fact {
	u32 chunk_id;	/* "fact",0X74636166 */
	u32 chunk_size;
	u32 num_of_samples;
};

struct chunk_data {
	u32 chunk_id;	/* "data",0X61746164 */
	u32 chunk_size;
};

struct wavctrl {
	u16 audioformat;
	u16 nchannels;
	u16 blockalign;
	u32 datasize;

	u32 totsec;
	u32 cursec;

	u32 bitrate;
	u32 samplerate;
	u16 bps;	/* 16bit,24bit,32bit */

	u32 datastart;
};

#define RIFF_CHUNKID			0X46464952 /* RIFF */
#define RIFF_FORMAT			0X45564157 /* WAVE */

#define FMT_CHUNKID			0X20746D66 /* fmt  */
#define FMT_AUDIOFORMAT_PCM		0x01
#define FMT_AUDIOFORMAT_IMA_ADPCM	0x11

#define FACT_CHUNKID			0X74636166 /* fact */
#define DATA_CHUNKID			0X61746164 /* data */

void test_wavplay(void);
#endif
