#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int data[36];
int p[2];

void solve(int cnt) {
	pipe(p);
	// printf("process %d start to solve, cnt: %d\n", getpid(), cnt);
	if (fork() == 0) {
		int num = 0;
		int x, ret;
		close(p[1]);
		while ((ret = read(p[0], &x, sizeof(x))) != 0) {	
			data[num++] = x;	
		}		
		// printf("child %d has read all data, num: %d\n", getpid(), num);
		close(p[0]);
		if (num) {
			solve(num);
		}
		exit(0);
	} else {
		close(p[0]);
		printf("prime %d\n", data[0]);	
		for (int i = 1; i < cnt; i++) {
			if (data[i] % data[0]) {
				// printf("parent %d send %d\n", getpid(), data[i]);
				write(p[1], &data[i], sizeof(data[i]));
			}
		}
		close(p[1]);
		// printf("parent process has close p[1]\n", getpid());
		wait((int*)0);
		// printf("parent exit 0\n");
		exit(0);
	}
}

int main(int argc, char* argv[]) {
	int num = 0;
	for (int i = 2; i <= 35; i++) {
		data[num++] = i;
	}
	solve(num);
	return 0;
}



