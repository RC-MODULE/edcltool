#include <edcl.h>
#include <errno.h>

#define CHK(x) if(x) { perror(#x); exit(errno); };

int main(int argc, char* argv[]) {
	if(edcl_init(argv[1], 
		     EDCL_DEFAULT_BOARD_ADDRESS, 
		     EDCL_DEFAULT_HOST_ADDRESS)) {
		perror("edcl_init");
		return -1;
	}
	char buf[16];
	CHK(edcl_read(0x40008000, buf, sizeof(buf)));
	return 0;
}
