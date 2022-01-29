#include "all.h"

#define VALID_PIN(x) ((x) >= 0 && (x) <= 15)

void Thing::updateTempC() { this->tempC = mgos_dht_get_temp(this->dht); }

void Thing::updateRh() { this->rh = mgos_dht_get_humidity(this->dht); }

static void int_door_contact_cb(int pin, void *obj) {
  static double debounce = -1.0;
  static int lastPin = -1;

  double interrupt_millis = mgos_uptime() * 1000.0;
  if (debounce < 0.0 || lastPin != pin || abs(interrupt_millis - debounce) >
                            mgos_sys_config_get_time_debounce_millis()) {
    auto doorProp = reinterpret_cast<homie::Property *>(obj);
    doorProp->publish();
  } else {
    LOG(LL_DEBUG, ("interrupt debounced pin %d", pin));
  }
  debounce = interrupt_millis;
  lastPin = pin;
}

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
                              LOG(LL_DEBUG, ("tempC read %2.1f", this->tempC));
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
          LOG(LL_DEBUG, ("tempC read %2.1f", this->tempC));
          this->tempCProp->setValue(homie::f2s(this->tempC));
        }
        return this->tempCProp->getValue();
      });
  this->tempCProp->setUnit(homie::DEGREE_SYMBOL + "C");

  // Humidity
  auto rhReadFn = [this]() {
    this->updateRh();
    if (isnan(this->rh)) {
      LOG(LL_ERROR, ("rh read returned NaN"));
    } else {
      LOG(LL_DEBUG, ("rh read %2.1f", this->rh));
      this->rhProp->setValue(homie::f2s(this->rh));
    }
    return this->rhProp->getValue();
  };
  this->rhProp =
      new homie::Property(this->dhtNode, PROP_NM_RH, "Relative Humidity",
                          homie::FLOAT, false, rhReadFn);
  this->rhProp->setUnit("%");

  // get a pointer to the "container" struct for all door configs
  const struct mgos_config_garage_door *doors =
      mgos_sys_config_get_garage_door();

  // Get a pointer to the first door in the container
  const struct mgos_config_garage_door_a *door = &doors->a;
  // if door count is bigger than the number of doors actually configured
  // there will be blood
  for (int i = 0; i < doors->count; i++) {
    if (VALID_PIN(door->pin.contact) || VALID_PIN(door->pin.activate)) {
      auto nodeName = "door";
      nodeName += ('a' + i);
      std::string humanStr("");
      if (door->name) { // could be NULL
        humanStr += door->name;
      } else {
        humanStr = "Door ";
        humanStr += 'A' + i;
      }

      auto doorNode = new homie::Node(this, nodeName, humanStr, "GarageDoor");

      if (VALID_PIN(door->pin.contact)) {

        auto ctcRdFn = [door]() {
          return std::string(mgos_gpio_read(door->pin.contact) == 0 ? "false"
                                                                    : "true");
        };
        auto contactProp = new homie::Property(doorNode, "isopen", "Is Open?",
                                               homie::BOOLEAN, false, ctcRdFn);
        mgos_gpio_set_int_handler(door->pin.contact, MGOS_GPIO_INT_EDGE_ANY,
                                  int_door_contact_cb, contactProp);
      }

      if (VALID_PIN(door->pin.activate)) {
        // Called when the property is read
        auto actRdFn = [door]() {
          return mgos_gpio_read(door->pin.activate) == RELAY_STATE_ACTIVE
                     ? "true"
                     : "false";
        };
        // Called when the property is written
        auto actWrFn = [door](std::string s) {
          if (s == "true") {
            mgos_gpio_write(door->pin.activate, RELAY_STATE_ACTIVE);
            mgos_msleep((uint32_t) RELAY_STATE_ACTIVE_DUR_MILLIS);
            mgos_gpio_write(door->pin.activate, RELAY_STATE_INACTIVE);
          } else {
            LOG(LL_DEBUG, ("activate noop"));
          }
        };
        auto activateProp = new homie::Property(
            doorNode, "activate", "Activate", homie::BOOLEAN, true, actRdFn);
        activateProp->setWriterFunc(actWrFn);
      }
    }

    // We rely on the impl detail:
    //  * All the doors are positioned next to each other in a parent struct
    //  * All the doors have the same size and member names
    // If these do not hold true, then, if we are lucky,
    // the program will crash.
    // If we are unlucky the program continues with bad data
    // it would be nice if the config schema provide a means to specify an
    // array of same-typed objects to help resolve this issue. Maybe they do
    // I just haven't figure it it out yet.
    door++;
  }
}

Thing::~Thing() {
  mgos_dht_close(this->dht);
  this->dht = NULL;
  LOG(LL_INFO, ("Closed dht"));
}