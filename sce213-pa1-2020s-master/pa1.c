/**********************************************************************
 * Copyright (c) 2020
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h> // for execvp function
#include <sys/types.h>
#include <sys/wait.h> // for wait function
#include <signal.h> // for sigaction

#include "types.h"
#include "parser.h"

/*====================================================================*/
/*          ****** DO NOT MODIFY ANYTHING FROM THIS LINE ******       */
/**
 * String used as the prompt (see @main()). You may change this to
 * change the prompt */
static char __prompt[MAX_TOKEN_LEN] = "$";

/**
 * Time out value. It's OK to read this value, but ** SHOULD NOT CHANGE
 * IT DIRECTLY **. Instead, use @set_timeout() function below.
 */
static unsigned int __timeout = 2;

static void set_timeout(unsigned int timeout)
{
	__timeout = timeout;

	if (__timeout == 0) {
		fprintf(stderr, "Timeout is disabled\n");
	} else {
		fprintf(stderr, "Timeout is set to %d second%s\n",
				__timeout,
				__timeout >= 2 ? "s" : "");
	}
}
/*          ****** DO NOT MODIFY ANYTHING UP TO THIS LINE ******      */
/*====================================================================*/

/***********************************************************************
 * run_command()
 *
 * DESCRIPTION
 *   Implement the specified shell features here using the parsed
 *   command tokens.
 *
 * RETURN VALUE
 *   Return 1 on successful command execution
 *   Return 0 when user inputs "exit"
 *   Return <0 on error
 */
static char* name;
static void sig_handler(int sig);
static int takenTime;
static pid_t pid; // process id

static int run_command(int nr_tokens, char *tokens[])
{
	/* This function is all yours. Good luck! */

	
	name = (char*)malloc(sizeof(char)*MAX_TOKEN_LEN);
	name = tokens[0];

	if (strncmp(tokens[0], "exit", strlen("exit")) == 0) {
		return 0;
	}

	else if(strncmp(tokens[0], "prompt", strlen("prompt")) == 0) { // command is prompt
		strcpy(__prompt, tokens[1]);
	}

	else if(strncmp(tokens[0], "cd", strlen("cd")) == 0) {
		// cd ~ and cd : go to the home directory of user
		if(strncmp(tokens[1], "~", strlen("~")) == 0 || tokens[1] == NULL)
		// getenv : searches the environment list to find the environment variable name
			chdir(getenv("HOME")); 
		else 
		// chdir : changes the current working directory of the calling process to the path directory
			chdir(tokens[1]); 
	}
	
	else if(strncmp(tokens[0], "timeout", strlen("timeout")) == 0) {
		takenTime = atoi(tokens[1]); // casting from char* to int
		if(tokens[1] == NULL) 
			fprintf(stderr, "Current timeout is %d second", takenTime);
		else
		{
			set_timeout(atoi(tokens[1])); 
			takenTime = atoi(tokens[1]);
		}
		
	}

	else if(strncmp(tokens[0], "for", strlen("for")) == 0) {
		int N_times = 1; 
		int num = 0;

		for(int i=0; i<nr_tokens; i++) {
			if(strncmp(tokens[i], "for", strlen("for")) == 0) {
				N_times *= atoi(tokens[i + 1]); // calculate N-times
				if(strncmp(tokens[i + 2], "for", strlen("for")) != 0) { 
					num = i + 2;	// if the tokens[i+2] is not 'for', save the index
				}
			}
		}

		if(strncmp(tokens[num], "cd", strlen("cd")) == 0) { // use the information of num 
			for(int i=0; i<N_times; i++)  { // 
				if(strncmp(tokens[num + 1], "~", strlen("~")) == 0 || tokens[num + 1] == NULL)
					chdir(getenv("HOME")); 
				else 
					chdir(tokens[num + 1]); 
			}
		}

		else { // tokens[num] 
			for(int i=0; i<N_times; i++) {
				pid = fork();

				if(pid < 0) {
					fprintf(stderr, "fork failed\n");
					exit(1); 
				}                                                                                                                             
				else if(pid == 0) { /* child process */
					
					execvp(tokens[num],&tokens[num]);
					if(tokens[num] != NULL){
						fprintf(stderr, "No such file or directory\n");
					}
					close(pid);
					exit(0); 
				} 
				else /* parent process */ 
					wait(NULL); // wait for process to change state
				}
		}
	}

	else {
			// change the action taken by a process on receipt of a specific signal
			struct sigaction act; 
			act.sa_handler = sig_handler;
			sigaction(SIGALRM, &act, 0);
			alarm(takenTime); // send SIGALRM to me at the takneTime

			pid = fork(); // create a child process 
			
			if(pid < 0) {
				fprintf(stderr, "fork failed\n");
				exit(1); // EXIT_FAILURE
			}                                                                                                                             
			else if(pid == 0) { /* child process */
				// searches for the location of the tokens[0] command 
				// passes arguments to the tokens[0] command in the tokens array
				execvp(tokens[0],tokens);
				if(tokens[0] != NULL){	
					fprintf(stderr, "No such file or directory\n");
				}
				close(pid);
				exit(0); // EXIT_SUCCESS
			} 
			else /* parent process */ 
				wait(NULL); // wait for process to change state
		}

	return 1;
}

static void sig_handler(int sig)
{
	if(sig == SIGALRM)
	{
		fprintf(stderr, "%s is timed out\n", name);
		kill(pid, SIGKILL);
	}
}
/***********************************************************************
 * initialize()
 *
 * DESCRIPTION
 *   Call-back function for your own initialization code. It is OK to
 *   leave blank if you don't need any initialization.
 *
 * RETURN VALUE
 *   Return 0 on successful initialization.
 *   Return other value on error, which leads the program to exit.
 */
static int initialize(int argc, char * const argv[])
{
	return 0;
}


/***********************************************************************
 * finalize()
 *
 * DESCRIPTION
 *   Callback function for finalizing your code. Like @initialize(),
 *   you may leave this function blank.
 */
static void finalize(int argc, char * const argv[])
{

}


/*====================================================================*/
/*          ****** DO NOT MODIFY ANYTHING BELOW THIS LINE ******      */

static bool __verbose = true;
static char *__color_start = "[0;31;40m";
static char *__color_end = "[0m";

/***********************************************************************
 * main() of this program.
 */
int main(int argc, char * const argv[])
{
	char command[MAX_COMMAND_LEN] = { '\0' };
	int ret = 0;
	int opt;

	while ((opt = getopt(argc, argv, "qm")) != -1) {
		switch (opt) {
		case 'q':
			__verbose = false;
			break;
		case 'm':
			__color_start = __color_end = "\0";
			break;
		}
	}

	if ((ret = initialize(argc, argv))) return EXIT_FAILURE;

	if (__verbose)
		fprintf(stderr, "%s%s%s ", __color_start, __prompt, __color_end);

	while (fgets(command, sizeof(command), stdin)) {	
		char *tokens[MAX_NR_TOKENS] = { NULL };
		int nr_tokens = 0;

		if (parse_command(command, &nr_tokens, tokens) == 0)
			goto more; /* You may use nested if-than-else, however .. */

		ret = run_command(nr_tokens, tokens);
		if (ret == 0) {
			break;
		} else if (ret < 0) {
			fprintf(stderr, "Error in run_command: %d\n", ret);
		}

more:
		if (__verbose)
			fprintf(stderr, "%s%s%s ", __color_start, __prompt, __color_end);
	}

	finalize(argc, argv);

	return EXIT_SUCCESS;
}
