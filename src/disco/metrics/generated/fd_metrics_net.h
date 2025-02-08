/* THIS FILE IS GENERATED BY gen_metrics.py. DO NOT HAND EDIT. */

#include "../fd_metrics_base.h"
#include "fd_metrics_enums.h"

#define FD_METRICS_COUNTER_NET_RX_PKT_CNT_OFF  (16UL)
#define FD_METRICS_COUNTER_NET_RX_PKT_CNT_NAME "net_rx_pkt_cnt"
#define FD_METRICS_COUNTER_NET_RX_PKT_CNT_TYPE (FD_METRICS_TYPE_COUNTER)
#define FD_METRICS_COUNTER_NET_RX_PKT_CNT_DESC "Packet receive count."
#define FD_METRICS_COUNTER_NET_RX_PKT_CNT_CVT  (FD_METRICS_CONVERTER_NONE)

#define FD_METRICS_COUNTER_NET_RX_BYTES_TOTAL_OFF  (17UL)
#define FD_METRICS_COUNTER_NET_RX_BYTES_TOTAL_NAME "net_rx_bytes_total"
#define FD_METRICS_COUNTER_NET_RX_BYTES_TOTAL_TYPE (FD_METRICS_TYPE_COUNTER)
#define FD_METRICS_COUNTER_NET_RX_BYTES_TOTAL_DESC "Total number of bytes received (including Ethernet header)."
#define FD_METRICS_COUNTER_NET_RX_BYTES_TOTAL_CVT  (FD_METRICS_CONVERTER_NONE)

#define FD_METRICS_COUNTER_NET_RX_UNDERSZ_CNT_OFF  (18UL)
#define FD_METRICS_COUNTER_NET_RX_UNDERSZ_CNT_NAME "net_rx_undersz_cnt"
#define FD_METRICS_COUNTER_NET_RX_UNDERSZ_CNT_TYPE (FD_METRICS_TYPE_COUNTER)
#define FD_METRICS_COUNTER_NET_RX_UNDERSZ_CNT_DESC "Number of incoming packets dropped due to being too small."
#define FD_METRICS_COUNTER_NET_RX_UNDERSZ_CNT_CVT  (FD_METRICS_CONVERTER_NONE)

#define FD_METRICS_COUNTER_NET_RX_FILL_BLOCKED_CNT_OFF  (19UL)
#define FD_METRICS_COUNTER_NET_RX_FILL_BLOCKED_CNT_NAME "net_rx_fill_blocked_cnt"
#define FD_METRICS_COUNTER_NET_RX_FILL_BLOCKED_CNT_TYPE (FD_METRICS_TYPE_COUNTER)
#define FD_METRICS_COUNTER_NET_RX_FILL_BLOCKED_CNT_DESC "Number of incoming packets dropped due to fill ring being full."
#define FD_METRICS_COUNTER_NET_RX_FILL_BLOCKED_CNT_CVT  (FD_METRICS_CONVERTER_NONE)

#define FD_METRICS_COUNTER_NET_RX_BACKPRESSURE_CNT_OFF  (20UL)
#define FD_METRICS_COUNTER_NET_RX_BACKPRESSURE_CNT_NAME "net_rx_backpressure_cnt"
#define FD_METRICS_COUNTER_NET_RX_BACKPRESSURE_CNT_TYPE (FD_METRICS_TYPE_COUNTER)
#define FD_METRICS_COUNTER_NET_RX_BACKPRESSURE_CNT_DESC "Number of incoming packets dropped due to backpressure."
#define FD_METRICS_COUNTER_NET_RX_BACKPRESSURE_CNT_CVT  (FD_METRICS_CONVERTER_NONE)

#define FD_METRICS_COUNTER_NET_TX_SUBMIT_CNT_OFF  (21UL)
#define FD_METRICS_COUNTER_NET_TX_SUBMIT_CNT_NAME "net_tx_submit_cnt"
#define FD_METRICS_COUNTER_NET_TX_SUBMIT_CNT_TYPE (FD_METRICS_TYPE_COUNTER)
#define FD_METRICS_COUNTER_NET_TX_SUBMIT_CNT_DESC "Number of packet transmit jobs submitted."
#define FD_METRICS_COUNTER_NET_TX_SUBMIT_CNT_CVT  (FD_METRICS_CONVERTER_NONE)

#define FD_METRICS_COUNTER_NET_TX_COMPLETE_CNT_OFF  (22UL)
#define FD_METRICS_COUNTER_NET_TX_COMPLETE_CNT_NAME "net_tx_complete_cnt"
#define FD_METRICS_COUNTER_NET_TX_COMPLETE_CNT_TYPE (FD_METRICS_TYPE_COUNTER)
#define FD_METRICS_COUNTER_NET_TX_COMPLETE_CNT_DESC "Number of packet transmit jobs marked as completed by the kernel."
#define FD_METRICS_COUNTER_NET_TX_COMPLETE_CNT_CVT  (FD_METRICS_CONVERTER_NONE)

#define FD_METRICS_COUNTER_NET_TX_BYTES_TOTAL_OFF  (23UL)
#define FD_METRICS_COUNTER_NET_TX_BYTES_TOTAL_NAME "net_tx_bytes_total"
#define FD_METRICS_COUNTER_NET_TX_BYTES_TOTAL_TYPE (FD_METRICS_TYPE_COUNTER)
#define FD_METRICS_COUNTER_NET_TX_BYTES_TOTAL_DESC "Total number of bytes transmitted (including Ethernet header)."
#define FD_METRICS_COUNTER_NET_TX_BYTES_TOTAL_CVT  (FD_METRICS_CONVERTER_NONE)

#define FD_METRICS_COUNTER_NET_TX_OVERRUN_CNT_OFF  (24UL)
#define FD_METRICS_COUNTER_NET_TX_OVERRUN_CNT_NAME "net_tx_overrun_cnt"
#define FD_METRICS_COUNTER_NET_TX_OVERRUN_CNT_TYPE (FD_METRICS_TYPE_COUNTER)
#define FD_METRICS_COUNTER_NET_TX_OVERRUN_CNT_DESC "Number of packet transmit jobs aborted due to overrun by upstream tile."
#define FD_METRICS_COUNTER_NET_TX_OVERRUN_CNT_CVT  (FD_METRICS_CONVERTER_NONE)

#define FD_METRICS_COUNTER_NET_TX_ROUTE_FAIL_CNT_OFF  (25UL)
#define FD_METRICS_COUNTER_NET_TX_ROUTE_FAIL_CNT_NAME "net_tx_route_fail_cnt"
#define FD_METRICS_COUNTER_NET_TX_ROUTE_FAIL_CNT_TYPE (FD_METRICS_TYPE_COUNTER)
#define FD_METRICS_COUNTER_NET_TX_ROUTE_FAIL_CNT_DESC "Number of packet transmit jobs dropped due to route failure."
#define FD_METRICS_COUNTER_NET_TX_ROUTE_FAIL_CNT_CVT  (FD_METRICS_CONVERTER_NONE)

#define FD_METRICS_COUNTER_NET_XSK_TX_WAKEUP_CNT_OFF  (26UL)
#define FD_METRICS_COUNTER_NET_XSK_TX_WAKEUP_CNT_NAME "net_xsk_tx_wakeup_cnt"
#define FD_METRICS_COUNTER_NET_XSK_TX_WAKEUP_CNT_TYPE (FD_METRICS_TYPE_COUNTER)
#define FD_METRICS_COUNTER_NET_XSK_TX_WAKEUP_CNT_DESC "Number of XSK sendto syscalls dispatched."
#define FD_METRICS_COUNTER_NET_XSK_TX_WAKEUP_CNT_CVT  (FD_METRICS_CONVERTER_NONE)

#define FD_METRICS_COUNTER_NET_XSK_RX_WAKEUP_CNT_OFF  (27UL)
#define FD_METRICS_COUNTER_NET_XSK_RX_WAKEUP_CNT_NAME "net_xsk_rx_wakeup_cnt"
#define FD_METRICS_COUNTER_NET_XSK_RX_WAKEUP_CNT_TYPE (FD_METRICS_TYPE_COUNTER)
#define FD_METRICS_COUNTER_NET_XSK_RX_WAKEUP_CNT_DESC "Number of XSK recvmsg syscalls dispatched."
#define FD_METRICS_COUNTER_NET_XSK_RX_WAKEUP_CNT_CVT  (FD_METRICS_CONVERTER_NONE)

#define FD_METRICS_COUNTER_NET_ARP_REQUEST_CNT_OFF  (28UL)
#define FD_METRICS_COUNTER_NET_ARP_REQUEST_CNT_NAME "net_arp_request_cnt"
#define FD_METRICS_COUNTER_NET_ARP_REQUEST_CNT_TYPE (FD_METRICS_TYPE_COUNTER)
#define FD_METRICS_COUNTER_NET_ARP_REQUEST_CNT_DESC "Number of ARP requests submitted."
#define FD_METRICS_COUNTER_NET_ARP_REQUEST_CNT_CVT  (FD_METRICS_CONVERTER_NONE)

#define FD_METRICS_COUNTER_NET_ARP_REQUEST_FAIL_CNT_OFF  (29UL)
#define FD_METRICS_COUNTER_NET_ARP_REQUEST_FAIL_CNT_NAME "net_arp_request_fail_cnt"
#define FD_METRICS_COUNTER_NET_ARP_REQUEST_FAIL_CNT_TYPE (FD_METRICS_TYPE_COUNTER)
#define FD_METRICS_COUNTER_NET_ARP_REQUEST_FAIL_CNT_DESC "Number of ARP requests failed to submit."
#define FD_METRICS_COUNTER_NET_ARP_REQUEST_FAIL_CNT_CVT  (FD_METRICS_CONVERTER_NONE)

#define FD_METRICS_COUNTER_NET_XDP_RX_DROPPED_OTHER_OFF  (30UL)
#define FD_METRICS_COUNTER_NET_XDP_RX_DROPPED_OTHER_NAME "net_xdp_rx_dropped_other"
#define FD_METRICS_COUNTER_NET_XDP_RX_DROPPED_OTHER_TYPE (FD_METRICS_TYPE_COUNTER)
#define FD_METRICS_COUNTER_NET_XDP_RX_DROPPED_OTHER_DESC "xdp_statistics_v0.rx_dropped: Dropped for other reasons"
#define FD_METRICS_COUNTER_NET_XDP_RX_DROPPED_OTHER_CVT  (FD_METRICS_CONVERTER_NONE)

#define FD_METRICS_COUNTER_NET_XDP_RX_INVALID_DESCS_OFF  (31UL)
#define FD_METRICS_COUNTER_NET_XDP_RX_INVALID_DESCS_NAME "net_xdp_rx_invalid_descs"
#define FD_METRICS_COUNTER_NET_XDP_RX_INVALID_DESCS_TYPE (FD_METRICS_TYPE_COUNTER)
#define FD_METRICS_COUNTER_NET_XDP_RX_INVALID_DESCS_DESC "xdp_statistics_v0.rx_invalid_descs: Dropped due to invalid descriptor"
#define FD_METRICS_COUNTER_NET_XDP_RX_INVALID_DESCS_CVT  (FD_METRICS_CONVERTER_NONE)

#define FD_METRICS_COUNTER_NET_XDP_TX_INVALID_DESCS_OFF  (32UL)
#define FD_METRICS_COUNTER_NET_XDP_TX_INVALID_DESCS_NAME "net_xdp_tx_invalid_descs"
#define FD_METRICS_COUNTER_NET_XDP_TX_INVALID_DESCS_TYPE (FD_METRICS_TYPE_COUNTER)
#define FD_METRICS_COUNTER_NET_XDP_TX_INVALID_DESCS_DESC "xdp_statistics_v0.tx_invalid_descs: Dropped due to invalid descriptor"
#define FD_METRICS_COUNTER_NET_XDP_TX_INVALID_DESCS_CVT  (FD_METRICS_CONVERTER_NONE)

#define FD_METRICS_COUNTER_NET_XDP_RX_RING_FULL_OFF  (33UL)
#define FD_METRICS_COUNTER_NET_XDP_RX_RING_FULL_NAME "net_xdp_rx_ring_full"
#define FD_METRICS_COUNTER_NET_XDP_RX_RING_FULL_TYPE (FD_METRICS_TYPE_COUNTER)
#define FD_METRICS_COUNTER_NET_XDP_RX_RING_FULL_DESC "xdp_statistics_v1.rx_ring_full: Dropped due to rx ring being full"
#define FD_METRICS_COUNTER_NET_XDP_RX_RING_FULL_CVT  (FD_METRICS_CONVERTER_NONE)

#define FD_METRICS_COUNTER_NET_XDP_RX_FILL_RING_EMPTY_DESCS_OFF  (34UL)
#define FD_METRICS_COUNTER_NET_XDP_RX_FILL_RING_EMPTY_DESCS_NAME "net_xdp_rx_fill_ring_empty_descs"
#define FD_METRICS_COUNTER_NET_XDP_RX_FILL_RING_EMPTY_DESCS_TYPE (FD_METRICS_TYPE_COUNTER)
#define FD_METRICS_COUNTER_NET_XDP_RX_FILL_RING_EMPTY_DESCS_DESC "xdp_statistics_v1.rx_fill_ring_empty_descs: Failed to retrieve item from fill ring"
#define FD_METRICS_COUNTER_NET_XDP_RX_FILL_RING_EMPTY_DESCS_CVT  (FD_METRICS_CONVERTER_NONE)

#define FD_METRICS_COUNTER_NET_XDP_TX_RING_EMPTY_DESCS_OFF  (35UL)
#define FD_METRICS_COUNTER_NET_XDP_TX_RING_EMPTY_DESCS_NAME "net_xdp_tx_ring_empty_descs"
#define FD_METRICS_COUNTER_NET_XDP_TX_RING_EMPTY_DESCS_TYPE (FD_METRICS_TYPE_COUNTER)
#define FD_METRICS_COUNTER_NET_XDP_TX_RING_EMPTY_DESCS_DESC "xdp_statistics_v1.tx_ring_empty_descs: Failed to retrieve item from tx ring"
#define FD_METRICS_COUNTER_NET_XDP_TX_RING_EMPTY_DESCS_CVT  (FD_METRICS_CONVERTER_NONE)

#define FD_METRICS_NET_TOTAL (20UL)
extern const fd_metrics_meta_t FD_METRICS_NET[FD_METRICS_NET_TOTAL];
