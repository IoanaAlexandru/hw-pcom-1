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
    pack *pack = default_pack();
    int i = 0, f = -1, eot = 0, prev = -1, ok = 0;
    char type;
    char filename[30];

    do {

    	r = receive_message_timeout(TIME * 1000);
    	if (r == NULL) {
    		if (f == -1) {
    			r = receive_message_timeout(TIME * 2000);
    			if (r == NULL) {
    				perror("RECV: Timeout error. Shutting down");
	    			return -2;
	    		}
    		} else {
	    		if (i == 3) {
	    			perror("RECV: Timeout error. Shutting down");
		    		return -2;
	    		}

	    		i++;
                printf("Try again\n");
	    		send_pack(pack);
	    		continue;
	    	}
    	} else {
    		i = 0;
    	}
        
        free(pack);
        pack = string_to_pack((unsigned char*)r->payload);

        if (pack->SEQ == prev && ok) {
            send_pack(pack);
            continue;
        }

        unsigned short received_crc = pack->CHECK,
                       computed_crc = crc16_ccitt(r->payload, r->len - 3);

        prev = pack->SEQ;
        pack->SEQ = (pack->SEQ + 1) % 64;
        int len = pack->LEN;
        pack->LEN = 5;

        if (received_crc == computed_crc) {
            ok = 1;

        	type = pack->TYPE;
            pack->TYPE = 'Y';

            switch(type) {
                case 'S':
                	pack->DATA = (unsigned char*)default_Sdata();
                    pack->LEN += 11;
                    break;
                case 'F':
                    strcpy(filename, "recv_");
                    strcat(filename, (char*)pack->DATA);
                    f = open(filename, O_WRONLY | O_CREAT, 0744);
                    break;
                case 'D':
                    write(f, pack->DATA, len - 5);
                    break;
                case 'Z':
                    close(f);
                    break;
                default:
                    eot = 1;
            }
        } else {
            ok = 0;

            printf("Received CRC = 0x%X | Computed CRC = 0x%X\n", received_crc, computed_crc);
            pack->TYPE = 'N';
        }
        
        //Sending response
        send_pack(pack);
        if (pack->DATA != NULL)
            free(pack->DATA);

    } while(!eot);

    return 0;
}
