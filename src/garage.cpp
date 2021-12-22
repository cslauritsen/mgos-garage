#include "garage.hpp"
#include <string>
#include <iostream>
#include <sstream>
extern "C" {
      // declared in build_info.c
    extern char *build_id;
    extern char *build_timestamp;
    extern char *build_version;
}
#define ENDL "\n"


namespace garage {
    static void activate_sub_handler(struct mg_connection *nc, const char *topic,
                                int topic_len, const char *msg, int msg_len,
                                void *ud) {
        Door *door = (Door*) ud;
        if (msg_len > 0 && *msg == '1') {
            door->activate();
        }
    }
    Device::Device() {
        memset(this->current_time, 0, sizeof(this->current_time));
        this->dhPin = mgos_sys_config_get_garage_dht_pin();
        mgos_gpio_setup_input(this->dhPin, MGOS_GPIO_PULL_UP);
        this->dht = NULL;
        this->dht = mgos_dht_create(this->dhPin, DHT22);
        if (this->dht) {
            LOG(LL_DEBUG, ("Dht connected at pin %d", this->dhPin));
        }
        else {
            LOG(LL_INFO, ("Dht failed to connect on pin %d", this->dhPin));
        }

        this->doorCount = mgos_sys_config_get_garage_door_count();
        memset(this->doors, 0, sizeof(this->doors));
        int doorIx = -1;
        for (int i=0; i < maxDoors; i++) {
            doors[i] = NULL;
        }

        if (doorCount > 0) {
            doorIx++;
            this->doors[doorIx] = new Door(
                mgos_sys_config_get_garage_door_a_name(),
                mgos_sys_config_get_garage_door_a_pin_contact(),
                mgos_sys_config_get_garage_door_a_pin_activate(), 
                doorIx
            );
        }

        if (doorCount > 1) {
            doorIx++;
            this->doors[doorIx] = new Door(
                mgos_sys_config_get_garage_door_b_name(),
                mgos_sys_config_get_garage_door_b_pin_contact(),
                mgos_sys_config_get_garage_door_b_pin_activate(), 
                doorIx
            );
        }

        for (int i=0; i < maxDoors && doors[i]; i++) {
          LOG(LL_DEBUG, ("Setup door %s (%d)", this->doors[i]->getName().c_str(), i));
        }

        this->deviceId = std::string(mgos_sys_config_get_device_id());

        
    }

    Device::~Device() {
        mgos_dht_close(this->dht);
        this->dht = NULL;
        LOG(LL_INFO, ("Closed dht"));
        for (Door *d = *doors; d; d++) {
            delete d;
        }
    }

    float Device::rh() {
        float reading = mgos_dht_get_humidity(this->dht);
        if (isnan(reading)) {
            LOG(LL_ERROR, ("rh read returned NaN"));
            return reading;
        }
        LOG(LL_DEBUG, ("rh read %2.1f", reading));
        return reading;
    }

    float Device::tempf() {
        float celsius = mgos_dht_get_temp(this->dht);
        if (isnan(celsius)) {
            LOG(LL_ERROR, ("temp read returned NaN"));
            return celsius;
        }
        LOG(LL_DEBUG, ("temp read deC %2.1f", celsius));
        return celsius * (9.0 / 5.0) + 32.0;
    }

    std::string Device::currentTime() {
        memset(this->current_time, 0, sizeof(this->current_time));
        time_t now = 0;
        time(&now);
        mgos_strftime(this->current_time, sizeof(this->current_time)-1, "%c", now);
        return std::string(this->current_time);
    }

    std::string Device::getStatusJson() {
        std::string bid(build_id);
        std::string bts(build_timestamp);
        std::string bver(build_version);
        std::string nm(mgos_sys_config_get_garage_name());

        std::ostringstream ret;

        ret << "{" << ENDL;
        ret << " \"name\": " << '"' << nm << '"' << ',' << ENDL;
        ret << " \"doorCount\": " << doorCount << ',' << ENDL;
        ret << " \"version\": " << '"' << bver << '"' << ',' << ENDL;
        ret << " \"build_timestamp\": " << '"' << bts << '"' << ',' << ENDL;
        ret << " \"build_id\": " << '"' << bid << '"' << ',' << ENDL;
        ret << " \"currentTime\": " << '"' << currentTime() << '"' << ',' << ENDL;
        if (! isnan(rh())) {
            ret << " \"rh\": " << rh() << ',' << ENDL;
        }
        if (! isnan(tempf())) {
            ret << " \"tempf\": " << tempf() << ',' << ENDL;
        }
        ret << " \"doors\": [" << ENDL;
        bool comma = false;
        for (int i=0; doors[i]; i++) {
            if (comma) ret << "," << ENDL;
            ret << "    {"  << ENDL;
            ret << "       \"name\": \"" << doors[i]->getName() << "\"," << ENDL;
            ret << "       \"status\": \"" << doors[i]->getStatusString()  << '"' << ENDL;
            ret << "    }"  << ENDL;
            comma = true;
        }

        ret << " ]" << ENDL;
        ret << "}";
        return ret.str();
    }

    Door* Device::getDoorAt(int ix) {
        if (ix >= maxDoors) {
            return NULL;
        }
        return doors[ix];
    }

    void Device::setIpAddr(std::string s) {
        this->ipAddr = std::string(s);
    }

    Door::Door(const char* aName, int aContactPin, int aActivatePin, int index) {
        this->activateRelayPin = aActivatePin;
        this->contactPin = aContactPin;
        this->name = std::string(aName);

        char *tmp;
        asprintf(&tmp, "door%c", ('a'+index));
        this->ordinalName = std::string(tmp);

        mgos_gpio_set_mode(this->contactPin, MGOS_GPIO_MODE_INPUT);
        mgos_gpio_setup_input(this->contactPin, MGOS_GPIO_PULL_UP);

        mgos_gpio_set_pull(this->activateRelayPin, MGOS_GPIO_PULL_UP);
        mgos_gpio_set_mode(this->activateRelayPin, MGOS_GPIO_MODE_OUTPUT);
        this->deactivate();

        // Message body will be 1 for "yes, open", or 0 for "no, closed"
        // the 1 or 0 correlates to HI or LO on the pin, respectively
        this->statusTopic = std::string(mgos_sys_config_get_device_id());
        this->statusTopic += "/";
        this->statusTopic += this->getOrdinalName();
        this->statusTopic += "/open";

        // Activation occurs when message body is 1
        // ignored otherwise
        this->activateTopic = std::string(mgos_sys_config_get_device_id());
        this->activateTopic += "/";
        this->activateTopic += this->getOrdinalName();
        this->activateTopic += "/activate";

        if (mgos_sys_config_get_mqtt_enable()) {
            mgos_mqtt_sub(this->activateTopic.c_str(), activate_sub_handler, this);
        }

        LOG(LL_INFO, ("Cfg door %s, ctc=%d act=%d ord=%s", this->name.c_str(), this->contactPin, this->activateRelayPin, this->ordinalName.c_str()));
    }

    static void _deactivate_cb(void *door) {
        Door *d = (Door *) door;
        // switch it off
        d->deactivate();
        LOG(LL_INFO, ("timed deact door %s", d->getName().c_str()));
    }

    void Door::activate(void) {
        mgos_gpio_write(this->activateRelayPin, RELAY_STATE_ACTIVE); 
        // async call to the future
        mgos_set_timer(mgos_sys_config_get_garage_door_activate_millis(), 0, _deactivate_cb, this);
        LOG(LL_INFO, ("act door %s pin %d", this->getName().c_str(), this->activateRelayPin));
    }

    void Door::deactivate(void) {
        mgos_gpio_write(this->activateRelayPin, RELAY_STATE_INACTIVE);
    }

    std::string Door::getName(void) {
        return this->name;
    }

    Status Door::getStatus(void) {
        bool val = mgos_gpio_read(this->contactPin);
        LOG(LL_DEBUG, ("%s status gpio %d", this->name.c_str(), val));
        return val ? kOpen : kClosed;
    }

    std::string Door::getStatusString(void) {
        switch (this->getStatus()) {
            case kClosed:
            return "closed";
            case kOpen:
            return "open";
            default:
            return "unknown";
        }
    }

    bool Door::publishIsOpen(int reading) {
      static const int qos = 0;
      static const bool retain = false;
      static const int len = 1;
      // we assume the topic is '..../open' so 
      // msg "0" -> "closed", i.e. "not open"; 
      // msg "1" -> "yes, open"
      // the contact pin is pulled HI and shorted to GND by a reed switch
      // so it will read 0 when the magnet is near the reed switch
      // which correpsonds to the door being fully closed
      int pktId = mgos_mqtt_pub(this->getStatusTopic().c_str(), reading ? "1" : "0", len, qos, retain);
      if (0 == pktId) {
        LOG(LL_ERROR, ("No MQTT connection"));
      }
      LOG(LL_DEBUG, ("Status pub %s", pktId==0 ? "Failed" : "OK"));
      return 0 != pktId;
    }

}