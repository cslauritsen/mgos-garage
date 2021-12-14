#include "garage.hpp"
#include <string>
namespace garage {

Door::Door(char* aName, int aOpenPin, int aClosedPin, int aActivatePin) {
    this->activateRelayPin = aActivatePin;
    this->openContactPin = aOpenPin;
    this->closedContactPin = aClosedPin;
    this->name = std::string(aName);
}

static void _deactivate_cb(void *door) {
    Door *d = (Door *) door;
    // switch it off
    d->deactivate();
}

void Door::activate(void) {
    mgos_gpio_write(this->activateRelayPin, RELAY_ACTIVATE); 
    // async call to the future
    mgos_set_timer(mgos_sys_config_get_garage_door_activate_millis(), 0, _deactivate_cb, this);
    return true;
}

void Door::deactivate(void) {
   mgos_gpio_write(this->activateRelayPin, RELAY_DEACTIVATE);
}

std::string  Door::getName(void) {
    return this->name;
}

Status Door::getStatus(void) {
    return this->status;
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