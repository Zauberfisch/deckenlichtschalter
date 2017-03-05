// (c) Bernhard Tittelbach, 2016
package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"os"
	"os/signal"
	"strings"
	"syscall"
	"time"

	pubsub "github.com/btittelbach/pubsub"
	mqtt "github.com/eclipse/paho.mqtt.golang"
	"github.com/realraum/door_and_sensors/r3events"
)

const (
	DEFAULT_GOLIGHTCTRL_MQTTBROKER     string = "tcp://mqtt.realraum.at:1883"
	DEFAULT_GOLIGHTCTRL_HTTP_INTERFACE string = ":80"
	DEFAULT_GOLIGHTCTRL_RF433TTYDEV    string = "/dev/ttyACM0"
	DEFAULT_GOLIGHTCTRL_BUTTONTTYDEV   string = "/dev/ttyACM1"
)

type SerialLine []byte

var (
	UseFakeGPIO_ bool
	DebugFlags_  string
	ps_          *pubsub.PubSub
)

func init() {
	flag.BoolVar(&UseFakeGPIO_, "fakegpio", false, "For testing")
	flag.StringVar(&DebugFlags_, "debug", "", "List of DebugFlags separated by ,")
	ps_ = pubsub.New(50)
}

//Connect and keep trying to connect to MQTT Broker
//And while we cannot, still provide as much functionality as possible
func goConnectToMQTTBrokerAndFunctionWithoutInTheMeantime() {
	//Start Channel Gobblers and Functionality that works without mqtt
	//These shut down once we send PS_SHUTDOWN_CONSUMER
	//consume MQTT_ledpattern_chan_ and MQTT_ir_chan_
	go func() {
		shutdown_c := ps_.SubOnce(PS_SHUTDOWN_CONSUMER)
		for {
			select {
			case <-shutdown_c:
				return
			case <-MQTT_ledpattern_chan_:
			case <-MQTT_ir_chan_:
			case <-MQTT_fancylight_chan_:
			}
		}
	}()
	//
	//
	//now try connect to mqtt daemon until it works once
	for {
		mqttc := ConnectMQTTBroker(EnvironOrDefault("GOLIGHTCTRL_MQTTBROKER", DEFAULT_GOLIGHTCTRL_MQTTBROKER), EnvironOrDefault("GOLIGHTCTRL_CLIENTID", r3events.CLIENTID_LIGHTCTRL))
		//start real goroutines after mqtt connected
		if mqttc != nil {
			SubscribeAndAttachCallback(mqttc, r3events.ACT_PIPELEDS_PATTERN, func(c mqtt.Client, msg mqtt.Message) {
				var lp r3events.SetPipeLEDsPattern
				if msg.Retained() {
					return
				}
				if err := json.Unmarshal(msg.Payload(), &lp); err != nil {
					//TODO: retain lates state somewhere and broadcast it to all websocket clients
				}
			})
			receive_fancylight_state_updates := func(clientid string, c mqtt.Client, msg mqtt.Message) {
				var fancy wsMsgFancyLight
				fancy.Name = clientid
				if msg.Retained() {
					return
				}
				if err := json.Unmarshal(msg.Payload(), &fancy.Setting); err != nil {
					//TODO: retain lates state somewhere and broadcast it to all websocket clients
				}
			}
			for _, cid := range []string{r3events.CLIENTID_CEILING1, r3events.CLIENTID_CEILING2, r3events.CLIENTID_CEILING3, r3events.CLIENTID_CEILING4, r3events.CLIENTID_CEILING5, r3events.CLIENTID_CEILING6, r3events.CLIENTID_CEILING7, r3events.CLIENTID_CEILING8, r3events.CLIENTID_CEILINGALL} {
				SubscribeAndAttachCallback(mqttc, r3events.TOPIC_ACTIONS+cid+"/"+r3events.TYPE_LIGHT, func(c mqtt.Client, msg mqtt.Message) { receive_fancylight_state_updates(cid, c, msg) })
			}
			ps_.Pub(true, PS_SHUTDOWN_CONSUMER) //shutdown all chan consumers for mqttc == nil
			time.Sleep(5 * time.Second)         //avoid goLinearizeRFSender that we start below to shutdown right away
			go goSendIRCmdToMQTT(mqttc, MQTT_ir_chan_)
			go goSetLEDPipePatternViaMQTT(mqttc, MQTT_ledpattern_chan_)
			go goSetFancyLightsViaMQTT(mqttc, MQTT_fancylight_chan_)
			//and LAST but not least:
			RequestStatusFromAllFancyLightsMQTT(mqttc)
			return // no need to keep on trying, mqtt-auto-reconnect will do the rest now
		} else {
			time.Sleep(5 * time.Minute)
		}
	}
}

func main() {
	flag.Parse()
	if len(DebugFlags_) > 0 {
		LogEnable(strings.Split(DebugFlags_, ",")...)
	}

	go goConnectToMQTTBrokerAndFunctionWithoutInTheMeantime()
	go goRunMartini()

	// wait on Ctrl-C or sigInt or sigKill
	func() {
		ctrlc_c := make(chan os.Signal, 1)
		signal.Notify(ctrlc_c, os.Interrupt, os.Kill, syscall.SIGTERM)
		<-ctrlc_c //block until ctrl+c is pressed || we receive SIGINT aka kill -1 || kill
		fmt.Println("SIGINT received, exiting gracefully ...")
	}()

}
