#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

int main() {
	char buf[10],bbuf[10];
	int i;
	scanf("%s",buf);

	i = open("ff",O_RDWR | O_CREAT | O_TRUNC, 0644);
	pwrite(i,buf,8,10);
	
	pread(i,bbuf,8,10);
	printf("%s\n",bbuf);
	return 0;
	
}
