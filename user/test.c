#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]) {
	int p[2];
	int x;
	pipe(p);
	if (fork() == 0) {
		close(p[1]);
		while (read(p[0], &x, sizeof(x))) {
			printf("child has received %d\n", x);	
		}
		printf("child has read all\n");
		close(p[0]);
		exit(0);
	} else {
		x = 2;
		write(p[1], &x, sizeof(int)); 
		x = 4;
		write(p[1], &x, sizeof(int)); 
		close(p[0]);
		close(p[1]);
		wait((int*)0);
		exit(0);
	}
	return 0;
}
