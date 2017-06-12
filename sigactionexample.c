/*  This program handles the sending and recieving of signals  */
/*  The program forks itself and has the child send signals (via kill) to the parent */
/*  The handler keeps track of how many of each type of signal has been handled      */



#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

volatile sig_atomic_t sighup_counter = 0;
volatile sig_atomic_t sigusr1_counter = 0;
volatile sig_atomic_t sigusr2_counter = 0;

void handler(int sig){  //Signal Handler 
		switch(sig){
			case SIGHUP:
				sighup_counter++;
				break;
			case SIGUSR1:
				sigusr1_counter++;
				break;
			case SIGUSR2:
				sigusr2_counter++;
				break;
		}
}

int main(void){ // Main

	int status;
	struct sigaction *pAction = (struct sigaction*)malloc( sizeof(struct sigaction) );
	
	if(!pAction){ 
		printf("Failed to allocate memory correctly\n");
		return 1; //error
	}

	pAction->sa_handler = handler;
	sigemptyset( &(pAction->sa_mask) );
	pAction->sa_flags = SA_RESTART | SA_NOCLDSTOP;
	if(sigaction(SIGUSR1, pAction, NULL) == 0)
		printf("[%i] SIGUSER1 action ready...\n", getpid());
	if(sigaction(SIGHUP, pAction, NULL) == 0)
		printf("[%i] SIGHUP action ready...\n", getpid());
	if(sigaction(SIGUSR2, pAction, NULL) == 0)
		printf("[%i] SIGUSR2 action ready...\n", getpid());

	printf("\n[%i] Initial counter values:   SIGHUP: %i, SIGUSR1: %i, SIGUSR2: %i\n\n", 
			getpid(), sighup_counter, sigusr1_counter, sigusr2_counter);

	fflush(stdout);  // or else whacky things happen
	pid_t wPID, parentPID;
	pid_t childPID = fork();
	
	if (childPID >= 0){
		if(childPID == 0){ //child process
			parentPID = getppid();
			printf("[%i] sending 1 of each signal...\n", getpid());
			kill( parentPID, SIGUSR1);
			kill( parentPID, SIGHUP);
			kill( parentPID, SIGUSR2);

			printf("[%i] sending 3 SIGHUP's...\n", getpid());
			kill( parentPID, SIGHUP);
			kill( parentPID, SIGHUP);
			kill( parentPID, SIGHUP);

			printf("[%i] sending 1 SIGUSR1...\n\n", getpid());
			kill( parentPID, SIGUSR1);
			fflush(stdout);
		  exit(0);

		}
		else{ //parent process

	
			wPID = waitpid(childPID, &status, 0);
			
			if(wPID == -1){ //things went wrong...
    		perror("wait error");
    		return -1;
    	}
    	else{ //waitpid went ok...
			free(pAction); 
			pAction = NULL;

			if (WIFEXITED(status))
      	printf("[%i] child exited, status=%d\n", getpid(), WEXITSTATUS(status));
      else if (WIFSIGNALED(status))
        printf("[%i] child killed (signal %d)\n", getpid(), WTERMSIG(status));
      else if (WIFSTOPPED(status))
        printf("[%i] child stopped (signal %d)\n", getpid(), WSTOPSIG(status));
      else  /* Non-standard case -- may never happen */
        printf("Unexpected status (0x%x)\n", status);

			printf("[%i] Final counter values:   SIGHUP: %i, SIGUSR1: %i, SIGUSR2: %i\n", 
			getpid(), sighup_counter, sigusr1_counter, sigusr2_counter);
			return 0;
		}
		}
	}
	else{
		printf("\n Fork failed, quitting!\n");
        return 2;  // error
	}
}
