
#include "all.h"

void homie::Device::computePsk()
{
    int rc = 0;
    unsigned char output[64];
    int is384 = 0;
    rc = mbedtls_sha512_ret(
        (const unsigned char *)this->topicBase.c_str(),
        this->topicBase.length(),
        output,
        is384);
    if (0 == rc)
    {
        char *hex = (char *)calloc(1, sizeof(output) * 2 + 1);
        char *p = hex;
        for (size_t i = 0; i < sizeof(output); i++)
        {
            sprintf(p, "%02x", output[i]);
            p += 2;
        }
        this->psk = std::string(static_cast<const char *>(hex));
        free(hex);
    }
    else
    {
        LOG(LL_ERROR, ("SHA512 failed: %d", rc));
    }
}

void homie::Device::publish(Message &m)
{
    mgos_mqtt_pub(m.topic.c_str(), m.payload.c_str(), m.payload.length(), m.qos, m.retained);
    LOG(LL_DEBUG, ("pub t=%s", m.topic.c_str()));
}