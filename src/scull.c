#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/wait.h>


#include "scull.h"

#define CDEV_NAME "/dev/scull"

/* Quantum command line option */
static int g_quantum;
static int numP;

static void usage(const char *cmd)
{
	printf("Usage: %s <command>\n"
	       "Commands:\n"
	       "  R          Reset quantum\n"
	       "  S <int>    Set quantum\n"
	       "  T <int>    Tell quantum\n"
	       "  G          Get quantum\n"
	       "  Q          Qeuery quantum\n"
	       "  X <int>    Exchange quantum\n"
	       "  H <int>    Shift quantum\n"
	       "  h          Print this message\n"
	       "  K 	     Testing\n"
	       "  p <n>	     Number of processes\n",		
	       cmd);
}

void current_task_info(task_info *info){
	printf("state %ld, ", info->state);
	printf("stack %ls, ", (long)info->stack);
	printf("cpu %d,", info->cpu);
	printf("prio %d,", info->prio);
	printf("sprio %d,", info->static_prio);
	printf("nprio %d,", info->normal_prio);
	printf("rtprio %i,", info->rt_priority);
	printf("pid %ld,", (long)info->pid);
	printf("tgid %ld,", (long)info->tgid);
	printf("nv %ls,", info->nvcsw);
	printf("niv %lu\n", info->nivcsw);
};



typedef int cmd_t;

static cmd_t parse_arguments(int argc, const char **argv)
{
	cmd_t cmd;

	if (argc < 2) {
		fprintf(stderr, "%s: Invalid number of arguments\n", argv[0]);
		cmd = -1;
		goto ret;
	}

	/* Parse command and optional int argument */
	cmd = argv[1][0];
	switch (cmd) {
	case 'S':
	case 'T':
	case 'H':
	case 'X':
		if (argc < 3) {
			fprintf(stderr, "%s: Missing quantum\n", argv[0]);
			cmd = -1;
			break;
		}
		g_quantum = atoi(argv[2]);
		break;
	case 'R':
	case 'G':
	case 'Q':
	case 'h':
	case 'K':
		break;
	case 'p':
		if (argc < 3 ){
			fprintf(stderr, "Missing arg");
			cmd = -1;
			break;
		}
		numP = atoi(argv[2]);
		break;
	default:
		fprintf(stderr, "%s: Invalid command\n", argv[0]);
		cmd = -1;
	}

ret:
	if (cmd < 0 || cmd == 'h') {
		usage(argv[0]);
		exit((cmd == 'h')? EXIT_SUCCESS : EXIT_FAILURE);
	}
	return cmd;
}

static int do_op(int fd, cmd_t cmd)
{
	int ret, q;
	task_info *info;

	switch (cmd) {
	case 'R':
		ret = ioctl(fd, SCULL_IOCRESET);
		if (ret == 0)
			printf("Quantum reset\n");
		break;
	case 'Q':
		q = ioctl(fd, SCULL_IOCQQUANTUM);
		printf("Quantum: %d\n", q);
		ret = 0;
		break;
	case 'G':
		ret = ioctl(fd, SCULL_IOCGQUANTUM, &q);
		if (ret == 0)
			printf("Quantum: %d\n", q);
		break;
	case 'T':
		ret = ioctl(fd, SCULL_IOCTQUANTUM, g_quantum);
		if (ret == 0)
			printf("Quantum set\n");
		break;
	case 'S':
		q = g_quantum;
		ret = ioctl(fd, SCULL_IOCSQUANTUM, &q);
		if (ret == 0)
			printf("Quantum set\n");
		break;
	case 'X':
		q = g_quantum;
		ret = ioctl(fd, SCULL_IOCXQUANTUM, &q);
		if (ret == 0)
			printf("Quantum exchanged, old quantum: %d\n", q);
		break;
	case 'H':
		q = ioctl(fd, SCULL_IOCHQUANTUM, g_quantum);
		printf("Quantum shifted, old quantum: %d\n", q);
		ret = 0;
		break;
	case 'K':
/*		info = (task_info*)malloc(sizeof(task_info));
		ret = ioctl(fd, SCULL_IOCKQUANTUM, info);
		current_task_info(info);
		free(info);
		break; */

	case 'p':

		info = (task_info*)malloc(sizeof(task_info));


//		pid_t id = 1;	
		for(int i = 0; i < numP; i++){
			pid_t id  = fork();
			printf("id: %d\n", id);		
			if(id == 0){
//				printf("before ioctl");
				ret = ioctl(fd, SCULL_IOCKQUANTUM, info);
//				printf("before print");
				current_task_info(info);
				exit(0);
			}else if (id < 0){
				printf("id failed");
			}else{
				if (wait(NULL < 0)){
					printf("wait failed");
				}
			}
		}
	
/*	
		printf("before wait");
		if(wait(NULL) != -1){
			printf("wait finished");
		} */

		
//		free(info);

		/*
		for (int i = 0; i < numP; i++){
			printf("before fork\n");
			pid_t id = fork();
			printf("after fork\n");
			printf("id: %d\n", id);
			if(id == 0 || id > 0){
				printf("entering id\n");
				if(ret = ioctl(fd, SCULL_IOCKQUANTUM, &info) < 0){
					printf("failed");
				}
				printf("before prints\n");
				current_task_info(info);
				printf("after prints\n");
				exit(0);
			}else if(id < 0){
				fprintf(stderr, "error");
			}else{
				wait(NULL);
			}
		}
		
		free(info);
		*/
		
		break;
	default:
		/* Should never occur */
		abort();
		ret = -1; /* Keep the compiler happy */
	}

	if (ret != 0)
		perror("ioctl");
	return ret;
}

int main(int argc, const char **argv)
{
	int fd, ret;
	cmd_t cmd;

	cmd = parse_arguments(argc, argv);

	fd = open(CDEV_NAME, O_RDONLY);
	if (fd < 0) {
		perror("cdev open");
		return EXIT_FAILURE;
	}

	printf("Device (%s) opened\n", CDEV_NAME);

	ret = do_op(fd, cmd);

	if (close(fd) != 0) {
		perror("cdev close");
		return EXIT_FAILURE;
	}

	printf("Device (%s) closed\n", CDEV_NAME);

	return (ret != 0)? EXIT_FAILURE : EXIT_SUCCESS;
}
