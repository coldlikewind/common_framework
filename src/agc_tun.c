
#include "agc_tun.h"
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <net/route.h>

#define	IF_NAMESIZE	16

agc_status_t agc_tun_open(agc_std_socket_t *sock, const char *ifname, int is_tap)
{
    agc_status_t rv;
    int fd = -1;

    char *dev = "/dev/net/tun";
    int rc;
    struct ifreq ifr;
    int flags = IFF_NO_PI;

    fd = open(dev, O_RDWR);
    if (fd < 0)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_tun_open: open() failed(%d:%s) : dev[%s]", errno, strerror(errno), dev);
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = (is_tap ? (flags | IFF_TAP) : (flags | IFF_TUN));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ-1);

    rc = ioctl(fd, TUNSETIFF, (void *)&ifr);
    if (rc < 0)
    {
    	close(fd);
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_tun_open: ioctl() failed(%d:%s) : dev[%s] flags[0x%x]",
                errno, strerror(errno), ifname, flags);
        return AGC_STATUS_FALSE;
    }

    *sock = fd;
    return AGC_STATUS_SUCCESS;
}

static agc_status_t agc_parse_network(agc_ipsubnet_t *ipsub, const char *network)
{
    /* legacy syntax for ip addrs: a.b.c. ==> a.b.c.0/24 for example */
    int shift;
    char *s, *t;
    int octet;
    char buf[sizeof("255.255.255.255")];

    if (buf == NULL || strlen(network) > sizeof(buf))
    {
        return AGC_STATUS_FALSE;
    }

    strcpy(buf, network);

    /* parse components */
    s = buf;
    ipsub->sub[0] = 0;
    ipsub->mask[0] = 0;
    shift = 24;
    while (*s)
    {
        t = s;
        if (!isdigit(*t))
        {
            return AGC_STATUS_FALSE;
        }
        while (isdigit(*t))
        {
            ++t;
        }
        if (*t == '.')
        {
            *t++ = 0;
        }
        else if (*t)
        {
            return AGC_STATUS_FALSE;
        }
        if (shift < 0)
        {
            return AGC_STATUS_FALSE;
        }
        octet = atoi(s);
        if (octet < 0 || octet > 255)
        {
            return AGC_STATUS_FALSE;
        }
        ipsub->sub[0] |= octet << shift;
        ipsub->mask[0] |= 0xFFUL << shift;
        s = t;
        shift -= 8;
    }
    ipsub->sub[0] = ntohl(ipsub->sub[0]);
    ipsub->mask[0] = ntohl(ipsub->mask[0]);
    ipsub->family = AF_INET;
    return AGC_STATUS_SUCCESS;
}

/* return values:
 * CORE_EINVAL     not an IP address; caller should see
 *                 if it is something else
 * CORE_BADIP      IP address portion is is not valid
 * CORE_BADMASK    mask portion is not valid
 */
static agc_status_t agc_parse_ip(
        agc_ipsubnet_t *ipsub, const char *ipstr, int network_allowed)
{
    /* supported flavors of IP:
     *
     * . IPv6 numeric address string (e.g., "fe80::1")
     * 
     *   IMPORTANT: Don't store IPv4-mapped IPv6 address as an IPv6 address.
     *
     * . IPv4 numeric address string (e.g., "127.0.0.1")
     *
     * . IPv4 network string (e.g., "9.67")
     *
     *   IMPORTANT: This network form is only allowed if network_allowed is on.
     */
    int rc;

    rc = inet_pton(AF_INET6, ipstr, ipsub->sub);
    if (rc == 1)
    {
        if (IN6_IS_ADDR_V4MAPPED((struct in6_addr *)ipsub->sub))
        {
            /* ipsubnet_test() assumes that we don't create IPv4-mapped IPv6
             * addresses; this of course forces the user to specify 
             * IPv4 addresses in a.b.c.d style instead of ::ffff:a.b.c.d style.
             */
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Cannot support IPv4-mapped IPv6: "
                    "Use IPv4 address in a.b.c.d style "
                    "instead of ::ffff:a.b.c.d style");
            return AGC_STATUS_FALSE;
        }
        ipsub->family = AF_INET6;
    }
    else
    {
        rc = inet_pton(AF_INET, ipstr, ipsub->sub);
        if (rc == 1)
        {
            ipsub->family = AF_INET;
        }
    }
    if (rc != 1)
    {
        if (network_allowed)
        {
            return agc_parse_network(ipsub, ipstr);
        }
        else
        {
            return AGC_STATUS_FALSE;
        }
    }
    return AGC_STATUS_SUCCESS;
}

static int agc_looks_like_ip(const char *ipstr)
{
    if (strlen(ipstr) == 0)
    {
        return 0;
    }
    
    if (strchr(ipstr, ':'))
    {
        /* definitely not a hostname;
         * assume it is intended to be an IPv6 address */
        return 1;
    }

    /* simple IPv4 address string check */
    while ((*ipstr == '.') || isdigit(*ipstr))
        ipstr++;
    return (*ipstr == '\0');
}

static void agc_fix_subnet(agc_ipsubnet_t *ipsub)
{
    /* in case caller specified more bits in network address than are
     * valid according to the mask, turn off the extra bits
     */
    int i;

    for (i = 0; i < sizeof ipsub->mask / sizeof(int32_t); i++)
    {
        ipsub->sub[i] &= ipsub->mask[i];
    }
}

/* be sure not to store any IPv4 address as a v4-mapped IPv6 address */
agc_status_t agc_core_ipsubnet(agc_ipsubnet_t *ipsub, const char *ipstr, const char *mask_or_numbits)
{
    agc_status_t rv;
    char *endptr;
    long bits, maxbits = 32;

    if (!agc_looks_like_ip(ipstr))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_looks_like_ip() is failed.\n");
        return AGC_STATUS_FALSE;
    }

    /* assume ipstr is an individual IP address, not a subnet */
    memset(ipsub->mask, 0xFF, sizeof ipsub->mask);

    rv = agc_parse_ip(ipsub, ipstr, mask_or_numbits == NULL);
    if (rv != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_parse_ip() is failed.\n");
        return rv;
    }

    if (mask_or_numbits)
    {
        if (ipsub->family == AF_INET6)
        {
            maxbits = 128;
        }
        bits = strtol(mask_or_numbits, &endptr, 10);
        if (*endptr == '\0' && bits > 0 && bits <= maxbits)
        {
            /* valid num-bits string; fill in mask appropriately */
            int cur_entry = 0;
            int32_t cur_bit_value;

            memset(ipsub->mask, 0, sizeof ipsub->mask);
            while (bits > 32)
            {
                ipsub->mask[cur_entry] = 0xFFFFFFFF; /* all 32 bits */
                bits -= 32;
                ++cur_entry;
            }
            cur_bit_value = 0x80000000;
            while (bits)
            {
                ipsub->mask[cur_entry] |= cur_bit_value;
                --bits;
                cur_bit_value /= 2;
            }
            ipsub->mask[cur_entry] = htonl(ipsub->mask[cur_entry]);
        }
        else if (inet_pton(AF_INET, mask_or_numbits, ipsub->mask) == 1 &&
            ipsub->family == AF_INET)
        {
            /* valid IPv4 netmask */
        }
        else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_core_ipsubnet: Bad netmask.\n");
            return AGC_STATUS_FALSE;
        }
    }

    agc_fix_subnet(ipsub);

    return AGC_STATUS_SUCCESS;
}