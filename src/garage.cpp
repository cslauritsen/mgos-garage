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
    Device::Device() {
        memset(this->current_time, 0, sizeof(this->current_time));
        this->dhPin = mgos_sys_config_get_garage_dht_pin();
        this->dht = NULL;
        this->dht = mgos_dht_create(this->dhPin, DHT22);
        if (this->dht) {
            LOG(LL_DEBUG, ("Dht connected at pin %d", this->dhPin));
        }
        else {
            LOG(LL_INFO, ("Dht failed to connect on pin %d", this->dhPin));
        }

        int doorCount = mgos_sys_config_get_garage_door_count();
        memset(this->doors, 0, sizeof(this->doors));
        int doorIx = -1;
        for (int i=0; i < maxDoors; i++) {
            doors[i] = NULL;
        }

        if (doorCount > 0) {
            this->doors[++doorIx] = new Door(
                mgos_sys_config_get_garage_door_a_name(),
                mgos_sys_config_get_garage_door_a_pin_contact(),
                mgos_sys_config_get_garage_door_a_pin_activate()
            );
        }

        if (doorCount > 1) {
        this->doors[++doorIx] = new Door(
            mgos_sys_config_get_garage_door_b_name(),
            mgos_sys_config_get_garage_door_b_pin_contact(),
            mgos_sys_config_get_garage_door_b_pin_activate()
        );
        }

        for (int i=0; i < maxDoors && doors[i]; i++) {
          LOG(LL_DEBUG, ("Setup door %s (%d)", this->doors[i]->getName().c_str(), i));
        }
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
        return mgos_dht_get_humidity(this->dht);
    }

    float Device::tempf() {
        float celsius = mgos_dht_get_temp(this->dht);
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

        std::ostringstream ret;

        ret << "{" << ENDL;
        ret << " version: " << '"' << bver << '"' << ',' << ENDL;
        ret << " build_timestamp: " << '"' << bts << '"' << ',' << ENDL;
        ret << " build_id: " << '"' << bid << '"' << ',' << ENDL;
        ret << " currentTime: " << '"' << currentTime() << '"' << ',' << ENDL;
        ret << " rh: " << rh() << ',' << ENDL;
        ret << " tempf: " << tempf() << ',' << ENDL;
        ret << " doors: [" << ENDL;
        bool comma = false;
        for (int i=0; doors[i]; i++) {
            if (comma) ret << "," << ENDL;
            ret << "    {"  << ENDL;
            ret << "       name: \"" << doors[i]->getName() << "\"," << ENDL;
            ret << "       status: \"" << doors[i]->getStatusString()  << '"' << ENDL;
            ret << "    }"  << ENDL;
            comma = true;
        }

        ret << " ]" << ENDL;
        ret << "}";
        return ret.str();
    }

    Door* Device::getDoorAt(int ix) {
        int i=0;
        for (Door *door = *doors; door && i <= ix; door++) {
            if (i == ix) {
                return door;
            }
            i++;
        }
        return NULL;
    }

    Door::Door(const char* aName, int aContactPin, int aActivatePin) {
        this->activateRelayPin = aActivatePin;
        this->contactPin = aContactPin;
        this->name = std::string(aName);
        this->status = kUnknown;
        mgos_gpio_setup_input(this->getContactPin(), MGOS_GPIO_PULL_UP);
        mgos_gpio_setup_output(this->getActivateRelayPin(), RELAY_STATE_INACTIVE);
        LOG(LL_INFO, ("Cfg door %s, ctc=%d act=%d", this->name.c_str(), this->contactPin, this->activateRelayPin));
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

    std::string  Door::getName(void) {
        return this->name;
    }

    Status Door::getStatus(void) {
        bool val = mgos_gpio_read_out(this->contactPin);
        return val ? kClosed : kOpen;
        // return this->status;
    }

    std::string Door::getStatusString(void) {
        switch (this->status) {
            case kClosed:
            return "closed";
            case kOpen:
            return "open";
            default:
            return "unknown";
        }
    }

}