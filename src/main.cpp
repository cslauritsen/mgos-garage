/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <garage.hpp>
#include <string>
#include <list>
#include <iostream>
#include <memory>
#include <homie.hpp>


extern "C" {
// declared in build_info.c
extern char* build_version;
extern char* build_id;
extern char* build_timestamp;

#include "mgos.h"
#include "mgos_app.h"
#include "mgos_dlsym.h"
#include "mgos_hal.h"
#include "mgos_rpc.h"
//#include "mjs.h"
#include <mgos_gpio.h>
#include <mgos_sys_config.h>
#include <common/cs_dbg.h>
#include <mgos_app.h>
#include <mgos_system.h>
#include <mgos_timers.h>
#include <mgos_time.h>
#include <mgos_mqtt.h>
#include <mg_rpc_channel_loopback.h>

#ifdef __DATE__
#define BUILD_DATE __DATE__ 
#else
#define BUILD_DATE "?"
#endif

#ifdef __TIME__
#define BUILD_TIME __TIME__ 
#else
#define BUILD_TIME "?"
#endif
}

using namespace garage;

static void test_homie() {
    homie::Device *d = new homie::Device("device123", "My Device");
    d->setIpAddr("192.168.1.39");

    homie::Node *dht22Node  = new homie::Node(d, "dht22", "DHT22 Temp/RH Sensor", "DHT22");
    d->addNode(dht22Node);

    auto tempProp = new homie::Property(dht22Node, "tempf", "Temperature in Fahrenheit", homie::FLOAT, false);
    tempProp->setUnit(homie::DEGREE_SYMBOL + "F");
    float tempF = 72.0;
    tempProp->setValue(tempF);
    dht22Node->addProperty(tempProp);

    auto rhProp = new homie::Property(dht22Node, "rh", "Temperature in Fahrenheit", homie::FLOAT, false);
    rhProp->setUnit("%");
    float rh = 61.0;
    rhProp->setValue(rh);
    dht22Node->addProperty(rhProp);

    auto doorNode = new homie::Node(d, "doora", "South Garage Door", "door");
    d->addNode(doorNode);

    auto openProp = new homie::Property(doorNode, "isopen", "Is Door Open", homie::BOOLEAN, false);
    doorNode->addProperty(openProp);

    auto relayProp = new homie::Property(doorNode, "relay", "Door Activator", homie::INTEGER, true);
    doorNode->addProperty(relayProp);

    auto introduction = new std::list<homie::Message *>;
    d->introduce(introduction);
    for (auto msg : *introduction) {
        std::cout << msg->topic 
        << "[q:" << msg->qos << ",r:" << msg->retained << ']' 
        << " → " 
        << msg->payload << std::endl;
    }
    delete introduction;
    delete relayProp;
    delete openProp;
    delete rhProp;
    delete tempProp;
    delete dht22Node;
    delete doorNode;
    delete d;
}

static void sys_ip_cb(struct mg_rpc *c, void *cb_arg, struct mg_rpc_frame_info *fi, struct mg_str result, int error_code, struct mg_str error_msg) {
  Device *device = (Device*) cb_arg;
  LOG(LL_INFO, ("error code: %d", error_code));
  char *ip = NULL;
  char *id = NULL;
  int scan_result = json_scanf(result.p, result.len, "{id: %Q, wifi: {sta_ip: %Q}}", &id, &ip);

  if (id) {
    LOG(LL_DEBUG, ("id: %s", id));
    free(id);
  }
  LOG(LL_DEBUG, ("jsonf_scan result: %d", scan_result));
  if (scan_result < 0) {
    LOG(LL_ERROR, ("json scanf error"));
  }
  else if (0 == scan_result) {
    LOG(LL_ERROR, ("json scanf keys not found"));
  }
  else {
    if (ip) {
      LOG(LL_INFO, ("ip: %s", ip));
      device->setIpAddr(std::string(ip));
      free(ip);
    }
    else {
      LOG(LL_ERROR, ("json_scanf failed to allocate"));
    }
  }
}

static void get_sys_ip(void *cb_arg) {
  struct mg_rpc_call_opts opts = {.dst = mg_mk_str(MGOS_RPC_LOOPBACK_ADDR) };
  mg_rpc_callf(mgos_rpc_get_global(), mg_mk_str("Sys.GetInfo"), sys_ip_cb, cb_arg, &opts, NULL);
}

static void activate_cb(struct mg_rpc_request_info *ri, void *cb_arg,
                   struct mg_rpc_frame_info *fi, struct mg_str args) {
  Door *door = (Door*) cb_arg;
  door->activate();
  mg_rpc_send_responsef(ri, "{ value: \"%s\" }", "OK");
  (void) fi;
}

static void door_count_cb(struct mg_rpc_request_info *ri, void *cb_arg,
                   struct mg_rpc_frame_info *fi, struct mg_str args) {
  mg_rpc_send_responsef(ri, "{ value: %d }", mgos_sys_config_get_garage_door_count());
  (void) fi;
}

static void door_read_cb(struct mg_rpc_request_info *ri, void *cb_arg,
                   struct mg_rpc_frame_info *fi, struct mg_str args) {
  Door *door = (Door*) cb_arg;
  mg_rpc_send_responsef(ri, "{ value: \"%s\" }", door->getStatusString().c_str());
  (void) fi;
}

static void status_cb(struct mg_rpc_request_info *ri, void *cb_arg,
                   struct mg_rpc_frame_info *fi, struct mg_str args) {
  Device *device = (Device*) cb_arg;
  mg_rpc_send_responsef(ri, "%s", device->getStatusJson().c_str());
  (void) fi;
}

static void rh_cb(struct mg_rpc_request_info *ri, void *cb_arg,
                   struct mg_rpc_frame_info *fi, struct mg_str args) {
  Device *device = (Device*) cb_arg;
  mg_rpc_send_responsef(ri, "{ value: %5.1f }", device->rh());
  (void) fi;
}

static void tempf_cb(struct mg_rpc_request_info *ri, void *cb_arg,
                   struct mg_rpc_frame_info *fi, struct mg_str args) {
  Device *device = (Device*) cb_arg;
  mg_rpc_send_responsef(ri, "{ value: %5.1f }", device->tempf());
  (void) fi;
}

static void repeat_cb(void *arg) {
  Device *device = (Device *) arg;
  float f = device->tempf();
  if (isnan(f)) {
    LOG(LL_ERROR, ("DHT22 returning NaN"));
  }
  else {
    LOG(LL_INFO, ("TempF %2.1f degF", device->tempf()));
    LOG(LL_INFO, ("RH %2.1f %%", device->rh()));
  }

  if (mgos_sys_config_get_mqtt_enable()) {
    LOG(LL_DEBUG, ("Publishing device status"));
    std::string deviceBaseTopic = std::string(device->getDeviceId());
    std::string msg = device->getStatusJson();
    int qos = 0;
    bool retain = true;
    uint16_t res = 0;
    char m[16];
    std::string topic;

    topic = deviceBaseTopic + "/status";
    res = mgos_mqtt_pub(topic.c_str(), msg.c_str(), (size_t) msg.length(), qos, retain);

    LOG(LL_DEBUG, ("Publishing tempf"));
    f = device->tempf();
    topic = deviceBaseTopic + "/tempf";
    if (!isnan(f)) {
      size_t len = snprintf(m, sizeof(m), "%2.1f", f);
      if (len > 0) {
        res = mgos_mqtt_pub(topic.c_str(), m, (size_t) len, qos, retain);
      }
    }
    else {
        res = mgos_mqtt_pub(topic.c_str(), "NaN", 3, qos, retain);
    }

    LOG(LL_DEBUG, ("Publishing rh"));
    f = device->rh();
    topic = deviceBaseTopic + "/rh";
    if (!isnan(f)) {
      size_t len = snprintf(m, sizeof(m), "%2.1f", f);
      if (len > 0) {
        res = mgos_mqtt_pub(topic.c_str(), m, len, qos, retain);
      }
    }
    else {
        res = mgos_mqtt_pub(topic.c_str(), "NaN", 3, qos, retain);
    }

    for (int i=0; i < device->getDoorCount(); i++) {
      LOG(LL_DEBUG, ("Publishing door %d info", i));
      Door *door = device->getDoorAt(i);
      if (door) {
        topic = std::string(device->getDeviceId());
        topic += "/";
        topic += door->getOrdinalName();
        topic += "/status";
        LOG(LL_DEBUG, ("door %d topic %s", i, topic.c_str()));

        std::string info = door->getStatusString();
        res = mgos_mqtt_pub(topic.c_str(), info.c_str(), (size_t) info.length(), qos, retain);
      }
    }
    (void) res;
  }
}

static void int_door_contact_cb(int pin, void *obj) {
  Door *door = (Door *) obj;
  int reading = mgos_gpio_read(pin);
  LOG(LL_DEBUG, ("Int pin %d: %d", pin, reading));

  double interrupt_millis = mgos_uptime() * 1000.0;
    if (abs(interrupt_millis - door->debounce) > mgos_sys_config_get_time_debounce_millis()) {
      door->publishIsOpen(reading);
    }
    else {
      LOG(LL_DEBUG, ("interrupt debounced pin %d", pin));
    }
    door->debounce = interrupt_millis;
}

/*
 from https://community.mongoose-os.com/t/enable-web-ui-to-setup-wifi/1753
static void disable_ap(void)
{
  struct mgos_config_wifi_ap ap_cfg;
  memcpy(&ap_cfg, mgos_sys_config_get_wifi_ap(), sizeof(ap_cfg));
  ap_cfg.enable = false;
  int result = mgos_wifi_setup_ap(&ap_cfg);

  // Seems to be the easiest way to track AP enabled/disabled but it's not
  // updated when the AP is turned on like your would expect
  mgos_sys_config_set_wifi_ap_enable(false);

  LOG(LL_INFO, ("Disabling AP"));
}

static void enable_ap(void)
{
  struct mgos_config_wifi_ap ap_cfg;
  memcpy(&ap_cfg, mgos_sys_config_get_wifi_ap(), sizeof(ap_cfg));
  ap_cfg.enable = true;
  int result = mgos_wifi_setup_ap(&ap_cfg);

  // Seems to be the easiest way to track AP enabled/disabled but it's not
  // updated when the AP is turned on like your would expect
  mgos_sys_config_set_wifi_ap_enable(true);

  LOG(LL_INFO, ("Enabling AP"));
}
*/

enum mgos_app_init_result mgos_app_init(void) {
  LOG(LL_INFO, ("Startup %s", __FILE__));
  int doorCount = mgos_sys_config_get_garage_door_count();
  #if 0
  if (doorCount > 0) {
    mgos_gpio_setup_output(mgos_sys_config_get_garage_door_a_pin_activate(), true);
    mgos_gpio_set_pull(mgos_sys_config_get_garage_door_a_pin_activate(), MGOS_GPIO_PULL_UP);
  }
  if (doorCount > 1) {
    mgos_gpio_setup_output(mgos_sys_config_get_garage_door_b_pin_activate(), true);
    mgos_gpio_set_pull(mgos_sys_config_get_garage_door_b_pin_activate(), MGOS_GPIO_PULL_UP);
  }
  #endif

  Device *device = new Device();

  char *uri = NULL;
  char handler_nm[32];
  memset(handler_nm, 0, sizeof(handler_nm));
  for (int i=0; i < doorCount; i++) {
    Door *door = device->getDoorAt(i);
    asprintf(&uri, "door%c.activate", ('a' + i));
    //snprintf(handler_nm, "door%c.%s", ('a' + i), "activate");
    mg_rpc_add_handler(mgos_rpc_get_global(), uri, NULL, activate_cb, door);

    asprintf(&uri, "door%c.read", ('a' + i));
    //snprintf(handler_nm, "door%c.%s", ('a' + i), "read");
    mg_rpc_add_handler(mgos_rpc_get_global(), uri, NULL, door_read_cb, door);

    mgos_gpio_enable_int(door->getContactPin());
    mgos_gpio_set_int_handler(door->getContactPin(), MGOS_GPIO_INT_EDGE_ANY, int_door_contact_cb, door);
  }

  mg_rpc_add_handler(mgos_rpc_get_global(), "door.count", NULL, door_count_cb, NULL);
  mg_rpc_add_handler(mgos_rpc_get_global(), "status.read", NULL, status_cb, device);
  mg_rpc_add_handler(mgos_rpc_get_global(), "rh.read", NULL, rh_cb, device);
  mg_rpc_add_handler(mgos_rpc_get_global(), "tempf.read", NULL, tempf_cb, device);

  mgos_set_timer(60000, 1, repeat_cb, device);
  mgos_set_timer(15000, 0, get_sys_ip, device);

  std::list<int> l = { 3,1,3,3,7 };
  for (int n : l) {
    std::cout << "num: " << n << std::endl;
    LOG(LL_DEBUG, ("n: %d", n));
  }

  auto ai = std::make_shared<int>();
  *ai = 31337;
  std::cout << "shared ptr deref: " << *ai << std::endl;

  test_homie();
  return MGOS_APP_INIT_SUCCESS;
}