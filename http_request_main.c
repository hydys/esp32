/* HTTP GET Example using plain POSIX sockets

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
//#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
//#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD
#define EXAMPLE_WIFI_SSID 	"Xiaomi_657E"//CONFIG_WIFI_SSID//"xiaoxiao"//
#define EXAMPLE_WIFI_PASS 	"350186585"//CONFIG_WIFI_PASSWORD//

#define SERVER_IP "192.168.31.110"

static const char *TAG = "example";

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

unsigned char which_io=0;


static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

static void http_get_task(void *pvParameters)
{
	#if 0
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
	#endif
	struct sockaddr_in local_addr,client_addr;
	struct hostent *host;
    int s, r;
    char recv_buf[64];

    while(1) {
        /* Wait for the callback to set the CONNECTED_BIT in the
           event group.
        */
        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                            false, true, portMAX_DELAY);
        ESP_LOGI(TAG, "Connected to AP");

		
		//socket()
		s = socket(AF_INET, SOCK_STREAM, 0);
		if(s<0)
		{
			//freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_RATE_MS);
            continue;			
		}
		local_addr.sin_family = AF_INET;
		local_addr.sin_port = htons(6666);
		local_addr.sin_addr.s_addr = INADDR_ANY;
		//bind(s,name,namelen)
		if(bind(s,(struct sockaddr*)&local_addr,sizeof(struct sockaddr))==-1)
		{
			//memset(local_addr,0,sizeof(struct sockaddr_in));
			bzero(&local_addr, sizeof(struct sockaddr_in));
			ESP_LOGE(TAG, "... Failed to bind!");
			continue;
		}
		//listen()
		if(listen(s,5)==-1)
		{
			ESP_LOGE(TAG, "... Failed to listen!");
			continue;
		}
		ESP_LOGE(TAG, "... pass to listen!");
		while(1)
		{
			unsigned int size;
			int con;
			size = sizeof(struct sockaddr);
			con=accept(s,(struct sockaddr*)&client_addr,&size);
			if(con<=0)
			{
				ESP_LOGE(TAG, "... Failed to accept!");
				//break;
				continue;
			}
			ESP_LOGE(TAG, "... get to read11!");
			while(1)
			{
				socklen_t len;
				bzero(recv_buf, sizeof(recv_buf));
				len= read(con, recv_buf, sizeof(recv_buf)-1);
				if(len<=0)
				{
					ESP_LOGE(TAG, "... Failed to read!");
					close(con);//
					break;
				}
				if(len >=20)
				{
					close(con);//
					break;					
				}
            	for(int i = 0; i < len; i++) 
				{
                	putchar(recv_buf[i]);
            	}
				ESP_LOGE(TAG, "... start to send!");

				if(strstr(recv_buf,"LED1") != NULL)
				{
					which_io = 0;
				}
				else if(strstr(recv_buf,"LED2") != NULL)
				{
					which_io = 1;
				}
				send(con,recv_buf,strlen(recv_buf), 0);
				vTaskDelay(20 / portTICK_RATE_MS);
			}	
		}
    }
}

#define BLINK_GPIO 21//CONFIG_BLINK_GPIO

#define LED1 21
#define LED2 22
#include "driver/gpio.h"

void blink_task(void *pvParameter)
{    
	/* 
	Configure the IOMUX register for pad BLINK_GPIO (some pads are      
	muxed to GPIO on reset already, but some default to other       
	functions and need to be switched to GPIO. Consult the       
	Technical Reference for a list of pads and their default       functions.)   
	*/    
	
	unsigned char io_num=LED1;
	gpio_pad_select_gpio(LED1);  
	gpio_pad_select_gpio(LED2);
	/* Set the GPIO as a push/pull output */    
	gpio_set_direction(LED1, GPIO_MODE_OUTPUT);  
	gpio_set_direction(LED2, GPIO_MODE_OUTPUT); 
	while(1) 
	{        
	/* Blink off (output low) */ 
	switch(which_io)
	{
		case 0:
			io_num = LED1;
			break;
		case 1:
			io_num =LED2;
			break;
		default:
			io_num = LED1;
			break;
		
	}
	gpio_set_level(io_num, 0);        
	vTaskDelay(1000 / portTICK_RATE_MS);        
	/* Blink on (output high) */        
	gpio_set_level(io_num, 1);        
	vTaskDelay(1000 / portTICK_RATE_MS);    
	}
}

void app_main()
{
    nvs_flash_init();
    system_init();
    initialise_wifi();
    xTaskCreate(&http_get_task, "http_get_task", 2048, NULL, 5, NULL);
	xTaskCreate(&blink_task, "blink_task", 512, NULL, 5, NULL);
}
