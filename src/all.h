#pragma once
#define NODE_NM_DHT "dht22"
#define PROP_NM_TEMPF "tempf"
#define PROP_NM_TEMPC "tempc"
#define PROP_NM_RH "rh"
#define NODE_NM_PIR "pir"
#define PROP_NM_MOTION "motion"

#include <homie.hpp>
#include "thing.h"

#include <string>

extern "C" {
#include "mgos.h"
#include "mgos_app.h"
#include "mgos_dlsym.h"
#include "mgos_hal.h"
#include "mgos_rpc.h"
#include <common/cs_dbg.h>
#include <mg_rpc_channel_loopback.h>
#include <mgos_app.h>
#include <mgos_dht.h>
#include <mgos_gpio.h>
#include <mgos_mqtt.h>
#include <mgos_sys_config.h>
#include <mgos_system.h>
#include <mgos_time.h>
#include <mgos_timers.h>

#include <mbedtls/sha512.h>

// declared in build_info.c
extern char *build_id;
extern char *build_timestamp;
extern char *build_version;
}