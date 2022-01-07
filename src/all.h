#pragma once

extern "C"
{
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
#include <mgos_dht.h>

#include <mbedtls/sha512.h>

    // declared in build_info.c
    extern char *build_id;
    extern char *build_timestamp;
    extern char *build_version;
}

#include <iostream>
#include <sstream>

#include <string>
#include <algorithm>
#include <cctype>

#include <tuple>
#include <list>
#include <vector>
#include <memory>

#include <homie.hpp>
#include <garage.hpp>

#define DHT_NODE_NM "dht22"
#define DHT_PROP_TEMPF "tempf"
#define DHT_PROP_RH "rh"
#define DOOR_ACTIVATE_PROP "activate"
#define DOOR_CONTACT_PROP "isopen"
#define WIFI_NODE_NM "wifi"
#define WIFI_NODE_RSSI_PROP "rssi"