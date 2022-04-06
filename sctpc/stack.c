#include "sctpc.h"

int sctp_noti_assoc_change(struct sctp_assoc_change *sac, const char **event_state)
{
	switch (sac->sac_state) {
		case SCTP_COMM_UP:
			*event_state = "communication up";
			return (0);
		case SCTP_COMM_LOST:
			*event_state = "communication lost";
			return (-1);
		case SCTP_RESTART:
			*event_state = "restart";
			return (0);
		case SCTP_SHUTDOWN_COMP:
			*event_state = "shutdown complete";
			return (-1);
		case SCTP_CANT_STR_ASSOC:
			*event_state = "can't start association";
			return (0);
		default:
			*event_state = "unknwon sac";
			return (-1);
	}
}

int sctp_noti_peer_addr_change(struct sctp_paddr_change *spc, const char **event_state)
{
	switch(spc->spc_state) {
		case SCTP_ADDR_AVAILABLE:
			*event_state = "address available";
			return (0);
		case SCTP_ADDR_UNREACHABLE:
			*event_state = "address unreachable";
			return (0);
		case SCTP_ADDR_REMOVED:
			*event_state = "address removed";
			return (0);
		case SCTP_ADDR_ADDED:
			*event_state = "address added";
			return (0);
		case SCTP_ADDR_MADE_PRIM:
			*event_state = "address made primary";
			return (0);
		case SCTP_ADDR_CONFIRMED:
			*event_state = "address confirmed";
			return (0);
		default:
			*event_state = "unknwon spc";
			return (-1);
	}
}

int handle_sctp_notification(union sctp_notification *notif, size_t notif_len, const char **event_str, const char **event_state)
{
	int notif_header_size = sizeof(((union sctp_notification *)NULL)->sn_header);
	if (notif_len < notif_header_size) {
		fprintf(stderr, "%s() recv invalid sctp noti!\n", __func__);
		return (-1);
	}

	switch (notif->sn_header.sn_type) {
		case SCTP_ASSOC_CHANGE:
			*event_str = "sctp_assoc_change";
			return sctp_noti_assoc_change(&notif->sn_assoc_change, event_state);
		case SCTP_PEER_ADDR_CHANGE:
			*event_str = "sctp_peer_addr_change";
			return sctp_noti_peer_addr_change(&notif->sn_paddr_change, event_state);
		case SCTP_SEND_FAILED:
			*event_str = "sctp_send_failed";
			return (0);
		case SCTP_REMOTE_ERROR:
			*event_str = "sctp_remote_error";
			return (-1);
		case SCTP_SHUTDOWN_EVENT:
			*event_str = "sctp_shutdown_event";
			return (-1);
		case SCTP_PARTIAL_DELIVERY_EVENT:
			*event_str = "sctp_partial_delivery_event";
			return (0);
		case SCTP_ADAPTATION_INDICATION:
			*event_str = "sctp_adaptation_indication";
			return (0);
		case SCTP_AUTHENTICATION_INDICATION:
			*event_str = "sctp_authentication_indication";
			return (0);
		case SCTP_SENDER_DRY_EVENT:
			*event_str = "sctp_sender_dry_event";
			return (0);
		default:
			*event_str = "sctp_unknown_event";
			return (-1);
	}
}

char *get_path_state_str(int state)
{
	switch (state) {
		case SCTP_INACTIVE:
			return "INACTIVE";
		case SCTP_PF:
			return "PF";
		case SCTP_ACTIVE:
			return "ACTIVE";
		case SCTP_UNCONFIRMED:
			return "UNCONFIRMED";
		default:
			return "UNKNOWN";
	}
}

int get_assoc_state(int sd, int assoc_id, const char **state_str)
{
	const char *sctp_state_str[] = {
		"sctp_empty", 
		"sctp_closed",
		"sctp_cookie_wait",
		"sctp_cookie_echoed",
		"sctp_established",
		"sctp_shutdown_pending",
		"sctp_shutdown_sent",
		"sctp_shutdown_received",
		"sctp_shutdown_ack_sent" };

	struct sctp_status status = {0,};
	socklen_t optlen = sizeof(struct sctp_status);
	if (sctp_opt_info(sd, assoc_id, SCTP_STATUS, (char *)&status, &optlen) < 0) {
		*state_str = "unknown_state";
		return -1;
	} else {
		*state_str = sctp_state_str[status.sstat_state];
		return status.sstat_state;
	}
}
