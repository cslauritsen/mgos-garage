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
                                 void *ud)
{
  Door *door = (Door *)ud;
  if ((msg_len > 0 && *msg == '1') || 0 == strncmp("true", msg, msg_len > 4 ? 4 : msg_len))
  {
    door->activate();
  }
}

static void activate_cb(struct mg_rpc_request_info *ri, void *cb_arg,
                        struct mg_rpc_frame_info *fi, struct mg_str args)
{
  Door *door = (Door *)cb_arg;
  door->activate();
  mg_rpc_send_responsef(ri, "{ value: \"%s\" }", "OK");
  (void)fi;
}

static void door_count_cb(struct mg_rpc_request_info *ri, void *cb_arg,
                          struct mg_rpc_frame_info *fi, struct mg_str args)
{
  mg_rpc_send_responsef(ri, "{ value: %d }", mgos_sys_config_get_garage_door_count());
  (void)fi;
}

static void door_read_cb(struct mg_rpc_request_info *ri, void *cb_arg,
                         struct mg_rpc_frame_info *fi, struct mg_str args)
{
  Door *door = (Door *)cb_arg;
  mg_rpc_send_responsef(ri, "{ value: \"%s\" }", door->getStatusString().c_str());
  (void)fi;
}

static void status_cb(struct mg_rpc_request_info *ri, void *cb_arg,
                      struct mg_rpc_frame_info *fi, struct mg_str args)
{
  Device *device = (Device *)cb_arg;
  mg_rpc_send_responsef(ri, "%s", device->getStatusJson().c_str());
  (void)fi;
}

static void rh_cb(struct mg_rpc_request_info *ri, void *cb_arg,
                  struct mg_rpc_frame_info *fi, struct mg_str args)
{
  Device *device = (Device *)cb_arg;
  mg_rpc_send_responsef(ri, "{ value: %5.1f }", device->rh());
  (void)fi;
}

static void tempf_cb(struct mg_rpc_request_info *ri, void *cb_arg,
                     struct mg_rpc_frame_info *fi, struct mg_str args)
{
  Device *device = (Device *)cb_arg;
  mg_rpc_send_responsef(ri, "{ value: %5.1f }", device->tempf());
  (void)fi;
}

static void publish_door(void *arg)
{
  if (mgos_sys_config_get_mqtt_enable())
  {
    int qos = 1;
    bool retain = true;
    Door *door = (Door *)arg;
    auto statusProp = door->homieNode->getProperty(DOOR_CONTACT_PROP);
    if (statusProp != nullptr)
    {
      statusProp->setValue(door->getStatusString());
      auto payload = door->getStatusString();
      auto res = mgos_mqtt_pub(statusProp->getPubTopic().c_str(), payload.c_str(), (size_t)payload.length(), qos, retain);
      LOG(LL_DEBUG, ("%s pub %s %d", door->getOrdinalName().c_str(), payload.c_str(), res));
    }
    else
    {
      LOG(LL_ERROR, ("%s no prop %s", door->getOrdinalName().c_str(), DOOR_CONTACT_PROP));
    }
  }
}
static mgos_timer_id introduce_timer_id;
static void introduce_cb(void *arg)
{
  static int qos = 1;
  static bool retain = true;

  auto pair = (std::pair<std::vector<homie::Message> *, int> *)arg;
  auto vect = *(pair->first);
  auto vectLen = (int)vect.size();
  if (pair->second < vectLen)
  {
    LOG(LL_DEBUG, ("about to publish qos=%d, retain=%d", qos, retain));
    LOG(LL_DEBUG, ("iter=%d", pair->second));
    auto msg = vect[pair->second];
    LOG(LL_DEBUG, ("topic=%s, msg=%s", msg.topic.c_str(), msg.payload.c_str()));
    auto unsent = mgos_mqtt_num_unsent_bytes();
    auto maxUnsent = static_cast<size_t>(mgos_sys_config_get_mqtt_max_unsent());
    auto res = mgos_mqtt_pub(msg.topic.c_str(), msg.payload.c_str(), msg.payload.length(), qos, retain);
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
    // no reason to keep the timer running
    mgos_clear_timer(introduce_timer_id);
    // no reason to keep the vector allocated
    delete pair->first;
    // no reason to keep the pair allocated
    delete pair;
    LOG(LL_DEBUG, ("Cleared timer %d", introduce_timer_id));
  }
}

static void repeat_cb(void *arg)
{
  Device *device = (Device *)arg;
  float f = 0.0;

  if (mgos_sys_config_get_mqtt_enable())
  {
    LOG(LL_DEBUG, ("Publishing device status"));
    int qos = 1;
    bool retain = true;
    std::string topic;

    // DHT Temp
    f = device->tempf();
    if (!isnan(f))
    {
      auto prop = device->homieDhtNode->getProperty(DHT_PROP_TEMPF);
      if (prop != nullptr)
      {
        prop->setValue(f);
        std::string payload = prop->getValue();
        auto res = mgos_mqtt_pub(prop->getPubTopic().c_str(), payload.c_str(), payload.length(), qos, retain);
        LOG(LL_DEBUG, ("pub %s %2.1f %d", prop->getName().c_str(), f, res));
      }
    }
    else
    {
      LOG(LL_DEBUG, ("tempf was NaN"));
    }

    // DHT RH
    f = device->rh();
    if (!isnan(f))
    {
      auto prop = device->homieDhtNode->getProperty(DHT_PROP_RH);
      if (prop != nullptr)
      {
        prop->setValue(f);
        std::string payload = prop->getValue();
        auto res = mgos_mqtt_pub(prop->getPubTopic().c_str(), payload.c_str(), payload.length(), qos, retain);
        LOG(LL_DEBUG, ("pub %s %2.1f %d", prop->getName().c_str(), f, res));
      }
    }
    else
    {
      LOG(LL_DEBUG, ("rh was NaN"));
    }

    // Door Sub (1x only) & Pub (each time)
    for (auto door : device->getDoors())
    {
      if (!door->subscribed)
      {
        auto prop = door->homieNode->getProperty(DOOR_ACTIVATE_PROP);
        if (nullptr != prop)
        {
          mgos_mqtt_sub(prop->getSubTopic().c_str(), activate_sub_handler, door);
          door->subscribed = true;
          LOG(LL_DEBUG, ("Publishing %s info", door->getOrdinalName().c_str()));
        }
        else
        {
          LOG(LL_ERROR, ("Door Node has no prop named %s", DOOR_ACTIVATE_PROP));
        }
      }
      publish_door(door);
    }
  }
  else
  {
    f = device->tempf();
    if (isnan(f))
    {
      LOG(LL_ERROR, ("DHT22 returning NaN"));
    }
    else
    {
      LOG(LL_INFO, ("TempF %2.1f degF", device->tempf()));
      LOG(LL_INFO, ("RH %2.1f %%", device->rh()));
    }
  }
}

static void int_door_contact_cb(int pin, void *obj)
{
  Door *door = (Door *)obj;
  int reading = mgos_gpio_read(pin);
  LOG(LL_DEBUG, ("Int pin %d: %d", pin, reading));

  double interrupt_millis = mgos_uptime() * 1000.0;
  if (abs(interrupt_millis - door->debounce) > mgos_sys_config_get_time_debounce_millis())
  {
    publish_door(door);
  }
  else
  {
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

enum mgos_app_init_result mgos_app_init(void)
{
  LOG(LL_INFO, ("Startup %s", __FILE__));

  int i;
  Device *device = new Device();

  char *uri = NULL;
  char handler_nm[32];
  memset(handler_nm, 0, sizeof(handler_nm));
  i = 0;
  for (auto door : device->getDoors())
  {
    asprintf(&uri, "door%c.activate", ('a' + i));
    // snprintf(handler_nm, "door%c.%s", ('a' + i), "activate");
    mg_rpc_add_handler(mgos_rpc_get_global(), uri, NULL, activate_cb, door);

    asprintf(&uri, "door%c.read", ('a' + i));
    // snprintf(handler_nm, "door%c.%s", ('a' + i), "read");
    mg_rpc_add_handler(mgos_rpc_get_global(), uri, NULL, door_read_cb, door);

    mgos_gpio_enable_int(door->getContactPin());
    mgos_gpio_set_int_handler(door->getContactPin(), MGOS_GPIO_INT_EDGE_ANY, int_door_contact_cb, door);
    i++;
  }

  mg_rpc_add_handler(mgos_rpc_get_global(), "door.count", NULL, door_count_cb, NULL);
  mg_rpc_add_handler(mgos_rpc_get_global(), "status.read", NULL, status_cb, device);
  mg_rpc_add_handler(mgos_rpc_get_global(), "rh.read", NULL, rh_cb, device);
  mg_rpc_add_handler(mgos_rpc_get_global(), "tempf.read", NULL, tempf_cb, device);

  mgos_set_timer(60000, 1, repeat_cb, device);

  auto msgList = device->homieDevice->introduce();
  auto pair = new std::pair<std::vector<homie::Message> *, int>;
  pair->first = new std::vector<homie::Message>(msgList.begin(), msgList.end());
  pair->second = 0;
  introduce_timer_id = mgos_set_timer(200, 1, introduce_cb, pair);

#ifdef HAS_MAX_QOS
  if (mgos_sys_config_get_mqtt_enable())
  {
    // in case server is weird and doesn't allow normal range of MQTT QoS
    mgos_mqtt_set_max_qos(mgos_sys_config_get_mqtt_maxqos());
  }
#endif

  return MGOS_APP_INIT_SUCCESS;
}