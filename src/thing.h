#pragma once
#include <string>
#include <homie.hpp>


#define RELAY_STATE_ACTIVE 0
#define RELAY_STATE_INACTIVE 1

class Thing : public homie::Device {

public:
  uint8_t dhtPin;
  struct mgos_dht *dht;
  homie::Node *dhtNode;

  float tempC;
  void updateTempC();
  float rh;
  void updateRh();
  homie::Property *tempFProp;
  homie::Property *tempCProp;
  homie::Property *rhProp;



  Thing(std::string aid, std::string aVersion, std::string aname);
  ~Thing();
};
