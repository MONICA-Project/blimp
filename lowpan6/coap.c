#include "msg.h"
#include "thread.h"
#include "net/gcoap.h"
// own
#include "config.h"

#define ENABLE_DEBUG 1
#include "debug.h"

static sock_udp_ep_t remote;

/*
 * Response callback.
 */
static void _resp_handler(unsigned req_state, coap_pkt_t* pdu,
                          sock_udp_ep_t *remote)
{
    DEBUG("[CoAP] %s\n", __func__);
    (void)remote;       /* not interested in the source currently */

    if (req_state == GCOAP_MEMO_TIMEOUT) {
        printf("gcoap: timeout for msg ID %02u\n", coap_get_id(pdu));
        return;
    }
    else if (req_state == GCOAP_MEMO_ERR) {
        printf("gcoap: error in response\n");
        return;
    }

    char *class_str = (coap_get_code_class(pdu) == COAP_CLASS_SUCCESS)
                            ? "Success" : "Error";
    printf("gcoap: response %s, code %1u.%02u", class_str,
                                                coap_get_code_class(pdu),
                                                coap_get_code_detail(pdu));
    if (pdu->payload_len) {
        unsigned content_type = coap_get_content_type(pdu);
        if (content_type == COAP_FORMAT_TEXT
                || content_type == COAP_FORMAT_LINK
                || coap_get_code_class(pdu) == COAP_CLASS_CLIENT_FAILURE
                || coap_get_code_class(pdu) == COAP_CLASS_SERVER_FAILURE) {
            /* Expecting diagnostic payload in failure cases */
            printf(", %u bytes\n%.*s\n", pdu->payload_len, pdu->payload_len,
                                                          (char *)pdu->payload);
        }
        else {
            printf(", %u bytes\n", pdu->payload_len);
        }
    }
    else {
        printf(", empty payload\n");
    }
}

void coap_put_data(char *data, char *path)
{
    DEBUG("[CoAP] %s\n", __func__);
    uint8_t buf[GCOAP_PDU_BUF_SIZE];
    coap_pkt_t pdu;
    size_t len;
    unsigned msg_type = COAP_TYPE_NON;

    gcoap_req_init(&pdu, &buf[0], GCOAP_PDU_BUF_SIZE, COAP_METHOD_PUT, path);
    coap_hdr_set_type(pdu.hdr, msg_type);

    memcpy(pdu.payload, data, strlen(data));
    len = gcoap_finish(&pdu, strlen(data), COAP_FORMAT_JSON);
    if (gcoap_req_send2(buf, len, &remote, _resp_handler) <= 0) {
        DEBUG("[CoAP] ERROR: send failed");
    }
}

/**
 * @brief init CoAP stuff
 */
int coap_init(void)
{
    ipv6_addr_t addr;

    remote.family = AF_INET6;
    remote.netif  = SOCK_ADDR_ANY_NETIF;

    /* parse destination address */
    if (ipv6_addr_from_str(&addr, CONFIG_PROXY_ADDR) == NULL) {
        DEBUG("[CoAP] ERROR: unable to parse destination address");
        return 0;
    }
    memcpy(&remote.addr.ipv6[0], &addr.u8[0], sizeof(addr.u8));

    /* parse port */
    remote.port = (uint16_t)atoi(CONFIG_PROXY_PORT);
    if (remote.port == 0) {
        DEBUG("[CoAP] ERROR: unable to parse destination port");
        return 0;
    }
    return 0;
}
