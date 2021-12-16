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

        /** pin conntected to a reed contact switch (Active LO) */
        int contactPin;   

        /** pin connected to a relay (Active LO) */
        int activateRelayPin; 

        /** the current state of the door */
        Status status;

    public:

        ~Door(void);

        Door(const char *aName, int aContactPin, int aActivatePin);

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

        inline int getContactPin() { return contactPin; }
        inline int getActivateRelayPin() { return activateRelayPin; }
    };
  class Device {
        private:
            int dhPin;
            char current_time[32];
            struct mgos_dht *dht;
            Door* doors[maxDoors];

        public:
            Device();
            ~Device();
            float rh();
            float tempf();
            int getDhPin();
            std::string currentTime();
            std::string getStatusJson();
            Door* getDoorAt(int ix);

    };

}