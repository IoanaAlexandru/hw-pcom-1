#include <stdlib.h>
#include "lib.h"
#include <string.h>
#include <stdio.h>

char* default_Sdata() {
	char *S = (char*) calloc(11, sizeof(char));

	if (S == NULL) {
		printf("Allocation failed! (default_Sdata)");
		return S;
	}

	S[0] = MAXL;
	S[1] = TIME;
	S[2] = NPAD;
	S[3] = PADC;
	S[4] = EOL;

	return S;
}

pack* default_pack() {
	pack *my_pack = (pack*) calloc(1, sizeof(pack));

	if (my_pack == NULL) {
		printf("Allocation failed! (default_pack)");
		return my_pack;
	}

	my_pack->SOH = 1;
	my_pack->LEN = 5; //initial data length is 0
	my_pack->SEQ = -1;
	my_pack->TYPE = 'S';
	my_pack->MARK = EOL;
	
	return my_pack;
}

unsigned char* pack_to_string(pack *my_pack) {
	unsigned char len = my_pack->LEN;
	unsigned char *S = (unsigned char*) calloc(len + 2, sizeof(char));

	if (S == NULL) {
		printf("Allocation failed! (pack_to_string)");
		return S;
	}
	
	S[0] = my_pack->SOH;
	S[1] = my_pack->LEN;
	S[2] = my_pack->SEQ;
	S[3] = my_pack->TYPE;

	for (int i = 0; i < len - 5; i++) {
		S[i + 4] = my_pack->DATA[i];
	}

	my_pack->CHECK = crc16_ccitt(S, len - 1);
	memmove(S + len - 1, &(my_pack->CHECK), 2);
	S[len + 1] = my_pack->MARK;

	return S;
}

pack* string_to_pack(unsigned char *S) {
	pack *pack = default_pack();
	
	pack->SOH = S[0];
	pack->LEN = S[1];
	pack->SEQ = S[2];
	pack->TYPE = S[3];
	pack->DATA = (unsigned char*)calloc(pack->LEN - 5, sizeof(unsigned char));

	for (int i = 0; i < pack->LEN - 5; i++) {
		pack->DATA[i] = S[i + 4];
	}

	memmove(&(pack->CHECK), S + pack->LEN - 1, 2);
	pack->MARK = S[pack->LEN + 1];

	return pack;
}

void print_pack(unsigned char* pack) {
	unsigned char len = pack[1], type = pack[3];

	printf("+------+------+------+------+------+------+\n");
	printf("| SOH  | LEN  | SEQ  | TYPE |CHECK | MARK |\n");
	for (int i = 0; i < 4; i++)
		printf("| 0x%02X ", pack[i]);
	printf("|0x%02X%02X| 0x%02X |\n", pack[len], pack[len - 1], pack[len + 1]);
	printf("+------+------+------+------+------+------+\nDATA:\n");

	if (type == 'S' || (type == 'Y' && pack[1] != 5)) {
		printf("+------+------+------+------+------+------+------+------+------+------+------+\n");
		printf("| MAXL | TIME | NPAD | PADC | EOL  | QCTL | QBIN | CHKT | REPT | CAPA |  R   |\n");
		for (int i = 4; i < len - 1; i++) {
			printf("| 0x%02X ", pack[i]);
		}
		printf("|\n");
		printf("+------+------+------+------+------+------+------+------+------+------+------+");
	}
	else if (type == 'F') {
		for (int i = 4; i < len - 1; i++) {
			printf("%c", pack[i]);
		}
	} else {
		//Simulate xxd output
		for (int i = 4; i < len - 1; i++) {
			printf("%02X", pack[i]);
			if (i % 2 != 0)
				printf(" ");
			if ((i - 4) % 16 == 15)
				printf("\n");
		}
	}

	printf("\n\n");
}

void update_and_send_pack(pack *pack, unsigned char data_len, unsigned char type, char *data) {
	//Updating pack
	pack->LEN = 5 + data_len;
	pack->SEQ = (pack->SEQ + 1) % 64;
	pack->TYPE = type;
	pack->DATA = (unsigned char*)data;

	//Converting to string
	unsigned char *string_pack = pack_to_string(pack);
	printf("%c pack:\n", type);
	print_pack(string_pack);

	//Sending string pack
	msg s;
	memmove(s.payload, string_pack, pack->LEN + 2);
	s.len = pack->LEN + 2;
	send_message(&s);

	//Clearing data
	pack->DATA = NULL;
	free(string_pack);
}

void send_pack(pack *pack) {
	//Converting to string
	unsigned char *string_pack = pack_to_string(pack);
	printf("%c pack:\n", pack->TYPE);
	print_pack(string_pack);

	//Sending string pack
	msg s;
	memmove(s.payload, string_pack, pack->LEN + 2);
	s.len = pack->LEN + 2;
	send_message(&s);

	//Clearing data
	free(string_pack);
}

int verified_send_pack(pack *pack, unsigned char data_len, unsigned char type, char *data) {
	int i;

	for (i = 0; i < 3; i++) {
		//Sending pack
	    update_and_send_pack(pack, data_len, type, data);

		//Getting response
		msg *r;
		r = receive_message_timeout(TIME * 1000);
		if (r == NULL) {
		    perror("SENDER: Timeout error");
		} else if (r->payload[3] == 'Y' && r->payload[2] == (pack->SEQ + 1)) {
		    pack->SEQ++;
		    break;
		} else if (r->payload[3] == 'N') {
			i--;
		}

		pack->SEQ--;
	}

	if (i == 3) {
		perror("SENDER: Communication error. Shutting down.");
		return 0;
	}

	return 1;
}