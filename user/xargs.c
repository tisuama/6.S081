#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char* argv[]) {
	char* args1[MAXARG];
	int cnt1 = 0;
	for (int i = 1; i < argc; i++) {
		args1[cnt1++] = argv[i];
	}
	char buf[MAXARG][20], c;
	int cnt = 0, num = 0;
	while (read(0, &c, sizeof(c))) {
		if (c == '\n') {
			buf[num][cnt] = 0;
			args1[cnt1++] = buf[num++];
			cnt = 0;			
			continue;
		}
		buf[num][cnt++] = c;	
	}
// 	for (int i = 0; i < cnt1; i++) {
// 		printf("args1 %d: %s\n", i, args1[i]);
// 	}
	args1[cnt1] = 0;
	exec(args1[0], args1);
	exit(0);
}
