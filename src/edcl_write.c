#include <stdio.h>
#include <edcl.h>


int main(int argc, char* argv[]) {
	size_t address;

	if(argc < 3 || sscanf(argv[2], "%x", &address) != 1) {
		fprintf(stderr, "usage: %s interface address\n", argv[0]);
		return -1;
	}

	if(edcl_init(argv[1])) {
		perror("edcl_init");
		return -1;
	}

	for(;;) {
		char buf[256];
		size_t n = fread(buf, 1, sizeof(buf), stdin);

		if(n == 0) break;

		if(edcl_write(address, buf, n)) {
			perror("edcl_write");
			return -1;
		}

		address += n;
	}

	return 0;
}
