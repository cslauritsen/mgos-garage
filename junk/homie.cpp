#define tr "homie/replaceme/"
#include "homie.hpp"

namespace garage {
HomieMsg::HomieMsg(string m, int qos, bool retain) {
  this->m = string(m);
  this->qos = qos;
  this->retain = retain;
}

HomieMsg **getMessages() {

  string r = string(tr);
  HomieMsg *ret[] = {new HomieMsg(r + "$homie", "4.0", 1, true)};
  return ret;
}
} // namespace garage

let homie_setup_msgs = [
  {t : tr + '$homie', m : "4.0", qos : 1, retain : true},
  {t : tr + '$name', m : nm, qos : 1, retain : true},
  {t : tr + '$state', m : 'init', qos : 1, retain : true},
  {
    t : tr + '$nodes',
    m : 'dht22,south-door,north-door,ip',
    qos : 1,
    retain : true
  },

  {t : tr + 'ip/$name', m : 'Device IP Config', qos : 1, retain : true},
  {t : tr + 'ip/$properties', m : 'address', qos : 1, retain : true},

  {t : tr + 'ip/address/$name', m : 'ip address', qos : 1, retain : true},
  {t : tr + 'ip/address/$datatype', m : 'string', qos : 1, retain : true},
  {t : tr + 'ip/address/$settable', m : 'false', qos : 1, retain : true},

  {
    t : tr + 'dht22/$name',
    m : 'DHT22 Temp & Humidity Sensor',
    qos : 1,
    retain : true
  },
  {t : tr + 'dht22/$type', m : 'DHT22', qos : 1, retain : true},
  {t : tr + 'dht22/$properties', m : 'tempc,tempf,rh', qos : 1, retain : true},

  {
    t : tr + 'dht22/tempf/$name',
    m : 'Temperature in Fahrenheit',
    qos : 1,
    retain : true
  },
  {t : tr + 'dht22/tempf/$datatype', m : 'float', qos : 1, retain : true},
  {t : tr + 'dht22/tempf/$settable', m : 'false', qos : 1, retain : true},
  {t : tr + 'dht22/tempf/$unit', m : '°F', qos : 1, retain : true},

  {
    t : tr + 'dht22/tempc/$name',
    m : 'Temperature in Celsius',
    qos : 1,
    retain : true
  },
  {t : tr + 'dht22/tempc/$settable', m : 'false', qos : 1, retain : true},
  {t : tr + 'dht22/tempc/$datatype', m : 'float', qos : 1, retain : true},
  {t : tr + 'dht22/tempc/$unit', m : '°C', qos : 1, retain : true},

  {t : tr + 'dht22/rh/$name', m : 'Relative Humidity', qos : 1, retain : true},
  {t : tr + 'dht22/rh/$settable', m : 'false', qos : 1, retain : true},
  {t : tr + 'dht22/rh/$datatype', m : 'float', qos : 1, retain : true},
  {t : tr + 'dht22/rh/$unit', m : '%', qos : 1, retain : true},

  {
    t : tr + 'south-door/$name',
    m : 'South Garage Door',
    qos : 1,
    retain : true
  },
  {t : tr + 'south-door/$type', m : 'door', qos : 1, retain : true},
  {
    t : tr + 'south-door/$properties',
    m : 'open,activate',
    qos : 1,
    retain : true
  },
  {
    t : tr + 'south-door/open/$name',
    m : 'South Door Open?',
    qos : 1,
    retain : true
  },
  {t : tr + 'south-door/open/$settable', m : 'false', qos : 1, retain : true},
  {t : tr + 'south-door/open/$datatype', m : 'boolean', qos : 1, retain : true},
  {
    t : tr + 'south-door/activate/$name',
    m : 'Activate Door Opener',
    qos : 1,
    retain : true
  },
  {
    t : tr + 'south-door/activate/$settable',
    m : 'true',
    qos : 1,
    retain : true
  },
  {
    t : tr + 'south-door/activate/$datatype',
    m : 'boolean',
    qos : 1,
    retain : true
  },

  {
    t : tr + 'north-door/$name',
    m : 'North Garage Door',
    qos : 1,
    retain : true
  },
  {t : tr + 'north-door/$type', m : 'door', qos : 1, retain : true},
  {t : tr + 'north-door/$properties', m : 'open', qos : 1, retain : true},
  {
    t : tr + 'north-door/open/$name',
    m : 'North Door Open?',
    qos : 1,
    retain : true
  },
  {t : tr + 'north-door/open/$settable', m : 'false', qos : 1, retain : true},
  {t : tr + 'north-door/open/$datatype', m : 'boolean', qos : 1, retain : true},
  {t : tr + '$state', m : 'ready', qos : 1, retain : true}
];