#pragma once

#include <string>
#include <list>
#include <tuple>
#include <iostream>

#define DEGREE_SYMBOL "°"

namespace homie {
    class Attribute;
    class Device;
    class Node;
    class Property;

    enum DataType {
        INTEGER, STRING, FLOAT, PERCENT, BOOLEAN, ENUM, COLOR, DATETIME, DURATION
    };

    std::string DATA_TYPES[] = {
        "integer", "string", "float", "percent", "boolean", "enum", "dateTime", "duration"
    };

    enum LifecycleState {
        INIT, READY, DISCONNECTED, SLEEPING, LOST, ALERT
    };

    std::string LIFECYCLE_STATES[] = {
        "init", "ready", "disconnected", "sleeping", "lost", "alert"
    };


/**
 * @brief  Models a homie device. A homie device has 0 or many nodes, and  has exactly one mqtt connection.
 * 
 */
class Device {
    public:
    std::string name;
    std::string version;
    std::list<Node*> nodes;
    std::string topicBase;

    Device(std::string aname) {
        name = aname;
        version = "3.0";
        topicBase = std::string("homie/") + name + "/";
    }
    ~Device() {
        std::cerr << "deleting device " << name << std::endl;
        for (auto n : nodes) {
            delete n;
        }
    }

    std::list<std::pair<std::string,std::string>>* intro() {
        auto ret = new std::list<std::pair<std::string, std::string>>;
        ret->push_back(make_pair("homie/" + name + "/$homie", version));
        ret->push_back(make_pair("homie/" + name + "/$name", name));
        return ret;
    }
};


class Node {
    public:
    std::string name;
    std::list<Property*> properties;
    Device *device;
    Node(Device *d, std::string aname) { device = d; name = aname; }
    ~Node() {
        std::cerr << "deleting node " << name << std::endl;
        for (auto p : properties) {
            delete p;
        }
    }
};

class Property {
    public:
        std::string name;
        int valueInt;
        std::string valueString;
        float valueFloat;
        float valuePercent;
        bool valueBoolean;
        DataType dataType;
        std::list<Attribute*> attributes;
        Node *node;

        Property(Node *anode, std::string aname) {
            node = anode;
            name = aname;
            dataType = STRING;
        }
        ~Property() {
            std::cerr << "deleting property " << name << std::endl;
            for (auto a : attributes) {
                delete a;
            }
        }

    std::string topic() {
        return node->device->topicBase + node->name + "/" + name;
    }
    std::string value() {
        switch(dataType) {
            case  INTEGER:
            break;
            case STRING :
            break;
            case FLOAT :
             return std::to_string(valueFloat);
            case PERCENT :
            break;
            case BOOLEAN :
            break;
            case ENUM :
            break;
            case COLOR :
            break;
            case DATETIME :
            break;
            case DURATION:
            break;
            default:
            break;
        }
        return valueString;
    }
};

class Attribute {
  public:

    std::string name; // $name: friendly name of the property(string)
    DataType dataType; // payload data type

    /**
     * @brief  Format:

For integer and float: Describes a range of payloads e.g. 10:15

For enum: payload,payload,payload for enumerating all valid payloads.

For color:

    rgb to provide colors in RGB format e.g. 255,255,0 for yellow.
    hsv to provide colors in HSV format e.g. 60,100,100 for yellow.

     * 
     */
    std::string format;
    bool settable;
    bool retained;
    Property *property;

    /**
     * @brief Recommended unit strings:
        <ul>
        <li>°C: Degree Celsius
        <li>°F: Degree Fahrenheit
        <li>°: Degree
        <li>L: Liter
        <li>gal: Galon
        <li>V: Volts
        <li>W: Watt
        <li>A: Ampere
        <li>%: Percent
        <li>m: Meter
        <li>ft: Feet
        <li>Pa: Pascal
        <li>psi: PSI
        #: Count or Amount
        </ul>
     * 
     */
    std::string unit;
    Attribute(Property *p, std::string nm) {
        name = nm;
        dataType = STRING;
        retained = true;
        settable = false;
        property = p;
    }
    ~Attribute() {
        std::cerr << "Deleting attribute "  << name << std::endl;
    }
};


}