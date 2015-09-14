/* main.c -- MQTT client example
 *
 * Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * * Neither the name of Redis nor the names of its contributors may be used
 * to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "ets_sys.h"
#include "uart.h"
#include "osapi.h"
#include "mqtt.h"
#include "wifi.h"
#include "config.h"
#include "debug.h"
#include "gpio.h"
#include "user_interface.h"
#include "mem.h"
#include "gpio16.h"

MQTT_Client mqttClient;
#define DELAY 1000/* milliseconds */
LOCAL os_timer_t hello_timer;
uint32_t count = 0;
disconnectCounter = 0;
uint32_t messagesPublished = 0;
char mqttPrefix[30];

#include "Msg.h"
extern void MsgPump();
extern void MsgPublish(void* src, Signal signal);
extern void MsgInit(void* mqtt);

void wifiConnectCb(uint8_t status) {
	if (status == STATION_GOT_IP) {
		MQTT_Connect(&mqttClient);
	} else {
		MQTT_Disconnect(&mqttClient);

	}
}
void mqttConnectedCb(uint32_t *args) {
	MQTT_Client* client = (MQTT_Client*) args;
	info("MQTT: Connected");

	char topic[30];

	ets_sprintf(topic,"PUT%s/#",mqttPrefix);
	MQTT_Subscribe(client, topic, 0);

	ets_sprintf(topic,"PUT%s/realTime",mqttPrefix);
	MQTT_Publish(client, topic, "12234578", 6, 0, 0);

	MsgPublish(client,SIG_CONNECTED);

	os_timer_arm(&hello_timer, DELAY, 1);

}

void mqttDisconnectedCb(uint32_t *args) {
	MQTT_Client* client = (MQTT_Client*) args;
	info("MQTT: Disconnected");
	disconnectCounter++;
	os_timer_disarm(&hello_timer);
	MsgPublish(client,SIG_CONNECTED);
}

void mqttPublishedCb(uint32_t *args) {
	MQTT_Client* client = (MQTT_Client*) args;
	info("MQTT: Published");
}

void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len,
		const char *data, uint32_t data_len) {
	char *topicBuf = (char*) os_zalloc(topic_len + 1), *dataBuf =
			(char*) os_zalloc(data_len + 1);

	MQTT_Client* client = (MQTT_Client*) args;

	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;

	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;

	info("Receive topic: %s, data: %s ", topicBuf, dataBuf);
	os_free(topicBuf);
	os_free(dataBuf);
}

bool ledOn = true;

#include "util.h"
LOCAL void IRAM hello_cb(void *arg) {
	MsgPublish(OS_CLOCK,SIG_TICK);
	MsgPump();
	info("hello world ");
//	info("HELLO WORLD ");
//	debug(" DEBUG THE WORLD ");
/*
	if (ledOn) {
		gpio16_output_set(1);
		ledOn = false;
	} else {
		gpio16_output_set(0);
		ledOn = true;
	}*/

	char buf[100];
	char topic[30];
	info(" Message %d ", count);

	ets_sprintf(buf, "%d", count++);
	ets_sprintf(topic,"%s/count",mqttPrefix);
	MQTT_Publish(&mqttClient, topic, buf, strlen(buf), 0, 0);

	ets_sprintf(buf, "%d", disconnectCounter);
	ets_sprintf(topic,"%s/disconnectCounter",mqttPrefix);
	MQTT_Publish(&mqttClient, topic, buf, strlen(buf),
			0, 0);
	ets_sprintf(buf, "%d", messagesPublished);
	ets_sprintf(topic,"%s/messagesPublished",mqttPrefix);
	MQTT_Publish(&mqttClient, topic, buf, strlen(buf),
			0, 0);

	os_timer_arm(&hello_timer, DELAY, 1);
	messagesPublished++;
}

LOCAL os_timer_t tick_timer;
LOCAL void IRAM tick_cb(void *arg) {
	MsgPublish(2,SIG_TICK);
	MsgPump();
	disconnectCounter++;
}

IROM void user_init(void) {
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	gpio_init();
	gpio16_output_conf();
	os_delay_us(1000000);
	info("Starting");
	MsgInit(&mqttClient);
	ets_sprintf(mqttPrefix,"/ESP_%08X",system_get_chip_id());
	uart_div_modify(0, UART_CLK_FREQ / 115200);

	CFG_Load();

	MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port,
			sysCfg.security);
	//MQTT_InitConnection(&mqttClient, "192.168.11.122", 1880, 0);

	MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user,
			sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);
	//MQTT_InitClient(&mqttClient, "client_id", "user", "pass", 120, 1);

	MQTT_InitLWT(&mqttClient, "/lwt", "offline", 0, 0);
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	MQTT_OnData(&mqttClient, mqttDataCb);

	WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, wifiConnectCb);
	// Set up a timer to send the message
	os_timer_disarm(&hello_timer);

	os_timer_setfn(&hello_timer, (os_timer_func_t *) hello_cb, (void *) 0);

	os_timer_disarm(&tick_timer);
	os_timer_setfn(&tick_timer, (os_timer_func_t *) tick_cb, (void *) 0);
	os_timer_arm(&tick_timer,1,1); // 1 msec repeat

	info("System started ...");
}
