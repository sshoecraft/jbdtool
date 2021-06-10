
#if !defined(__WIN32) && !defined(__WIN64)
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.

*/

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <linux/can.h>
#include <net/if.h>
#include <linux/if_link.h>
#include <linux/rtnetlink.h>
#include <linux/netlink.h>
#include "mybmm.h"

#define DEFAULT_BITRATE 250000
#define CAN_INTERFACE_LEN 16

struct can_session {
	int fd;
	char interface[CAN_INTERFACE_LEN+1];
	int bitrate;
	struct can_filter *f;
};
typedef struct can_session can_session_t;

#define _getshort(p) ((short) ((*((p)) << 8) | *((p)+1) ))

static int can_init(mybmm_config_t *conf) {
	return 0;
}

static void *can_new(mybmm_config_t *conf,...) {
	can_session_t *s;
	va_list ap;
	char *target, *p;

	va_start(ap,conf);
	target = va_arg(ap,char *);
	va_end(ap);
	dprintf(3,"target: %s\n", target);

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("can_new: malloc");
		return 0;
	}
	s->fd = -1;
	s->interface[0] = 0;
	strncat(s->interface,strele(0,",",target),sizeof(s->interface)-1);
	p = strele(1,",",target);
	if (strlen(p)) s->bitrate = atoi(p);
	else s->bitrate = DEFAULT_BITRATE;
	dprintf(3,"interface: %s, bitrate: %d\n",s->interface,s->bitrate);

        return s;
}

/* The below get/set speed code was pulled from libsocketcan
https://git.pengutronix.de/cgit/tools/libsocketcan

*/

/* libsocketcan.c
 *
 * (C) 2009 Luotao Fu <l.fu@pengutronix.de>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#define IF_UP 1
#define IF_DOWN 2

#define GET_STATE 1
#define GET_RESTART_MS 2
#define GET_BITTIMING 3
#define GET_CTRLMODE 4
#define GET_CLOCK 5
#define GET_BITTIMING_CONST 6
#define GET_BERR_COUNTER 7
#define GET_XSTATS 8
#define GET_LINK_STATS 9

/*
 * CAN bit-timing parameters
 *
 * For further information, please read chapter "8 BIT TIMING
 * REQUIREMENTS" of the "Bosch CAN Specification version 2.0"
 * at http://www.semiconductors.bosch.de/pdf/can2spec.pdf.
 */
struct can_bittiming {
	__u32 bitrate;		/* Bit-rate in bits/second */
	__u32 sample_point;	/* Sample point in one-tenth of a percent */
	__u32 tq;		/* Time quanta (TQ) in nanoseconds */
	__u32 prop_seg;		/* Propagation segment in TQs */
	__u32 phase_seg1;	/* Phase buffer segment 1 in TQs */
	__u32 phase_seg2;	/* Phase buffer segment 2 in TQs */
	__u32 sjw;		/* Synchronisation jump width in TQs */
	__u32 brp;		/* Bit-rate prescaler */
};

/*
 * CAN harware-dependent bit-timing constant
 *
 * Used for calculating and checking bit-timing parameters
 */
struct can_bittiming_const {
	char name[16];		/* Name of the CAN controller hardware */
	__u32 tseg1_min;	/* Time segement 1 = prop_seg + phase_seg1 */
	__u32 tseg1_max;
	__u32 tseg2_min;	/* Time segement 2 = phase_seg2 */
	__u32 tseg2_max;
	__u32 sjw_max;		/* Synchronisation jump width */
	__u32 brp_min;		/* Bit-rate prescaler */
	__u32 brp_max;
	__u32 brp_inc;
};

/*
 * CAN clock parameters
 */
struct can_clock {
	__u32 freq;		/* CAN system clock frequency in Hz */
};

/*
 * CAN operational and error states
 */
enum can_state {
	CAN_STATE_ERROR_ACTIVE = 0,	/* RX/TX error count < 96 */
	CAN_STATE_ERROR_WARNING,	/* RX/TX error count < 128 */
	CAN_STATE_ERROR_PASSIVE,	/* RX/TX error count < 256 */
	CAN_STATE_BUS_OFF,		/* RX/TX error count >= 256 */
	CAN_STATE_STOPPED,		/* Device is stopped */
	CAN_STATE_SLEEPING,		/* Device is sleeping */
	CAN_STATE_MAX
};

/*
 * CAN bus error counters
 */
struct can_berr_counter {
	__u16 txerr;
	__u16 rxerr;
};

/*
 * CAN controller mode
 */
struct can_ctrlmode {
	__u32 mask;
	__u32 flags;
};

#define CAN_CTRLMODE_LOOPBACK		0x01	/* Loopback mode */
#define CAN_CTRLMODE_LISTENONLY		0x02	/* Listen-only mode */
#define CAN_CTRLMODE_3_SAMPLES		0x04	/* Triple sampling mode */
#define CAN_CTRLMODE_ONE_SHOT		0x08	/* One-Shot mode */
#define CAN_CTRLMODE_BERR_REPORTING	0x10	/* Bus-error reporting */
#define CAN_CTRLMODE_FD			0x20	/* CAN FD mode */
#define CAN_CTRLMODE_PRESUME_ACK	0x40	/* Ignore missing CAN ACKs */

/*
 * CAN device statistics
 */
struct can_device_stats {
	__u32 bus_error;	/* Bus errors */
	__u32 error_warning;	/* Changes to error warning state */
	__u32 error_passive;	/* Changes to error passive state */
	__u32 bus_off;		/* Changes to bus off state */
	__u32 arbitration_lost; /* Arbitration lost errors */
	__u32 restarts;		/* CAN controller re-starts */
};

/*
 * CAN netlink interface
 */
enum {
	IFLA_CAN_UNSPEC,
	IFLA_CAN_BITTIMING,
	IFLA_CAN_BITTIMING_CONST,
	IFLA_CAN_CLOCK,
	IFLA_CAN_STATE,
	IFLA_CAN_CTRLMODE,
	IFLA_CAN_RESTART_MS,
	IFLA_CAN_RESTART,
	IFLA_CAN_BERR_COUNTER,
	IFLA_CAN_DATA_BITTIMING,
	IFLA_CAN_DATA_BITTIMING_CONST,
	__IFLA_CAN_MAX
};

#define IFLA_CAN_MAX	(__IFLA_CAN_MAX - 1)
struct get_req {
	struct nlmsghdr n;
	struct ifinfomsg i;
};

struct set_req {
	struct nlmsghdr n;
	struct ifinfomsg i;
	char buf[1024];
};

struct req_info {
	__u8 restart;
	__u8 disable_autorestart;
	__u32 restart_ms;
	struct can_ctrlmode *ctrlmode;
	struct can_bittiming *bittiming;
};

#define parse_rtattr_nested(tb, max, rta) \
        (parse_rtattr((tb), (max), RTA_DATA(rta), RTA_PAYLOAD(rta)))

#define NLMSG_TAIL(nmsg) \
        ((struct rtattr *) (((void *) (nmsg)) + NLMSG_ALIGN((nmsg)->nlmsg_len)))

#define IFLA_CAN_MAX    (__IFLA_CAN_MAX - 1)

static int send_dump_request(int fd, const char *name, int family, int type)
{
	struct get_req req;

	memset(&req, 0, sizeof(req));

	req.n.nlmsg_len = sizeof(req);
	req.n.nlmsg_type = type;
	req.n.nlmsg_flags = NLM_F_REQUEST;
	req.n.nlmsg_pid = 0;
	req.n.nlmsg_seq = 0;

	req.i.ifi_family = family;
	/*
	 * If name is null, set flag to dump link information from all
	 * interfaces otherwise, just dump specified interface's link
	 * information.
	 */
	if (name == NULL) {
		req.n.nlmsg_flags |= NLM_F_DUMP;
	} else {
		req.i.ifi_index = if_nametoindex(name);
		if (req.i.ifi_index == 0) {
			fprintf(stderr, "Cannot find device \"%s\"\n", name);
			return -1;
		}
	}

	return send(fd, (void *)&req, sizeof(req), 0);
}
static void parse_rtattr(struct rtattr **tb, int max, struct rtattr *rta, int len) {
	memset(tb, 0, sizeof(*tb) * (max + 1));
	while (RTA_OK(rta, len)) {
		if (rta->rta_type <= max) {
			tb[rta->rta_type] = rta;
		}

		rta = RTA_NEXT(rta, len);
	}
}

static int do_get_nl_link(int fd, __u8 acquire, const char *name, void *res) {
	struct sockaddr_nl peer;

	char cbuf[64];
	char nlbuf[1024 * 8];

	int ret = -1;
	int done = 0;

	struct iovec iov = {
		.iov_base = (void *)nlbuf,
		.iov_len = sizeof(nlbuf),
	};

	struct msghdr msg = {
		.msg_name = (void *)&peer,
		.msg_namelen = sizeof(peer),
		.msg_iov = &iov,
		.msg_iovlen = 1,
		.msg_control = &cbuf,
		.msg_controllen = sizeof(cbuf),
		.msg_flags = 0,
	};
	struct nlmsghdr *nl_msg;
	ssize_t msglen;

	struct rtattr *linkinfo[IFLA_INFO_MAX + 1];
	struct rtattr *can_attr[IFLA_CAN_MAX + 1];

	if (send_dump_request(fd, name, AF_PACKET, RTM_GETLINK) < 0) {
		perror("Cannot send dump request");
		return ret;
	}

	while (!done && (msglen = recvmsg(fd, &msg, 0)) > 0) {
		size_t u_msglen = (size_t) msglen;
		/* Check to see if the buffers in msg get truncated */
		if (msg.msg_namelen != sizeof(peer) ||
		    (msg.msg_flags & (MSG_TRUNC | MSG_CTRUNC))) {
			fprintf(stderr, "Uhoh... truncated message.\n");
			return -1;
		}

		for (nl_msg = (struct nlmsghdr *)nlbuf;
		     NLMSG_OK(nl_msg, u_msglen);
		     nl_msg = NLMSG_NEXT(nl_msg, u_msglen)) {
			int type = nl_msg->nlmsg_type;
			int len;

			if (type == NLMSG_DONE) {
				done++;
				continue;
			}
			if (type != RTM_NEWLINK)
				continue;

			struct ifinfomsg *ifi = NLMSG_DATA(nl_msg);
			struct rtattr *tb[IFLA_MAX + 1];

			len =
				nl_msg->nlmsg_len - NLMSG_LENGTH(sizeof(struct ifaddrmsg));
			parse_rtattr(tb, IFLA_MAX, IFLA_RTA(ifi), len);

			/* Finish process if the reply message is matched */
			if (strcmp((char *)RTA_DATA(tb[IFLA_IFNAME]), name) == 0)
				done++;
			else
				continue;

			if (acquire == GET_LINK_STATS) {
				if (!tb[IFLA_STATS64]) {
					fprintf(stderr, "no link statistics (64-bit) found\n");
				} else {
					memcpy(res, RTA_DATA(tb[IFLA_STATS64]), sizeof(struct rtnl_link_stats64));
					ret = 0;
				}
				continue;
			}

			if (tb[IFLA_LINKINFO])
				parse_rtattr_nested(linkinfo,
						    IFLA_INFO_MAX, tb[IFLA_LINKINFO]);
			else
				continue;

			if (acquire == GET_XSTATS) {
				if (!linkinfo[IFLA_INFO_XSTATS])
					fprintf(stderr, "no can statistics found\n");
				else {
					memcpy(res, RTA_DATA(linkinfo[IFLA_INFO_XSTATS]),
					       sizeof(struct can_device_stats));
					ret = 0;
				}
				continue;
			}

			if (!linkinfo[IFLA_INFO_DATA]) {
				fprintf(stderr, "no link data found\n");
				return ret;
			}

			parse_rtattr_nested(can_attr, IFLA_CAN_MAX,
					    linkinfo[IFLA_INFO_DATA]);

			switch (acquire) {
			case GET_STATE:
				if (can_attr[IFLA_CAN_STATE]) {
					*((int *)res) = *((__u32 *)
							  RTA_DATA(can_attr
								   [IFLA_CAN_STATE]));
					ret = 0;
				} else {
					fprintf(stderr, "no state data found\n");
				}

				break;
			case GET_RESTART_MS:
				if (can_attr[IFLA_CAN_RESTART_MS]) {
					*((__u32 *) res) = *((__u32 *)
							     RTA_DATA(can_attr
								      [IFLA_CAN_RESTART_MS]));
					ret = 0;
				} else
					fprintf(stderr, "no restart_ms data found\n");

				break;
			case GET_BITTIMING:
				if (can_attr[IFLA_CAN_BITTIMING]) {
					memcpy(res,
					       RTA_DATA(can_attr[IFLA_CAN_BITTIMING]),
					       sizeof(struct can_bittiming));
					ret = 0;
				} else
					fprintf(stderr, "no bittiming data found\n");

				break;
			case GET_CTRLMODE:
				if (can_attr[IFLA_CAN_CTRLMODE]) {
					memcpy(res,
					       RTA_DATA(can_attr[IFLA_CAN_CTRLMODE]),
					       sizeof(struct can_ctrlmode));
					ret = 0;
				} else
					fprintf(stderr, "no ctrlmode data found\n");

				break;
			case GET_CLOCK:
				if (can_attr[IFLA_CAN_CLOCK]) {
					memcpy(res,
					       RTA_DATA(can_attr[IFLA_CAN_CLOCK]),
					       sizeof(struct can_clock));
					ret = 0;
				} else
					fprintf(stderr,
						"no clock parameter data found\n");

				break;
			case GET_BITTIMING_CONST:
				if (can_attr[IFLA_CAN_BITTIMING_CONST]) {
					memcpy(res,
					       RTA_DATA(can_attr[IFLA_CAN_BITTIMING_CONST]),
					       sizeof(struct can_bittiming_const));
					ret = 0;
				} else
					fprintf(stderr, "no bittiming_const data found\n");

				break;
			case GET_BERR_COUNTER:
				if (can_attr[IFLA_CAN_BERR_COUNTER]) {
					memcpy(res,
					       RTA_DATA(can_attr[IFLA_CAN_BERR_COUNTER]),
					       sizeof(struct can_berr_counter));
					ret = 0;
				} else
					fprintf(stderr, "no berr_counter data found\n");

				break;

			default:
				fprintf(stderr, "unknown acquire mode\n");
			}
		}
	}

	return ret;
}

static int open_nl_sock()
{
	int fd;
	int sndbuf = 32768;
	int rcvbuf = 32768;
	unsigned int addr_len;
	struct sockaddr_nl local;

	fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd < 0) {
		perror("Cannot open netlink socket");
		return -1;
	}

	setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void *)&sndbuf, sizeof(sndbuf));

	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *)&rcvbuf, sizeof(rcvbuf));

	memset(&local, 0, sizeof(local));
	local.nl_family = AF_NETLINK;
	local.nl_groups = 0;

	if (bind(fd, (struct sockaddr *)&local, sizeof(local)) < 0) {
		perror("Cannot bind netlink socket");
		return -1;
	}

	addr_len = sizeof(local);
	if (getsockname(fd, (struct sockaddr *)&local, &addr_len) < 0) {
		perror("Cannot getsockname");
		return -1;
	}
	if (addr_len != sizeof(local)) {
		fprintf(stderr, "Wrong address length %u\n", addr_len);
		return -1;
	}
	if (local.nl_family != AF_NETLINK) {
		fprintf(stderr, "Wrong address family %d\n", local.nl_family);
		return -1;
	}
	return fd;
}

static int get_link(const char *name, __u8 acquire, void *res) {
	int err, fd;

	fd = open_nl_sock();
	if (fd < 0)
		return -1;

	err = do_get_nl_link(fd, acquire, name, res);
	close(fd);

	return err;

}

int can_get_bittiming(const char *name, struct can_bittiming *bt) {
	return get_link(name, GET_BITTIMING, bt);
}

static int addattr32(struct nlmsghdr *n, size_t maxlen, int type, __u32 data)
{
	int len = RTA_LENGTH(4);
	struct rtattr *rta;

	if (NLMSG_ALIGN(n->nlmsg_len) + len > maxlen) {
		fprintf(stderr,
			"addattr32: Error! max allowed bound %zu exceeded\n",
			maxlen);
		return -1;
	}

	rta = NLMSG_TAIL(n);
	rta->rta_type = type;
	rta->rta_len = len;
	memcpy(RTA_DATA(rta), &data, 4);
	n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + len;

	return 0;
}

static int addattr_l(struct nlmsghdr *n, size_t maxlen, int type,
		     const void *data, int alen)
{
	int len = RTA_LENGTH(alen);
	struct rtattr *rta;

	if (NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len) > maxlen) {
		fprintf(stderr,
			"addattr_l ERROR: message exceeded bound of %zu\n",
			maxlen);
		return -1;
	}

	rta = NLMSG_TAIL(n);
	rta->rta_type = type;
	rta->rta_len = len;
	memcpy(RTA_DATA(rta), data, alen);
	n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len);

	return 0;
}

static int send_mod_request(int fd, struct nlmsghdr *n)
{
	int status;
	struct sockaddr_nl nladdr;
	struct nlmsghdr *h;

	struct iovec iov = {
		.iov_base = (void *)n,
		.iov_len = n->nlmsg_len
	};
	struct msghdr msg = {
		.msg_name = &nladdr,
		.msg_namelen = sizeof(nladdr),
		.msg_iov = &iov,
		.msg_iovlen = 1,
	};
	char buf[16384];

	memset(&nladdr, 0, sizeof(nladdr));

	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid = 0;
	nladdr.nl_groups = 0;

	n->nlmsg_seq = 0;
	n->nlmsg_flags |= NLM_F_ACK;

	status = sendmsg(fd, &msg, 0);

	if (status < 0) {
		perror("Cannot talk to rtnetlink");
		return -1;
	}

	iov.iov_base = buf;
	while (1) {
		iov.iov_len = sizeof(buf);
		status = recvmsg(fd, &msg, 0);
		for (h = (struct nlmsghdr *)buf; (size_t) status >= sizeof(*h);) {
			int len = h->nlmsg_len;
			int l = len - sizeof(*h);
			if (l < 0 || len > status) {
				if (msg.msg_flags & MSG_TRUNC) {
					fprintf(stderr, "Truncated message\n");
					return -1;
				}
				fprintf(stderr,
					"!!!malformed message: len=%d\n", len);
				return -1;
			}

			if (h->nlmsg_type == NLMSG_ERROR) {
				struct nlmsgerr *err =
				    (struct nlmsgerr *)NLMSG_DATA(h);
				if ((size_t) l < sizeof(struct nlmsgerr)) {
					fprintf(stderr, "ERROR truncated\n");
				} else {
					errno = -err->error;
					if (errno == 0)
						return 0;

					perror("RTNETLINK answers");
				}
				return -1;
			}
			status -= NLMSG_ALIGN(len);
			h = (struct nlmsghdr *)((char *)h + NLMSG_ALIGN(len));
		}
	}

	return 0;
}

static int do_set_nl_link(int fd, __u8 if_state, const char *name, struct req_info *req_info)
{
	struct set_req req;

	const char *type = "can";

	memset(&req, 0, sizeof(req));

	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	req.n.nlmsg_type = RTM_NEWLINK;
	req.i.ifi_family = 0;

	req.i.ifi_index = if_nametoindex(name);
	if (req.i.ifi_index == 0) {
		fprintf(stderr, "Cannot find device \"%s\"\n", name);
		return -1;
	}

	if (if_state) {
		switch (if_state) {
		case IF_DOWN:
			req.i.ifi_change |= IFF_UP;
			req.i.ifi_flags &= ~IFF_UP;
			break;
		case IF_UP:
			req.i.ifi_change |= IFF_UP;
			req.i.ifi_flags |= IFF_UP;
			break;
		default:
			fprintf(stderr, "unknown state\n");
			return -1;
		}
	}

	if (req_info != NULL) {
		/* setup linkinfo section */
		struct rtattr *linkinfo = NLMSG_TAIL(&req.n);
		addattr_l(&req.n, sizeof(req), IFLA_LINKINFO, NULL, 0);
		addattr_l(&req.n, sizeof(req), IFLA_INFO_KIND, type, strlen(type));
		/* setup data section */
		struct rtattr *data = NLMSG_TAIL(&req.n);
		addattr_l(&req.n, sizeof(req), IFLA_INFO_DATA, NULL, 0);

		if (req_info->restart_ms > 0 || req_info->disable_autorestart)
			addattr32(&req.n, 1024, IFLA_CAN_RESTART_MS, req_info->restart_ms);

		if (req_info->restart) addattr32(&req.n, 1024, IFLA_CAN_RESTART, 1);

		if (req_info->bittiming != NULL) {
			addattr_l(&req.n, 1024, IFLA_CAN_BITTIMING, req_info->bittiming, sizeof(struct can_bittiming));
		}

		if (req_info->ctrlmode != NULL) {
			addattr_l(&req.n, 1024, IFLA_CAN_CTRLMODE, req_info->ctrlmode, sizeof(struct can_ctrlmode));
		}

		/* mark end of data section */
		data->rta_len = (void *)NLMSG_TAIL(&req.n) - (void *)data;

		/* mark end of link info section */
		linkinfo->rta_len = (void *)NLMSG_TAIL(&req.n) - (void *)linkinfo;
	}

	return send_mod_request(fd, &req.n);
}

static int set_link(const char *name, __u8 if_state, struct req_info *req_info)
{
	int err, fd;

	fd = open_nl_sock();
	if (fd < 0)
		return -1;

	err = do_set_nl_link(fd, if_state, name, req_info);
	close(fd);

	return err;
}

int can_set_bittiming(const char *name, struct can_bittiming *bt)
{
	struct req_info req_info = {
		.bittiming = bt,
	};

	return set_link(name, 0, &req_info);
}

int can_set_bitrate(const char *name, __u32 bitrate)
{
	struct can_bittiming bt;

	memset(&bt, 0, sizeof(bt));
	bt.bitrate = bitrate;

	return can_set_bittiming(name, &bt);
}

/* End of libsocketcan code */

static int up_down_up(int fd, char *interface) {
	struct ifreq ifr;

	memset(&ifr,0,sizeof(ifr));
	strcpy(ifr.ifr_name,interface);

	/* Bring up the IF */
	ifr.ifr_flags |= (IFF_UP | IFF_RUNNING | IFF_NOARP);
	if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
//		perror("up_down_up: SIOCSIFFLAGS IFF_UP");
		return 1;
	}

	/* Bring down the IF */
	ifr.ifr_flags = 0;
	if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
//		perror("up_down_up: SIOCSIFFLAGS IFF_DOWN");
		return 1;
	}

	/* Bring up the IF */
	ifr.ifr_flags |= (IFF_UP | IFF_RUNNING | IFF_NOARP);
	if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
//		perror("up_down_up: SIOCSIFFLAGS IFF_UP");
		return 1;
	}
	return 0;
}

static int can_open(void *handle) {
	can_session_t *s = handle;
	struct can_bittiming bt;
	struct sockaddr_can addr;
	struct ifreq ifr;
	int if_up;

	/* If already open, dont try again */
	if (s->fd >= 0) return 0;

	/* Open socket */
	dprintf(3,"opening socket...\n");
	s->fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (s->fd < 0) {
		perror("can_open: socket");
		return 1;
	}
	dprintf(3,"fd: %d\n", s->fd);

	/* Get the state */
	memset(&ifr,0,sizeof(ifr));
	strcpy(ifr.ifr_name,s->interface);
	if (ioctl(s->fd, SIOCGIFFLAGS, &ifr) < 0) {
		perror("can_open: SIOCGIFFLAGS");
		goto can_open_error;
	}
	if_up = ((ifr.ifr_flags & IFF_UP) != 0 ? 1 : 0);

	dprintf(3,"if_up: %d\n",if_up);
	if (!if_up) {
		/* Set the bitrate */
		if (can_set_bitrate(s->interface, s->bitrate) < 0) {
			printf("ERROR: unable to set bitrate on %s!\n", s->interface);
			goto can_open_error;
		}
		up_down_up(s->fd,s->interface);
	} else {
		/* Get the current timing */
		if (can_get_bittiming(s->interface,&bt) < 0) {
			perror("can_open: can_get_bittiming");
			goto can_open_error;
		}

		/* Check the bitrate */
		dprintf(3,"current bitrate: %d, wanted bitrate: %d\n", bt.bitrate, s->bitrate);
		if (bt.bitrate != s->bitrate) {
			/* Bring down the IF */
			ifr.ifr_flags = 0;
			dprintf(3,"can_open: SIOCSIFFLAGS clear\n");
			if (ioctl(s->fd, SIOCSIFFLAGS, &ifr) < 0) {
				perror("can_open: SIOCSIFFLAGS IFF_DOWN");
				goto can_open_error;
			}

			/* Set the bitrate */
			dprintf(3,"setting bitrate...\n");
			if (can_set_bitrate(s->interface, s->bitrate) < 0) {
				printf("ERROR: unable to set bitrate on %s!\n", s->interface);
				goto can_open_error;
			}
			up_down_up(s->fd,s->interface);

		}
	}

	/* Get IF index */
	strcpy(ifr.ifr_name, s->interface);
	if (ioctl(s->fd, SIOCGIFINDEX, &ifr) < 0) {
		perror("can_open: SIOCGIFINDEX");
		goto can_open_error;
	}

	/* Bind the socket */
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	if (bind(s->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("can_open: bind");
		goto can_open_error;
	}

	/* If caller added a filter, apply it */
	if (s->f) {
		if (setsockopt(s->fd, SOL_CAN_RAW, CAN_RAW_FILTER, s->f, sizeof(*s->f)) < 0) {
			perror("can_open: setsockopt");
			goto can_open_error;
		}
	}
	dprintf(3,"done!\n");
	return 0;
can_open_error:
	close(s->fd);
	s->fd = -1;
	return 1;
}

static int can_read(void *handle, ...) {
	can_session_t *s = handle;
	struct can_frame frame;
	uint8_t *buf;
	int id,buflen,bytes,len;
	va_list ap;

	/* If not open, error */
	if (s->fd < 0) return -1;

	va_start(ap,handle);
	id = va_arg(ap,int);
	buf = va_arg(ap,uint8_t *);
	buflen = va_arg(ap,int);
	va_end(ap);

	dprintf(8,"id: %03x, buf: %p, buflen: %d\n", id, buf, buflen);
	/* Keep reading until we get our ID */
	do {
		bytes = read(s->fd, &frame, sizeof(struct can_frame));
		dprintf(8,"frame: id: %x\n", frame.can_id);
		dprintf(8,"fd: %d, read bytes: %d\n", s->fd, bytes);
		if (bytes < 0) {
			if (errno != EAGAIN) bytes = -1;
			break;
		}
		if (bytes < sizeof(struct can_frame)) {
			fprintf(stderr, "read: incomplete CAN frame\n");
			continue;
		}
		dprintf(8,"bytes: %d, id: %x, frame.can_id: %x\n", bytes, id, frame.can_id);
	} while(id != 0xFFFF && frame.can_id != id);
	/* If the id is 0xFFFF, return the whole frame */
	if (id == 0x0FFFF) {
		dprintf(8,"returning frame...\n");
		len = buflen < sizeof(frame) ? buflen : sizeof(frame);
		memcpy(buf,&frame,len);
	} else {
		len = buflen > frame.can_dlc ? frame.can_dlc : buflen;
		dprintf(8,"len: %d, can_dlc: %d\n", len, frame.can_dlc);
		memcpy(buf,&frame.data,len);
	}
	if (bytes > 0 && debug >= 8) bindump("FROM BMS",buf,buflen);
	dprintf(6,"returning: %d\n", bytes);
	return len;
}

static int can_write(void *handle, ...) {
	can_session_t *s = handle;
	struct can_frame frame;
	uint8_t *buf;
	int bytes,id,buflen,len;
	va_list ap;

	/* If not open, error */
	if (s->fd < 0) return -1;

	va_start(ap,handle);
	id = va_arg(ap,int);
	buf = va_arg(ap,uint8_t *);
	buflen = va_arg(ap,int);
	va_end(ap);

	dprintf(5,"id: %03x, buf: %p, buflen: %d\n", id, buf, buflen);
	len = buflen > 8 ? 8 : buflen;
	dprintf(5,"len: %d\n", len);
	memset(&frame,0,sizeof(frame));
	frame.can_id = id;
	frame.can_dlc = len;
	memcpy(&frame.data,buf,len);
	if (debug >= 5) bindump("TO BMS",&frame,sizeof(struct can_frame));
	bytes = write(s->fd, &frame, sizeof(struct can_frame));
	dprintf(5,"fd: %d, returning: %d\n", s->fd, bytes);
	if (bytes < 0) perror("write");
	return (bytes < 0 ? -1 : len);
}

static int can_close(void *handle) {
	can_session_t *s = handle;

	if (s->fd >= 0) {
		close(s->fd);
		s->fd = -1;
	}
	return 0;
}

mybmm_module_t can_module = {
	MYBMM_MODTYPE_TRANSPORT,
	"can",
	0,
	can_init,
	can_new,
	can_open,
	can_read,
	can_write,
	can_close,
};
#endif
