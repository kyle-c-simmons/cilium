/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause) */
/* Copyright Authors of Cilium */

#pragma once

#include <bpf/ctx/ctx.h>
#include <bpf/api.h>

#include <linux/if_ether.h>
#include <linux/ipv6.h>
#include <linux/in.h>
#include <linux/socket.h>

#include "endian.h"
#include "eth.h"
#include "mono.h"
#include "config.h"
#include "tunnel.h"
#include "notify.h"

#include "source_info.h"

#ifndef AF_INET
#define AF_INET 2
#endif

#ifndef AF_INET6
#define AF_INET6 10
#endif

#ifndef IP_DF
#define IP_DF 0x4000
#endif

#ifndef EVENT_SOURCE
#define EVENT_SOURCE 0
#endif

#ifdef PREALLOCATE_MAPS
#define CONDITIONAL_PREALLOC 0
#else
#define CONDITIONAL_PREALLOC BPF_F_NO_PREALLOC
#endif

#if defined(ENABLE_EGRESS_GATEWAY)
#define ENABLE_EGRESS_GATEWAY_COMMON
#endif

#if defined(ENCAP_IFINDEX) || defined(ENABLE_EGRESS_GATEWAY_COMMON) || \
    (defined(ENABLE_DSR) && DSR_ENCAP_MODE == DSR_ENCAP_GENEVE)
#define HAVE_ENCAP	1

/* NOT_VTEP_DST is passed to an encapsulation function when the
 * destination of the tunnel is not a VTEP.
 */
#define NOT_VTEP_DST 0
#endif

/* XFER_FLAGS that get transferred from XDP to SKB */
enum {
	XFER_PKT_NO_SVC		= (1 << 0),  /* Skip upper service handling. */
	XFER_UNUSED		= (1 << 1),
	XFER_PKT_SNAT_DONE	= (1 << 2),  /* SNAT is done */
};

/* For use in ctx_get_xfer(), after XDP called ctx_move_xfer(). */
enum {
	XFER_FLAGS = 0,		/* XFER_PKT_* */
};

/* FIB errors from BPF neighbor map. */
#define BPF_FIB_MAP_NO_NEIGH	100

#define CILIUM_CALL_DROP_NOTIFY			1
#define CILIUM_CALL_ERROR_NOTIFY		2
/*
 * A gap in the macro numbering sequence was created by #24921.
 * It can be reused for a new macro in the future, but caution is needed when
 * backporting changes as it may conflict with older versions of the code.
 */
#define CILIUM_CALL_HANDLE_ICMP6_NS		4
#define CILIUM_CALL_SEND_ICMP6_TIME_EXCEEDED	5
#define CILIUM_CALL_ARP				6
#define CILIUM_CALL_IPV4_FROM_LXC		7
#define CILIUM_CALL_IPV4_FROM_NETDEV		CILIUM_CALL_IPV4_FROM_LXC
#define CILIUM_CALL_IPV4_FROM_OVERLAY		CILIUM_CALL_IPV4_FROM_LXC
#define CILIUM_CALL_IPV46_RFC6052		8
#define CILIUM_CALL_IPV64_RFC6052		9
#define CILIUM_CALL_IPV6_FROM_LXC		10
#define CILIUM_CALL_IPV6_FROM_NETDEV		CILIUM_CALL_IPV6_FROM_LXC
#define CILIUM_CALL_IPV6_FROM_OVERLAY		CILIUM_CALL_IPV6_FROM_LXC
#define CILIUM_CALL_IPV4_TO_LXC_POLICY_ONLY	11
#define CILIUM_CALL_IPV4_TO_HOST_POLICY_ONLY	CILIUM_CALL_IPV4_TO_LXC_POLICY_ONLY
#define CILIUM_CALL_IPV6_TO_LXC_POLICY_ONLY	12
#define CILIUM_CALL_IPV6_TO_HOST_POLICY_ONLY	CILIUM_CALL_IPV6_TO_LXC_POLICY_ONLY
#define CILIUM_CALL_IPV4_TO_ENDPOINT		13
#define CILIUM_CALL_IPV6_TO_ENDPOINT		14
#define CILIUM_CALL_IPV4_NODEPORT_NAT_EGRESS	15
#define CILIUM_CALL_IPV6_NODEPORT_NAT_EGRESS	16
#define CILIUM_CALL_IPV4_NODEPORT_REVNAT	17
#define CILIUM_CALL_IPV6_NODEPORT_REVNAT	18
#define CILIUM_CALL_IPV4_NODEPORT_NAT_FWD	19
#define CILIUM_CALL_IPV4_NODEPORT_DSR		20
#define CILIUM_CALL_IPV6_NODEPORT_DSR		21
#define CILIUM_CALL_IPV4_FROM_HOST		22
#define CILIUM_CALL_IPV6_FROM_HOST		23
#define CILIUM_CALL_IPV6_NODEPORT_NAT_FWD	24
#define CILIUM_CALL_IPV4_FROM_LXC_CONT		25
#define CILIUM_CALL_IPV6_FROM_LXC_CONT		26
#define CILIUM_CALL_IPV4_CT_INGRESS		27
#define CILIUM_CALL_IPV4_CT_INGRESS_POLICY_ONLY	28
#define CILIUM_CALL_IPV4_CT_EGRESS		29
#define CILIUM_CALL_IPV6_CT_INGRESS		30
#define CILIUM_CALL_IPV6_CT_INGRESS_POLICY_ONLY	31
#define CILIUM_CALL_IPV6_CT_EGRESS		32
#define CILIUM_CALL_SRV6_ENCAP			33
#define CILIUM_CALL_SRV6_DECAP			34
#define CILIUM_CALL_IPV4_NODEPORT_NAT_INGRESS	35
#define CILIUM_CALL_IPV6_NODEPORT_NAT_INGRESS	36
#define CILIUM_CALL_IPV4_NODEPORT_SNAT_FWD	37
#define CILIUM_CALL_IPV6_NODEPORT_SNAT_FWD	38
#define CILIUM_CALL_IPV4_INTER_CLUSTER_REVSNAT	39
#define CILIUM_CALL_IPV4_CONT_FROM_HOST		40
#define CILIUM_CALL_IPV4_CONT_FROM_NETDEV	41
#define CILIUM_CALL_IPV6_CONT_FROM_HOST		42
#define CILIUM_CALL_IPV6_CONT_FROM_NETDEV	43
#define CILIUM_CALL_IPV4_NO_SERVICE		44
#define CILIUM_CALL_IPV6_NO_SERVICE		45
#define CILIUM_CALL_MULTICAST_EP_DELIVERY	46
#define CILIUM_CALL_SIZE			47

typedef __u64 mac_t;

union v6addr {
	struct {
		__u32 p1;
		__u32 p2;
		__u32 p3;
		__u32 p4;
	};
	struct {
		__u64 d1;
		__u64 d2;
	};
	__u8 addr[16];
} __packed;

static __always_inline bool validate_ethertype_l2_off(struct __ctx_buff *ctx,
						      int l2_off, __u16 *proto)
{
	const __u64 tot_len = l2_off + ETH_HLEN;
	void *data_end = ctx_data_end(ctx);
	void *data = ctx_data(ctx);
	struct ethhdr *eth;

	if (ETH_HLEN == 0) {
		/* The packet is received on L2-less device. Determine L3
		 * protocol from skb->protocol.
		 */
		*proto = ctx_get_protocol(ctx);
		return true;
	}

	if (data + tot_len > data_end)
		return false;

	eth = data + l2_off;

	*proto = eth->h_proto;

	return eth_is_supported_ethertype(*proto);
}

static __always_inline bool validate_ethertype(struct __ctx_buff *ctx,
					       __u16 *proto)
{
	return validate_ethertype_l2_off(ctx, 0, proto);
}

static __always_inline __maybe_unused bool
____revalidate_data_pull(struct __ctx_buff *ctx, void **data_, void **data_end_,
			 void **l3, const __u32 l3_len, const bool pull,
			 __u32 l3_off)
{
	const __u64 tot_len = l3_off + l3_len;
	void *data_end;
	void *data;

	/* Verifier workaround, do this unconditionally: invalid size of register spill. */
	if (pull)
		ctx_pull_data(ctx, (__u32)tot_len);
	data_end = ctx_data_end(ctx);
	data = ctx_data(ctx);
	if (data + tot_len > data_end)
		return false;

	/* Verifier workaround: pointer arithmetic on pkt_end prohibited. */
	*data_ = data;
	*data_end_ = data_end;

	*l3 = data + l3_off;
	return true;
}

static __always_inline __maybe_unused bool
__revalidate_data_pull(struct __ctx_buff *ctx, void **data, void **data_end,
		       void **l3, const __u32 l3_off, const __u32 l3_len,
		       const bool pull)
{
	return ____revalidate_data_pull(ctx, data, data_end, l3, l3_len, pull,
					l3_off);
}

static __always_inline __u32 get_tunnel_id(__u32 identity)
{
#if defined ENABLE_IPV4 && defined ENABLE_IPV6
	if (identity == WORLD_IPV4_ID || identity == WORLD_IPV6_ID)
		return WORLD_ID;
#endif
	return identity;
}

static __always_inline __u32 get_id_from_tunnel_id(__u32 tunnel_id, __u16 proto  __maybe_unused)
{
#if defined ENABLE_IPV4 && defined ENABLE_IPV6
	if (tunnel_id == WORLD_ID) {
		switch (proto) {
		case bpf_htons(ETH_P_IP):
			return WORLD_IPV4_ID;
		case bpf_htons(ETH_P_IPV6):
			return WORLD_IPV6_ID;
		}
	}
#endif
	return tunnel_id;
}

/* revalidate_data_pull() initializes the provided pointers from the ctx and
 * ensures that the data is pulled in for access. Should be used the first
 * time that the ctx data is accessed, subsequent calls can be made to
 * revalidate_data() which is cheaper.
 * Returns true if 'ctx' is long enough for an IP header of the provided type,
 * false otherwise.
 */
#define revalidate_data_pull(ctx, data, data_end, ip)			\
	__revalidate_data_pull(ctx, data, data_end, (void **)ip, ETH_HLEN, sizeof(**ip), true)

#define revalidate_data_l3_off(ctx, data, data_end, ip, l3_off)		\
	__revalidate_data_pull(ctx, data, data_end, (void **)ip, l3_off, sizeof(**ip), false)

/* revalidate_data() initializes the provided pointers from the ctx.
 * Returns true if 'ctx' is long enough for an IP header of the provided type,
 * false otherwise.
 */
#define revalidate_data(ctx, data, data_end, ip)			\
	revalidate_data_l3_off(ctx, data, data_end, ip, ETH_HLEN)

#define ENDPOINT_KEY_IPV4 1
#define ENDPOINT_KEY_IPV6 2

/* Structure representing an IPv4 or IPv6 address, being used as the key
 * for the endpoints map.
 */
struct endpoint_key {
	union {
		struct {
			__u32		ip4;
			__u32		pad1;
			__u32		pad2;
			__u32		pad3;
		};
		union v6addr	ip6;
	};
	__u8 family;
	__u8 key;
	__u16 cluster_id;
} __packed;

struct tunnel_key {
	union {
		struct {
			__u32		ip4;
			__u32		pad1;
			__u32		pad2;
			__u32		pad3;
		};
		union v6addr	ip6;
	};
	__u8 family;
	__u8 pad;
	__u16 cluster_id;
} __packed;

struct tunnel_value {
	union {
		struct {
			__u32		ip4;
			__u32		pad1;
			__u32		pad2;
			__u32		pad3;
		};
		union v6addr	ip6;
	};
	__u8 family;
	__u8 key;
	__u16 pad;
} __packed;

#define ENDPOINT_F_HOST			1 /* Special endpoint representing local host */
#define ENDPOINT_F_ATHOSTNS		2 /* Endpoint located at the host networking namespace */
#define ENDPOINT_MASK_HOST_DELIVERY	(ENDPOINT_F_HOST | ENDPOINT_F_ATHOSTNS)

/* Value of endpoint map */
struct endpoint_info {
	__u32		ifindex;
	__u16		unused; /* used to be sec_label, no longer used */
	__u16		lxc_id;
	__u32		flags;
	mac_t		mac;
	mac_t		node_mac;
	__u32		sec_id;
	__u32		parent_ifindex;
	__u32		pad[2];
};

#define DIRECTION_EGRESS 0
#define DIRECTION_INGRESS 1

struct edt_id {
	__u32		id;
	__u8		direction;
	__u8		pad[3];
};

struct edt_info {
	__u64		bps;
	__u64		t_last;
	union {
		__u64	t_horizon_drop;
		__u64	tokens;
	};
	__u32		prio;
	__u32		pad_32;
	__u64		pad[3];
};

struct remote_endpoint_info {
	__u32		sec_identity;
	union {
		struct {
			__u32	ip4;
			__u32	pad1;
			__u32	pad2;
			__u32	pad3;
		};
		union v6addr	ip6;
	} tunnel_endpoint;
	__u16		pad;
	__u8		key;
	__u8		flag_skip_tunnel:1,
			flag_has_tunnel_ep:1,
			flag_ipv6_tunnel_ep:1,
			pad2:5;
};

/*
 * Longest-prefix match map lookup only matches the number of bits from the
 * beginning of the key stored in the map indicated by the 'lpm_key' field in
 * the same stored map key, not including the 'lpm_key' field itself. Note that
 * the 'lpm_key' value passed in the lookup function argument needs to be a
 * "full prefix" (POLICY_FULL_PREFIX defined below).
 *
 * Since we need to be able to wildcard 'sec_label' independently on 'protocol'
 * and 'dport' fields, we'll need to do that explicitly with a separate lookup
 * where 'sec_label' is zero. For the 'protocol' and 'port' we can use the
 * longest-prefix match by placing them at the end ot the key in this specific
 * order, as we want to be able to wildcard those fields in a specific pattern:
 * 'protocol' can only be wildcarded if dport is also fully wildcarded.
 * 'protocol' is never partially wildcarded, so it is either fully wildcarded or
 * not wildcarded at all. 'dport' can be partially wildcarded, but only when
 * 'protocol' is fully specified. This follows the logic that the destination
 * port is a property of a transport protocol and can not be specified without
 * also specifying the protocol.
 */
struct policy_key {
	struct bpf_lpm_trie_key lpm_key;
	__u32		sec_label;
	__u8		egress:1,
			pad:7;
	__u8		protocol; /* can be wildcarded if 'dport' is fully wildcarded */
	__be16		dport; /* can be wildcarded with CIDR-like prefix */
};

/* POLICY_FULL_PREFIX gets full prefix length of policy_key */
#define POLICY_FULL_PREFIX						\
  (8 * (sizeof(struct policy_key) - sizeof(struct bpf_lpm_trie_key)))

struct policy_entry {
	__be16		proxy_port;
	__u8		deny:1,
			reserved:2, /* bits used in Cilium 1.16, keep unused for Cilium 1.17 */
			lpm_prefix_length:5; /* map key protocol and dport prefix length */
	__u8		auth_type:7,
			has_explicit_auth_type:1;
	__u8		proxy_port_priority;
	__u8		pad1;
	__u16	        pad2;
};

/*
 * LPM_FULL_PREFIX_BITS is the maximum length in 'lpm_prefix_length' when none of the protocol or
 * dport bits in the key are wildcarded.
 */
#define LPM_PROTO_PREFIX_BITS 8                             /* protocol specified */
#define LPM_FULL_PREFIX_BITS (LPM_PROTO_PREFIX_BITS + 16)   /* protocol and dport specified */

/*
 * policy_stats_key has the same layout as policy_key, apart from the first four bytes.
 */
struct policy_stats_key {
	__u16		endpoint_id;
	__u8		pad1;
	__u8		prefix_len;
	__u32		sec_label;
	__u8		egress:1,
			pad:7;
	__u8		protocol; /* can be wildcarded if 'dport' is fully wildcarded */
	__be16		dport; /* can be wildcarded with CIDR-like prefix */
};

struct policy_stats_value {
	__u64		packets;
	__u64		bytes;
};

struct auth_key {
	__u32       local_sec_label;
	__u32       remote_sec_label;
	__u16       remote_node_id; /* zero for local node */
	__u8        auth_type;
	__u8        pad;
};

/* expiration is Unix epoch time in unit nanosecond/2^9 (ns/512). */
struct auth_info {
	__u64       expiration;
};

struct metrics_key {
	__u8      reason;	/* 0: forwarded, >0 dropped */
	__u8      dir:2,	/* 1: ingress 2: egress */
		  pad:6;
	__u16	  line;		/* __MAGIC_LINE__ */
	__u8	  file;		/* __MAGIC_FILE__, needs to fit __id_for_file */
	__u8	  reserved[3];	/* reserved for future extension */
};


struct metrics_value {
	__u64	count;
	__u64	bytes;
};

struct egress_gw_policy_key {
	struct bpf_lpm_trie_key lpm_key;
	__u32 saddr;
	__u32 daddr;
};

struct egress_gw_policy_entry {
	__u32 egress_ip;
	__u32 gateway_ip;
};

struct egress_gw_policy_key6 {
	struct bpf_lpm_trie_key lpm_key;
	union v6addr saddr;
	union v6addr daddr;
};

struct egress_gw_policy_entry6 {
	union v6addr egress_ip;
	__u32 gateway_ip;
	__u32 reserved[3]; /* reserved for future extension, e.g. v6 gateway_ip */
};

struct srv6_vrf_key4 {
	struct bpf_lpm_trie_key lpm;
	__u32 src_ip;
	__u32 dst_cidr;
};

struct srv6_vrf_key6 {
	struct bpf_lpm_trie_key lpm;
	union v6addr src_ip;
	union v6addr dst_cidr;
};

struct srv6_policy_key4 {
	struct bpf_lpm_trie_key lpm;
	__u32 vrf_id;
	__u32 dst_cidr;
};

struct srv6_policy_key6 {
	struct bpf_lpm_trie_key lpm;
	__u32 vrf_id;
	union v6addr dst_cidr;
};

struct node_key {
	__u16 pad1;
	__u8 pad2;
	__u8 family;
	union {
		struct {
			__u32 ip4;
			__u32 pad4;
			__u32 pad5;
			__u32 pad6;
		};
		union v6addr ip6;
	};
};

struct node_value {
	__u16 id;
	__u8  spi;
	__u8  pad;
};

enum {
	POLICY_INGRESS = 1,
	POLICY_EGRESS = 2,
};

enum {
	POLICY_MATCH_NONE = 0,
	POLICY_MATCH_L3_ONLY = 1,
	POLICY_MATCH_L3_L4 = 2,
	POLICY_MATCH_L4_ONLY = 3,
	POLICY_MATCH_ALL = 4,
	POLICY_MATCH_L3_PROTO = 5,
	POLICY_MATCH_PROTO_ONLY = 6,
};

enum {
	CAPTURE_INGRESS = 1,
	CAPTURE_EGRESS = 2,
};

#ifndef TRACE_PAYLOAD_LEN
#define TRACE_PAYLOAD_LEN 128ULL
#endif

#ifndef BPF_F_PSEUDO_HDR
# define BPF_F_PSEUDO_HDR                (1ULL << 4)
#endif

#define IS_ERR(x) (unlikely((x < 0) || (x == CTX_ACT_DROP)))

/* Return value to indicate that proxy redirection is required */
#define POLICY_ACT_PROXY_REDIRECT (1 << 16)

/* Cilium error codes, must NOT overlap with TC return codes.
 * These also serve as drop reasons for metrics,
 * where reason > 0 corresponds to -(DROP_*)
 *
 * These are shared with pkg/monitor/api/drop.go and api/v1/flow/flow.proto.
 * When modifying any of the below, those files should also be updated.
 */
#define DROP_UNUSED1		-130 /* unused */
#define DROP_UNUSED2		-131 /* unused */
#define DROP_INVALID_SIP	-132
#define DROP_POLICY		-133
#define DROP_INVALID		-134
#define DROP_CT_INVALID_HDR	-135
#define DROP_FRAG_NEEDED	-136
#define DROP_CT_UNKNOWN_PROTO	-137
#define DROP_UNUSED4		-138 /* unused */
#define DROP_UNKNOWN_L3		-139
#define DROP_MISSED_TAIL_CALL	-140
#define DROP_WRITE_ERROR	-141
#define DROP_UNKNOWN_L4		-142
#define DROP_UNKNOWN_ICMP4_CODE	-143
#define DROP_UNKNOWN_ICMP4_TYPE	-144
#define DROP_UNKNOWN_ICMP6_CODE	-145
#define DROP_UNKNOWN_ICMP6_TYPE	-146
#define DROP_NO_TUNNEL_KEY	-147
#define DROP_UNUSED5		-148 /* unused */
#define DROP_UNUSED6		-149 /* unused */
#define DROP_UNKNOWN_TARGET	-150
#define DROP_UNROUTABLE		-151
#define DROP_UNUSED7		-152 /* unused */
#define DROP_CSUM_L3		-153
#define DROP_CSUM_L4		-154
#define DROP_CT_CREATE_FAILED	-155
#define DROP_INVALID_EXTHDR	-156
#define DROP_FRAG_NOSUPPORT	-157
#define DROP_NO_SERVICE		-158
#define DROP_UNSUPP_SERVICE_PROTO	-159
#define DROP_NO_TUNNEL_ENDPOINT -160
#define DROP_NAT_46X64_DISABLED	-161
#define DROP_EDT_HORIZON	-162
#define DROP_UNKNOWN_CT		-163
#define DROP_HOST_UNREACHABLE	-164
#define DROP_NO_CONFIG		-165
#define DROP_UNSUPPORTED_L2	-166
#define DROP_NAT_NO_MAPPING	-167
#define DROP_NAT_UNSUPP_PROTO	-168
#define DROP_NO_FIB		-169
#define DROP_ENCAP_PROHIBITED	-170
#define DROP_INVALID_IDENTITY	-171
#define DROP_UNKNOWN_SENDER	-172
#define DROP_NAT_NOT_NEEDED	-173 /* Mapped as drop code, though drop not necessary. */
#define DROP_IS_CLUSTER_IP	-174
#define DROP_FRAG_NOT_FOUND	-175
#define DROP_FORBIDDEN_ICMP6	-176
#define DROP_NOT_IN_SRC_RANGE	-177
#define DROP_PROXY_LOOKUP_FAILED	-178
#define DROP_PROXY_SET_FAILED	-179
#define DROP_PROXY_UNKNOWN_PROTO	-180
#define DROP_POLICY_DENY	-181
#define DROP_VLAN_FILTERED	-182
#define DROP_INVALID_VNI	-183
#define DROP_INVALID_TC_BUFFER  -184
#define DROP_NO_SID		-185
#define DROP_MISSING_SRV6_STATE	-186 /* unused */
#define DROP_NAT46		-187
#define DROP_NAT64		-188
#define DROP_POLICY_AUTH_REQUIRED	-189
#define DROP_CT_NO_MAP_FOUND	-190
#define DROP_SNAT_NO_MAP_FOUND	-191
#define DROP_INVALID_CLUSTER_ID	-192
#define DROP_DSR_ENCAP_UNSUPP_PROTO	-193
#define DROP_NO_EGRESS_GATEWAY	-194
#define DROP_UNENCRYPTED_TRAFFIC	-195
#define DROP_TTL_EXCEEDED	-196
#define DROP_NO_NODE_ID		-197
#define DROP_RATE_LIMITED	-198
#define DROP_IGMP_HANDLED	-199
#define DROP_IGMP_SUBSCRIBED    -200
#define DROP_MULTICAST_HANDLED  -201
#define DROP_HOST_NOT_READY	-202
#define DROP_EP_NOT_READY	-203
#define DROP_NO_EGRESS_IP	-204
#define DROP_PUNT_PROXY		-205 /* Mapped as drop code, though drop not necessary. */

#define NAT_PUNT_TO_STACK	DROP_NAT_NOT_NEEDED
#define LB_PUNT_TO_STACK	DROP_PUNT_PROXY

#define NAT_NEEDED		CTX_ACT_OK
#define NAT_46X64_RECIRC	100

/* Cilium metrics reasons for forwarding packets and other stats.
 * If reason is larger than below then this is a drop reason and
 * value corresponds to -(DROP_*), see above.
 *
 * These are shared with pkg/monitor/api/drop.go.
 * When modifying any of the below, those files should also be updated.
 */
#define REASON_FORWARDED		0
#define REASON_PLAINTEXT		3
#define REASON_DECRYPT			4
#define REASON_LB_NO_BACKEND_SLOT	5
#define REASON_LB_NO_BACKEND		6
#define REASON_LB_REVNAT_UPDATE		7
#define REASON_LB_REVNAT_STALE		8
#define REASON_FRAG_PACKET		9
#define REASON_FRAG_PACKET_UPDATE	10
#define REASON_MISSED_CUSTOM_CALL	11
#define REASON_DECRYPTING			12
#define REASON_ENCRYPTING			13

/* Lookup scope for externalTrafficPolicy=Local */
#define LB_LOOKUP_SCOPE_EXT	0
#define LB_LOOKUP_SCOPE_INT	1

/* Cilium metrics direction for dropping/forwarding packet */
enum metric_dir {
	METRIC_INGRESS = 1,
	METRIC_EGRESS,
	METRIC_SERVICE
} __packed;

/* Magic ctx->mark identifies packets origination and encryption status.
 *
 * The upper 16 bits plus lower 8 bits (e.g. mask 0XFFFF00FF) contain the
 * packets security identity. The lower/upper halves are swapped to recover
 * the identity.
 *
 * In case of MARK_MAGIC_PROXY_EGRESS_EPID the upper 16 bits carry the Endpoint
 * ID instead of the security identity and the lower 8 bits will be zeroes.
 *
 * The 4 bits at 0X0F00 provide
 *  - the magic marker values which indicate whether the packet is coming from
 *    an ingress or egress proxy, a local process and its current encryption
 *    status.
 *
 * The 4 bits at 0xF000 provide
 *  - the key index to use for encryption when multiple keys are in-flight.
 *    In the IPsec case this becomes the SPI on the wire.
 */
#define MARK_MAGIC_HOST_MASK		0x0F00
#define MARK_MAGIC_PROXY_TO_WORLD	0x0800
#define MARK_MAGIC_PROXY_EGRESS_EPID	0x0900 /* mark carries source endpoint ID */
#define MARK_MAGIC_PROXY_INGRESS	0x0A00
#define MARK_MAGIC_PROXY_EGRESS		0x0B00
#define MARK_MAGIC_HOST			0x0C00
#define MARK_MAGIC_DECRYPT		0x0D00
#define MARK_MAGIC_ENCRYPT		0x0E00
#define MARK_MAGIC_IDENTITY		0x0F00 /* mark carries identity */
#define MARK_MAGIC_TO_PROXY		0x0200
#define MARK_MAGIC_SNAT_DONE		0x0300
#define MARK_MAGIC_OVERLAY		0x0400 /* mark carries identity */
/* used to indicate encrypted traffic was tunnel encapsulated
 * this is useful in the IPsec code paths where we need to know if overlay
 * traffic is encrypted or not.
 *
 * the SPI bit can be reused since this magic mark is only used POST encryption.
 */
#define MARK_MAGIC_OVERLAY_ENCRYPTED	(MARK_MAGIC_OVERLAY | 0x1000)
#define MARK_MAGIC_EGW_DONE		0x0500 /* mark carries identity */

#define MARK_MAGIC_KEY_MASK		0xFF00


/* The mark is used to indicate that the WireGuard tunnel device is done
 * encrypting a packet. The MSB invades the Kubernetes mark "space" which is
 * fine, as it's not used by K8s. See pkg/datapath/linux/linux_defaults/mark.go
 * for more details.
 */
#define MARK_MAGIC_WG_ENCRYPTED		0x1E00

/* MARK_MAGIC_HEALTH_IPIP_DONE can overlap with MARK_MAGIC_SNAT_DONE with both
 * being mutual exclusive given former is only under DSR. Used to push health
 * probe packets to ipip tunnel device & to avoid looping back.
 */
#define MARK_MAGIC_HEALTH_IPIP_DONE	MARK_MAGIC_SNAT_DONE

/* MARK_MAGIC_HEALTH can overlap with MARK_MAGIC_DECRYPT with both being
 * mutual exclusive. Note, MARK_MAGIC_HEALTH is user-facing UAPI for LB!
 */
#define MARK_MAGIC_HEALTH		MARK_MAGIC_DECRYPT

/* MARK_MAGIC_CLUSTER_ID shouldn't interfere with MARK_MAGIC_TO_PROXY. Lower
 * 8bits carries cluster_id, and when extended via the 'max-connected-clusters'
 * option, the upper 16bits may also be used for cluster_id, starting at the
 * most significant bit.
 */
#define MARK_MAGIC_CLUSTER_ID		MARK_MAGIC_TO_PROXY

/* IPv4 option used to carry service addr and port for DSR.
 *
 * Copy = 1 (option is copied to each fragment)
 * Class = 0 (control option)
 * Number = 26 (not used according to [1])
 * Len = 8 (option type (1) + option len (1) + addr (4) + port (2))
 *
 * [1]: https://www.iana.org/assignments/ip-parameters/ip-parameters.xhtml
 */
#define DSR_IPV4_OPT_TYPE	(IPOPT_COPY | 0x1a)

/* IPv6 option type of Destination Option used to carry service IPv6 addr and
 * port for DSR.
 *
 * 0b00		- "skip over this option and continue processing the header"
 *     0	- "Option Data does not change en-route"
 *      11011   - Unassigned [1]
 *
 * [1]:  https://www.iana.org/assignments/ipv6-parameters/ipv6-parameters.xhtml#ipv6-parameters-2
 */
#define DSR_IPV6_OPT_TYPE	0x1B
#define DSR_IPV6_OPT_LEN	(sizeof(struct dsr_opt_v6) - 4)
#define DSR_IPV6_EXT_LEN	((sizeof(struct dsr_opt_v6) - 8) / 8)

/* encrypt_config is the current encryption context on the node */
struct encrypt_config {
	__u8 encrypt_key;
} __packed;

/*
 * ctx->tc_index uses
 *
 * cilium_host @egress
 *   bpf_host -> bpf_lxc
 */
#define TC_INDEX_F_FROM_INGRESS_PROXY	1
#define TC_INDEX_F_FROM_EGRESS_PROXY	2
#define TC_INDEX_F_SKIP_NODEPORT	4
#define TC_INDEX_F_UNUSED		8
#define TC_INDEX_F_SKIP_HOST_FIREWALL	16

#define CB_NAT_FLAGS_REVDNAT_ONLY	(1 << 0)

/*
 * For use in ctx_{load,store}_meta(), which operates on sk_buff->cb or
 * the cilium_xdp_scratch pad.
 * The verifier only exposes the first 5 slots in cb[], so this enum
 * only contains 5 entries. Aliases are added to the slots to re-use
 * them under different names in different parts of the datapath.
 * Take care to not clobber slots used by other functions in the same
 * code path.
 */
/* ctx_{load,store}_meta() usage: */
enum {
	CB_SRC_LABEL,
#define	CB_PORT			CB_SRC_LABEL	/* Alias, non-overlapping */
#define	CB_HINT			CB_SRC_LABEL	/* Alias, non-overlapping */
#define	CB_PROXY_MAGIC		CB_SRC_LABEL	/* Alias, non-overlapping */
#define	CB_ENCRYPT_MAGIC	CB_SRC_LABEL	/* Alias, non-overlapping */
#define	CB_DST_ENDPOINT_ID	CB_SRC_LABEL    /* Alias, non-overlapping */
#define CB_SRV6_SID_1		CB_SRC_LABEL	/* Alias, non-overlapping */
	CB_1,
#define	CB_DELIVERY_REDIRECT	CB_1		/* Alias, non-overlapping */
#define	CB_NAT_46X64		CB_1		/* Alias, non-overlapping */
#define	CB_ADDR_V4		CB_1		/* Alias, non-overlapping */
#define	CB_ADDR_V6_1		CB_1		/* Alias, non-overlapping */
#define	CB_IPCACHE_SRC_LABEL	CB_1		/* Alias, non-overlapping */
#define	CB_SRV6_SID_2		CB_1		/* Alias, non-overlapping */
#define	CB_CLUSTER_ID_EGRESS	CB_1		/* Alias, non-overlapping */
#define	CB_TRACED		CB_1		/* Alias, non-overlapping */
	CB_2,
#define	CB_ADDR_V6_2		CB_2		/* Alias, non-overlapping */
#define CB_SRV6_SID_3		CB_2		/* Alias, non-overlapping */
#define	CB_CLUSTER_ID_INGRESS	CB_2		/* Alias, non-overlapping */
#define CB_NAT_FLAGS		CB_2		/* Alias, non-overlapping */
	CB_3,
#define	CB_ADDR_V6_3		CB_3		/* Alias, non-overlapping */
#define	CB_FROM_HOST		CB_3		/* Alias, non-overlapping */
#define CB_SRV6_SID_4		CB_3		/* Alias, non-overlapping */
#define CB_DSR_L3_OFF		CB_3		/* Alias, non-overlapping */
	CB_CT_STATE,
#define	CB_ADDR_V6_4		CB_CT_STATE	/* Alias, non-overlapping */
#define	CB_ENCRYPT_IDENTITY	CB_CT_STATE	/* Alias, non-overlapping,
						 * Not used by xfrm.
						 */
#define	CB_CUSTOM_CALLS		CB_CT_STATE	/* Alias, non-overlapping */
#define	CB_SRV6_VRF_ID		CB_CT_STATE	/* Alias, non-overlapping */
#define	CB_FROM_TUNNEL		CB_CT_STATE	/* Alias, non-overlapping */
};

/* Magic values for CB_FROM_HOST.
 * CB_FROM_HOST overlaps with CB_NAT46_STATE, so this value must be distinct
 * from any in enum NAT46 below!
 */
#define FROM_HOST_L7_LB 0xFACADE42

#define TUPLE_F_OUT		0	/* Outgoing flow */
#define TUPLE_F_IN		1	/* Incoming flow */
#define TUPLE_F_RELATED		2	/* Flow represents related packets */
#define TUPLE_F_SERVICE		4	/* Flow represents packets to service */

enum ct_dir {
	CT_EGRESS,
	CT_INGRESS,
	CT_SERVICE,
} __packed;

#ifdef ENABLE_NODEPORT
#define NAT_MIN_EGRESS		NODEPORT_PORT_MIN_NAT
#else
#define NAT_MIN_EGRESS		EPHEMERAL_MIN
#endif

enum ct_status {
	CT_NEW,
	CT_ESTABLISHED,
	CT_REPLY,
	CT_RELATED,
} __packed;

/* Service flags (lb{4,6}_service->flags) */
enum {
	SVC_FLAG_EXTERNAL_IP     = (1 << 0),	/* External IPs */
	SVC_FLAG_NODEPORT        = (1 << 1),	/* NodePort service */
	SVC_FLAG_EXT_LOCAL_SCOPE = (1 << 2),	/* externalTrafficPolicy=Local */
	SVC_FLAG_HOSTPORT        = (1 << 3),	/* hostPort forwarding */
	SVC_FLAG_AFFINITY        = (1 << 4),	/* sessionAffinity=clientIP */
	SVC_FLAG_LOADBALANCER    = (1 << 5),	/* LoadBalancer service */
	SVC_FLAG_ROUTABLE        = (1 << 6),	/* Not a surrogate/ClusterIP entry */
	SVC_FLAG_SOURCE_RANGE    = (1 << 7),	/* Check LoadBalancer source range */
};

/* Service flags (lb{4,6}_service->flags2) */
enum {
	SVC_FLAG_LOCALREDIRECT     = (1 << 0),	/* Local redirect service */
	SVC_FLAG_NAT_46X64         = (1 << 1),	/* NAT-46/64 entry */
	SVC_FLAG_L7_LOADBALANCER   = (1 << 2),	/* TPROXY redirect to local L7 load-balancer */
	SVC_FLAG_LOOPBACK          = (1 << 3),	/* HostPort with a loopback hostIP */
	SVC_FLAG_L7_DELEGATE       = (1 << 3),	/* If set then delegate unmodified to local L7 proxy */
	SVC_FLAG_INT_LOCAL_SCOPE   = (1 << 4),	/* internalTrafficPolicy=Local */
	SVC_FLAG_TWO_SCOPES        = (1 << 5),	/* Two sets of backends are used for external/internal connections */
	SVC_FLAG_QUARANTINED       = (1 << 6),	/* Backend slot (key: backend_slot > 0) is quarantined */
	SVC_FLAG_SOURCE_RANGE_DENY = (1 << 6),	/* Master slot: LoadBalancer source range check is inverted */
	SVC_FLAG_FWD_MODE_DSR      = (1 << 7),	/* If bit is set, use DSR instead of SNAT in annotation mode */
};

/* Backend flags (lb{4,6}_backends->flags) */
enum {
	BE_STATE_ACTIVE		= 0,
	BE_STATE_TERMINATING,
	BE_STATE_QUARANTINED,
	BE_STATE_MAINTENANCE,
};

struct ipv6_ct_tuple {
	/* Address fields are reversed, i.e.,
	 * these field names are correct for reply direction traffic.
	 */
	union v6addr	daddr;
	union v6addr	saddr;
	/* The order of dport+sport must not be changed!
	 * These field names are correct for original direction traffic.
	 */
	__be16		dport;
	__be16		sport;
	__u8		nexthdr;
	__u8		flags;
} __packed;

struct ipv4_ct_tuple {
	/* Address fields are reversed, i.e.,
	 * these field names are correct for reply direction traffic.
	 */
	__be32		daddr;
	__be32		saddr;
	/* The order of dport+sport must not be changed!
	 * These field names are correct for original direction traffic.
	 */
	__be16		dport;
	__be16		sport;
	__u8		nexthdr;
	__u8		flags;
} __packed;

struct ct_entry {
	__u64 reserved0;	/* unused since v1.16 */
	__u64 backend_id;
	__u64 packets;
	__u64 bytes;
	__u32 lifetime;
	__u16 rx_closing:1,
	      tx_closing:1,
	      reserved1:1,	/* unused since v1.12 */
	      lb_loopback:1,
	      seen_non_syn:1,
	      node_port:1,
	      proxy_redirect:1,	/* Connection is redirected to a proxy */
	      dsr_internal:1,	/* DSR is k8s service related, cluster internal */
	      from_l7lb:1,	/* Connection is originated from an L7 LB proxy */
	      reserved2:1,	/* unused since v1.14 */
	      from_tunnel:1,	/* Connection is over tunnel */
	      reserved3:5;
	__u16 rev_nat_index;
	/* In the kernel ifindex is u32, so we need to check in cilium-agent
	 * that ifindex of a NodePort device is <= MAX(u16).
	 * Unused when HAVE_FIB_INDEX is available.
	 */
	__u16 ifindex;

	/* *x_flags_seen represents the OR of all TCP flags seen for the
	 * transmit/receive direction of this entry.
	 */
	__u8  tx_flags_seen;
	__u8  rx_flags_seen;

	__u32 src_sec_id; /* Used from userspace proxies, do not change offset! */

	/* last_*x_report is a timestamp of the last time a monitor
	 * notification was sent for the transmit/receive direction.
	 */
	__u32 last_tx_report;
	__u32 last_rx_report;
};

struct lb6_key {
	union v6addr address;	/* Service virtual IPv6 address */
	__be16 dport;		/* L4 port filter, if unset, all ports apply */
	__u16 backend_slot;	/* Backend iterator, 0 indicates the svc frontend */
	__u8 proto;		/* L4 protocol, 0 indicates any protocol */
	__u8 scope;		/* LB_LOOKUP_SCOPE_* for externalTrafficPolicy=Local */
	__u8 pad[2];
};

/* See lb4_service comments for all fields. */
struct lb6_service {
	union {
		__u32 backend_id;
		/* See lb4_service for storage internals. */
		__u32 affinity_timeout;
		__u32 l7_lb_proxy_port;
	};
	__u16 count;
	__u16 rev_nat_index;
	__u8 flags;
	__u8 flags2;
	__u16 qcount;
};

/* See lb4_backend comments */
struct lb6_backend {
	union v6addr address;
	__be16 port;
	__u8 proto;
	__u8 flags;
	__u16 cluster_id;	/* With this field, we can distinguish two
				 * backends that have the same IP address,
				 * but belong to the different cluster.
				 */
	__u8 zone;
	__u8 pad;
};

struct lb6_health {
	struct lb6_backend peer;
};

struct lb6_reverse_nat {
	union v6addr address;
	__be16 port;
} __packed;

struct ipv6_revnat_tuple {
	__sock_cookie cookie;
	union v6addr address;
	__be16 port;
	__u16 pad;
};

struct ipv6_revnat_entry {
	union v6addr address;
	__be16 port;
	__u16 rev_nat_index;
};

struct lb4_key {
	__be32 address;		/* Service virtual IPv4 address */
	__be16 dport;		/* L4 port filter, if unset, all ports apply */
	__u16 backend_slot;	/* Backend iterator, 0 indicates the svc frontend */
	__u8 proto;		/* L4 protocol, 0 indicates any protocol */
	__u8 scope;		/* LB_LOOKUP_SCOPE_* for externalTrafficPolicy=Local */
	__u8 pad[2];
};

#define LB_ALGORITHM_SHIFT	24
#define AFFINITY_TIMEOUT_MASK	((1 << LB_ALGORITHM_SHIFT) - 1)

struct lb4_service {
	union {
		/* Non-master entry: backend ID in lb4_backends */
		__u32 backend_id;
		/* For master entry:
		 * - Upper  8 bits: load balancer algorithm,
		 *                  values:
		 *                     1 - random
		 *                     2 - maglev
		 * - Lower 24 bits: timeout in seconds
		 * Note: We don't use bitfield here given storage is
		 * compiler implementation dependent and the map needs
		 * to be populated from Go.
		 */
		__u32 affinity_timeout;
		/* For master entry: proxy port in host byte order,
		 * only when flags2 & SVC_FLAG_L7_LOADBALANCER is set.
		 */
		__u32 l7_lb_proxy_port;
	};
	/* For the service frontend, count denotes number of service backend
	 * slots (otherwise zero).
	 */
	__u16 count;
	__u16 rev_nat_index;	/* Reverse NAT ID in lb4_reverse_nat */
	__u8 flags;
	__u8 flags2;
	/* For the service frontend, qcount denotes number of service backend
	 * slots under quarantine (otherwise zero).
	 */
	__u16 qcount;
};

struct lb4_backend {
	__be32 address;		/* Service endpoint IPv4 address */
	__be16 port;		/* L4 port filter */
	__u8 proto;		/* L4 protocol, currently not used (set to 0) */
	__u8 flags;
	__u16 cluster_id;	/* With this field, we can distinguish two
				 * backends that have the same IP address,
				 * but belong to the different cluster.
				 */
	__u8 zone;
	__u8 pad;
};

struct lb4_health {
	struct lb4_backend peer;
};

struct lb4_reverse_nat {
	__be32 address;
	__be16 port;
} __packed;

struct ipv4_revnat_tuple {
	__sock_cookie cookie;
	__be32 address;
	__be16 port;
	__u16 pad;
};

struct ipv4_revnat_entry {
	__be32 address;
	__be16 port;
	__u16 rev_nat_index;
};

union lb4_affinity_client_id {
	__u32 client_ip;
	__net_cookie client_cookie;
} __packed;

struct lb4_affinity_key {
	union lb4_affinity_client_id client_id;
	__u16 rev_nat_id;
	__u8 netns_cookie:1,
	     reserved:7;
	__u8 pad1;
	__u32 pad2;
} __packed;

union lb6_affinity_client_id {
	union v6addr client_ip;
	__net_cookie client_cookie;
} __packed;

struct lb6_affinity_key {
	union lb6_affinity_client_id client_id;
	__u16 rev_nat_id;
	__u8 netns_cookie:1,
	     reserved:7;
	__u8 pad1;
	__u32 pad2;
} __packed;

struct lb_affinity_val {
	__u64 last_used;
	__u32 backend_id;
	__u32 pad;
} __packed;

struct lb_affinity_match {
	__u32 backend_id;
	__u16 rev_nat_id;
	__u16 pad;
} __packed;

struct ct_state {
	__u16 rev_nat_index;
#ifndef DISABLE_LOOPBACK_LB
	__u16 loopback:1,
#else
	__u16 loopback_disabled:1,
#endif
	      node_port:1,
	      dsr_internal:1,   /* DSR is k8s service related, cluster internal */
	      syn:1,
	      proxy_redirect:1,	/* Connection is redirected to a proxy */
	      from_l7lb:1,	/* Connection is originated from an L7 LB proxy */
	      reserved1:1,	/* Was auth_required, not used in production anywhere */
	      from_tunnel:1,	/* Connection is from tunnel */
		  closing:1,
	      reserved:7;
	__u32 src_sec_id;
#ifndef HAVE_FIB_IFINDEX
	__u16 ifindex;
#endif
	__u32 backend_id;	/* Backend ID in lb4_backends */
};

static __always_inline bool ct_state_is_from_l7lb(const struct ct_state *ct_state __maybe_unused)
{
#ifdef ENABLE_L7_LB
	return ct_state->from_l7lb;
#else
	return false;
#endif
}

#define SRC_RANGE_STATIC_PREFIX(STRUCT)		\
	(8 * (sizeof(STRUCT) - sizeof(struct bpf_lpm_trie_key)))

struct lb4_src_range_key {
	struct bpf_lpm_trie_key lpm_key;
	__u16 rev_nat_id;
	__u16 pad;
	__u32 addr;
};

struct lb6_src_range_key {
	struct bpf_lpm_trie_key lpm_key;
	__u16 rev_nat_id;
	__u16 pad;
	union v6addr addr;
};

static __always_inline __u64 ctx_adjust_hroom_flags(void)
{
#ifdef HAVE_CSUM_LEVEL
	return BPF_F_ADJ_ROOM_NO_CSUM_RESET;
#else
	return 0;
#endif
}

struct lpm_v4_key {
	struct bpf_lpm_trie_key lpm;
	__u8 addr[4];
};

struct lpm_v6_key {
	struct bpf_lpm_trie_key lpm;
	__u8 addr[16];
};

struct lpm_val {
	/* Just dummy for now. */
	__u8 flags;
};

struct skip_lb4_key {
	__u64 netns_cookie;     /* Source pod netns cookie */
	__u32 address;          /* Destination service virtual IPv4 address */
	__u16 port;             /* Destination service virtual layer4 port */
	__u16 pad;
};

struct skip_lb6_key {
	__u64 netns_cookie;     /* Source pod netns cookie */
	union v6addr address;   /* Destination service virtual IPv6 address */
	__u32 pad;
	__u16 port;             /* Destination service virtual layer4 port */
	__u16 pad2;
};

/* Older kernels don't support the larger tunnel key structure and we don't
 * need it since we only want to retrieve the tunnel ID anyway.
 */
#define TUNNEL_KEY_WITHOUT_SRC_IP offsetof(struct bpf_tunnel_key, local_ipv4)

#include "overloadable.h"
