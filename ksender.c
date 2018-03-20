#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

int main(int argc, char** argv) {
	init(HOST, PORT);
	
	pack *pack = default_pack();
	char *Sdata = default_Sdata();

	//Sending Send-Init pack
	if (!verified_send_pack(pack, 11, 'S', Sdata))
		return -2;

	free(Sdata);

	for (int i = 1; i < argc; i++) {

		//Sending File Header pack
		if (!verified_send_pack(pack, strlen(argv[i]) + 1, 'F', argv[i]))
			return -2;


		//Opening file
		int f = open(argv[i], O_RDONLY);

		if (f == -1) {
			printf("Could not open file %s!", argv[i]);
			return -1;
		}

		char buf[250];
		int size = read(f, buf, 250);
		while(size) {
			if (size == -1) {
				printf("Failed to read from file!\n");
			}

			//Sending Data pack
			if (!verified_send_pack(pack, size, 'D', buf))
				return -2;
			
			size = read(f, buf, 250);
		}
		
		close(f);

		//Sending EOF pack
		if (!verified_send_pack(pack, 0, 'Z', NULL))
			return -2;
	}

	//Sending EOT pack
	if (!verified_send_pack(pack, 0, 'B', NULL))
		return -2;

	free(pack);
	
	return 0;
}