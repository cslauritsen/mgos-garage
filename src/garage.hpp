#pragma once
#include <string>

#define RELAY_ACTIVATE false 
#define RELAY_DEACTIVATE true

extern "C" {
#include <mgos_gpio.h>
#include <mgos_sys_config.h>
#include <common/cs_dbg.h>
#include <mgos_app.h>
#include <mgos_system.h>
#include <mgos_time.h>
#include <mgos_timers.h>
}

namespace garage {

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
        int openContactPin;   

        /** pin connected to a reed contact switch (Active LO) */
        int closedContactPin; 

        /** pin connected to a relay (Active LO) */
        int activateRelayPin; 

        /** the current state of the door */
        Status status;

    public:

        ~Door(void);

        Door(char *aName, int aOpenPin, int aClosedPin, int aActivatePin);

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
};

}