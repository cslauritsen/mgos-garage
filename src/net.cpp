#include "all.h"

static void network_config_rpc_cb(struct mg_rpc *c, void *cb_arg, struct mg_rpc_frame_info *fi, struct mg_str result, int error_code, struct mg_str error_msg)
{
    auto *homieDevice = (homie::Device *)cb_arg;
    LOG(LL_INFO, ("error code: %d", error_code));
    char *ip = NULL;
    char *mac = NULL;
    char *id = NULL;
    int scan_result = json_scanf(result.p, result.len, "{id: %Q, mac: %Q, wifi: {sta_ip: %Q}}", &id, &mac, &ip);

    if (id)
    {
        LOG(LL_DEBUG, ("id: %s", id));
        free(id);
    }
    LOG(LL_DEBUG, ("jsonf_scan result: %d", scan_result));
    if (scan_result < 0)
    {
        LOG(LL_ERROR, ("json scanf error"));
    }
    else if (0 == scan_result)
    {
        LOG(LL_ERROR, ("json scanf keys not found"));
    }
    else
    {
        if (ip)
        {
            LOG(LL_INFO, ("ip: %s", ip));
            std::string str = std::string(ip);
            free(ip);
            auto oldVal = homieDevice->getLocalIp();
            homieDevice->setLocalIp(str);
            if (oldVal != str)
            {
                homieDevice->publish(homie::Message(homieDevice->getTopicBase() + "$localip", str));
            }
        }
        else
        {
            LOG(LL_ERROR, ("json_scanf failed to find ip address"));
        }
        if (mac)
        {
            LOG(LL_INFO, ("mac: %s", mac));
            auto str = std::string(mac);
            free(mac);
            auto oldVal = homieDevice->getMac();
            homieDevice->setMac(str);
            if (oldVal != str)
            {
                homieDevice->publish(homie::Message(homieDevice->getTopicBase() + "$mac", str));
            }
        }
        else
        {
            LOG(LL_ERROR, ("json_scanf failed to find mac address"));
        }
    }
}

void homie::Device::inquireNetConfig()
{
    static int callCount = 1;
    LOG(LL_DEBUG, ("net_inquire_config called %d x", callCount));
    struct mg_rpc_call_opts opts = {.dst = mg_mk_str(MGOS_RPC_LOOPBACK_ADDR)};
    mg_rpc_callf(mgos_rpc_get_global(), mg_mk_str("Sys.GetInfo"), network_config_rpc_cb, this, &opts, NULL);
    callCount++;
}