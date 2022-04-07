#include "rtthread.h"
#include "string.h"
#include "sys/time.h"
#include "liz_bsp.h"
#include "interface.h"
#include "spectrum_gui.h"
#include "spectrum.h"
#include "adc_fft.h"
#include "liz_config.h"
#define DBG_TAG    "liz.app.spectrum"
#if defined(LIZ_APP_SPECTRUM_DEBUG)
    #define DBG_LVL    DBG_LOG
#else
    #define DBG_LVL    DBG_INFO
#endif
#include <rtdbg.h>
// 0 - 500Hz  500Hz - 4KHz  4K-20k 20 20 8
//
/* 显示频率表 freq/40 因为fft得到的频谱的间隔是40Hz */
// uint16_t display_freq_table[NPT] = {40 / 40, 40 / 40, 80 / 40, 80 / 40, 120 / 40, 120 / 40, \
//                                     160 / 40, 160 / 40, 200 / 40, 240 / 40, 280 / 40, 320 / 40, \
//                                     360 / 40, 400 / 40, 480 / 40, 520 / 40, 600 / 40, 640 / 40, \
//                                     720 / 40, 760 / 40, 840 / 40, 880 / 40, 960 / 40, 1000 / 40, \
//                                     1200 / 40, 1240 / 40, 1360 / 40, 1400 / 40, 1640 / 40, 1680 / 40, \
//                                     1920 / 40, 1960 / 40, 2200 / 40, 2240 / 40, 2320 / 40, 2360 / 40, \
//                                     5000 / 40, 5080 / 40, 6000 / 40, 6080 / 40, 7000 / 40, 7080 / 40, \
//                                     8000 / 40, 8080 / 40, 9000 / 40, 9080 / 40, 9320 / 40, 9420 / 40
//                                    };
float fft_freq_bost[NPT] = {0.80f, 0.80f, 0.80f, 0.80f, 0.80f, 0.80f, \
                            0.80f, 0.80f, 0.80f, 0.80f, 0.80f, 0.80f, \
                            1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, \
                            1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, \
                            1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, \
                            1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, \
                            1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, \
                            1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 
                           };
#define SPECTRUM_GAIN 1
// uint16_t display_freq_table[NPT]={40/40,40/40,80/40,80/40,120/40,120/40, \
//                              160/40,160/40,200/40,240/40,280/40,320/40, \
//                              320/40,360/40,400/40,400/40,440/40,440/40, \
//                              480/40,520/40, \
//                              680/40,840/40,1000/40,1160/40,1320/40,1480/40, \
//                              1640/40,1800/40,1960/40,2120/40,2280/40,2440/40, \
//                              2600/40,2760/40,2920/40,3080/40,3240/40,3400/40, \
//                              3560/40,3720/40, \
//                              3920/40,4120/40,5120/40,6120/40,7120/40,8120/40,9120/40,10120/40};

static rt_thread_t info_update_tid = RT_NULL;

static int app_exit_flag = 0; //app退出标志

static void adc_fft_finish_cb(float *buf, int32_t len);
static void data_sperad(float *data);
static void generate_spectrum(float *fft_buf, int32_t len);
static float fft_add(float *fft_bin, int from, int to);

static uint8_t is_init = 0;

static void spectrum_init()
{
    if (is_init) return;
    is_init = 1;
    adc_fft_init(adc_fft_finish_cb);
    spectrum_gui_init();
}

static void adc_fft_finish_cb(float *buf, int32_t len)
{
    generate_spectrum(buf, len);
}

static void spectrum_load(lv_scr_load_anim_t anim_type)
{
    spectrum_gui_load(anim_type);
    adc_fft_start();
}
static void process(struct APP_CONTROLLER *sys, struct _app_action_info *act_info)
{

}
static void exit_callback()
{
    adc_fft_stop();
    spectrum_gui_unload();
}

/**
 * @brief fft累加函数
 * @param fft_bin 原始数据数组 from 起始 to 结束
 * @return 累加值
 */
static float fft_add(float *fft_bin, int from, int to)
{
    int i = from;
    float result = 0;
    while (i <= to)
    {
        result += fft_bin[i++];
    }
    return result;
}

static void data_sperad(float *data)
{
    float val = *data;
    if (val < 5)
    {
        val *= 0.2;
    }
    else if (val < 10)
    {
        val *= 0.7;
    }
    else if (val < 20)
    {
        val *= 0.8;
    }
    else if (val < 30)
    {
        val *= 1;
    }
    else if (val < 40)
    {
        val *= 1.1;
    }
    else if (val < 50)
    {
        val *= 1.1;
    }
    else if (val < 60)
    {
        val *= 1.2;
    }
    else if (val < 70)
    {
        val *= 1.1;
    }
    else if (val > 70)
    {
        val = 70;
    }
    *data = val;
}
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

static void generate_spectrum(float *fft_buf, int32_t len)
{

    double tmep_height = 0.0;
    target_height[0]  = (fft_add(fft_buf, 2, 2)) / 1;    // 40-120Hz
    target_height[1]  = (fft_add(fft_buf, 2, 3)) / 2;    // 80-120Hz
    target_height[2]  = (fft_add(fft_buf, 3, 4)) / 2;    // 120-140Hz
    target_height[3]  = (fft_add(fft_buf, 4, 5)) / 2;    // 160-160Hz
    target_height[4]  = (fft_add(fft_buf, 5, 6)) / 2;    // 200-180Hz
    target_height[5]  = (fft_add(fft_buf, 6, 7)) / 2;    // 240-200Hz
    target_height[6]  = (fft_add(fft_buf, 7, 8)) / 2;   // 280-220Hz
    target_height[7]  = (fft_add(fft_buf, 8, 9)) / 2;  // 320-240Hz
    target_height[8]  = (fft_add(fft_buf, 9, 10)) / 2;  // 360-260Hz
    target_height[9]  = (fft_add(fft_buf, 10, 11)) / 2;  // 400-280Hz
    target_height[10] = (fft_add(fft_buf, 11, 12)) / 2;  // 440-300Hz
    target_height[11] = (fft_add(fft_buf, 12, 13)) / 2;  // 480-320Hz
    target_height[12] = (fft_add(fft_buf, 13, 14)) / 2;  // 520-340Hz
    target_height[13] = (fft_add(fft_buf, 14, 15)) / 2;  // 560-360Hz
    target_height[14] = (fft_add(fft_buf, 15, 16)) / 2;  // 600-380Hz
    target_height[15] = (fft_add(fft_buf, 16, 17)) / 2;  // 640-400Hz
    target_height[16] = (fft_add(fft_buf, 17, 18)) / 2;  // 680-420Hz
    target_height[17] = (fft_add(fft_buf, 18, 19)) / 2;  // 720-440Hz
    target_height[18] = (fft_add(fft_buf, 19, 20)) / 2;  // 760-460Hz
    target_height[19] = (fft_add(fft_buf, 20, 21)) / 2;  // 800-480Hz
    target_height[20] = (fft_add(fft_buf, 21, 22)) / 2;  // 840-500Hz
    target_height[21] = (fft_add(fft_buf, 22, 23)) / 2;  // 880-520Hz

    target_height[22] = (fft_add(fft_buf, 23, 24)) / 2;  // 920-560Hz
    target_height[23] = (fft_add(fft_buf, 24, 25)) / 2;  // 1000-600Hz
    target_height[24] = (fft_add(fft_buf, 25, 26)) / 2;  // 1080-640Hz
    target_height[25] = (fft_add(fft_buf, 26, 27)) / 2;  // 1160-680Hz
    target_height[26] = (fft_add(fft_buf, 27, 28)) / 2;  // 660-720Hz
    target_height[27] = (fft_add(fft_buf, 28, 29)) / 2;  // 700-760Hz
    target_height[28] = (fft_add(fft_buf, 29, 30)) / 2;  // 1160-800Hz

    target_height[29] = (fft_add(fft_buf, 30, 32)) / 3;  // 780-880Hz
    target_height[30] = (fft_add(fft_buf, 32, 34)) / 3;  // 860-960Hz
    target_height[31] = (fft_add(fft_buf, 34, 36)) / 3;  // 940-1040Hz

    target_height[32] = (fft_add(fft_buf, 36, 40)) / 5;  // 1100-1240Hz
    target_height[33] = (fft_add(fft_buf, 40, 44)) / 5;  // 1220-1360Hz
    target_height[34] = (fft_add(fft_buf, 44, 48)) / 5;  // 1340-1480Hz

    target_height[35] = (fft_add(fft_buf, 48, 54)) / 7;  // 1580-1760Hz
    target_height[36] = (fft_add(fft_buf, 54, 60)) / 7;  // 1740-1920Hz
    target_height[37] = (fft_add(fft_buf, 60, 66)) / 7;  // 1900-2080Hz

    target_height[38] = (fft_add(fft_buf, 66, 74)) / 9;  // 2060-2280Hz
    target_height[39] = (fft_add(fft_buf, 74, 82)) / 9;  // 2260-2480Hz

    target_height[40] = (fft_add(fft_buf, 82, 92)) / 11;  // 2460-2740Hz
    target_height[41] = (fft_add(fft_buf, 92, 102)) / 11;  // 2720-3000Hz

    target_height[42] = (fft_add(fft_buf, 102, 115)) / 14;  // 2980-3360Hz
    target_height[43] = (fft_add(fft_buf, 115, 128)) / 14;  // 3340-3720Hz

    target_height[44] = (fft_add(fft_buf, 128, 146)) / 19;  // 1020-1120Hz
    target_height[45] = (fft_add(fft_buf, 146, 164)) / 19;  // 1020-1120Hz
    target_height[46] = (fft_add(fft_buf, 164, 182)) / 19;  // 1020-1120Hz
    target_height[47] = (fft_add(fft_buf, 182, 200)) / 19;  // 1020-5460Hz
    uint8_t index = 0;
    float amp = 0.0f;
    float data ;
    for (int i = 0; i < NPT; i++)
    {
        tmep_height = target_height[i]  * fft_freq_bost[i] * SPECTRUM_GAIN / 64.0f;
        tmep_height = constrain(tmep_height,0,80);
        target_height[i] = tmep_height;
        // index = display_freq_table[i];
        // amp = fft_buf[index]; // 得到fft运算结果
        // /* h = amp^(1/2) * 2.8 / 47 * 9.2  */
        // arm_sqrt_f32(amp, &data);
        // data *= 2.8;
        // target_height[i] = data / 47 * 9.2; //(int) ((int)(data/48) * 9.2);
        // data_sperad(&target_height[i]);
    }
}

struct APP_OBJ spectrum_app =
{
    .name = "Spectrum",
    .image = RT_NULL,
    .info = "",
    .init = spectrum_init,
    .load = spectrum_load,
    .process = process,
    .exit_callback = exit_callback,
};
