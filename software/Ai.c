#include "common.h"
#include "tinymaix.h"
#include "Ai.h"
#include "canvas.h"

#include "model.h"

uint8_t xdata mdl_buf[MDL_BUF_LEN];
uint8_t xdata input_image[28 * 28] = {0};
// 这里可以用bitmap优化，但是没必要，内存够用

tm_mdl_t mdl;
tm_mat_t in_uint8;
tm_mat_t in;
tm_mat_t outs[1];
tm_err_t res;

static tm_err_t layer_cb(tm_mdl_t *mdl, tml_head_t *lh)
{
#if TM_ENABLE_STAT
	// dump middle result
	int x, y, c;
	int h = lh->out_dims[1];
	int w = lh->out_dims[2];
	int ch = lh->out_dims[3];
	mtype_t *output = TML_GET_OUTPUT(mdl, lh);
	return TM_OK;
	TM_PRINTF("Layer %d callback ========\n", mdl->layer_i);
#if 1
	for (y = 0; y < h; y++)
	{
		TM_PRINTF("[");
		for (x = 0; x < w; x++)
		{
			TM_PRINTF("[");
			for (c = 0; c < ch; c++)
			{
#if TM_MDL_TYPE == TM_MDL_FP32
				TM_PRINTF("%.3f,", output[(y * w + x) * ch + c]);
#else
				TM_PRINTF("%.3f,", TML_DEQUANT(lh, output[(y * w + x) * ch + c]));
#endif
			}
			TM_PRINTF("],");
		}
		TM_PRINTF("],\n");
	}
	TM_PRINTF("\n");
#endif
#endif
	return TM_OK;
}

void clean_input_image()
{
	memset(input_image, 0, 28 * 28 * sizeof(input_image[0]));
}

// 处理模型输出的结果，返回识别出的数字
uint8_t parse_output(tm_mat_t *outs)
{
	tm_mat_t out = outs[0];
	float *dat = (float *)out.dat;
	float maxp = 0;
	int maxi = -1;
	int i = 0;
	for (; i < CLASS_N; i++)
	{
		if (dat[i] > maxp)
		{
			maxi = i;
			maxp = dat[i];
		}
	}
	TM_PRINTF("### Predict output is: Number %d , Prob %.3f\r\n", maxi, maxp);
	return maxi;
}

void Ai_init()
{
	in_uint8.dims = 3;
	in_uint8.h = IMG_L;
	in_uint8.w = IMG_L;
	in_uint8.c = IMG_CH;
	in_uint8.dat = (mtype_t *)input_image;

	in.dims = 3;
	in.h = IMG_L;
	in.w = IMG_L;
	in.c = IMG_CH;
	in.dat = NULL;

	res = tm_load(&mdl, mdl_data, mdl_buf, layer_cb, &in);
	if (res != TM_OK)
	{
		TM_PRINTF("tm model load err %d\r\n", res);
		return;
	}
}

uint8_t Ai_run()
{
	tm_preprocess(&mdl, TMPP_UINT2INT, &in_uint8, &in);
	tm_run(&mdl, &in, outs);
	return parse_output(outs);
}
