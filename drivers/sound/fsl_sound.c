// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019-2021 NXP
 */

#include <common.h>
#include <audio_codec.h>
#include <dm.h>
#include <i2s.h>
#include <sound.h>

static int fsl_sound_setup(struct udevice *dev)
{
	struct sound_uc_priv *uc_priv = dev_get_uclass_priv(dev);
	struct i2s_uc_priv *i2s_priv = dev_get_uclass_priv(uc_priv->i2s);

	return audio_codec_set_params(uc_priv->codec, i2s_priv->id,
				      i2s_priv->samplingrate,
				      i2s_priv->samplingrate * i2s_priv->rfs,
				      i2s_priv->bitspersample,
				      i2s_priv->channels);
}

static int fsl_sound_play(struct udevice *dev, void *data, uint data_size)
{
	struct sound_uc_priv *uc_priv = dev_get_uclass_priv(dev);

	return i2s_tx_data(uc_priv->i2s, data, data_size);
}

static int fsl_sound_probe(struct udevice *dev)
{
	struct sound_uc_priv *uc_priv = dev_get_uclass_priv(dev);
	int ret;

	ret = uclass_get_device_by_phandle(UCLASS_AUDIO_CODEC, dev,
					   "audio-codec", &uc_priv->codec);
	if (ret) {
		printf("Failed to probe audio codec\n");
		return ret;
	}

	ret = uclass_get_device_by_phandle(UCLASS_I2S, dev, "audio-cpu",
					   &uc_priv->i2s);
	if (ret) {
		printf("Cannot find i2s: %d\n", ret);
		return ret;
	}
	printf("Probed sound '%s' with codec '%s' and i2s '%s'\n", dev->name,
	       uc_priv->codec->name, uc_priv->i2s->name);

	return 0;
}

static const struct sound_ops fsl_sound_ops = {
	.setup	= fsl_sound_setup,
	.play	= fsl_sound_play,
};

static const struct udevice_id fsl_sound_ids[] = {
	{ .compatible = "fsl,audio-sgtl5000" },
	{ }
};

U_BOOT_DRIVER(fsl_sound) = {
	.name		= "fsl_sound",
	.id		= UCLASS_SOUND,
	.of_match	= fsl_sound_ids,
	.probe		= fsl_sound_probe,
	.ops		= &fsl_sound_ops,
};
