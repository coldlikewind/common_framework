#ifndef AGC_TUN_H
#define AGC_TUN_H

#include <agc.h>

AGC_BEGIN_EXTERN_C

typedef struct agc_ipsubnet_t {
    int family;

    uint32_t sub[4]; /* big enough for IPv4 and IPv6 addresses */
    uint32_t mask[4];
} agc_ipsubnet_t;

AGC_DECLARE(agc_status_t) agc_tun_open(agc_std_socket_t *sock, const char *ifname, int is_tap);
AGC_DECLARE(agc_status_t) agc_tun_set_ip(agc_std_socket_t *sock, const char *ifname, agc_ipsubnet_t *gw, agc_ipsubnet_t *sub);

AGC_DECLARE(agc_status_t)  agc_core_ipsubnet(agc_ipsubnet_t *ipsub, const char *ipstr, const char *mask_or_numbits);

AGC_END_EXTERN_C

#endif
