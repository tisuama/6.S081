#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char* fmtname(char* path) {
	static char buf[DIRSIZ + 1];
    char* p;
	for (p = path + strlen(path); p >= path && *p != '/'; p--);
    p++;	
	if (strlen(p) >= DIRSIZ) {
		return p;
	}
	memmove(buf, p, strlen(p));
	memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));
	return buf;
}	

static int check_dir(char* name) {
	if (strcmp(name, ".") == 0 || 
		strcmp(name, "..") == 0) {
		return 1;
	}
	return 0;
}

void find(char* path, char* name) {
	char buf[512], *p;
	int fd;
	struct dirent de;
	struct stat st;
	if ((fd = open(path, 0)) < 0) {
		fprintf(2, "find: cannot open %s\n", path);
		return ;
	}
	if (fstat(fd, &st) < 0) {
		fprintf(2, "find: cannot stat %s\n", path);
		return ;
	}
	
	switch(st.type) {
	case T_FILE:
		if (strcmp(fmtname(path), name) == 0) {
			printf("%s\n", path);
		}
		break;
	case T_DIR:
		if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
			printf("find: path too long\n");
			break;
		}		
		strcpy(buf, path);
		p = buf + strlen(buf);
		*p++ = '/';
		while (read(fd, &de, sizeof(de)) == sizeof(de)) {
			if (de.inum == 0) {
				continue;
			}
			memmove(p, de.name, DIRSIZ);
			p[DIRSIZ] = 0;
			if (stat(buf, &st) < 0) {
				printf("find: cannot stat %s\n", buf);
				continue;
			}
			if (st.type == T_DIR && check_dir(de.name)) {
				continue;
			}
			if (st.type == T_DIR) {
				find(buf, name);
			} else if (strcmp(de.name, name) == 0) { 
				printf("%s\n", buf);
			}
		}
	}
}

int main(int argc, char* argv[]) {
	if (argc < 3) {
		exit(0);
	}
	
	find(argv[1], argv[2]);
	exit(0);
}
