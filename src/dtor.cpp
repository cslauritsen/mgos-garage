#include "all.h"
#include <iostream>

homie::Device::~Device() {
  for (auto node : this->nodes) {
    delete node.second;
  }
}

homie::Node::~Node() {
  std::cerr << " . Deleting node " << this->id << std::endl;
  for (auto prop : this->properties) {
    delete prop.second;
  }
}

homie::Property::~Property() {
  std::cerr << " .   Deleting prop " << this->id << std::endl;
}