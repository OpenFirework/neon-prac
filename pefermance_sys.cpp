#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <asm/unistd.h>
#include <linux/perf_event.h>

#define N 32768

#define SIZE 128<<20

#define A15 1

#if ARM11
#define CLOCK_RARTE 700.0e6
#elif A9
#define CLOCL_RATE 666.0e6
#elif A15
#define CLOCK_RATE 2230.5e6
#endif


#define CYCLES 0
#define INSTRUCTIONS 1
#define CACHEREFS 2
#define CACHEMISSES 3

long perf_event_open(struct perf_event_attr *hw_event,
										 pid_t pid,
										 int cpu,
										 int group_fd,
										 unsigned long flags) {
	int ret;
	ret = syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
	return ret;									 
}

static inline unsigned long long perf_count(unsigned long long *values) {
	return (unsigned long long)((float)values[0]*
																		(float)values[1]/(float)values[2]);
}

int fd[5];
unsigned long long cnts[4][3];

int cnts_open(int *fd) {
	struct perf_event_attr attr;
	memset(&attr, 0, sizeof(attr));
	attr.type = PERF_TYPE_HARDWARE;
	attr.size = sizeof(struct perf_event_attr);
	attr.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED|PERF_FORMAT_TOTAL_TIME_RUNNING;
	attr.config = PERF_COUNT_HW_CPU_CYCLES;
	
	fd[CYCLES] = perf_event_open(&attr, 0, -1, -1, 0);
	
	printf("ret is %d\n", fd[CYCLES]);
	if(fd[CYCLES] == -1) {
		printf("cannot open perf_counter for cycles\n");
		exit(0);		
	}
	
	attr.config  = PERF_FORMAT_TOTAL_TIME_ENABLED;
	fd[INSTRUCTIONS] = perf_event_open(&attr, 0, -1, -1, 0);
	if (fd[INSTRUCTIONS] == -1) {
		printf("cannot open perf_counter for instructions\n");
		exit(0);	
	}
	
	#ifdef ARM11
	attr.type = PERF_TYPE_RAW;
	attr.config = 0x9;
	fd[CACHEREFS] = perf_event_open(&attr, 0, -1, -1, 0);
	if(fd[CACHEREFS] == -1) {
		printf("cannot open perf_counter for cacherefs\n");
		exit(0);
	}
	
	attr.config = 0xB;
	fd[CACHEMISSES] = perf_event_open(&attr, 0, -1, -1, 0);
	if(fd[CACHEMISSES] == -1) {
		printf("cannot open perf_counter for cachermisses\n");
		exit(0);
	}
	#else         
	attr.config = PERF_COUNT_HW_CACHE_REFERENCES;
	fd[CACHEREFS] = perf_event_open(&attr, 0, -1, -1, 0);
	if(fd[CACHEREFS] == -1) {
		printf("cannot open perf_counter for cacherefs\n");
		exit(0);
	}
	
	attr.config = PERF_COUNT_HW_CACHE_MISSES;
	fd[CACHEMISSES] = perf_event_open(&attr, 0, -1, -1, 0);
	if(fd[CACHEMISSES] == -1) {
		printf("cannot open perf_counter for cachermisses\n");
		exit(0);
	}
	#endif
		
}
void cnts_close(int *fd) {
	int i;
	for(i=0;i<4;i++) close(fd[i]);
}

void cnts_tick(int *fd) {
	int i,ret;
	cnts_open(fd);
	for(i=0;i<4;i++) {
		ret = ioctl(fd[i], PERF_EVENT_IOC_RESET);
		if(ret == -1) {
			printf("ioctls() in cnts_tick() failed");
		}
	}
}

void cnts_tock(int *fd, unsigned long long cnts[][3]) {
	int i, ret;
	for(i=0;i<4;i++) {
		ret = read(fd[i], cnts[i], sizeof(cnts[i]));
		if(ret != 24) {
			printf("ioctls() in cnts_tick() failed");
		}
	}
	cnts_close(fd);
}


void cnts_dump(unsigned long long cnts[][3]) {
	float time, membw;
	time = ((float)perf_count(cnts[CYCLES]))/CLOCK_RATE;
	membw = (float)N/time/(1024.0f*1024.0f);
	printf("[perf_event] cycles = %llu (%0.0f us) \n",
				perf_count(cnts[CYCLES]), 
				(float)(perf_count(cnts[CYCLES])/(CLOCK_RATE/10.E6)));
				
	printf("[perf_event] %0.2f MB/s \n", membw);
	printf("[perf_event instructions = %llu (CPI = %0.2f) \n", 
					perf_count(cnts[INSTRUCTIONS]),
					(float)(perf_count(cnts[CYCLES]))/(float)(perf_count(cnts[INSTRUCTIONS]))
					);
					
	printf("[perf_event] misses = %llu, references = %llu (miss rate=%0.4f) \n",
				perf_count(cnts[CACHEMISSES]),
				perf_count(cnts[CACHEREFS]),
				(float)(perf_count(cnts[CACHEMISSES]))/(float)(perf_count(cnts[CACHEREFS]))
				);
}


struct timeval time1,time2;


void memory_test() {
	int fd[5];
	unsigned long long cnets[4][3];
	unsigned int us;
	struct timeval time1, time2;
	unsigned int *test_array;
	int i,j,n,z,sum=0;
	test_array = (unsigned int*)malloc(N+0x10);
	// align on 16-byte boundary
	test_array = (unsigned int*)(((unsigned int)test_array & ~0x1F) + 0x10);
	
	for(i=0;i<(N>>2);i++) {
		test_array[i] = 0;
	}
	
	printf("-----read test-------\n");
	cnts_tick(fd);
	gettimeofday(&time1,0);
	for(i=0;i<(N>>2);i++) {
		sum += test_array[i];
	}
	gettimeofday(&time2,0);
	cnts_tock(fd, cnts);
	us = time2.tv_sec*1000000 + time2.tv_usec - time1.tv_sec*1000000 - time1.tv_usec;
	cnts_dump(cnts);
	printf("[sum]=%d\n",sum);
	
	printf("-----write test-------\n");
	cnts_tick(fd);
	gettimeofday(&time1, 0);
	for(i=0;i<(N>>2);i++) {
		test_array[i] = 0;
	}
	gettimeofday(&time2, 0);
	cnts_tock(fd, cnts);
	us = time2.tv_sec*1000000 + time2.tv_usec - time1.tv_sec*1000000 - time1.tv_usec;
	printf("[system clock] time = %d us \n", us);
	cnts_dump(cnts);
	

}

int main() {
	memory_test();
	return 0;
}




