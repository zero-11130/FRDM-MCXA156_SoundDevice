#include "user_thread.h"

/* Task1 任务 配置  获取ADC数据 */ 
#define TASK1_PRIORITY 9 
#define TASK1_STACK_DEPTH 1024 
rt_thread_t task1_handler; 
void Task1(void *pvParameters); 
 
/* Task2 任务 配置 向云端发送数据 */ 
#define TASK2_PRIORITY 10
#define TASK2_STACK_DEPTH 2048 
rt_thread_t task2_handler; 
void Task2(void *pvParameters); 
void send_temp_data(void);
uint8_t temp = 23;
uint8_t warning;

/* Task3 任务配置 处理任务一的数据 */ 
#define TASK3_PRIORITY 10 
#define TASK3_STACK_DEPTH 1024 
rt_thread_t task3_handler; 
void Task3(void *pvParameters);
rt_sem_t sem_1;
rt_sem_t sem_2;
void RT_Theard_start(void)
{
	
	/* 检查RT-Thread系统是否正常运行 */
    if (rt_thread_self() == RT_NULL)
    {
        rt_kprintf("RT-Thread not running!\r\n");
        return;
    }
	sem_1 = rt_sem_create("sem_1", 1, RT_IPC_FLAG_FIFO);
	if(sem_1 == RT_NULL)
	{
		LOG_E("sem_1 rt_sem_create failed!\r\n");

	}
	sem_2 = rt_sem_create("sem_2", 1, RT_IPC_FLAG_FIFO);
	if(sem_2 == RT_NULL)
	{
		LOG_E("sem_2 rt_sem_create failed!\r\n");

	}
	
	 /* 创建任务1 */
    task1_handler = rt_thread_create("Task1", Task1, NULL, TASK1_STACK_DEPTH, TASK1_PRIORITY, 10);
    if(task1_handler == RT_NULL)
    {
        rt_kprintf("Task1 create failed!\r\n");
        return;
    }
    
    /* 创建任务2 */
    task2_handler = rt_thread_create("Task2", Task2, NULL, TASK2_STACK_DEPTH, TASK2_PRIORITY, 10);
    if(task2_handler == RT_NULL)
    {
        rt_kprintf("Task2 create failed!\r\n");
        return;
    }
    /* 创建任务3 */
    task3_handler = rt_thread_create("Task3", Task3, NULL, TASK3_STACK_DEPTH, TASK3_PRIORITY, 10);
    if(task3_handler == RT_NULL)
    {
        rt_kprintf("Task3 create failed!\r\n");
        return;
    }
    
    /* 启动所有任务 */
    
    rt_thread_startup(task2_handler);
    rt_thread_startup(task1_handler);
	rt_thread_startup(task3_handler);
    rt_kprintf("All tasks started successfully!\r\n");
	
}


void Task1(void *pvParameters)
{

	/* Find ADC device */
    adc_dev = (rt_adc_device_t)rt_device_find(ADC_DEV_NAME);
    if (adc_dev == RT_NULL)
    {
        rt_kprintf("ADC sample failed! Cannot find device: %s\n", ADC_DEV_NAME);
    }
	/* Enable ADC channel */
    rt_adc_enable(adc_dev, ADC_DEV_CHANNEL);

    /* Initialize FFT structure */
    arm_cfft_radix4_init_f32(&scfft, FFT_LENGTH, 0, 1);

    rt_kprintf("MCXA156 Motor Sound Anomaly Detector Initialized\n");

    /* Calibrate normal motor baseline (run once at startup) */
    calibrate_normal_baseline();
	
	
	rt_kprintf("Task1 started\r\n");
	while(1)
	{
		/* Collect 1024 ADC samples */
        for (sample_cnt = 0; sample_cnt < FFT_LENGTH; sample_cnt++)
        {
            ADC_value[sample_cnt] = rt_adc_read(adc_dev, ADC_DEV_CHANNEL);
            rt_thread_mdelay(1);  /* 1ms sampling interval */
        }

        /* Analyze signal and calculate power */
        SignalAnalyzer_handler();
        rt_kprintf("Total Power: %.4f V2\n", totalPower);

        rt_sem_release(sem_1);

        rt_thread_mdelay(200);  /* 1s loop interval */
	}
	
}

void Task2(void *pvParameters)
{
	onenet_mqtt_init();
	rt_kprintf("Task2 started\r\n");
	while(1)
	{
		if(rt_sem_take(sem_2, 5) == RT_EOK)
		{
			send_temp_data();			
		}
		rt_thread_mdelay(100);
	}
}

void send_temp_data(void)
{
	char temp_str[256];
	if(motor_status == RT_TRUE)
	{
		warning = 0;
	}else{
		warning = 1;
	}
	snprintf(temp_str, sizeof(temp_str),
			"{\"id\":\"123\",\"params\":{\"temp\":{\"value\": %d},\"warning\":{\"value\": %d}}}",temp,warning
	);
	rt_err_t result = onenet_mqtt_publish(ONENET_TOPIC_DP, (uint8_t *)temp_str, strlen(temp_str));
	if(result != RT_EOK)
	{
		LOG_E("Failed to upload combined sensor data!");
	}else{
		LOG_I("Combined sensor data uploaded sucessull!");
	}
}

void Task3(void *pvParameters)
{
	
	rt_kprintf("Task3 started\r\n");
	while(1)
	{
		if(rt_sem_take(sem_1, 5) == RT_EOK)
		{
			/* Run anomaly detection */
			motor_abnormal_detect();
			rt_sem_release(sem_2);
		}	
		rt_thread_mdelay(100);	
	}
}