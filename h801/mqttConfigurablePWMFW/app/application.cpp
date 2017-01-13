#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Network/TelnetServer.h>
#include <defaultlightconfig.h>
#include <SmingCore/Debug.h>
#include <pwmchannels.h>
#include "application.h"
#ifdef ENABLE_SSL
	#include <ssl/private_key.h>
	#include <ssl/cert.h>
#endif

const uint32_t period = 5000; // * 200ns ^= 1 kHz

Timer flashTimer;
Timer procMQTTTimer;

////////////////////
//// PWM Stuff ////
///////////////////

//init PWM and restore stored pwm values
void setupPWM()
{
	// PWM setup
	uint32 io_info[PWM_CHANNELS][3] = {
		// MUX, FUNC, PIN
		{PERIPHS_IO_MUX_MTDI_U,  FUNC_GPIO12, 12}, //B-
		{PERIPHS_IO_MUX_MTDO_U,  FUNC_GPIO15, 15}, //R-
		{PERIPHS_IO_MUX_MTCK_U,  FUNC_GPIO13, 13}, //G-
		{PERIPHS_IO_MUX_MTMS_U,  FUNC_GPIO14, 14}, //W1-
		{PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4 ,  4}, //W2-
	};
	// uint32 pwm_duty_initial[PWM_CHANNELS] = {0, 0, 0, 0, 0};
	uint32 pwm_duty_initial[PWM_CHANNELS] = {0, 0, 0, 0, 0};

	DefaultLightConfig.load(pwm_duty_initial); //load initial default values

	pwm_init(period, pwm_duty_initial, PWM_CHANNELS, io_info);
	pwm_start();
}

///////////////////////////////////////////
//// Userfeedback and Flash-LEDs Stuff ////
///////////////////////////////////////////


uint32_t flashme_num = 0;
uint8_t flashme_channel = 0;
uint32_t flashme_origvalue = 0;

void flashMeNow()
{
	if (flashme_num > 0)
	{
		if (flashme_num % 2 == 0)
		{
			pwm_set_duty(period, flashme_channel);
			pwm_start();
		} else {
			pwm_set_duty(0, flashme_channel);
			pwm_start();
		}
		flashme_num--;
	} else {
		// stop timer
		flashTimer.stop();
		// restore values
		pwm_set_duty(flashme_origvalue, flashme_channel);
		pwm_start();
	}
}

void flashChannel(uint8_t times, uint8_t channel)
{
	flashme_channel = channel;
	flashme_origvalue = pwm_get_duty(channel);
	flashme_num = times*2;
	pwm_set_duty(0,channel);
	pwm_start();
	flashTimer.initializeMs(500, flashMeNow).start(); // every 500ms
}


///////////////////////////////////////
///// WIFI Stuff
///////////////////////////////////////

// Will be called when WiFi station was connected to AP
void wifiConnectOk()
{
	debugf("WiFi CONNECTED");
	//Serial.println(WifiStation.getIP().toString());
	startTelnetServer();
	startMqttClient();
	// Start publishing loop (also needed for mqtt reconnect)
	procMQTTTimer.initializeMs(20 * 1000, publishMessage).start(); // every 20 seconds
	flashChannel(1,CHAN_GREEN);
}

// Will be called when WiFi station timeout was reached
void wifiConnectFail()
{
	debugf("WiFi NOT CONNECTED!");

	flashChannel(1,CHAN_RED);

	WifiStation.waitConnection(wifiConnectOk, 10, wifiConnectFail); // Repeat and check again
}

void connectToWifi()
{
	debugf("connecting 2 WiFi");
	WifiAccessPoint.enable(false);
	WifiStation.enable(true);
	WifiStation.enableDHCP(NetConfig.enabledhcp);
	WifiStation.setHostname(NetConfig.mqtt_clientid+".realraum.at");
	WifiStation.config(NetConfig.wifi_ssid, NetConfig.wifi_pass); // Put you SSID and Password here
	WifiStation.setIP(NetConfig.ip,NetConfig.netmask,NetConfig.gw);

	// Run our method when station was connected to AP (or not connected)
	WifiStation.waitConnection(wifiConnectOk, 30, wifiConnectFail); // We recommend 20+ seconds at start
}

///////////////////////////////////////
///// Telnet Backup command interface
///////////////////////////////////////

void telnetCmdNetSettings(String commandLine  ,CommandOutput* commandOutput)
{
	Vector<String> commandToken;
	int numToken = splitString(commandLine, ' ' , commandToken);
	if (numToken != 3)
	{
		commandOutput->printf("Usage set ip|nm|gw|dhcp|wifissid|wifipass|mqttbroker|mqttport|mqttclientid|mqttuser|mqttpass <value>\r\n");
	}
	else if (commandToken[1] == "ip")
	{
		IPAddress newip(commandToken[2]);
		commandOutput->printf("%s: '%s'\r\n",commandToken[1].c_str(),newip.toString().c_str());
		if (!newip.isNull())
			NetConfig.ip = newip;
	}
	else if (commandToken[1] == "nm")
	{
		IPAddress newip(commandToken[2]);
		commandOutput->printf("%s: '%s'\r\n",commandToken[1].c_str(),newip.toString().c_str());
		if (!newip.isNull())
			NetConfig.netmask = newip;
	}
	else if (commandToken[1] == "gw")
	{
		IPAddress newip(commandToken[2]);
		commandOutput->printf("%s: '%s'\r\n",commandToken[1].c_str(),newip.toString().c_str());
		if (!newip.isNull())
			NetConfig.gw = newip;
	}
	else if (commandToken[1] == "wifissid")
	{
		commandOutput->printf("%s: '%s'\r\n",commandToken[1].c_str(),commandToken[2].c_str());
		NetConfig.wifi_ssid = commandToken[2];
	}
	else if (commandToken[1] == "wifipass")
	{
		commandOutput->printf("%s: '%s'\r\n",commandToken[1].c_str(),commandToken[2].c_str());
		NetConfig.wifi_pass = commandToken[2];
	}
	else if (commandToken[1] == "mqttbroker")
	{
		commandOutput->printf("%s: '%s'\r\n",commandToken[1].c_str(),commandToken[2].c_str());
		NetConfig.mqtt_broker = commandToken[2];
	}
	else if (commandToken[1] == "mqttport")
	{
		uint32_t newport = atoi(commandToken[2].c_str());
		commandOutput->printf("%s: '%d'\r\n",commandToken[1].c_str(),newport);
		if (newport > 0 && newport < 65536)
			NetConfig.mqtt_port = newport;
	}
	else if (commandToken[1] == "mqttclientid")
	{
		commandOutput->printf("%s: '%s'\r\n",commandToken[1].c_str(),commandToken[2].c_str());
		NetConfig.mqtt_clientid = commandToken[2];
	}
	else if (commandToken[1] == "mqttuser")
	{
		commandOutput->printf("%s: '%s'\r\n",commandToken[1].c_str(),commandToken[2].c_str());
		NetConfig.mqtt_user = commandToken[2];
	}
	else if (commandToken[1] == "mqttpass")
	{
		commandOutput->printf("%s: '%s'\r\n",commandToken[1].c_str(),commandToken[2].c_str());
		NetConfig.mqtt_pass = commandToken[2];
	}
	else if (commandToken[1] == "dhcp")
	{
		NetConfig.enabledhcp = commandToken[2] == "1" || commandToken[2] == "true" || commandToken[2] == "yes" || commandToken[2] == "on";
		commandOutput->printf("%s: '%s'\r\n",commandToken[1].c_str(),(NetConfig.enabledhcp)?"on":"off");
	} else {
		commandOutput->printf("Invalid subcommand. Try %s list\r\n", commandToken[0].c_str());
	}
}

void telnetCmdPrint(String commandLine  ,CommandOutput* commandOutput)
{
	commandOutput->println("Dumping Configuration");
	commandOutput->println("WiFi SSID: " + NetConfig.wifi_ssid);
	commandOutput->println("WiFi Pass: " + NetConfig.wifi_pass);
	commandOutput->println("IP: " + NetConfig.ip.toString());
	commandOutput->println("NM: " + NetConfig.netmask.toString());
	commandOutput->println("GW: " + NetConfig.gw.toString());
	commandOutput->println((NetConfig.enabledhcp)?"DHCP: on":"DHCP: off");
	commandOutput->println("MQTT Broker: " + NetConfig.mqtt_broker + ":" + String(NetConfig.mqtt_port));
	commandOutput->println("MQTT ClientID: " + NetConfig.mqtt_clientid);
	commandOutput->println("MQTT Login: " + NetConfig.mqtt_user +"/"+ NetConfig.mqtt_pass);
}


void telnetCmdSave(String commandLine  ,CommandOutput* commandOutput)
{
	commandOutput->println("OK, saving values...");
	NetConfig.save();
}

void telnetCmdLs(String commandLine  ,CommandOutput* commandOutput)
{
	Vector<String> list = fileList();
	for (int i = 0; i < list.count(); i++)
		commandOutput->println(String(fileGetSize(list[i])) + " " + list[i]);

}

void telnetCmdCatFile(String commandLine  ,CommandOutput* commandOutput)
{
	Vector<String> commandToken;
	int numToken = splitString(commandLine, ' ' , commandToken);

	if (numToken != 2)
	{
		commandOutput->println("Usage: cat <file>");
		return;
	}
	if (fileExist(commandToken[1]))
	{
		commandOutput->println("Contents of "+commandToken[1]);
		commandOutput->println(fileGetContent(commandToken[1]));
	} else {
		commandOutput->println("File '"+commandToken[1]+"' does not exist");
	}
}

void telnetCmdLoad(String commandLine  ,CommandOutput* commandOutput)
{
	commandOutput->printf("OK, reloading values...\r\n");
	NetConfig.load();
}

void telnetCmdReboot(String commandLine  ,CommandOutput* commandOutput)
{
	commandOutput->printf("OK, restarting...\r\n");
	telnetServer.flush();
	telnetServer.close();
	System.restart();
}

void startTelnetServer()
{
	telnetServer.listen(2323);
	telnetServer.enableCommand(true);
	//TODO: use encryption and client authentification
#ifdef ENABLE_SSL
	telnetServer.addSslOptions(SSL_SERVER_VERIFY_LATER);
	telnetServer.setSslClientKeyCert(default_private_key, default_private_key_len,
							  default_certificate, default_certificate_len, NULL, true);
	telnetServer.useSsl = true;
#endif
}

//////////////////////////////////
/////// MQTT Stuff ///////////////
//////////////////////////////////


// Check for MQTT Disconnection
void checkMQTTDisconnect(TcpClient& client, bool flag){

	// Called whenever MQTT connection is failed.
	if (flag == true)
	{
		//Serial.println("MQTT Broker Disconnected!!");
		flashChannel(2,CHAN_RED);
	}
	else
	{
		//Serial.println("MQTT Broker Unreachable!!");
		flashChannel(3,CHAN_RED);
	}

	// Restart connection attempt after few seconds
	// changes procMQTTTimer callback function
	procMQTTTimer.initializeMs(2 * 1000, startMqttClient).start(); // every 2 seconds
}

void onMessageDelivered(uint16_t msgId, int type) {
	//Serial.printf("Message with id %d and QoS %d was delivered successfully.", msgId, (type==MQTT_MSG_PUBREC? 2: 1));
}

// Publish our message
void publishMessage()
{
	if (mqtt->getConnectionState() != eTCS_Connected)
		startMqttClient(); // Auto reconnect

	//Serial.println("Let's publish message now!");
	//mqtt->publish("main/frameworks/sming", "Hello friends, from Internet of things :)");

	//mqtt->publishWithQoS("important/frameworks/sming", "Request Return Delivery", 1, false, onMessageDelivered); // or publishWithQoS
}

inline void setArrayFromKey(JsonObject& root, uint32_t a[5], String key, uint8_t pwm_channel)
{
	if (root.containsKey(key) && root[key].is<unsigned int>())
	{
		a[pwm_channel] = ((uint32_t)root[key]) * period/1000;
		if (a[pwm_channel] > period)
			a[pwm_channel] = period;
	}
}

inline void setPWMDutyFromKey(JsonObject& root, String key, uint8_t pwm_channel)
{
	if (root.containsKey(key) && root[key].is<unsigned int>())
	{
		uint32_t newvalue = ((uint32_t)root[key])*period/1000;
		if (newvalue > period)
			newvalue = period;
		pwm_set_duty(newvalue, pwm_channel);
	}
}

// Callback for messages, arrived from MQTT server
void onMessageReceived(String topic, String message)
{
	//Serial.print(topic);
	//Serial.print(":\r\n\t"); // Pretify alignment for printing
	//Serial.println(message);

	StaticJsonBuffer<200> jsonBuffer;

	JsonObject& root = jsonBuffer.parseObject(message);

	if (!root.success())
	{
	  //Serial.println("JSON parseObject() failed");
	  return;
	}

	if (topic.endsWith("/light"))
	{
		setPWMDutyFromKey(root, JSONKEY_RED, CHAN_RED);
		setPWMDutyFromKey(root, JSONKEY_GREEN, CHAN_GREEN);
		setPWMDutyFromKey(root, JSONKEY_BLUE, CHAN_BLUE);
		setPWMDutyFromKey(root, JSONKEY_CW, CHAN_CW);
		setPWMDutyFromKey(root, JSONKEY_WW, CHAN_WW);
		pwm_start();
	} else if (topic.endsWith("/defaultlight"))
	{
		uint32_t pwm_duty_default[PWM_CHANNELS] = {0,0,0,0,0};
		setArrayFromKey(root, pwm_duty_default, JSONKEY_RED, CHAN_RED);
		setArrayFromKey(root, pwm_duty_default, JSONKEY_GREEN, CHAN_GREEN);
		setArrayFromKey(root, pwm_duty_default, JSONKEY_BLUE, CHAN_BLUE);
		setArrayFromKey(root, pwm_duty_default, JSONKEY_CW, CHAN_CW);
		setArrayFromKey(root, pwm_duty_default, JSONKEY_WW, CHAN_WW);
		DefaultLightConfig.save(pwm_duty_default);
		flashChannel(1,CHAN_BLUE);
	}
}

// Run MQTT client, connect to server, subscribe topics
void startMqttClient()
{
	procMQTTTimer.stop();
/*	if(!mqtt->setWill("last/will","The connection from this device is lost:(", 1, true)) {
		debugf("Unable to set the last will and testament. Most probably there is not enough memory on the device.");
	}
*/
	mqtt->connect(NetConfig.mqtt_clientid, NetConfig.mqtt_user, NetConfig.mqtt_pass, true);
	mqtt->setKeepAlive(42);
	mqtt->setPingRepeatTime(21);
#ifdef ENABLE_SSL
	mqtt->addSslOptions(SSL_SERVER_VERIFY_LATER);

	mqtt->setSslClientKeyCert(default_private_key, default_private_key_len,
							  default_certificate, default_certificate_len, NULL, true);

#endif
	// Assign a disconnect callback function
	mqtt->setCompleteDelegate(checkMQTTDisconnect);
	mqtt->subscribe("action/ceilingAll/light");
	mqtt->subscribe("action/ceilingAll/defaultlight");
	mqtt->subscribe(String("action/")+NetConfig.mqtt_clientid+"/light");
	mqtt->subscribe(String("action/")+NetConfig.mqtt_clientid+"/defaultlight");
}

//////////////////////////////////////
////// Base System Stuff  ////////////
//////////////////////////////////////


// Will be called when WiFi hardware and software initialization was finished
// And system initialization was completed
void ready()
{
	setupPWM(); //also loads previously saved default settings
	NetConfig.load(); //loads netsettings from fs
	//Serial.println(NetConfig.wifi_ssid);
	//Serial.println(NetConfig.wifi_pass);
	//Serial.println(NetConfig.ip.toString());
	mqtt = new MqttClient(NetConfig.mqtt_broker, NetConfig.mqtt_port);
	connectToWifi();
}

void init()
{
	//Serial.begin(115200);
	//Serial.systemDebugOutput(true); // Allow debug print to serial
	spiffs_mount(); // Mount file system, in order to work with files
	commandHandler.registerCommand(CommandDelegate("set","Change network settings","configGroup", telnetCmdNetSettings));
	commandHandler.registerCommand(CommandDelegate("save","Save network settings","configGroup", telnetCmdSave));
	commandHandler.registerCommand(CommandDelegate("load","Save network settings","configGroup", telnetCmdSave));
	commandHandler.registerCommand(CommandDelegate("show","Show network settings","configGroup", telnetCmdPrint));
	commandHandler.registerCommand(CommandDelegate("ls","List files","configGroup", telnetCmdLs));
	commandHandler.registerCommand(CommandDelegate("cat","List files","configGroup", telnetCmdCatFile));
	commandHandler.registerCommand(CommandDelegate("restart","restart ESP8266","systemGroup", telnetCmdReboot));
	commandHandler.registerSystemCommands();
	//Serial.commandProcessing(true);
	// Set system ready callback method
	System.onReady(ready);
}