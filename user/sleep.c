#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]) {
	if (argc <= 1) {
		fprintf(2, "sleep need 1 args\n");
		exit(1);
	}
	int sleep_time = atoi(argv[1]);	
	if (sleep(sleep_time) < 0) {
		fprintf(2, "sleep failed to \n");
	}
	exit(0);
}
