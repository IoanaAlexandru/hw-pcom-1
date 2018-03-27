#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

int main(int argc, char** argv) {
	init(HOST, PORT);

	msg *r;
	int i = 0, f = -1, eot = 0;
	char filename[30], *Sdata = default_Sdata();
    int time = TIME;

	if (Sdata == NULL)
		return -2;

	pack *pack, *ack_pack = default_pack();

	if (ack_pack == NULL)
		return -2;

	do {

		r = receive_message_timeout(time * 1000);
		if (r == NULL) {
			if (f == -1) {
				r = receive_message_timeout(time * 2000);
				if (r == NULL) { //waited 3 * TIME seconds for Send-init pack => exit
					perror("RECV: Timeout error. Shutting down");
					return -1;
				}
			} else {
				if (i == 3) { //three consecutive timeouts => exit
					perror("RECV: Timeout error. Shutting down");
					return -1;
				}

				i++;
				send_pack(ack_pack); //send last ACK pack again
				free(r);
				continue;
			}
		} else {
			i = 0; //reset counter
		}
		

		pack = string_to_pack((unsigned char*)r->payload);

		if (pack == NULL) {
			free(Sdata);
			free(ack_pack);
			return -2;
		}

		/* Check if we received the right pack in the sequence,
		 * namely the next pack if the last ack was positive,
		 * or the previous pack otherwise.
		 */
		if (((pack->SEQ != (ack_pack->SEQ + 1) % 64) && ack_pack->TYPE == 'Y') ||
			((pack->SEQ != (ack_pack->SEQ - 1) % 64) && ack_pack->TYPE == 'N')) {
			send_pack(ack_pack);
			if (pack->DATA != NULL)
				free(pack->DATA);
			free(pack);
			free(r);
			continue;
		}

		unsigned short received_crc = pack->CHECK,
					   computed_crc = crc16_ccitt(r->payload, r->len - 3);

		ack_pack->LEN = 5;
		ack_pack->SEQ = (pack->SEQ + 1) % 64;

		if (received_crc == computed_crc) {
			ack_pack->TYPE = 'Y';

			switch(pack->TYPE) {
				case 'S':
                    time = pack->DATA[1];
					ack_pack->DATA = (unsigned char*)Sdata;
					ack_pack->LEN += 11;
					break;
				case 'F':
					strcpy(filename, "recv_");
					strcat(filename, (char*)pack->DATA);
					f = open(filename, O_WRONLY | O_CREAT, 0744);
					break;
				case 'D':
					write(f, pack->DATA, pack->LEN - 5);
					break;
				case 'Z':
					close(f);
					break;
				default:
					eot = 1;
			}
		} else {
			printf("Received CRC = 0x%X | Computed CRC = 0x%X\n", received_crc, computed_crc);
			ack_pack->TYPE = 'N';
		}
		
		//Sending response
		send_pack(ack_pack);

		//Free allocated memory
		if (pack->DATA != NULL)
			free(pack->DATA);
		free(pack);
		free(r);

	} while(!eot);

	free(ack_pack);
	free(Sdata);

	return 0;
}