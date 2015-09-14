/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: source.cpp - An application that demonstrates how to use pipes for communicating between processes
--
-- PROGRAM: ASN01
--
-- FUNCTIONS:
-- int main (void)
-- void input(int writeToOutput, int writeToTranslate, pid_t pInput, pid_t pOutput, pid_t pTranslate);
-- void output(int readFromInput, int readFromTranslate);
-- void translate(int readFromInput, int writeToOutput);
--
--
-- DATE: January 22, 2014
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Joshua Campbell
--
-- PROGRAMMER: Joshua Campbell
--
-- NOTES:
-- This program will take input in from the user and then print it to the screen. When the user presses the 'E' key, the
-- input is sent to the translation process that handles things such as line kills, backspaces, and translating 'a' to 'z'
-- and 'z' to 'a'. This translated data is then sent to output and the program carries on.
----------------------------------------------------------------------------------------------------------------------*/

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <signal.h>
#include <fcntl.h>
#include <cstring>
#include <cstdio>

#define MSGSIZE 16
#define MAXMSGSIZE 256

using namespace std;

/*----------------Function Prototypes-----------------------------------------------------------------------------*/
void input(int writeToOutput, int writeToTranslate, pid_t pInput, pid_t pOutput, pid_t pTranslate);
void output(int readFromInput, int readFromTranslate);
void translate(int readFromInput, int writeToOutput);

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: main
--
-- DATE: January 21, 2014
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Joshua Campbell
--
-- PROGRAMMER: Joshua Campbell
--
-- INTERFACE: int main(void)
--
-- RETURNS: int.
--
-- NOTES:
-- The main process. Handles creating the pipes, child processes, and assigning tasks to the child processes.
-- Turns off terminal processing.
----------------------------------------------------------------------------------------------------------------------*/
int main (void)
{
	system("stty raw igncr -echo");
  	pid_t tmppid;
	pid_t pid;
	pid_t pinput = 0;
	pid_t poutput = 0;
	pid_t ptranslate = 0;
	int pipeInputToOutput[2];
	int pipeInputToTranslate[2];
	int pipeTranslateToOutput[2];
	
	
	pipe(pipeInputToOutput);
	pipe(pipeInputToTranslate);
	pipe(pipeTranslateToOutput);

	/*---- Set the O_NDELAY flag for p[0] -----------*/
	fcntl (pipeInputToOutput[0], F_SETFL, O_NDELAY);
	fcntl (pipeTranslateToOutput[0], F_SETFL, O_NDELAY);

	/*-------- fork ---------------*/
	tmppid = fork();
	switch(tmppid) {
		case -1:
		break;
		case 0:
			pid = getpid();
			pinput = getppid();
			output(pipeInputToOutput[0], pipeTranslateToOutput[0]);
		break;
		default:
			poutput = tmppid;
			tmppid = fork();
			switch(tmppid) {
				case -1:
				break;
				case 0:
					pid = getpid();
					pinput = getppid();
					translate(pipeInputToTranslate[0], pipeTranslateToOutput[1]);
				break;
				default:
					ptranslate = tmppid;
					pid = getpid();
					pinput = pid;
					input(pipeInputToOutput[1], pipeInputToTranslate[1], pinput, poutput, ptranslate);
				break;
			}
		break;
	}
	system("stty sane");
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: input
--
-- DATE: January 21, 2014
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Joshua Campbell
--
-- PROGRAMMER: Joshua Campbell
--
-- INTERFACE: void input(int writeToOutput, int writeToTranslate, pid_t pInput, pid_t pOutput, pid_t pTranslate)
--							int writeToOutput: the writing portion of the pipe between the Input and Output processes.
--							int writeToTranslate: the writing portion of the pipe between the Input and 
--													Translate processes.
--							pid_t pInput: the process id for the Input process.
--							pid_t pOutput: the process id for the Output process.
--							pid_t pTranslate: the process id for the Translate process.
--
-- RETURNS: void.
--
-- NOTES:
-- The parent process executes this function.
-- This function takes input data from the user and then sends it to the translate process and the output process.
-- Output process receives data after every keystroke.
-- Translate process receives buffered data after the 'E' key is pressed.
----------------------------------------------------------------------------------------------------------------------*/
void input(int writeToOutput, int writeToTranslate, pid_t pInput, pid_t pOutput, pid_t pTranslate) {
	std::string data = "";
	std::string buffer = "";
	bool read = true;
	while (read)
	{
		char ch = fgetc(stdin);
		data += ch;
		write (writeToOutput, data.c_str(), MSGSIZE);
		data = "";
		
		switch(ch) {
			case 11: //ctrl-k
				kill(pTranslate, SIGINT);
				kill(pOutput, SIGINT);
				kill(pInput, SIGINT);
			break;
			case 69: //E (enter)
				if (buffer.length() <= MAXMSGSIZE) {
					write(writeToTranslate, buffer.c_str(), MAXMSGSIZE);
					buffer = "";
				}
			break;
			case 84: //T (normal termination)
				read = false;
				kill(pTranslate, SIGINT);
				kill(pOutput, SIGINT);
			break;
			default: //adding text to line
				buffer += ch;
			break;
		}
	}
	system("stty sane");
	exit(0);
}


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: output
--
-- DATE: January 21, 2014
--
-- REVISIONS: January 23, 2014 - added in /r after a new line to reset spacing to flush left.
--
-- DESIGNER: Joshua Campbell
--
-- PROGRAMMER: Joshua Campbell
--
-- INTERFACE: void output(int readFromInput, int readFromTranslate)
--							int readFromInput: the reading portion of the pipe between the Input and Output processes.
--							int readFromTranslate: the reading portion of the pipe between the Translate and 
--													Output processes.
--
-- RETURNS: void.
--
-- NOTES:
-- One of the child processes executes this function.
-- This function receives data from the input process and translate process via pipes.
-- This function prints the received data to the terminal.
----------------------------------------------------------------------------------------------------------------------*/
void output(int readFromInput, int readFromTranslate) {
	int nread;
	char inbuf1[MAXMSGSIZE];
	char inbuf2[MAXMSGSIZE];

	for (;;)
	{
		switch (nread = read(readFromInput, inbuf1, MAXMSGSIZE))
		{
			case -1:
			case 0:
			//printf ("(pipe empty)\n");
			break;
			default:
				printf("%c", inbuf1[0]);
				fflush(stdout);
		}
		switch (nread = read(readFromTranslate, inbuf2, MAXMSGSIZE))
		{
			case -1:
			case 0:
			//printf ("(pipe empty)\n");
			break;
			default:
				printf("\n%s\n", inbuf2);
				fflush(stdout);
		}
	}
}


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: translate
--
-- DATE: January 21, 2014
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Joshua Campbell
--
-- PROGRAMMER: Joshua Campbell
--
-- INTERFACE: void translate(int readFromInput, int writeToOutput)
--							int readFromInput: the reading portion of the pipe between the Input and Translate processes.
--							int writeToOutput: the writing portion of the pipe between the Translate and Output processes.
--
-- RETURNS: void.
--
-- NOTES:
-- One of the child processes executes this function.
-- This function receives data from the input process via a pipe.
-- This function processes said data (including line kills, backspaces, and translating characters).
-- The translated data is then sent to the output process via a pipe.
----------------------------------------------------------------------------------------------------------------------*/
void translate(int readFromInput, int writeToOutput) {
	do {
		size_t i = 0;
		size_t size = 0;
		char inbuf[MAXMSGSIZE];
		std::string buffer = "";
		read(readFromInput, inbuf, MAXMSGSIZE);
		size = strlen(inbuf);
		for (i = 0; i < size; i++) {
			switch(inbuf[i]) {
				case 'a':
					buffer += 'z';
				break;
				case 'z':
					buffer += 'a';
				break;
				case 'X':
					buffer.erase(buffer.length() - 1, 1);
				break;
				case 'K':
					buffer.erase(0, buffer.length());
				break;
				default:
					buffer += inbuf[i];
				break;
			}
		}
		write(writeToOutput, buffer.c_str(), MAXMSGSIZE);
	} while(true);
}
