/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "driver/gpio.h"
#include "protocol_examples_common.h"
#include "esp_wifi.h"

#include "DHT22.h"

#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"

#include <time.h>

/**
 * Brief:
 * This test code shows how to configure gpio and how to use gpio interrupt.
 *
 * GPIO status:
 * GPIO18: output
 * GPIO19: output
 * GPIO4:  input, pulled up, interrupt from rising edge and falling edge
 * GPIO5:  input, pulled up, interrupt from rising edge.
 *
 * Test:
 * Connect GPIO18 with GPIO4
 * Connect GPIO19 with GPIO5
 * Generate pulses on GPIO18/19, that triggers interrupt on GPIO4/5
 *
 */

#define GPIO_OUTPUT_IO_0    2
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0))
#define ESP_INTR_FLAG_DEFAULT 0
#define ON 1
#define OFF 0
#define LED_CHANGE_DELAY_MS    250
#define DHT_PIN GPIO_NUM_4

static const char *TAG = "dht22";

typedef struct {
    esp_mqtt_client_handle_t client;
} mqtt_task_params_t;

// Set your local broker URI
#define BROKER_URI "mqtts://mosquitto.daiot.com.ar:8883"

extern const uint8_t client_cert_pem_start[] asm("_binary_client_crt_start");
extern const uint8_t client_cert_pem_end[] asm("_binary_client_crt_end");
extern const uint8_t client_key_pem_start[] asm("_binary_client_key_start");
extern const uint8_t client_key_pem_end[] asm("_binary_client_key_end");
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_crt_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_crt_end");

// Función para obtener los milisegundos desde 1970
unsigned long long getTime() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (unsigned long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void led_toggle_task(void* arg)
{
    static uint8_t led_state = OFF;

    while(true) {
        if (led_state == OFF) {
            led_state = ON;
            gpio_set_level(GPIO_OUTPUT_IO_0, ON);
        }
        else {
                led_state = OFF;
                gpio_set_level(GPIO_OUTPUT_IO_0, OFF);
        }

        vTaskDelay(LED_CHANGE_DELAY_MS / portTICK_PERIOD_MS);
        //printf("Toggle LED\n");
    }
}


static void counter_task(void* arg)
{
    int cnt = 0;
    while(true) {
        printf("Ejemplo TASKS AdP_2022 - Counts: %d\n", cnt++);
        //vTaskDelay(1000 / portTICK_RATE_MS);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

void DHT_task(void *pvParameter)
{
	setDHTgpio( 4 );
	printf( "Starting DHT Task\n\n");

	while(1) {
	
		printf("=== Reading DHT ===\n" );
		int ret = readDHT();
		
		errorHandler(ret);

		printf( "Hum %.1f\n", getHumidity() );
		printf( "Tmp %.1f\n", getTemperature() );
		
		// -- wait at least 2 sec before reading again ------------
		// The interval of whole process must be beyond 2 seconds !! 
		//vTaskDelay( 3000 / portTICK_RATE_MS );
        vTaskDelay( 10000 / portTICK_PERIOD_MS );
	}
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    // ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    // int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    // case MQTT_EVENT_CONNECTED:
    //      {
    //         ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    //         break;
    //     }
    // case MQTT_EVENT_DISCONNECTED:
    //     ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
    //     break;

    case MQTT_EVENT_SUBSCRIBED:
        // ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        // msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    // case MQTT_EVENT_UNSUBSCRIBED:
    //     ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
    //     break;
    // case MQTT_EVENT_PUBLISHED:
    //     ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
    //     break;
    case MQTT_EVENT_DATA:
        // ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    // case MQTT_EVENT_ERROR:
    //     // ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
    //     if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
    //         log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
    //         log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
    //         log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
    //         ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

    //     }
    //     break;
    default:
        printf("Other event id:%d", event->event_id);
        // ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

// static void mqtt_app_start(void)
// {
//     const esp_mqtt_client_config_t mqtt_cfg = {
//         .broker.address.uri = BROKER_URI,
//         // .broker.verification = (const char *)client_cert_pem_start,
//         // .broker.client_cert_pem = (const char *)client_cert_pem_start,
//         // .broker.client_key_pem = (const char *)client_key_pem_start,
//         // .broker.cert_pem = (const char *)server_cert_pem_start,
//         // .uri = BROKER_URI,
//         // .client_cert_pem = (const char *)client_cert_pem_start,
//         // .client_key_pem = (const char *)client_key_pem_start,
//         // .cert_pem = (const char *)server_cert_pem_start,
//     };

//     // ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
//     esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
//     /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
//     esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
//     esp_mqtt_client_start(client);

//     // ESP_ERROR_CHECK(i2cdev_init());

//     mqtt_task_params_t task_params = {
//         .client = client
//     };

//     // xTaskCreate(&http_get_task, "http_get_task", 4096,&task_params, 5, NULL);



// }

void app_main(void)
{
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    //start toggle led task
    xTaskCreate(led_toggle_task, "led_toggle_task", 2048, NULL, 10, NULL);

    //start counter task
    xTaskCreate(counter_task, "counter_task", 2048, NULL, 10, NULL);

    //printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());
    printf("Minimum free heap size: %lu bytes\n", esp_get_minimum_free_heap_size());


	xTaskCreate( &DHT_task, "DHT_task", 2048, NULL, 5, NULL );

    
    
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());

    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    ESP_ERROR_CHECK(example_connect());

    // mqtt_app_start();
    // const esp_mqtt_client_config_t mqtt_cfg = {
    //     .broker.address.uri = BROKER_URI,
    //     .broker.verification.certificate = (const char *)server_cert_pem_start,
    //     .credentials.authentication.certificate = (const char *)client_cert_pem_start,
    //     .credentials.authentication.key = (const char *)client_key_pem_start,
    // };

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = BROKER_URI,
        .broker.verification.skip_cert_common_name_check = true,
        .broker.verification.certificate = (const char *)server_cert_pem_start,
        .credentials.authentication.certificate = (const char *)client_cert_pem_start,
        .credentials.authentication.key = (const char *)client_key_pem_start,
        .credentials.username = "daiot",
        .credentials.authentication.password = "daiot",
    };

    // ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    //ESP_ERROR_CHECK(example_connect());

    // ESP_ERROR_CHECK(i2cdev_init());

    mqtt_task_params_t task_params = {
        .client = client
    };

    while(1) {
        // Crear un objeto JSON
        cJSON *root = cJSON_CreateObject();
        //cJSON_AddStringToObject(root, "type", "message");
        cJSON_AddNumberToObject(root, "dispositivoId", 1);
        cJSON_AddStringToObject(root, "ubicacion", "Terraza");
        cJSON_AddNumberToObject(root, "temperatura", getTemperature());
        cJSON_AddNumberToObject(root, "humedad", getHumidity());
        cJSON_AddNumberToObject(root, "ts", getTime());
        cJSON_AddNumberToObject(root, "nodoId", 0);

        // Convertir el objeto JSON a una cadena
        const char *json_string = cJSON_Print(root);

        printf("json_string %s\n", json_string);
        // printf( "Hum %.1f\n", getHumidity() );
        // Publicar el JSON
        esp_mqtt_client_publish(client, "/test_node/", json_string, 0, 1, 0);

        // Liberar la memoria utilizada por el objeto JSON
        cJSON_Delete(root);
        free((void*)json_string); // cJSON_Print uses malloc

        //vTaskDelay(1000 / portTICK_RATE_MS);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        // idle
    }
}
