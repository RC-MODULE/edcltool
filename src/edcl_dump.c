#include <stdio.h>
#include <edcl.h>

int main(int argc, char* argv[]) {
	size_t address;
	size_t length;

	if(argc < 4 
			|| sscanf(argv[2], "%x", &address) != 1 
			|| (sscanf(argv[3], "0x%x", &length) != 1 && sscanf(argv[3], "%d", &length) != 1)) {
		fprintf(stderr, "usage: %s interface address length\n", argv[0]);
		return -1;
	}

	if(edcl_init(argv[1], 
		     EDCL_DEFAULT_BOARD_ADDRESS, 
		     EDCL_DEFAULT_HOST_ADDRESS)) {
		perror("edcl_init");
		return -1;
	}

	for(;length != 0;) {
		char buf[256];
		size_t n = sizeof(buf) < length ? sizeof(buf) : length;

		if(edcl_read(address, buf, n)) {
			perror("edcl_read");
			return -1;
		}

		fwrite(buf, 1, n, stdout);

		address += n;
		length -= n;
	}

	return 0;
}
