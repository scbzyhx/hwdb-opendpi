/*
 * mail_imap.c
 * Copyright (C) 2009-2010 by ipoque GmbH
 * 
 * This file is part of OpenDPI, an open source deep packet inspection
 * library based on the PACE technology by ipoque GmbH
 * 
 * OpenDPI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * OpenDPI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with OpenDPI.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */


#include "ipq_protocols.h"
#ifdef IPOQUE_PROTOCOL_MAIL_IMAP

static void ipoque_int_mail_imap_add_connection(struct ipoque_detection_module_struct
												*ipoque_struct)
{
	struct ipoque_packet_struct *packet = &ipoque_struct->packet;
	struct ipoque_flow_struct *flow = ipoque_struct->flow;
	struct ipoque_id_struct *src = ipoque_struct->src;
	struct ipoque_id_struct *dst = ipoque_struct->dst;

	flow->detected_protocol = IPOQUE_PROTOCOL_MAIL_IMAP;
	packet->detected_protocol = IPOQUE_PROTOCOL_MAIL_IMAP;

	if (src != NULL) {
		IPOQUE_ADD_PROTOCOL_TO_BITMASK(src->detected_protocol_bitmask, IPOQUE_PROTOCOL_MAIL_IMAP);
	}
	if (dst != NULL) {
		IPOQUE_ADD_PROTOCOL_TO_BITMASK(dst->detected_protocol_bitmask, IPOQUE_PROTOCOL_MAIL_IMAP);
	}
}

void ipoque_search_mail_imap_tcp(struct ipoque_detection_module_struct
								 *ipoque_struct)
{
	struct ipoque_packet_struct *packet = &ipoque_struct->packet;
	struct ipoque_flow_struct *flow = ipoque_struct->flow;

	u16 i = 0;
	u16 space_pos = 0;
	u16 command_start = 0;
	u8 saw_command = 0;
	const u8 *command = 0;


	IPQ_LOG(IPOQUE_PROTOCOL_MAIL_IMAP, ipoque_struct, IPQ_LOG_DEBUG, "search IMAP.\n");



	if (packet->payload_packet_len >= 4 && ntohs(get_u16(packet->payload, packet->payload_packet_len - 2)) == 0x0d0a) {
		// the DONE command appears without a tag
		if (packet->payload_packet_len == 6 && ((packet->payload[0] == 'D' || packet->payload[0] == 'd')
												&& (packet->payload[1] == 'O' || packet->payload[1] == 'o')
												&& (packet->payload[2] == 'N' || packet->payload[2] == 'n')
												&& (packet->payload[3] == 'E' || packet->payload[3] == 'e'))) {
			flow->mail_imap_stage += 1;
			saw_command = 1;
		} else {

			// search for the first space character (end of the tag)
			while (i < 20 && i < packet->payload_packet_len) {
				if (i > 0 && packet->payload[i] == ' ') {
					space_pos = i;
					break;
				}
				if (!((packet->payload[i] >= 'a' && packet->payload[i] <= 'z') ||
					  (packet->payload[i] >= 'A' && packet->payload[i] <= 'Z') ||
					  (packet->payload[i] >= '0' && packet->payload[i] <= '9') || packet->payload[i] == '*')) {
					goto imap_excluded;
				}
				i++;
			}
			if (space_pos == 0 || space_pos == (packet->payload_packet_len - 1)) {
				goto imap_excluded;
			}
			// now walk over a possible mail number to the next space
			i++;
			if (i < packet->payload_packet_len && (packet->payload[i] >= '0' && packet->payload[i] <= '9')) {
				while (i < 20 && i < packet->payload_packet_len) {
					if (i > 0 && packet->payload[i] == ' ') {
						space_pos = i;
						break;
					}
					if (!(packet->payload[i] >= '0' && packet->payload[i] <= '9')) {
						goto imap_excluded;
					}
					i++;
				}
				if (space_pos == 0 || space_pos == (packet->payload_packet_len - 1)) {
					goto imap_excluded;
				}
			}

			command_start = space_pos + 1;
			command = &(packet->payload[command_start]);

			if ((command_start + 3) < packet->payload_packet_len) {
				if ((packet->payload[command_start] == 'O' || packet->payload[command_start] == 'o')
					&& (packet->payload[command_start + 1] == 'K' || packet->payload[command_start + 1] == 'k')
					&& packet->payload[command_start + 2] == ' ') {
					flow->mail_imap_stage += 1;
					saw_command = 1;
				} else if ((packet->payload[command_start] == 'U' || packet->payload[command_start] == 'u')
						   && (packet->payload[command_start + 1] == 'I' || packet->payload[command_start + 1] == 'i')
						   && (packet->payload[command_start + 2] == 'D' || packet->payload[command_start + 2] == 'd')) {
					flow->mail_imap_stage += 1;
					saw_command = 1;
				}
			}
			if ((command_start + 10) < packet->payload_packet_len) {
				if ((packet->payload[command_start] == 'C' || packet->payload[command_start] == 'c')
					&& (packet->payload[command_start + 1] == 'A' || packet->payload[command_start + 1] == 'a')
					&& (packet->payload[command_start + 2] == 'P' || packet->payload[command_start + 2] == 'p')
					&& (packet->payload[command_start + 3] == 'A' || packet->payload[command_start + 3] == 'a')
					&& (packet->payload[command_start + 4] == 'B' || packet->payload[command_start + 4] == 'b')
					&& (packet->payload[command_start + 5] == 'I' || packet->payload[command_start + 5] == 'i')
					&& (packet->payload[command_start + 6] == 'L' || packet->payload[command_start + 6] == 'l')
					&& (packet->payload[command_start + 7] == 'I' || packet->payload[command_start + 7] == 'i')
					&& (packet->payload[command_start + 8] == 'T' || packet->payload[command_start + 8] == 't')
					&& (packet->payload[command_start + 9] == 'Y' || packet->payload[command_start + 9] == 'y')) {
					flow->mail_imap_stage += 1;
					saw_command = 1;
				}
			}
			if ((command_start + 8) < packet->payload_packet_len) {
				if ((packet->payload[command_start] == 'S' || packet->payload[command_start] == 's')
					&& (packet->payload[command_start + 1] == 'T' || packet->payload[command_start + 1] == 't')
					&& (packet->payload[command_start + 2] == 'A' || packet->payload[command_start + 2] == 'a')
					&& (packet->payload[command_start + 3] == 'R' || packet->payload[command_start + 3] == 'r')
					&& (packet->payload[command_start + 4] == 'T' || packet->payload[command_start + 4] == 't')
					&& (packet->payload[command_start + 5] == 'T' || packet->payload[command_start + 5] == 't')
					&& (packet->payload[command_start + 6] == 'L' || packet->payload[command_start + 6] == 'l')
					&& (packet->payload[command_start + 7] == 'S' || packet->payload[command_start + 7] == 's')) {
					flow->mail_imap_stage += 1;
					saw_command = 1;
				}
			}
			if ((command_start + 5) < packet->payload_packet_len) {
				if ((packet->payload[command_start] == 'L' || packet->payload[command_start] == 'l')
					&& (packet->payload[command_start + 1] == 'O' || packet->payload[command_start + 1] == 'o')
					&& (packet->payload[command_start + 2] == 'G' || packet->payload[command_start + 2] == 'g')
					&& (packet->payload[command_start + 3] == 'I' || packet->payload[command_start + 3] == 'i')
					&& (packet->payload[command_start + 4] == 'N' || packet->payload[command_start + 4] == 'n')) {
					flow->mail_imap_stage += 1;
					saw_command = 1;
				} else if ((packet->payload[command_start] == 'F' || packet->payload[command_start] == 'f')
						   && (packet->payload[command_start + 1] == 'E' || packet->payload[command_start + 1] == 'e')
						   && (packet->payload[command_start + 2] == 'T' || packet->payload[command_start + 2] == 't')
						   && (packet->payload[command_start + 3] == 'C' || packet->payload[command_start + 3] == 'c')
						   && (packet->payload[command_start + 4] == 'H' || packet->payload[command_start + 4] == 'j')) {
					flow->mail_imap_stage += 1;
					saw_command = 1;
				} else if ((packet->payload[command_start] == 'F' || packet->payload[command_start] == 'f')
						   && (packet->payload[command_start + 1] == 'L' || packet->payload[command_start + 1] == 'l')
						   && (packet->payload[command_start + 2] == 'A' || packet->payload[command_start + 2] == 'a')
						   && (packet->payload[command_start + 3] == 'G' || packet->payload[command_start + 3] == 'g')
						   && (packet->payload[command_start + 4] == 'S' || packet->payload[command_start + 4] == 's')) {
					flow->mail_imap_stage += 1;
					saw_command = 1;
				} else if ((packet->payload[command_start] == 'C' || packet->payload[command_start] == 'c')
						   && (packet->payload[command_start + 1] == 'H' || packet->payload[command_start + 1] == 'h')
						   && (packet->payload[command_start + 2] == 'E' || packet->payload[command_start + 2] == 'e')
						   && (packet->payload[command_start + 3] == 'C' || packet->payload[command_start + 3] == 'c')
						   && (packet->payload[command_start + 4] == 'K' || packet->payload[command_start + 4] == 'k')) {
					flow->mail_imap_stage += 1;
					saw_command = 1;
				}
			}
			if ((command_start + 12) < packet->payload_packet_len) {
				if ((packet->payload[command_start] == 'A' || packet->payload[command_start] == 'a')
					&& (packet->payload[command_start + 1] == 'U' || packet->payload[command_start + 1] == 'u')
					&& (packet->payload[command_start + 2] == 'T' || packet->payload[command_start + 2] == 't')
					&& (packet->payload[command_start + 3] == 'H' || packet->payload[command_start + 3] == 'h')
					&& (packet->payload[command_start + 4] == 'E' || packet->payload[command_start + 4] == 'e')
					&& (packet->payload[command_start + 5] == 'N' || packet->payload[command_start + 5] == 'n')
					&& (packet->payload[command_start + 6] == 'T' || packet->payload[command_start + 6] == 't')
					&& (packet->payload[command_start + 7] == 'I' || packet->payload[command_start + 7] == 'i')
					&& (packet->payload[command_start + 8] == 'C' || packet->payload[command_start + 8] == 'c')
					&& (packet->payload[command_start + 9] == 'A' || packet->payload[command_start + 9] == 'a')
					&& (packet->payload[command_start + 10] == 'T' || packet->payload[command_start + 10] == 't')
					&& (packet->payload[command_start + 11] == 'E' || packet->payload[command_start + 11] == 'e')) {
					flow->mail_imap_stage += 1;
					saw_command = 1;
				}
			}
			if ((command_start + 9) < packet->payload_packet_len) {
				if ((packet->payload[command_start] == 'N' || packet->payload[command_start] == 'n')
					&& (packet->payload[command_start + 1] == 'A' || packet->payload[command_start + 1] == 'a')
					&& (packet->payload[command_start + 2] == 'M' || packet->payload[command_start + 2] == 'm')
					&& (packet->payload[command_start + 3] == 'E' || packet->payload[command_start + 3] == 'e')
					&& (packet->payload[command_start + 4] == 'S' || packet->payload[command_start + 4] == 's')
					&& (packet->payload[command_start + 5] == 'P' || packet->payload[command_start + 5] == 'p')
					&& (packet->payload[command_start + 6] == 'A' || packet->payload[command_start + 6] == 'a')
					&& (packet->payload[command_start + 7] == 'C' || packet->payload[command_start + 7] == 'c')
					&& (packet->payload[command_start + 8] == 'E' || packet->payload[command_start + 8] == 'e')) {
					flow->mail_imap_stage += 1;
					saw_command = 1;
				}
			}
			if ((command_start + 4) < packet->payload_packet_len) {
				if ((packet->payload[command_start] == 'L' || packet->payload[command_start] == 's')
					&& (packet->payload[command_start + 1] == 'S' || packet->payload[command_start + 1] == 's')
					&& (packet->payload[command_start + 2] == 'U' || packet->payload[command_start + 2] == 'u')
					&& (packet->payload[command_start + 3] == 'B' || packet->payload[command_start + 3] == 'b')) {
					flow->mail_imap_stage += 1;
					saw_command = 1;
				} else if ((packet->payload[command_start] == 'L' || packet->payload[command_start] == 'l')
						   && (packet->payload[command_start + 1] == 'I' || packet->payload[command_start + 1] == 'i')
						   && (packet->payload[command_start + 2] == 'S' || packet->payload[command_start + 2] == 's')
						   && (packet->payload[command_start + 3] == 'T' || packet->payload[command_start + 3] == 't')) {
					flow->mail_imap_stage += 1;
					saw_command = 1;
				} else if ((packet->payload[command_start] == 'N' || packet->payload[command_start] == 'n')
						   && (packet->payload[command_start + 1] == 'O' || packet->payload[command_start + 1] == 'o')
						   && (packet->payload[command_start + 2] == 'O' || packet->payload[command_start + 2] == 'o')
						   && (packet->payload[command_start + 3] == 'P' || packet->payload[command_start + 3] == 'p')) {
					flow->mail_imap_stage += 1;
					saw_command = 1;
				} else if ((packet->payload[command_start] == 'I' || packet->payload[command_start] == 'i')
						   && (packet->payload[command_start + 1] == 'D' || packet->payload[command_start + 1] == 'd')
						   && (packet->payload[command_start + 2] == 'L' || packet->payload[command_start + 2] == 'l')
						   && (packet->payload[command_start + 3] == 'E' || packet->payload[command_start + 3] == 'e')) {
					flow->mail_imap_stage += 1;
					saw_command = 1;
				}
			}
			if ((command_start + 6) < packet->payload_packet_len) {
				if ((packet->payload[command_start] == 'S' || packet->payload[command_start] == 's')
					&& (packet->payload[command_start + 1] == 'E' || packet->payload[command_start + 1] == 'e')
					&& (packet->payload[command_start + 2] == 'L' || packet->payload[command_start + 2] == 'l')
					&& (packet->payload[command_start + 3] == 'E' || packet->payload[command_start + 3] == 'e')
					&& (packet->payload[command_start + 4] == 'C' || packet->payload[command_start + 4] == 'c')
					&& (packet->payload[command_start + 5] == 'T' || packet->payload[command_start + 5] == 't')) {
					flow->mail_imap_stage += 1;
					saw_command = 1;
				} else if ((packet->payload[command_start] == 'E' || packet->payload[command_start] == 'e')
						   && (packet->payload[command_start + 1] == 'X' || packet->payload[command_start + 1] == 'x')
						   && (packet->payload[command_start + 2] == 'I' || packet->payload[command_start + 2] == 'i')
						   && (packet->payload[command_start + 3] == 'S' || packet->payload[command_start + 3] == 's')
						   && (packet->payload[command_start + 4] == 'T' || packet->payload[command_start + 4] == 't')
						   && (packet->payload[command_start + 5] == 'S' || packet->payload[command_start + 5] == 's')) {
					flow->mail_imap_stage += 1;
					saw_command = 1;
				}
			}

		}

		if (saw_command == 1) {
			if (flow->mail_imap_stage == 3) {
				IPQ_LOG(IPOQUE_PROTOCOL_MAIL_IMAP, ipoque_struct, IPQ_LOG_DEBUG, "mail imap identified\n");
				ipoque_int_mail_imap_add_connection(ipoque_struct);
				return;
			}
		}
	}

  imap_excluded:

	// skip over possible authentication hashes etc. that cannot be identified as imap commands or responses
	// if the packet count is low enough and at least one command or response was seen before
	if ((packet->payload_packet_len >= 2 && ntohs(get_u16(packet->payload, packet->payload_packet_len - 2)) == 0x0d0a)
		&& flow->packet_counter < 6 && flow->mail_imap_stage >= 1) {
		IPQ_LOG(IPOQUE_PROTOCOL_MAIL_IMAP, ipoque_struct, IPQ_LOG_DEBUG,
				"no imap command or response but packet count < 6 and imap stage >= 1 -> skip\n");
		return;
	}

	IPQ_LOG(IPOQUE_PROTOCOL_MAIL_IMAP, ipoque_struct, IPQ_LOG_DEBUG, "exclude IMAP.\n");
	IPOQUE_ADD_PROTOCOL_TO_BITMASK(flow->excluded_protocol_bitmask, IPOQUE_PROTOCOL_MAIL_IMAP);
}
#endif
