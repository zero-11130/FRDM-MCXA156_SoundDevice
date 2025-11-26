/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 * Copyright (c) 2019-2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-10-24     Magicoe      first version
 * 2020-01-10     Kevin/Karl   Add PS demo
 * 2020-09-21     supperthomas fix the main.c
 *
 */
 
#include "user_thread.h"



//void send_temperature_test(void)
//{
//	rt_err_t send_result;
//	const char *ds_name = "temp";
//	uint8_t temp_value = 18;
//	
//	send_result = onenet_mqtt_upload_digit(ds_name, temp_value);
//	if(send_result == 0)
//	{
//		LOG_I("send OK");
//	}
//	else 
//	{
//		LOG_E("send err");
//	}
//}




int main(void)
{
#if defined(__CC_ARM)
    rt_kprintf("using armcc, version: %d\n", __ARMCC_VERSION);
#elif defined(__clang__)
    rt_kprintf("using armclang, version: %d\n", __ARMCC_VERSION);
#elif defined(__ICCARM__)
    rt_kprintf("using iccarm, version: %d\n", __VER__);
#elif defined(__GNUC__)
    rt_kprintf("using gcc, version: %d.%d\n", __GNUC__, __GNUC_MINOR__);
#endif

    rt_kprintf("MCXA156 HelloWorld\r\n");
	RT_Theard_start();
	
    while (1)
    {
		
        rt_thread_mdelay(1000);               /* Delay 1000mS */
    }
}

// end file
