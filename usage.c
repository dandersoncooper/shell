#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define CALL "grep 'cpu ' /proc/stat | awk '{usage=($2+$4)*100/($2+$4+$5)} END {print usage \"%\"}'"

#define CALL2 "{echo $usage}"

int CPUUTILIZATION() {
	float name;
	puts("\n The percent CPU usage over the past 24 hours");

	// This will get the percent of cpu used and get it twice and divide by the 24 hours.
	sleep(1);
	name = system(CALL);
	name = name + name / 2;
	printf("\n");
	//return 0;
}
