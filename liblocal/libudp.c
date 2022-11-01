#include "libudp.h"

int create_udp_sock(int port)
{
    int udp_sock = socket(PF_INET, SOCK_DGRAM, 0);

	if (port == 0) {
		return udp_sock;
	}

    int sockopt_flag = 1;
    if (setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, &sockopt_flag, sizeof(int)) < 0) {
        ERRLOG(LLE, FL, "%s() fail to setsockopt!\n", __func__);
        exit(0);
    }

    struct sockaddr_in addr;
    memset(&addr, 0x00, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(udp_sock, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        ERRLOG(LLE, FL, "%s() fail to bindaddr (port=:%d)!\n", __func__, port);
        exit(0);
    }

    return udp_sock;
}

int udpfromto_init(int s)
{
	int proto, flag = 0, opt = 1;
	struct sockaddr_storage si;
	socklen_t si_len = sizeof(si);

	errno = ENOSYS;

	memset(&si, 0, sizeof(si));

	if (getsockname(s, (struct sockaddr *) &si, &si_len) < 0) {
		return -1;
	}

	if (si.ss_family == AF_INET) {
		proto = IPPROTO_IP;
		flag = IP_PKTINFO;
	} else if (si.ss_family == AF_INET6) {
		proto = IPPROTO_IPV6;
		flag = IPV6_RECVPKTINFO;
	} else {
		return -1;
	}

	return setsockopt(s, proto, flag, &opt, sizeof(opt));
}

int recvfromto(int s, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen, struct sockaddr *to, socklen_t *tolen)
{
	struct msghdr msgh;
	struct cmsghdr *cmsg;
	struct iovec iov;
	char cbuf[256];
	int err;
	struct sockaddr_storage si;
	socklen_t si_len = sizeof(si);

	/*
	 *	Catch the case where the caller passes invalid arguments.
	 */
	if (!to || !tolen) return recvfrom(s, buf, len, flags, from, fromlen);

	memset(&si, 0, sizeof(si));

	/*
	 *	recvmsg doesn't provide sin_port so we have to
	 *	retrieve it using getsockname().
	 */
	if (getsockname(s, (struct sockaddr *)&si, &si_len) < 0) {
		return -1;
	}

	/*
	 *	Initialize the 'to' address.  It may be INADDR_ANY here,
	 *	with a more specific address given by recvmsg(), below.
	 */
	if (si.ss_family == AF_INET) {
		struct sockaddr_in *dst = (struct sockaddr_in *) to;
		struct sockaddr_in *src = (struct sockaddr_in *) &si;

		if (*tolen < sizeof(*dst)) {
			errno = EINVAL;
			return -1;
		}
		*tolen = sizeof(*dst);
		*dst = *src;
	} else if (si.ss_family == AF_INET6) {
		struct sockaddr_in6 *dst = (struct sockaddr_in6 *) to;
		struct sockaddr_in6 *src = (struct sockaddr_in6 *) &si;

		if (*tolen < sizeof(*dst)) {
			errno = EINVAL;
			return -1;
		}
		*tolen = sizeof(*dst);
		*dst = *src;
	} else {
		errno = EINVAL;
		return -1;
	}

	/* Set up iov and msgh structures. */
	memset(&cbuf, 0, sizeof(cbuf));
	memset(&msgh, 0, sizeof(struct msghdr));
	iov.iov_base = buf;
	iov.iov_len  = len;
	msgh.msg_control = cbuf;
	msgh.msg_controllen = sizeof(cbuf);
	msgh.msg_name = from;
	msgh.msg_namelen = fromlen ? *fromlen : 0;
	msgh.msg_iov  = &iov;
	msgh.msg_iovlen = 1;
	msgh.msg_flags = 0;

	/* Receive one packet. */
	if ((err = recvmsg(s, &msgh, flags)) < 0) {
		return err;
	}

	if (fromlen) *fromlen = msgh.msg_namelen;

	/* Process auxiliary received data in msgh */
	for (cmsg = CMSG_FIRSTHDR(&msgh);
	     cmsg != NULL;
	     cmsg = CMSG_NXTHDR(&msgh,cmsg)) {

        if ((cmsg->cmsg_level == IPPROTO_IP) &&
            (cmsg->cmsg_type == IP_PKTINFO)) {
            struct in_pktinfo *i =
                (struct in_pktinfo *) CMSG_DATA(cmsg);
            ((struct sockaddr_in *)to)->sin_addr = i->ipi_addr;
            *tolen = sizeof(struct sockaddr_in);
            break;
        }

		if ((cmsg->cmsg_level == IPPROTO_IPV6) &&
		    (cmsg->cmsg_type == IPV6_PKTINFO)) {
			struct in6_pktinfo *i =
				(struct in6_pktinfo *) CMSG_DATA(cmsg);
			((struct sockaddr_in6 *)to)->sin6_addr = i->ipi6_addr;
			*tolen = sizeof(struct sockaddr_in6);
			break;
#if 0
			// schlee : maybe ...
			ifindex = i->ifindex;
#endif
		}
	}

	return err;
}

int sendfromto(int s, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t fromlen, struct sockaddr *to, socklen_t tolen)
{
	struct msghdr msgh;
	struct iovec iov;
	char cbuf[256];

	/*
	 *	Unknown address family, die.
	 */
	if (from && (from->sa_family != AF_INET) && (from->sa_family != AF_INET6)) {
		errno = EINVAL;
		return -1;
	}

	/*
	 *	If the sendmsg() flags aren't defined, fall back to
	 *	using sendto().  These flags are defined on FreeBSD,
	 *	but laying it out this way simplifies the look of the
	 *	code.
	 */

	/*
	 *	No "from", just use regular sendto.
	 */
	if (!from || (fromlen == 0)) {
		return sendto(s, buf, len, flags, to, tolen);
	}

	/* Set up control buffer iov and msgh structures. */
	memset(&cbuf, 0, sizeof(cbuf));
	memset(&msgh, 0, sizeof(msgh));
	memset(&iov, 0, sizeof(iov));
	iov.iov_base = buf;
	iov.iov_len = len;

	msgh.msg_iov = &iov;
	msgh.msg_iovlen = 1;
	msgh.msg_name = to;
	msgh.msg_namelen = tolen;

	if (from->sa_family == AF_INET) {
		struct sockaddr_in *s4 = (struct sockaddr_in *) from;

		struct cmsghdr *cmsg;
		struct in_pktinfo *pkt;

		msgh.msg_control = cbuf;
		msgh.msg_controllen = CMSG_SPACE(sizeof(*pkt));

		cmsg = CMSG_FIRSTHDR(&msgh);
		cmsg->cmsg_level = IPPROTO_IP;
		cmsg->cmsg_type = IP_PKTINFO;
		cmsg->cmsg_len = CMSG_LEN(sizeof(*pkt));

		pkt = (struct in_pktinfo *) CMSG_DATA(cmsg);
		memset(pkt, 0, sizeof(*pkt));
		pkt->ipi_spec_dst = s4->sin_addr;
	}

	if (from->sa_family == AF_INET6) {
		struct sockaddr_in6 *s6 = (struct sockaddr_in6 *) from;

		struct cmsghdr *cmsg;
		struct in6_pktinfo *pkt;

		msgh.msg_control = cbuf;
		msgh.msg_controllen = CMSG_SPACE(sizeof(*pkt));

		cmsg = CMSG_FIRSTHDR(&msgh);
		cmsg->cmsg_level = IPPROTO_IPV6;
		cmsg->cmsg_type = IPV6_PKTINFO;
		cmsg->cmsg_len = CMSG_LEN(sizeof(*pkt));

		pkt = (struct in6_pktinfo *) CMSG_DATA(cmsg);
		memset(pkt, 0, sizeof(*pkt));
		pkt->ipi6_addr = s6->sin6_addr;
#if 0
		// schlee : maybe ...
		pkt->ipi6_ifindex = ifindex 
#endif
	}

	return sendmsg(s, &msgh, flags);
}

int check_v4_mapped_v6(struct sockaddr *addr)
{
	if (addr->sa_family == AF_INET6) {
		struct sockaddr_in6* addr6 = (struct sockaddr_in6*)addr;
		if (IN6_IS_ADDR_V4MAPPED(&addr6->sin6_addr)) {
			struct sockaddr_in addr4;
			memset(&addr4, 0x00, sizeof(struct sockaddr_in));
			addr4.sin_family = AF_INET;
			addr4.sin_port = addr6->sin6_port;
			memcpy(&addr4.sin_addr.s_addr,addr6->sin6_addr.s6_addr+12, sizeof(addr4.sin_addr.s_addr));
			memcpy(addr, &addr4, sizeof(addr4));

			return 1;
		}
	}
	return 0;
}
