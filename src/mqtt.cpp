#include "all.h"

void homie::Device::publish(Message m) {
  mgos_mqtt_pub(m.topic.c_str(), m.payload.c_str(), m.payload.length(), m.qos,
                m.retained);
  LOG(LL_DEBUG, ("mqtt bytes: %d", mgos_mqtt_num_unsent_bytes()));
  LOG(LL_DEBUG, ("pub t=%s", m.topic.c_str()));
}

static void my_mgos_sub_handler(struct mg_connection *nc, const char *topic,
                                int topic_len, const char *msg, int msg_len,
                                void *ud) {
  Thing *t = reinterpret_cast<Thing*>(ud);
  homie::Message m(std::string(topic), std::string(msg, msg_len));
  // dispatch the command to the property
  t->onMessage(m);
}

void homie::Device::subscribe(std::string commandTopic) {
  mgos_mqtt_sub(commandTopic.c_str(), my_mgos_sub_handler, this);
}