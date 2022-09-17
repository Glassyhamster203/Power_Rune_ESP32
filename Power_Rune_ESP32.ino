
#include <WiFiUdp.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_attr.h"
#include "driver/rmt.h"
#include "led_strip.h"

#define LED_STRIP_COLUMN 8
#define LED_STRIP_ROW 40
#define EXTRA_LED_NUM 100
#define LED_REFRESH_TIMEOUT 10
#define HIT_SENSOR_PORT GPIO_NUM_33
#define selfId 1        //ESP32 ID
#define allId 6
#define AD_Num 3000

typedef struct RGBW_COLOR {
	uint32_t red;
	uint32_t green;
	uint32_t blue;
}RGBW_COLOR;

/*RX2 主灯条*/
rmt_config_t _rmt_config = {
	_rmt_config.rmt_mode = RMT_MODE_TX,
	_rmt_config.channel = RMT_CHANNEL_0,
	_rmt_config.gpio_num = GPIO_NUM_16,
	_rmt_config.clk_div = 80,
	_rmt_config.mem_block_num = 1,
	_rmt_config.flags = 0,
	_rmt_config.tx_config = {
		_rmt_config.tx_config.carrier_freq_hz = 38000,
		_rmt_config.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH,
		_rmt_config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW,
		_rmt_config.tx_config.carrier_duty_percent = 33,
		_rmt_config.tx_config.carrier_en = false,
		_rmt_config.tx_config.loop_en = false,
		_rmt_config.tx_config.idle_output_en = true,
	}
};

/*TX2 装甲板灯条*/
rmt_config_t armor_rmt_config = {
	armor_rmt_config.rmt_mode = RMT_MODE_TX,
	armor_rmt_config.channel = RMT_CHANNEL_1,
	armor_rmt_config.gpio_num = GPIO_NUM_17,      //27->17
	armor_rmt_config.clk_div = 80,
	armor_rmt_config.mem_block_num = 1,
	armor_rmt_config.flags = 0,
	armor_rmt_config.tx_config = {
		armor_rmt_config.tx_config.carrier_freq_hz = 38000,
		armor_rmt_config.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH,
		armor_rmt_config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW,
		armor_rmt_config.tx_config.carrier_duty_percent = 33,
		armor_rmt_config.tx_config.carrier_en = false,
		armor_rmt_config.tx_config.loop_en = false,
		armor_rmt_config.tx_config.idle_output_en = true,
	}
};

/*D21 侧灯条*/
rmt_config_t side_rmt_config = {
	side_rmt_config.rmt_mode = RMT_MODE_TX,
	side_rmt_config.channel = RMT_CHANNEL_4,
	side_rmt_config.gpio_num = GPIO_NUM_21,
	side_rmt_config.clk_div = 80,
	side_rmt_config.mem_block_num = 1,
	side_rmt_config.flags = 0,
	side_rmt_config.tx_config = {
		side_rmt_config.tx_config.carrier_freq_hz = 38000,
		side_rmt_config.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH,
		side_rmt_config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW,
		side_rmt_config.tx_config.carrier_duty_percent = 33,
		side_rmt_config.tx_config.carrier_en = false,
		side_rmt_config.tx_config.loop_en = false,
		side_rmt_config.tx_config.idle_output_en = true,
	}
};

RGBW_COLOR WS2812[LED_STRIP_ROW][LED_STRIP_COLUMN];
led_strip_t* main_strip;
led_strip_t* armor_strip;
led_strip_t* side_strip;
int udpPort = 2333;
int returnUdpPort = 2334;
WiFiUDP returnUdp;
char* ssid = (char*)"robomasterserver";   //添加了(char*)
char* password = (char*)"12345678";       //
int startFlag = 0;
int statusFlag = 0;
int R_default = 0;
int G_default = 0;
int B_default = 100;


void init_led();
void ws2812_refresh();
void ws2812_set_all(uint16_t Red, uint16_t Green, uint16_t Blue);
void ws2812_reset();
void ws2812_test();
const void broadcastUdpData(String data);

static void main_task(void* arg)
{
	ws2812_reset();
	uint delta = 0;
	uint row_count = 0;
	int hitFlag = 0;
	while (true)
	{
		if (startFlag)
		{
			if (!hitFlag)
			{
				/*
				000111000
				000011100
				000001110
				000000111
				000000111
				000001110
				000011100
				000111000
				*/
				switch ((row_count - delta) % 8)
				{
				case 0:
				case 1:
					for (int t = 0; t < LED_STRIP_COLUMN; t++)
					{
						WS2812[row_count][t].red = 0;
						WS2812[row_count][t].green = 0;
						WS2812[row_count][t].blue = 0;
					}
					break;
				case 2:
				{
					WS2812[row_count][0].blue = B_default;
					WS2812[row_count][7].blue = B_default;

					WS2812[row_count][0].red = R_default;
					WS2812[row_count][7].red = R_default;

					WS2812[row_count][0].green = G_default;
					WS2812[row_count][7].green = G_default;
				}
				break;
				case 3:
				{
					WS2812[row_count][0].blue = B_default;
					WS2812[row_count][1].blue = B_default;
					WS2812[row_count][6].blue = B_default;
					WS2812[row_count][7].blue = B_default;

					WS2812[row_count][0].green = G_default;
					WS2812[row_count][1].green = G_default;
					WS2812[row_count][6].green = G_default;
					WS2812[row_count][7].green = G_default;

					WS2812[row_count][0].red = R_default;
					WS2812[row_count][1].red = R_default;
					WS2812[row_count][6].red = R_default;
					WS2812[row_count][7].red = R_default;

				}
				break;
				case 4:
				{
					WS2812[row_count][0].blue = B_default;
					WS2812[row_count][1].blue = B_default;
					WS2812[row_count][2].blue = B_default;
					WS2812[row_count][5].blue = B_default;
					WS2812[row_count][6].blue = B_default;
					WS2812[row_count][7].blue = B_default;

					WS2812[row_count][0].green = G_default;
					WS2812[row_count][1].green = G_default;
					WS2812[row_count][2].green = G_default;
					WS2812[row_count][5].green = G_default;
					WS2812[row_count][6].green = G_default;
					WS2812[row_count][7].green = G_default;

					WS2812[row_count][0].red = R_default;
					WS2812[row_count][1].red = R_default;
					WS2812[row_count][2].red = R_default;
					WS2812[row_count][5].red = R_default;
					WS2812[row_count][6].red = R_default;
					WS2812[row_count][7].red = R_default;
				}
				break;
				case 5:
				{
					WS2812[row_count][1].blue = B_default;
					WS2812[row_count][2].blue = B_default;
					WS2812[row_count][3].blue = B_default;
					WS2812[row_count][4].blue = B_default;
					WS2812[row_count][5].blue = B_default;
					WS2812[row_count][6].blue = B_default;

					WS2812[row_count][1].green = G_default;
					WS2812[row_count][2].green = G_default;
					WS2812[row_count][3].green = G_default;
					WS2812[row_count][4].green = G_default;
					WS2812[row_count][5].green = G_default;
					WS2812[row_count][6].green = G_default;

					WS2812[row_count][1].red = R_default;
					WS2812[row_count][2].red = R_default;
					WS2812[row_count][3].red = R_default;
					WS2812[row_count][4].red = R_default;
					WS2812[row_count][5].red = R_default;
					WS2812[row_count][6].red = R_default;					
				}
				break;
				case 6:
				{
					WS2812[row_count][2].blue = B_default;
					WS2812[row_count][3].blue = B_default;
					WS2812[row_count][4].blue = B_default;
					WS2812[row_count][5].blue = B_default;

					WS2812[row_count][2].green = G_default;
					WS2812[row_count][3].green = G_default;
					WS2812[row_count][4].green = G_default;
					WS2812[row_count][5].green = G_default;

					WS2812[row_count][2].red = R_default;
					WS2812[row_count][3].red = R_default;
					WS2812[row_count][4].red = R_default;
					WS2812[row_count][5].red = R_default;
				}
				break;
				case 7:
				{
					WS2812[row_count][3].blue = B_default;
					WS2812[row_count][4].blue = B_default;

					WS2812[row_count][3].green = G_default;
					WS2812[row_count][4].green = G_default;

					WS2812[row_count][3].red = R_default;
					WS2812[row_count][4].red = R_default;
				}
				break;
				default:
					break;
				}
				Serial.println("rowcount+");
				row_count++;
				if (row_count == LED_STRIP_ROW)
				{
					Serial.println("ttt");
					ws2812_refresh();
					row_count = 0;
					delta++;
					for (int i = 0; i < LED_STRIP_ROW; i++)
					{
						for (int j = 0; j < LED_STRIP_COLUMN; j++)
						{
							WS2812[i][j].blue = 0;
							WS2812[i][j].green = 0;
							WS2812[i][j].red = 0;
						}
					}
				}
				if (delta == LED_STRIP_ROW) delta = 0;
				vTaskDelay(5 / portTICK_RATE_MS);
				if (analogRead(HIT_SENSOR_PORT) >= AD_Num)
				{
					hitFlag = 1;
					Serial.println("hit");
					ws2812_set_all(R_default, G_default, B_default);
					ws2812_EXset_all(armor_strip, R_default, G_default, B_default);
					ws2812_EXset_all(side_strip, R_default, G_default, B_default);
					// vTaskDelay(2000/portTICK_RATE_MS);
					startFlag = 0;
					broadcastUdpData("{\"id\":" + String(selfId) + ",\"status\":\"hit\"}");
				}
				else
				{
					//broadcastUdpData("{\"id\":" + String(selfId) + ",\"status\":\"waiting\"}");
				}
			}
		}
		else
		{
			hitFlag = 0;
		}
		vTaskDelay(1 / portTICK_RATE_MS);
	}
}

static void net_task(void* arg)
{
	WiFiUDP myUdp;
	myUdp.begin(udpPort);
	Serial.println("net task");
  char SerialFlag;
	while (1)
	{
//		int packageSize = myUdp.parsePacket();
		String udpData = "";
		if( Serial.available() )	//(packageSize)
		{
			Serial.println("data R");
			udpData = Serial.read();	//myUdp.readString();
			if (udpData != "")
			{
				Serial.println(udpData);
				DynamicJsonDocument json(1024);
				DeserializationError err = deserializeJson(json, udpData);
				if (err)
				{
					Serial.println("json ERR");
					continue;
				}
				else
				{
					int targetId = json["id"].as<int>();
          Serial.println(targetId);   //始终为零
					if (targetId == selfId || targetId == allId)
					{
            Serial.println("Flag1");  //跑不到
						if (json["command"] == "start"&&!startFlag)
						{
							startFlag = 1;
							ws2812_EXset_all(armor_strip, R_default, G_default, B_default);
							Serial.println("start");
						}
						if (json["command"] == "stop")
						{
							startFlag = 0;
							ws2812_reset();
							ws2812_EXset_all(armor_strip, 0, 0, 0);
							ws2812_EXset_all(side_strip, 0, 0, 0);
							broadcastUdpData("{\"id\":" + String(selfId) + ",\"status\":\"off\"}");
						}
						if (json["command"] == "on")
						{
							startFlag = 0;
							ws2812_set_all(R_default, G_default, B_default);
							ws2812_EXset_all(armor_strip, R_default, G_default, B_default);
							ws2812_EXset_all(side_strip, R_default, G_default, B_default);
							broadcastUdpData("{\"id\":" + String(selfId) + ",\"status\":\"on\"}");
						}
						if (json["command"] == "color")
						{
							startFlag = 0;
							R_default = json["R"].as<int>();
							G_default = json["G"].as<int>();
							B_default = json["B"].as<int>();
							broadcastUdpData("{\"id\":" + String(selfId) + ",\"status\":\"color changed\"}");
							Serial.println(String(R_default)+String(G_default)+String(B_default));
						}
					}
					//process Data Here
				}
			}
			vTaskDelay(100 / portTICK_RATE_MS);
			myUdp.flush();
		}
		vTaskDelay(100 / portTICK_RATE_MS);
	}
}



void setup() {
	delay(10);
	Serial.begin(115200);

	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
	Serial.println("booting");
	//while (WiFi.status() != WL_CONNECTED) {
	//	Serial.print(".");
	//	delay(500);
	//}
	returnUdp.begin(returnUdpPort);
	pinMode(HIT_SENSOR_PORT, INPUT);
	//init_led();
	init_led();
	ws2812_test();
	delay(100);
	//ws2812_test();
	Serial.println("booted");
	broadcastUdpData("booted");
	xTaskCreatePinnedToCore(net_task, "net_task", 4096, NULL, 4, NULL, 0);
	//startFlag=1;
	xTaskCreatePinnedToCore(main_task, "main_task", 2048, NULL, 8, NULL, 1);
	//xTaskCreate(cedeng_task, "cedeng_task", 2048, NULL, 12, NULL);
	//xTaskCreate(main_task, "main_task", 2048, NULL, 8, NULL);

}

// the loop function runs over and over again until power down or reset
void loop() {
	//vTaskDelay(100/portTICK_RATE_MS);
}

//checked
void init_led()
{
	// set counter clock to 40MHz
	_rmt_config.clk_div = 2;
	armor_rmt_config.clk_div = 2;
	side_rmt_config.clk_div = 2;
	ESP_ERROR_CHECK(rmt_config(&_rmt_config));
	ESP_ERROR_CHECK(rmt_driver_install(_rmt_config.channel, 0, 0));
	delay(50);
	ESP_ERROR_CHECK(rmt_config(&armor_rmt_config));
	ESP_ERROR_CHECK(rmt_driver_install(armor_rmt_config.channel, 0, 0));
	delay(50);
	ESP_ERROR_CHECK(rmt_config(&side_rmt_config));
	rmt_driver_install(side_rmt_config.channel, 0, 0);
	delay(50);
	// install ws2812 driver
	led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(LED_STRIP_COLUMN * LED_STRIP_ROW, (led_strip_dev_t)_rmt_config.channel);
	led_strip_config_t armor_strip_config = LED_STRIP_DEFAULT_CONFIG(EXTRA_LED_NUM, (led_strip_dev_t)armor_rmt_config.channel);
	led_strip_config_t side_strip_config = LED_STRIP_DEFAULT_CONFIG(EXTRA_LED_NUM, (led_strip_dev_t)side_rmt_config.channel);
	main_strip = led_strip_new_rmt_ws2812(&strip_config);
	armor_strip = led_strip_new_rmt_ws2812(&armor_strip_config);
	side_strip = led_strip_new_rmt_ws2812(&side_strip_config);
	if (!main_strip||!armor_strip||!side_strip)
	{
		ESP_LOGE(TAG, "install WS2812 driver failed");
	}
	// Clear LED strip (turn off all LEDs)
	ESP_ERROR_CHECK(main_strip->clear(main_strip, 100));
	ESP_ERROR_CHECK(armor_strip->clear(armor_strip, 100));
	ESP_ERROR_CHECK(side_strip->clear(side_strip, 100));
}

//checked
void ws2812_set_all(uint16_t Red, uint16_t Green, uint16_t Blue)
{
	for (int i = 0; i < LED_STRIP_ROW; i++)
	{
		for (int j = 0; j < LED_STRIP_COLUMN; j++)
		{
			WS2812[i][j].red = Red;
			WS2812[i][j].green = Green;
			WS2812[i][j].blue = Blue;
		}
	}
	ws2812_refresh();
}
//checked
void ws2812_refresh()
{
	for (int i = 0; i < LED_STRIP_ROW; i++)
	{
		for (int j = 0; j < LED_STRIP_COLUMN; j++)
		{
			main_strip->set_pixel(main_strip, (uint32_t)j + i * LED_STRIP_COLUMN, WS2812[i][j].red, WS2812[i][j].green, WS2812[i][j].blue);
		}
	}
	main_strip->refresh(main_strip, LED_REFRESH_TIMEOUT);
}

//only for side &armor
void ws2812_EXset_all(led_strip_t* strip,uint16_t R,uint16_t G,uint16_t B)
{
	strip->set_pixel(strip,EXTRA_LED_NUM,R,G,B);
	strip->refresh(strip, LED_REFRESH_TIMEOUT);
}


//checked
void ws2812_reset()
{
	for (int i = 0; i < LED_STRIP_ROW; i++)
	{
		for (int j = 0; j < LED_STRIP_COLUMN; j++)
		{
			WS2812[i][j].blue = 0;
			WS2812[i][j].green = 0;
			WS2812[i][j].red = 0;
		}
	}
	ws2812_refresh();
}

//checked
void ws2812_test()
{
	for (int i = 0; i < LED_STRIP_ROW; i++)
	{
		for (int j = 0; j < LED_STRIP_COLUMN; j++)
		{
			WS2812[i][j].red = 0;
			WS2812[i][j].green = 0;
			WS2812[i][j].blue = 10;
			ws2812_refresh();
			delay(10);
		}
	}
	ws2812_reset();
}

//checked
const void broadcastUdpData(String data)
{
	//while(!returnUdp.availableForWrite()){vTaskDelay(100 / portTICK_RATE_MS);}
	Serial.println("UDP SEND:" + data);
	returnUdp.beginPacket("255.255.255.255", returnUdpPort);
	returnUdp.print(data);
	returnUdp.endPacket();
}
