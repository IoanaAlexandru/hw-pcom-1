#pragma pack(1)

#ifndef LIB
#define LIB

typedef struct {
    int len;
    char payload[1400];
} msg;

typedef struct {
	unsigned char SOH, LEN, SEQ, TYPE, MARK;
	unsigned short CHECK;
	unsigned char* DATA;
} pack;

void init(char* remote, int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);
msg* receive_message_timeout(int timeout); //timeout in milliseconds
unsigned short crc16_ccitt(const void *buf, int len);

char* default_Sdata();
pack* default_pack();
unsigned char* pack_to_string(pack *my_pack);
pack* string_to_pack(unsigned char *S);
void print_pack(unsigned char* pack);
void update_and_send_pack(pack *pack, unsigned char len, unsigned char type, char *data);
void send_pack(pack *pack);
int verified_send_pack(pack *pack, unsigned char len, unsigned char type, char *data);

#endif

#ifndef MAXL
#define MAXL 250
#endif

#ifndef TIME
#define TIME 1
#endif

#ifndef NPAD
#define NPAD 0
#endif

#ifndef PADC
#define PADC 0
#endif

#ifndef EOL
#define EOL 0x0D
#endif