#ifndef __USER_THREAD_H
#define __USER_THREAD_H


#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include "rtdbg.h"

#include "collect_handle.h"
#include <rtdevice.h>
#include "drv_pin.h"
#include "onenet.h"
#include "stdio.h"
#include "string.h"
#include "stdbool.h"

#define ONENET_TOPIC_DP "$sys/" ONENET_INFO_PROID "/" ONENET_INFO_DEVID "/thing/property/post"


#define LED_PIN        ((3*32)+12)


void RT_Theard_start(void);


#endif
