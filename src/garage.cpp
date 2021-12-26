#include "all.h"

namespace garage
{

    Device::Device()
    {
        std::string projectName = std::string(mgos_sys_config_get_project_name());
        const char *deviceId = mgos_sys_config_get_device_id();
        int ofs = strlen(deviceId) - 6;
        const char *suffix = deviceId + ofs;
        this->deviceId = projectName;
        this->deviceId += "-";
        this->deviceId += std::string(suffix);

        // homie (or mqtt?) says that topic elements must be lower-case
        // do this incase the configurator doesnt know this
        std::transform(
            this->deviceId.begin(),
            this->deviceId.end(),
            this->deviceId.begin(),
            [](unsigned char c)
            { return std::tolower(c); });

        // Homie setup
        std::string ip;  // cannot get this until device has fully configured, which is not now
        std::string mac; // cannot get this until device has fully configured, which is not now

        homieDevice = new homie::Device(
            this->deviceId,
            std::string(build_version),
            std::string(mgos_sys_config_get_project_name()),
            ip,
            mac);

        memset(this->current_time, 0, sizeof(this->current_time));
        this->dhPin = mgos_sys_config_get_garage_dht_pin();
        mgos_gpio_setup_input(this->dhPin, MGOS_GPIO_PULL_UP);
        this->dht = NULL;
        this->dht = mgos_dht_create(this->dhPin, DHT22);
        if (this->dht)
        {
            LOG(LL_DEBUG, ("Dht connected at pin %d", this->dhPin));
        }
        else
        {
            LOG(LL_INFO, ("Dht failed to connect on pin %d", this->dhPin));
        }
        auto dhtNode = new homie::Node(this->homieDevice, DHT_NODE_NM, "Temperature/Humidity Sensor", "DHT22");

        auto tempFProp = new homie::Property(dhtNode, DHT_PROP_TEMPF, "Temperature in Fahrenheit", homie::FLOAT, false);
        dhtNode->addProperty(tempFProp);
        tempFProp->setValue((float)72.0);
        tempFProp->setUnit(homie::DEGREE_SYMBOL + "F");

        auto rhProp = new homie::Property(dhtNode, DHT_PROP_RH, "Relative Humidity", homie::FLOAT, false);
        rhProp->setValue((float)50);
        rhProp->setUnit("%");
        dhtNode->addProperty(rhProp);

        this->homieDhtNode = dhtNode;
        this->homieDevice->addNode(dhtNode);

        this->doorCount = mgos_sys_config_get_garage_door_count();
        int doorIx = -1;

        if (doorCount > 0)
        {
            doorIx++;
            auto door = new Door(
                mgos_sys_config_get_garage_door_a_name(),
                mgos_sys_config_get_garage_door_a_pin_contact(),
                mgos_sys_config_get_garage_door_a_pin_activate(),
                doorIx);
            this->doors.push_back(door);

            auto doorNode = new homie::Node(this->homieDevice, door->getOrdinalName(), door->getName(), "GarageDoor");
            homieDevice->addNode(doorNode);
            auto relayProp = new homie::Property(doorNode, DOOR_ACTIVATE_PROP, "Activation Relay", homie::INTEGER, true);
            relayProp->setValue(0);
            doorNode->addProperty(relayProp);
            auto contactProp = new homie::Property(doorNode, "contact", "Open/Closed Reed Switch", homie::ENUM, false);
            contactProp->setFormat("open,closed,unknown");
            contactProp->setValue(std::string("unknown"));
            doorNode->addProperty(contactProp);
            door->homieNode = doorNode;
        }

        if (doorCount > 1)
        {
            doorIx++;
            auto door = new Door(
                mgos_sys_config_get_garage_door_b_name(),
                mgos_sys_config_get_garage_door_b_pin_contact(),
                mgos_sys_config_get_garage_door_b_pin_activate(),
                doorIx);
            this->doors.push_back(door);

            auto doorNode = new homie::Node(this->homieDevice, door->getOrdinalName(), door->getName(), "GarageDoor");
            homieDevice->addNode(doorNode);
            auto relayProp = new homie::Property(doorNode, DOOR_ACTIVATE_PROP, "Activation Relay", homie::INTEGER, true);
            relayProp->setValue(0);
            doorNode->addProperty(relayProp);
            auto contactProp = new homie::Property(doorNode, DOOR_CONTACT_PROP, "Open/Closed Reed Switch", homie::ENUM, false);
            contactProp->setFormat("open,closed,unknown");
            contactProp->setValue(std::string("unknown"));
            doorNode->addProperty(contactProp);
            door->homieNode = doorNode;
        }

        int i = 0;
        for (auto d : doors)
        {
            LOG(LL_DEBUG, ("Setup door %s (%d)", d->getName().c_str(), i++));
        }
    }

    Device::~Device()
    {
        mgos_dht_close(this->dht);
        this->dht = NULL;
        LOG(LL_INFO, ("Closed dht"));
    }

    float Device::rh()
    {
        float reading = mgos_dht_get_humidity(this->dht);
        if (isnan(reading))
        {
            LOG(LL_ERROR, ("rh read returned NaN"));
            return reading;
        }
        LOG(LL_DEBUG, ("rh read %2.1f", reading));
        return reading;
    }

    float Device::tempf()
    {
        float celsius = mgos_dht_get_temp(this->dht);
        if (isnan(celsius))
        {
            LOG(LL_ERROR, ("temp read returned NaN"));
            return celsius;
        }
        LOG(LL_DEBUG, ("temp read deC %2.1f", celsius));
        return celsius * (9.0 / 5.0) + 32.0;
    }

    std::string Device::currentTime()
    {
        memset(this->current_time, 0, sizeof(this->current_time));
        time_t now = 0;
        time(&now);
        mgos_strftime(this->current_time, sizeof(this->current_time) - 1, "%c", now);
        return std::string(this->current_time);
    }

    std::string Device::getStatusJson()
    {
        std::string bid(build_id);
        std::string bts(build_timestamp);
        std::string bver(build_version);
        std::string nm(mgos_sys_config_get_garage_name());

        std::ostringstream ret;

        ret << "{" << std::endl;
        ret << " \"name\": " << '"' << nm << '"' << ',' << std::endl;
        ret << " \"doorCount\": " << doorCount << ',' << std::endl;
        ret << " \"version\": " << '"' << bver << '"' << ',' << std::endl;
        ret << " \"build_timestamp\": " << '"' << bts << '"' << ',' << std::endl;
        ret << " \"build_id\": " << '"' << bid << '"' << ',' << std::endl;
        ret << " \"currentTime\": " << '"' << currentTime() << '"' << ',' << std::endl;
        if (!isnan(rh()))
        {
            ret << " \"rh\": " << rh() << ',' << std::endl;
        }
        if (!isnan(tempf()))
        {
            ret << " \"tempf\": " << tempf() << ',' << std::endl;
        }
        ret << " \"doors\": [" << std::endl;
        bool comma = false;
        for (auto d : doors)
        {
            if (comma)
                ret << "," << std::endl;
            ret << "    {" << std::endl;
            ret << "       \"name\": \"" << d->getName() << "\"," << std::endl;
            ret << "       \"status\": \"" << d->getStatusString() << '"' << std::endl;
            ret << "    }" << std::endl;
            comma = true;
        }

        ret << " ]" << std::endl;
        ret << "}";
        return ret.str();
    }

    void Device::setIpAddr(std::string s)
    {
        this->ipAddr = std::string(s);
    }

    Door::Door(const char *aName, int aContactPin, int aActivatePin, int index)
    {
        this->activateRelayPin = aActivatePin;
        this->contactPin = aContactPin;
        this->name = std::string(aName);

        char *tmp;
        asprintf(&tmp, "door%c", ('a' + index));
        this->ordinalName = std::string(tmp);

        mgos_gpio_set_mode(this->contactPin, MGOS_GPIO_MODE_INPUT);
        mgos_gpio_setup_input(this->contactPin, MGOS_GPIO_PULL_UP);

        mgos_gpio_set_pull(this->activateRelayPin, MGOS_GPIO_PULL_UP);
        mgos_gpio_set_mode(this->activateRelayPin, MGOS_GPIO_MODE_OUTPUT);
        this->deactivate();

        LOG(LL_INFO, ("Cfg door %s, ctc=%d act=%d ord=%s", this->name.c_str(), this->contactPin, this->activateRelayPin, this->ordinalName.c_str()));
    }

    static void _deactivate_cb(void *door)
    {
        Door *d = (Door *)door;
        // switch it off
        d->deactivate();
        LOG(LL_INFO, ("timed deact door %s", d->getName().c_str()));
    }

    void Door::activate(void)
    {
        mgos_gpio_write(this->activateRelayPin, RELAY_STATE_ACTIVE);
        // async call to the future
        mgos_set_timer(mgos_sys_config_get_garage_door_activate_millis(), 0, _deactivate_cb, this);
        LOG(LL_INFO, ("act door %s pin %d", this->getName().c_str(), this->activateRelayPin));
    }

    void Door::deactivate(void)
    {
        mgos_gpio_write(this->activateRelayPin, RELAY_STATE_INACTIVE);
    }

    std::string Door::getName(void)
    {
        return this->name;
    }

    Status Door::getStatus(void)
    {
        bool val = mgos_gpio_read(this->contactPin);
        LOG(LL_DEBUG, ("%s status gpio %d", this->name.c_str(), val));
        return val ? kOpen : kClosed;
    }

    std::string Door::getStatusString(void)
    {
        switch (this->getStatus())
        {
        case kClosed:
            return "closed";
        case kOpen:
            return "open";
        default:
            return "unknown";
        }
    }
}