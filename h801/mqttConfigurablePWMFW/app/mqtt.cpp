#include <SmingCore/SmingCore.h>
#include <defaultlightconfig.h>
#include <spiffsconfig.h>
#include "pwmchannels.h"
#include "lightcontrol.h"
#include "mqtt.h"

//////////////////////////////////
/////// MQTT Stuff ///////////////
//////////////////////////////////


Timer procMQTTTimer;
MqttClient *mqtt = nullptr;

String getMQTTTopic(String topic3, bool all=false)
{
	return MQTT_TOPIC1+((all) ? MQTT_TOPIC2_ALL : NetConfig.mqtt_clientid)+topic3;
}

#ifdef ENABLE_BUTTON
extern bool button_used_;
#endif

// Check for MQTT Disconnection
void checkMQTTDisconnect(TcpClient& client, bool flag){

	// Called whenever MQTT connection is failed.
	if (flag == true)
	{
		//Serial.println("MQTT Broker Disconnected!!");
		flashSingleChannel(2,CHAN_RED);
	}
#ifdef ENABLE_BUTTON
	else
	{
		//Serial.println("MQTT Broker Unreachable!!");
		if (!button_used_)
			flashSingleChannel(3,CHAN_RED);
	}
#endif

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

inline void setArrayFromKey(JsonObject& root, uint32_t a[PWM_CHANNELS], String key, uint8_t pwm_channel)
{
	//BUG: can't check type because .is is buggy and does not compile in Sming 3.0.1.
	//if (root.containsKey(key) && root[key].is<unsigned int>())
	if (root.containsKey(key))
	{
		uint32_t value = (uint32_t)root[key];
		if (value > NetConfig.chan_range[pwm_channel])
		{
			return;
		}
		a[pwm_channel] = value * pwm_period / NetConfig.chan_range[pwm_channel];
		if (a[pwm_channel] > pwm_period)
			a[pwm_channel] = pwm_period;
	} else
	{
		//this allows us to set the default to current light values by sending an empty json object to /defaultlight
		a[pwm_channel] = effect_target_values_[pwm_channel];
	}
}

inline void setPWMDutyFromKey(JsonObject& root, String key, uint8_t pwm_channel)
{
	//BUG: can't check type because .is is buggy and does not compile in Sming 3.0.1
	// if (root.containsKey(key) && root[key].is<unsigned int>())
	if (root.containsKey(key))
	{
		uint32_t value = (uint32_t)root[key];
		if (value > 1000)
		{
			return;
		}
		value = value * pwm_period / 1000;
		if (value > pwm_period)
			value = pwm_period;
		pwm_set_duty(value, pwm_channel);
	}
}

//if present and an array of strings, will forward entire json to next element in list
//minus that element in the list
void checkForwardInJsonAndSetCC(JsonObject& root, JsonObject& checkme)
{
	if (checkme.containsKey(JSONKEY_FORWARD))
	{
		JsonArray& cc_list = checkme[JSONKEY_FORWARD];
		if (cc_list.success() && cc_list.size() > 0)
		{
			mqtt_forward_to_ = cc_list.get<String>(0);
			cc_list.removeAt(0);
			root.printTo(mqtt_payload_);
		}
	}
}

#ifdef REPLACE_CW_WITH_UV
void simulateCWwithRGB(JsonObject& root, uint32_t a[PWM_CHANNELS])
{
	if (NetConfig.simulatecw_w_rgb)
	{
		if (root.containsKey(JSONKEY_CW))
		{
			uint32_t value = (uint32_t)root[JSONKEY_CW];
			if (value > NetConfig.chan_range[CHAN_BLUE])
			{
				return;
			}
			value = value * pwm_period / NetConfig.chan_range[CHAN_BLUE];
			if (value > pwm_period)
				value = pwm_period;
			a[CHAN_RED] = max(a[CHAN_RED],value);
			a[CHAN_GREEN] = max(a[CHAN_GREEN],value);
			a[CHAN_BLUE] = max(a[CHAN_BLUE],value);
		}
	}
}
#endif

void publishCurrentLightSetting(StaticJsonBuffer<1024> &jsonBuffer, String &message)
{
	if (nullptr == mqtt)
		return;
	JsonObject& root = jsonBuffer.createObject();
	root[JSONKEY_RED] = effect_target_values_[CHAN_RED] * NetConfig.chan_range[CHAN_RED] / pwm_period;
	root[JSONKEY_GREEN] = effect_target_values_[CHAN_GREEN] * NetConfig.chan_range[CHAN_GREEN] / pwm_period;
	root[JSONKEY_BLUE] = effect_target_values_[CHAN_BLUE] * NetConfig.chan_range[CHAN_BLUE] / pwm_period;
	// root[JSONKEY_CW] = effect_target_values_[CHAN_CW] * NetConfig.chan_range[CHAN_CW] / pwm_period;
#ifdef REPLACE_CW_WITH_UV
	root[JSONKEY_UV] = effect_target_values_[CHAN_UV] * NetConfig.chan_range[CHAN_UV] / pwm_period;
#else
	root[JSONKEY_CW] = effect_target_values_[CHAN_CW] * NetConfig.chan_range[CHAN_CW] / pwm_period;
#endif
	root[JSONKEY_WW] = effect_target_values_[CHAN_WW] * NetConfig.chan_range[CHAN_WW] / pwm_period;
	root.printTo(message);
	//publish to myself (where presumably everybody else also listens), the current settings
	mqtt->publish(getMQTTTopic(MQTT_TOPIC3_LIGHT), message, false);
}

void mqttPublishCurrentLightSetting()
{
	StaticJsonBuffer<1024> jsonBuffer;
	String message;
	publishCurrentLightSetting(jsonBuffer, message);
}

// Callback for messages, arrived from MQTT server
void onMessageReceived(String topic, String message)
{
	// debugf("topic: %s",topic.c_str());
	// debugf("msg: %s",message.c_str());
	//GRML BUG :-( It would be really nice to filter out retained messages,
	//             to avoid the light powering up, going into defaultlight settings, then getting wifi and switching to a retained /light setting
	//GRML :-( unfortunately we can't distinguish between retained and fresh messages here

	StaticJsonBuffer<1024> jsonBuffer;

	// pleaserepeat does not care about message content, thus it is checked before using the jsonBuffer for parsing
	// (as JsonBuffer should not be reused) This allows us to use that buffer for sending a message ourselves
	if (topic.endsWith(MQTT_TOPIC3_PLEASEREPEAT))
	{
		publishCurrentLightSetting(jsonBuffer, message);
		return; //return so we don't reuse the now used jsonBuffer
	}

	//use jsonBuffer to parse message
	JsonObject& root = jsonBuffer.parseObject(message);

	if (!root.success())
	{
	  //Serial.println("JSON parseObject() failed");
	  return;
	}

	if (topic.endsWith(MQTT_TOPIC3_LIGHT))
	{
		setArrayFromKey(root, effect_target_values_, JSONKEY_RED, CHAN_RED);
		setArrayFromKey(root, effect_target_values_, JSONKEY_GREEN, CHAN_GREEN);
		setArrayFromKey(root, effect_target_values_, JSONKEY_BLUE, CHAN_BLUE);
		setArrayFromKey(root, effect_target_values_, JSONKEY_WW, CHAN_WW);
#ifdef REPLACE_CW_WITH_UV
		setArrayFromKey(root, effect_target_values_, JSONKEY_UV, CHAN_UV);
		simulateCWwithRGB(root, effect_target_values_);
#else
		setArrayFromKey(root, effect_target_values_, JSONKEY_CW, CHAN_CW);
#endif
		//-----
		if (root.containsKey(JSONKEY_FLASH))
		{
			JsonObject& effectobj = root[JSONKEY_FLASH];
			uint32_t repetitions = DEFAULT_EFFECT_REPETITIONS;
			if (effectobj.containsKey(JSONKEY_REPETITIONS))
				repetitions = effectobj[JSONKEY_REPETITIONS];
			uint32_t period = DEFAULT_FLASH_PERIOD_;
			if (effectobj.containsKey(JSONKEY_PERIOD))
				period = effectobj[JSONKEY_PERIOD];
			checkForwardInJsonAndSetCC(root, effectobj);
			startFlash(repetitions, FLASH_INTERMED_ORIG, period);
		}
		else if (root.containsKey(JSONKEY_FADE))
		{
			JsonObject& effectobj = root[JSONKEY_FADE];
			uint32_t duration = DEFAULT_EFFECT_DURATION;
			if (effectobj.containsKey(JSONKEY_DURATION))
				duration = effectobj[JSONKEY_DURATION];
			checkForwardInJsonAndSetCC(root, effectobj);
			startFade(duration);
		} else
		{
			//apply Values right now
			stopAndRestoreValues(true); //disable any Effects
			applyValues(effect_target_values_); //apply light
		}
	} else if (topic.endsWith(MQTT_TOPIC3_DEFAULTLIGHT))
	{
		uint32_t pwm_duty_default[PWM_CHANNELS] = {0,0,0,0,0};
		setArrayFromKey(root, pwm_duty_default, JSONKEY_RED, CHAN_RED);
		setArrayFromKey(root, pwm_duty_default, JSONKEY_GREEN, CHAN_GREEN);
		setArrayFromKey(root, pwm_duty_default, JSONKEY_BLUE, CHAN_BLUE);
		setArrayFromKey(root, pwm_duty_default, JSONKEY_WW, CHAN_WW);
#ifdef REPLACE_CW_WITH_UV
		setArrayFromKey(root, pwm_duty_default, JSONKEY_UV, CHAN_UV);
		simulateCWwithRGB(root, pwm_duty_default);
#else
		setArrayFromKey(root, pwm_duty_default, JSONKEY_CW, CHAN_CW);
#endif
		DefaultLightConfig.save(pwm_duty_default);
		flashSingleChannel(1,CHAN_BLUE);
	} else if (topic.endsWith(MQTT_TOPIC3_BUTTONONLIGHT))
	{
		setArrayFromKey(root, button_on_values_, JSONKEY_RED, CHAN_RED);
		setArrayFromKey(root, button_on_values_, JSONKEY_GREEN, CHAN_GREEN);
		setArrayFromKey(root, button_on_values_, JSONKEY_BLUE, CHAN_BLUE);
		setArrayFromKey(root, button_on_values_, JSONKEY_WW, CHAN_WW);
#ifdef REPLACE_CW_WITH_UV
		setArrayFromKey(root, button_on_values_, JSONKEY_UV, CHAN_UV);
		simulateCWwithRGB(root, button_on_values_);
#else
		setArrayFromKey(root, button_on_values_, JSONKEY_CW, CHAN_CW);
#endif
		ButtonLightConfig.save(button_on_values_);
	}
}

// Run MQTT client, connect to server, subscribe topics
void startMqttClient()
{
	procMQTTTimer.stop();

	if (nullptr == mqtt)
		mqtt = new MqttClient(NetConfig.mqtt_broker, NetConfig.mqtt_port, onMessageReceived);

/*	if(!mqtt->setWill("last/will","The connection from this device is lost:(", 1, true)) {
		debugf("Unable to set the last will and testament. Most probably there is not enough memory on the device.");
	}
*/
	mqtt->setKeepAlive(42);
	mqtt->setPingRepeatTime(21);
	bool usessl=false;
#ifdef ENABLE_SSL
	usessl=true;
	mqtt->addSslOptions(SSL_SERVER_VERIFY_LATER);

	mqtt->setSslClientKeyCert(default_private_key, default_private_key_len,
							  default_certificate, default_certificate_len, NULL, true);
#endif

	//prepare last will
	StaticJsonBuffer<256> jsonBuffer;
	String message;
	JsonObject& root = jsonBuffer.createObject();
	root[JSONKEY_IP] = WifiStation.getIP().toString();
	root[JSONKEY_ONLINE] = false;
	root.printTo(message);
	mqtt->setWill(getMQTTTopic(MQTT_TOPIC3_DEVICEONLINE),message,0,true);

	// Assign a disconnect callback function
	mqtt->setCompleteDelegate(checkMQTTDisconnect);
	// debugf("connecting to to MQTT broker");
	mqtt->connect(NetConfig.mqtt_clientid, NetConfig.mqtt_user, NetConfig.mqtt_pass, true);
	// debugf("connected to MQTT broker");
	mqtt->subscribe(getMQTTTopic(MQTT_TOPIC3_LIGHT,true));
	mqtt->subscribe(getMQTTTopic(MQTT_TOPIC3_DEFAULTLIGHT,true));
	mqtt->subscribe(getMQTTTopic(MQTT_TOPIC3_PLEASEREPEAT,true));
	mqtt->subscribe(getMQTTTopic(MQTT_TOPIC3_LIGHT,false));
	mqtt->subscribe(getMQTTTopic(MQTT_TOPIC3_DEFAULTLIGHT,false));
	mqtt->subscribe(getMQTTTopic(MQTT_TOPIC3_PLEASEREPEAT,false));
#ifdef ENABLE_BUTTON
	mqtt->subscribe(getMQTTTopic(MQTT_TOPIC3_BUTTONONLIGHT,false));
#endif

	//publish fact that we are online
	root[JSONKEY_ONLINE] = true;
	message="";
	root.printTo(message);
	mqtt->publish(getMQTTTopic(MQTT_TOPIC3_DEVICEONLINE),message,true);

	//enable periodic status updates
	procMQTTTimer.initializeMs(20 * 1000, publishMessage).start(); // every 20 seconds
}

void stopMqttClient()
{
	mqtt->unsubscribe(getMQTTTopic(MQTT_TOPIC3_LIGHT,true));
	mqtt->unsubscribe(getMQTTTopic(MQTT_TOPIC3_DEFAULTLIGHT,true));
	mqtt->unsubscribe(getMQTTTopic(MQTT_TOPIC3_PLEASEREPEAT,true));
	mqtt->unsubscribe(getMQTTTopic(MQTT_TOPIC3_LIGHT,false));
	mqtt->unsubscribe(getMQTTTopic(MQTT_TOPIC3_DEFAULTLIGHT,false));
	mqtt->unsubscribe(getMQTTTopic(MQTT_TOPIC3_PLEASEREPEAT,false));
#ifdef ENABLE_BUTTON
	mqtt->unsubscribe(getMQTTTopic(MQTT_TOPIC3_BUTTONONLIGHT,false));
#endif
	mqtt->setKeepAlive(0);
	mqtt->setPingRepeatTime(0);
	procMQTTTimer.stop();
}

