#pragma once
#include "all.h"

#define RELAY_STATE_ACTIVE 0
#define RELAY_STATE_INACTIVE 1

namespace garage
{

    const int maxDoors = 5;
    enum Status
    {
        kUnknown = -1,
        kOpen = 0,
        kClosed = 1
    };

    class Door
    {
    private:
        /** Human-friendly name of the door, e.g. "North Door" */
        std::string name;

        /** unique, sortable name useful in MQTT topic */
        std::string ordinalName;

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

        /** Value to control frequency of interrupt firing */
        double debounce;

        homie::Node *homieNode;
        bool subscribed = false;
    };

    class Device
    {
    private:
        int dhPin;
        char current_time[32];
        struct mgos_dht *dht;
        std::list<Door *> doors;
        int doorCount;
        std::string deviceId;
        std::string ipAddr;

    public:
        Device();
        ~Device();
        float rh();
        float tempf();
        int getDhPin();
        std::string currentTime();
        std::string getStatusJson();
        std::string getDeviceId() { return deviceId; }
        int getDoorCount() { return doorCount; }
        std::string getIpAddr() { return ipAddr; }
        void setIpAddr(std::string s);
        std::list<Door *> &getDoors() { return doors; }
        homie::Device *homieDevice;
        homie::Node *homieDhtNode;
    };

}