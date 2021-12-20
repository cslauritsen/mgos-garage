#pragma once

#include <string>
#include <list>
#include <ostream>

#define RELAY_STATE_ACTIVE 0
#define RELAY_STATE_INACTIVE 1

extern "C" {
#include <mgos.h>
#include <mgos_gpio.h>
#include <mgos_sys_config.h>
#include <common/cs_dbg.h>
#include <mgos_app.h>
#include <mgos_system.h>
#include <mgos_time.h>
#include <mgos_timers.h>
#include <mgos_dht.h>
#include <mgos_mqtt.h>
}

namespace garage {
  
const int maxDoors = 5;
enum Status {
    kUnknown = -1,
    kOpen = 0,
    kClosed = 1
};

class Door {
    private:
        /** Human-friendly name of the door, e.g. "North Door" */
        std::string name;

        /** unique, sortable name useful in MQTT topic */
        char* ordinalName;

        /** MQTT topic for open/closed status updates */
        std::string statusTopic;

        /** MQTT topic for activation command */
        std::string activateTopic;

        /** pin conntected to a reed contact switch (Active LO) */
        int contactPin;   

        /** pin connected to a relay (Active LO) */
        int activateRelayPin; 

    public:

        ~Door(void);

        Door(const char *aName, int aContactPin, int aActivatePin, int index);

        /** Momentarily activates the relay to similuate a doorbell press. */
        void activate(void);

        /** unconditionally de-energizes the door relay */
        void deactivate(void);

        /** Returns the name of this door */
        std::string getName(void);

        /** Returns door status */
        Status getStatus(void);

        /** Returns door status as string */
        std::string getStatusString(void);

        int getContactPin() { return contactPin; }
        int getActivateRelayPin() { return activateRelayPin; }
        std::string getOrdinalName() { return ordinalName; }
        std::string getStatusTopic() { return statusTopic; }
        std::string getActivateTopic() { return activateTopic; }

        bool publishIsOpen(int reading);

        /** Value to control frequency of interrupt firing */
        double debounce;
    };
  class Device {
        private:
            int dhPin;
            char current_time[32];
            struct mgos_dht *dht;
            Door* doors[maxDoors];
            int doorCount;
            std::string deviceId;

        public:
            Device();
            ~Device();
            float rh();
            float tempf();
            int getDhPin();
            std::string currentTime();
            std::string getStatusJson();
            std::string getDeviceId() {return deviceId;}
            Door* getDoorAt(int ix);


    };

}