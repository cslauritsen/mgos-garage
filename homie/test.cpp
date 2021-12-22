#include "homie.hpp"
#include <iostream>
#include <string>
using namespace std;
int main(int argc, char** argv) {
    cout << "hi" << endl;

    homie::Device *d = new homie::Device("test1");
    homie::Node *node  = new homie::Node(d, "dht22");

    auto tempProp = new homie::Property(node, "temp");
 //   auto tempAttr = new homie::Attribute(tempProp, "tempf");
  //  tempAttr->unit = std::string(DEGREE_SYMBOL) + std::string("F");
   // tempProp->attributes.push_back(tempAttr);
    tempProp->dataType = homie::FLOAT;
    tempProp->valueFloat = 72.0;
    node->properties.push_back(tempProp);

    auto rhProp = new homie::Property(node, "temp");
//    auto rhAttr = new homie::Attribute(tempProp, "tempf");
    //rhAttr->unit = "%";
    //rhProp->attributes.push_back(rhAttr);
    rhProp->dataType = homie::FLOAT;
    rhProp->valueFloat = 69.0;

    node->properties.push_back(rhProp);

    d->nodes.push_back(node);

/*
    for (auto n : d->nodes) {
    }
    auto l = d->intro();
    for (auto e : *l) {
        std::cout << e.first << " →  " << e.second << std::endl;
    }
    delete l;
    */
   for (auto n : d->nodes) {
       for (auto p : n->properties) {
            std::cout << p->topic() << " →  " << p->value() << std::endl;
       }
   }
    delete d;
}