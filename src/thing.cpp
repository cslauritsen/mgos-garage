#include "all.h"

void Thing::updateTempC() { this->tempC = mgos_dht_get_temp(this->dht); }

void Thing::updateRh() { this->rh = mgos_dht_get_humidity(this->dht); }

Thing::Thing(std::string aid, std::string aVersion, std::string aname)
    : homie::Device(aid, aVersion, aname) {

  this->dhtPin = mgos_sys_config_get_garage_dht_pin();
  mgos_gpio_set_mode(this->dhtPin, MGOS_GPIO_MODE_INPUT);
  this->dht = mgos_dht_create(this->dhtPin, DHT22);
  this->dhtNode =
      new homie::Node(this, NODE_NM_DHT, "DHT22 Temp/Humidity Sensor", "DHT22");

  // Fahrenheit
  this->tempFProp =
      new homie::Property(this->dhtNode, PROP_NM_TEMPF, "Temp in Fahrenheit",
                          homie::FLOAT, false, [this]() {
                            this->updateTempC();
                            if (isnan(this->tempC)) {
                              LOG(LL_ERROR, ("temp read returned NaN"));
                            } else {
                              LOG(LL_ERROR, ("tempC read %2.1f", this->tempC));
                              this->tempFProp->setValue(homie::f2s(
                                  homie::to_fahrenheit(this->tempC)));
                            }
                            return this->tempFProp->getValue();
                          });
  this->tempFProp->setUnit(homie::DEGREE_SYMBOL + "F");

  // Celsius
  this->tempCProp = new homie::Property(
      dhtNode, PROP_NM_TEMPC, "Temp in Celsius", homie::FLOAT, false, [this]() {
        this->updateTempC();
        if (isnan(this->tempC)) {
          LOG(LL_ERROR, ("temp read returned NaN"));
        } else {
          LOG(LL_ERROR, ("tempC read %2.1f", this->tempC));
          this->tempCProp->setValue(homie::f2s(this->tempC));
        }
        return this->tempFProp->getValue();
      });
  this->tempCProp->setUnit(homie::DEGREE_SYMBOL + "C");

  // Humidity
  this->rhProp =
      new homie::Property(this->dhtNode, PROP_NM_RH, "Relative Humidity",
                          homie::FLOAT, false, [this]() {
                            this->updateRh();
                            if (isnan(this->rh)) {
                              LOG(LL_ERROR, ("rh read returned NaN"));
                            } else {
                              LOG(LL_DEBUG, ("rh read %2.1f", this->rh));
                              this->rhProp->setValue(homie::f2s(this->rh));
                            }
                            return this->rhProp->getValue();
                          });
  this->rhProp->setUnit("%");

  auto doorCount = mgos_sys_config_get_garage_door_count();
  for (int i = 0; i < doorCount; i++) {
    int contactPin;
    int activatePin;
    if (i == 0) {
      contactPin = mgos_sys_config_get_garage_door_a_pin_contact();
      activatePin = mgos_sys_config_get_garage_door_a_pin_activate();
    } else if (i == 1) {
      contactPin = mgos_sys_config_get_garage_door_b_pin_contact();
      activatePin = mgos_sys_config_get_garage_door_b_pin_activate();
    } else {
      break;
    }
    char nm[10];
    sprintf(nm, "door%c", ('a' + i));
    auto nmstr = std::string(nm);
    auto humanStr = "Door ";
    humanStr += i;
    auto doorNode = new homie::Node(this, nmstr, humanStr, "GarageDoor");
    new homie::Property(
        doorNode, "isopen", "Is Open?", homie::BOOLEAN, false, [contactPin]() {
          return std::string(mgos_gpio_read(contactPin) == 0 ? "false"
                                                             : "true");
        });
    auto activateProp = new homie::Property(doorNode, "activate", "Activate",
                                            homie::BOOLEAN, true, []() {
                                              // this is a "write-only" function
                                              return std::string("false") :
                                            });
    activateProp->setWriterFunc([activatePin]() {
      mgos_gpio_write(activatePin, RELAY_STATE_ACTIVE);
      mgos_msleep((uint32_t)100);
      mgos_gpio_write(activatePin, RELAY_STATE_INACTIVE);
    });
  }
}

Thing::~Thing() {
  mgos_dht_close(this->dht);
  this->dht = NULL;
  LOG(LL_INFO, ("Closed dht"));
}