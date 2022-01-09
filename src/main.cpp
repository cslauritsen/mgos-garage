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

#include "all.h"

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

using namespace garage;

static void activate_sub_handler(struct mg_connection *nc, const char *topic,
                                 int topic_len, const char *msg, int msg_len,
                                 void *ud) {
  Door *door = (Door *)ud;
  if ((msg_len > 0 && *msg == '1') ||
      0 == strncmp("true", msg, msg_len > 4 ? 4 : msg_len)) {
    door->activate();
  }
}

static void activate_cb(struct mg_rpc_request_info *ri, void *cb_arg,
                        struct mg_rpc_frame_info *fi, struct mg_str args) {
  Door *door = (Door *)cb_arg;
  door->activate();
  mg_rpc_send_responsef(ri, "{ value: \"%s\" }", "OK");
  (void)fi;
}

static void door_count_cb(struct mg_rpc_request_info *ri, void *cb_arg,
                          struct mg_rpc_frame_info *fi, struct mg_str args) {
  mg_rpc_send_responsef(ri, "{ value: %d }",
                        mgos_sys_config_get_garage_door_count());
  (void)fi;
}

static void door_read_cb(struct mg_rpc_request_info *ri, void *cb_arg,
                         struct mg_rpc_frame_info *fi, struct mg_str args) {
  Door *door = (Door *)cb_arg;
  mg_rpc_send_responsef(ri, "{ value: \"%s\" }",
                        door->getStatusString().c_str());
  (void)fi;
}

static void status_cb(struct mg_rpc_request_info *ri, void *cb_arg,
                      struct mg_rpc_frame_info *fi, struct mg_str args) {
  Device *device = (Device *)cb_arg;
  mg_rpc_send_responsef(ri, "%s", device->getStatusJson().c_str());
  (void)fi;
}

static void rh_cb(struct mg_rpc_request_info *ri, void *cb_arg,
                  struct mg_rpc_frame_info *fi, struct mg_str args) {
  Device *device = (Device *)cb_arg;
  mg_rpc_send_responsef(ri, "{ value: %5.1f }", device->rh());
  (void)fi;
}

static void tempf_cb(struct mg_rpc_request_info *ri, void *cb_arg,
                     struct mg_rpc_frame_info *fi, struct mg_str args) {
  Device *device = (Device *)cb_arg;
  mg_rpc_send_responsef(ri, "{ value: %5.1f }", device->tempf());
  (void)fi;
}

static void publish_door(void *arg) {
  if (mgos_sys_config_get_mqtt_enable()) {
    int qos = 1;
    bool retain = true;

    Door *door = (Door *)arg;
    auto statusProp = door->homieNode->getProperty(DOOR_CONTACT_PROP);
    if (statusProp != nullptr) {
      statusProp->setValue(kOpen == door->getStatus());
      auto payload = std::string(kOpen == door->getStatus()
                                     ? "true"
                                     : "false"); // door->getStatusString();
      auto res =
          mgos_mqtt_pub(statusProp->getPubTopic().c_str(), payload.c_str(),
                        (size_t)payload.length(), qos, retain);
      LOG(LL_DEBUG, ("%s pub %s %d", door->getOrdinalName().c_str(),
                     payload.c_str(), res));
    } else {
      LOG(LL_ERROR,
          ("%s no prop %s", door->getOrdinalName().c_str(), DOOR_CONTACT_PROP));
    }
  }
}

#if 0
static mgos_timer_id introduce_timer_id = MGOS_INVALID_TIMER_ID;
static void homie_introduce_cb(void *arg)
{
  static int qos = 1;
  static int invocationCount = 0;
  static bool retain = true;
  auto enabled = mgos_sys_config_get_mqtt_enable();

  auto pair = (std::pair<std::vector<homie::Message> *, int> *)arg;
  auto vect = *(pair->first);
  auto vectLen = (int)vect.size();
  if (invocationCount++ > vectLen * 4)
  {
    LOG(LL_WARN, ("Giving up introduction attempts after %d tries", invocationCount));
    enabled = false;
  }

  LOG(LL_DEBUG, ("vectLen=%d", vectLen));
  if (enabled && pair->second < vectLen)
  {
    LOG(LL_DEBUG, ("about to publish qos=%d, retain=%d", qos, retain));
    LOG(LL_DEBUG, ("iter=%d", pair->second));
    auto msg = vect[pair->second];
    LOG(LL_DEBUG, ("topic=%s, msg=%s", msg.topic.c_str(), msg.payload.c_str()));
    auto unsent = mgos_mqtt_num_unsent_bytes();
    auto maxUnsent = static_cast<size_t>(mgos_sys_config_get_mqtt_max_unsent());
    auto res = mgos_mqtt_pub(msg.topic.c_str(), msg.payload.c_str(), msg.payload.length(), qos, retain);
    LOG(LL_DEBUG, ("pub res: %d", res));
    if (unsent >= maxUnsent)
    {
      LOG(LL_WARN, ("MQTT buffer too full %d > %d", unsent, maxUnsent));
    }
    else if (res > 0)
    {
      // only advance if publication was successful
      // we will re-try on next invocation if it wasn't
      pair->second++;
    }
  }
  else
  {
    enabled = false;
  }

  if (!enabled)
  {
    // prevent this function from being invoked again
    if (MGOS_INVALID_TIMER_ID != introduce_timer_id)
    {
      mgos_clear_timer(introduce_timer_id);
      LOG(LL_DEBUG, ("Cleared timer %d", introduce_timer_id));
      introduce_timer_id = MGOS_INVALID_TIMER_ID;
    }
    // no reason to keep the vector allocated
    if (pair->first)
    {
      delete pair->first;
      pair->first = nullptr;
    }
    // no reason to keep the pair allocated
    if (pair)
    {
      pair = nullptr;
      delete pair;
    }
  }
}
#endif

static void publish_ip(Device *device) {
  int qos = 1;
  bool retain = true;
  auto topic = device->homieDevice->getTopicBase();
  auto newIp = device->homieDevice->getLocalIp();
  topic += "$localip";
  LOG(LL_DEBUG, ("ip pub %s: %s", topic.c_str(), newIp.c_str()));
  mgos_mqtt_pub(topic.c_str(), newIp.c_str(), newIp.length(), qos, retain);
}

static void publish_mac(Device *device) {
  int qos = 1;
  bool retain = true;
  auto topic = device->homieDevice->getTopicBase();
  auto newMac = device->homieDevice->getMac();
  topic += "$mac";
  LOG(LL_DEBUG, ("mac pub %s: %s", topic.c_str(), newMac.c_str()));
  mgos_mqtt_pub(topic.c_str(), newMac.c_str(), newMac.length(), qos, retain);
}

static void ip_rpc_cb(struct mg_rpc *c, void *cb_arg,
                      struct mg_rpc_frame_info *fi, struct mg_str result,
                      int error_code, struct mg_str error_msg) {
  Device *device = (Device *)cb_arg;
  auto currentIp = device->homieDevice->getLocalIp();
  auto currentMac = device->homieDevice->getMac();
  LOG(LL_INFO, ("error code: %d", error_code));
  char *ip = NULL;
  char *mac = NULL;
  int scan_result = json_scanf(result.p, result.len,
                               "{mac: %Q, wifi: {sta_ip: %Q}}", &mac, &ip);

  LOG(LL_DEBUG, ("jsonf_scan result: %d", scan_result));
  if (scan_result < 0) {
    LOG(LL_ERROR, ("json scanf error"));
  } else if (0 == scan_result) {
    LOG(LL_ERROR, ("json scanf keys not found"));
  } else {
    if (ip) {
      LOG(LL_INFO, ("ip: %s", ip));
      std::string str = std::string(ip);
      device->setIpAddr(str);
      device->homieDevice->setLocalIp(str);
      free(ip);
      if (str != currentIp) {
        publish_ip(device);
      }
    } else {
      LOG(LL_ERROR, ("json_scanf failed to find ip address"));
    }

    if (mac) {
      LOG(LL_INFO, ("mac: %s", mac));
      std::string str = std::string(mac);
      device->homieDevice->setMac(str);
      free(mac);
      if (str != currentMac) {
        publish_mac(device);
      }
    } else {
      LOG(LL_ERROR, ("json_scanf failed to find ip address"));
    }
  }
}

static void repeat_cb(void *arg) {
  Device *device = (Device *)arg;
  float f = 0.0;

  if (mgos_sys_config_get_mqtt_enable()) {
    LOG(LL_DEBUG, ("Publishing device status"));
    int qos = 1;
    bool retain = true;
    std::string topic;

    device->homieDevice->setLifecycleState(homie::READY);
    auto lcsMsg = device->homieDevice->getLifecycleMsg();

    mgos_mqtt_pub(lcsMsg.topic.c_str(), lcsMsg.payload.c_str(),
                  lcsMsg.payload.length(), qos, retain);

    // DHT Temp
    f = device->tempf();
    if (!isnan(f)) {
      auto prop = device->homieDevice->getNode(DHT_NODE_NM)
                      ->getProperty(DHT_PROP_TEMPF);
      if (prop != nullptr) {
        prop->setValue(f);
        std::string payload = prop->getValue();
        auto res = mgos_mqtt_pub(prop->getPubTopic().c_str(), payload.c_str(),
                                 payload.length(), qos, retain);
        LOG(LL_DEBUG, ("pub %s %2.1f %d", prop->getName().c_str(), f, res));
      }
    } else {
      LOG(LL_DEBUG, ("tempf was NaN"));
    }

    // DHT RH
    f = device->rh();
    if (!isnan(f)) {
      auto prop =
          device->homieDevice->getNode(DHT_NODE_NM)->getProperty(DHT_PROP_RH);
      if (prop != nullptr) {
        prop->setValue(f);
        std::string payload = prop->getValue();
        auto res = mgos_mqtt_pub(prop->getPubTopic().c_str(), payload.c_str(),
                                 payload.length(), qos, retain);
        LOG(LL_DEBUG, ("pub %s %2.1f %d", prop->getName().c_str(), f, res));
      }
    } else {
      LOG(LL_DEBUG, ("rh was NaN"));
    }

    // Door Sub (1x only) & Pub (each time)
    for (auto door : device->getDoors()) {
      if (!door->subscribed) {
        auto prop = door->homieNode->getProperty(DOOR_ACTIVATE_PROP);
        if (nullptr != prop) {
          mgos_mqtt_sub(prop->getSubTopic().c_str(), activate_sub_handler,
                        door);
          LOG(LL_INFO, ("Subscribed to %s", prop->getSubTopic().c_str()));
          door->subscribed = true;
        } else {
          LOG(LL_ERROR, ("Door Node has no prop named %s", DOOR_ACTIVATE_PROP));
        }
      }
      LOG(LL_DEBUG, ("Publishing %s info", door->getOrdinalName().c_str()));
      publish_door(door);
    }

    auto currentIp = device->homieDevice->getLocalIp();
    LOG(LL_DEBUG, ("currentIp %s", currentIp.c_str()));
    if (currentIp.length() < 7) // shortest possible IPv4 addr is 7 chars long
    {
      LOG(LL_DEBUG, ("Inquiring network ip"));
      struct mg_rpc_call_opts opts = {.dst = mg_mk_str(MGOS_RPC_LOOPBACK_ADDR)};
      auto newIp = device->homieDevice->getLocalIp();
      mg_rpc_callf(mgos_rpc_get_global(), mg_mk_str("Sys.GetInfo"), ip_rpc_cb,
                   device, &opts, NULL);
    }

    // WiFi
    auto rssi = device->getWifiSignalStrength();
    LOG(LL_DEBUG, ("rssi: %d", rssi));
    auto wifiNode = device->homieDevice->getNode(WIFI_NODE_NM);
    auto rssiProp = wifiNode->getProperty(WIFI_NODE_RSSI_PROP);
    rssiProp->setValue(rssi);
    auto payload = rssiProp->getValue();
    mgos_mqtt_pub(rssiProp->getPubTopic().c_str(), payload.c_str(),
                  payload.length(), qos, retain);
  } else {
    f = device->tempf();
    if (isnan(f)) {
      LOG(LL_ERROR, ("DHT22 returning NaN"));
    } else {
      LOG(LL_INFO, ("TempF %2.1f degF", device->tempf()));
      LOG(LL_INFO, ("RH %2.1f %%", device->rh()));
    }
  }
}

static void int_door_contact_cb(int pin, void *obj) {
  Door *door = (Door *)obj;
  int reading = mgos_gpio_read(pin);
  LOG(LL_DEBUG, ("Int pin %d: %d", pin, reading));

  double interrupt_millis = mgos_uptime() * 1000.0;
  if (abs(interrupt_millis - door->debounce) >
      mgos_sys_config_get_time_debounce_millis()) {
    publish_door(door);
  } else {
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

static void sys_ready_cb(int ev, void *ev_data, void *userdata) {
  LOG(LL_INFO, ("Got system event %d", ev));
  Device *device = (Device *)userdata;

  static int callCount = 1;
  if (device->homieDevice && device->homieDevice->getLocalIp().length() == 0) {
    net_inquire_config(device->homieDevice);
    LOG(LL_DEBUG, ("Inquiring network config %d", callCount));
    struct mg_rpc_call_opts opts = {.dst = mg_mk_str(MGOS_RPC_LOOPBACK_ADDR)};
    mg_rpc_callf(mgos_rpc_get_global(), mg_mk_str("Sys.GetInfo"),
                 network_config_rpc_cb, device, &opts, NULL);
    callCount++;
  }

  device->homieDevice->setLifecycleState(homie::READY);
  if (mgos_sys_config_get_homie_enable()) {
    device->homieDevice
        ->introduce(); // may require tuning of mqtt.max_queue_length
  }

  LOG(LL_DEBUG, ("last will topic: %s", mgos_sys_config_get_mqtt_will_topic()));

  (void)ev;
  (void)ev_data;
}

enum mgos_app_init_result mgos_app_init(void) {
  LOG(LL_INFO, ("Startup %s", __FILE__));

  int i;
  Device *device = new Device();

  char *uri = NULL;
  char handler_nm[32];
  memset(handler_nm, 0, sizeof(handler_nm));
  i = 0;
  for (auto door : device->getDoors()) {
    asprintf(&uri, "door%c.activate", ('a' + i));
    // snprintf(handler_nm, "door%c.%s", ('a' + i), "activate");
    mg_rpc_add_handler(mgos_rpc_get_global(), uri, NULL, activate_cb, door);

    asprintf(&uri, "door%c.read", ('a' + i));
    // snprintf(handler_nm, "door%c.%s", ('a' + i), "read");
    mg_rpc_add_handler(mgos_rpc_get_global(), uri, NULL, door_read_cb, door);

    mgos_gpio_enable_int(door->getContactPin());
    mgos_gpio_set_int_handler(door->getContactPin(), MGOS_GPIO_INT_EDGE_ANY,
                              int_door_contact_cb, door);
    i++;
  }

  mg_rpc_add_handler(mgos_rpc_get_global(), "door.count", NULL, door_count_cb,
                     NULL);
  mg_rpc_add_handler(mgos_rpc_get_global(), "status.read", NULL, status_cb,
                     device);
  mg_rpc_add_handler(mgos_rpc_get_global(), "rh.read", NULL, rh_cb, device);
  mg_rpc_add_handler(mgos_rpc_get_global(), "tempf.read", NULL, tempf_cb,
                     device);

  mgos_set_timer(60000, 1, repeat_cb, device);
  device->homieDevice->setLifecycleState(homie::INIT);
  auto initMsg = device->homieDevice->getLifecycleMsg();

  mgos_mqtt_pub(initMsg.topic.c_str(), initMsg.payload.c_str(),
                initMsg.payload.length(), 1, false);
  LOG(LL_INFO, ("%s -> %s", initMsg.topic.c_str(), initMsg.payload.c_str()));

  // Register callback when sys init is complete
  mgos_event_add_handler(MGOS_EVENT_INIT_DONE, sys_ready_cb, device);

#ifdef HAS_MAX_QOS
  if (mgos_sys_config_get_mqtt_enable()) {
    // in case server is weird and doesn't allow normal range of MQTT QoS
    mgos_mqtt_set_max_qos(mgos_sys_config_get_mqtt_maxqos());
  }
#endif

  return MGOS_APP_INIT_SUCCESS;
}