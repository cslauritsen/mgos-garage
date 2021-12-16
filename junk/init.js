load('api_gpio.js');
load('api_config.js');
load('api_rpc.js');
load('api_dht.js');
load('api_timer.js');
load('api_sensor_utils.js');
load('api_adc.js');

let device_id = Cfg.get('project.name');
let device_ip = "unknown";

// homie topic root
let tr = 'homie/' + device_id + '/';
Cfg.set({mqtt: {will_topic: tr + '$state'}});
load('api_mqtt.js');

let nm = Cfg.get('project.name');
let dht_pin = Cfg.get('garage.dht_pin');
print('dht_pin:', dht_pin);
let pubInt = Cfg.get('garage.mqttPubInterval');
let dht = DHT.create(dht_pin, DHT.DHT22);
let counter = 0;

let n_door_contact = Cfg.get('garage.north_door_contact');
let s_door_contact = Cfg.get('garage.south_door_contact');
let s_door_activate = Cfg.get('garage.south_door_activation_pin');

//let time_now_fmt = ffi('char* time_now_fmt(void)');
/**
 * Implementation of how to trigger the door opener relay
 */
function activate_door_south() {
  GPIO.write(s_door_activate, 0);
  // Toggle the pin after the activation interval
  let wait = Cfg.get('garage.door_activate_millis');
  Timer.set(wait, 0, function() {
    GPIO.write(s_door_activate, 1);
    print('Activate state off');
  }, null);
}

let mgos_mqtt_num_unsent_bytes = ffi('int mgos_mqtt_num_unsent_bytes(void)');

let homie_setup_msgs = [
  {t: tr + '$homie', m: "4.0", qos: 1, retain: true},
  {t: tr + '$name',  m:nm, qos: 1, retain: true},
  {t: tr + '$state',  m:'init', qos: 1, retain: true},
  {t: tr + '$nodes',  m:'dht22,south-door,north-door,ip', qos: 1, retain: true},

  {t: tr + 'ip/$name',  m:'Device IP Config', qos: 1, retain: true},
  {t: tr + 'ip/$properties',  m:'address', qos: 1, retain: true},

  {t: tr + 'ip/address/$name',  m:'ip address', qos: 1, retain: true},
  {t: tr + 'ip/address/$datatype',  m:'string', qos: 1, retain: true},
  {t: tr + 'ip/address/$settable',  m:'false', qos: 1, retain: true},

  {t: tr + 'dht22/$name',  m:'DHT22 Temp & Humidity Sensor', qos: 1, retain: true},
  {t: tr + 'dht22/$type',  m:'DHT22', qos: 1, retain: true},
  {t: tr + 'dht22/$properties',  m:'tempc,tempf,rh', qos: 1, retain: true},

  {t: tr + 'dht22/tempf/$name',  m:'Temperature in Fahrenheit', qos: 1, retain: true},
  {t: tr + 'dht22/tempf/$datatype',  m:'float', qos: 1, retain: true},
  {t: tr + 'dht22/tempf/$settable',  m:'false', qos: 1, retain: true},
  {t: tr + 'dht22/tempf/$unit',  m:'°F', qos: 1, retain: true},

  {t: tr + 'dht22/tempc/$name',  m:'Temperature in Celsius', qos: 1, retain: true},
  {t: tr + 'dht22/tempc/$settable',  m:'false', qos: 1, retain: true},
  {t: tr + 'dht22/tempc/$datatype',  m:'float', qos: 1, retain: true},
  {t: tr + 'dht22/tempc/$unit',  m:'°C', qos: 1, retain: true},

  {t: tr + 'dht22/rh/$name',  m:'Relative Humidity', qos: 1, retain: true},
  {t: tr + 'dht22/rh/$settable',  m:'false', qos: 1, retain: true},
  {t: tr + 'dht22/rh/$datatype',  m:'float', qos: 1, retain: true},
  {t: tr + 'dht22/rh/$unit',  m:'%', qos: 1, retain: true},

  {t: tr + 'south-door/$name',  m:'South Garage Door', qos: 1, retain: true},
  {t: tr + 'south-door/$type',  m:'door', qos: 1, retain: true},
  {t: tr + 'south-door/$properties',  m:'open,activate', qos: 1, retain: true},
  {t: tr + 'south-door/open/$name',  m:'South Door Open?', qos: 1, retain: true},
  {t: tr + 'south-door/open/$settable',  m:'false', qos: 1, retain: true},
  {t: tr + 'south-door/open/$datatype',  m:'boolean', qos: 1, retain: true},
  {t: tr + 'south-door/activate/$name',  m:'Activate Door Opener', qos: 1, retain: true},
  {t: tr + 'south-door/activate/$settable',  m:'true', qos: 1, retain: true},
  {t: tr + 'south-door/activate/$datatype',  m:'boolean', qos: 1, retain: true},

  {t: tr + 'north-door/$name',  m:'North Garage Door', qos: 1, retain: true},
  {t: tr + 'north-door/$type',  m:'door', qos: 1, retain: true},
  {t: tr + 'north-door/$properties',  m:'open', qos: 1, retain: true},
  {t: tr + 'north-door/open/$name',  m:'North Door Open?', qos: 1, retain: true},
  {t: tr + 'north-door/open/$settable',  m:'false', qos: 1, retain: true},
  {t: tr + 'north-door/open/$datatype',  m:'boolean', qos: 1, retain: true},
  {t: tr + '$state',  m:'ready', qos: 1, retain: true}
];

// subscribe to homie set commands
MQTT.sub(tr + '+/+/set', function(conn, topic, msg) {
  print('Topic:', topic, 'message:', msg);
  if (msg === '1' || msg === 'true') {
    if (topic.indexOf('south-door/activate') !== -1) {
      activate_door_south();
      return;
    }
  }
  print("Message ", msg, ' was ignored');
}, null);
print('subscribed');


let homie_msg_ix = 0;
let homie_init = false;

// Asynchronously advance through the homie setup stuff until done
Timer.set(Cfg.get("homie.pubinterval"), true, function() {
  if (!MQTT.isConnected()) {
    print('Waiting for MQTT connect...');
    return;
  }
  let br = mgos_mqtt_num_unsent_bytes();
  let maxbr = Cfg.get("mqtt.max_unsent");
  if (br > maxbr) {
    print('home setup: waiting for MQTT queue to clear: ', br);
  }
  if (homie_msg_ix >= 0 && homie_msg_ix < homie_setup_msgs.length) {
    let msg = homie_setup_msgs[homie_msg_ix];
    let ret = MQTT.pub(msg.t, msg.m, 2, msg.retain);
    if (ret === 0) {
      print("homie pub failed: ", ret);
    }
    else {
      // if publication was successful, on the next go around, send the next message
      homie_msg_ix += 1;
    }
  }
}, null);

// Garage relay is activated by shorting to ground
GPIO.set_pull(s_door_activate, GPIO.PULL_UP);
GPIO.setup_output(s_door_activate, 1);

// Setup array for interrupt reading pin
let cpins = [
    {pin: n_door_contact, topic: "north-door/open", last_r: -1},
    {pin: s_door_contact, topic: "south-door/open", last_r: -1}
];

for (let i=0; i < cpins.length; i++) {
  let cp = cpins[i];
  GPIO.set_pull(cp.pin, GPIO.PULL_UP);
  GPIO.set_mode(cp.pin, GPIO.MODE_INPUT);
  cp.last_r = GPIO.read(cp.pin);
  MQTT.pub(tr + cp.topic, cp.last_r === 0 ? 'false' : 'true');
  GPIO.set_int_handler(cp.pin, GPIO.INT_EDGE_ANY, function(pin) {
    let v = GPIO.read(pin);
    if (v !== cp.last_r) {
      print('Pin', pin, cp.topic + ' got interrupt: ', v);
      MQTT.pub(tr + cp.topic, v === 0 ? 'false' : 'true');
    }
    cp.last_r = v;
  }, null);
  GPIO.enable_int(cp.pin); 
}

Timer.set(1000, true, function() {
  if (device_id === "unknown") {
    RPC.call(RPC.LOCAL, 'Sys.GetInfo', null, function(resp, ud) {
      device_id = resp.id;
      print('Response:', JSON.stringify(resp));
      print('device ID :', device_id);
    }, null);
  }

  let sdata = {
    dht22: {
      celsius: dht.getTemp(),
      fahrenheit: SensorUtils.fahrenheit(dht.getTemp()),
      rh: dht.getHumidity(),
    },
    doors: {
      south: { 
        is_open: (GPIO.read(s_door_contact) === 0)
      },
      north: {
        is_open: (GPIO.read(n_door_contact) === 0)
      }
    }
  };

  //print(JSON.stringify(sdata));

  if (counter++ % pubInt === 0 && homie_msg_ix >= homie_setup_msgs.length-1) {
    MQTT.pub(tr + '$state', 'ready', 1, true);
    MQTT.pub(tr + 'dht22/rh', JSON.stringify(sdata.dht22.rh));
    MQTT.pub(tr + 'dht22/tempc', JSON.stringify(sdata.dht22.celsius));
    MQTT.pub(tr + 'dht22/tempf', JSON.stringify(sdata.dht22.fahrenheit));
    MQTT.pub(tr + 'south-door/open', JSON.stringify(sdata.doors.south.is_open))
    MQTT.pub(tr + 'north-door/open', JSON.stringify(sdata.doors.north.is_open))
    MQTT.pub(tr + 'ip/address', device_ip);
  }
}, null);

// *****************************************************************
// RPC Handler Registration 
//
RPC.addHandler('Temp.Read', function(args) {
  return { value: dht.getTemp() };
});

RPC.addHandler('TempF.Read', function(args) {
  return { value: SensorUtils.fahrenheit(dht.getTemp()) };
});

RPC.addHandler('RH.Read', function(args) {
  return { value: dht.getHumidity() };
});

RPC.addHandler('SouthDoor.Activate', function(args) {
  activate_door_south();
  return { value: "Activated" };
});

/*
RPC.addHandler('Status.Read', function(args) {
  let doors = [
    {
      name: Cfg.get("doors.0.name"),
      status: 
    }
  ];
  return { 
    cc: {
      tempf: SensorUtils.fahrenheit(dht.getTemp()),
      rh: dht.getHumidity(),
      now: time_now_fmt()
    },
    doors: [
      {
        name: 
      }
    ]
  };
});
*/