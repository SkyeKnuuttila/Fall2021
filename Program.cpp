/*
FILE:	Program.cpp
AUTHOR:	S. Knuuttila
DATE:	09/2021
	
	Main code for Project 1 of CSC35500. Allows the user to enter a command and executes that command.
*/

#include <iostream>
#include <unistd.h>
#include <csignal>
#include <cstring>
#include <sys/wait.h>
#include <cstdio>

#include "Command.hpp"

using namespace std;

int savePID;
Command saveCommand;

void myHandler(int sigNum) {
	if(saveCommand.backgrounded()) { //only have this run if the command is backgrounded
		//print the PID of the backgrounded command and the command itself
		cout << "Completed: PID=" << savePID << " : ";
		for(int i = 0; i < saveCommand.numArgs(); i++) {
			cout << saveCommand.args()[i] << " ";
		}
		cout << "&" << endl;
	}
}

int main(int argc, char *argv[]) {
	//create some variables
	Command command;
	bool pipeFound = false;
	int pipeSave;
	
	//prompt the user to enter a command and read in the command
	cout << ">>>>";
	command.read();
	
	//if the command "exit" is ever entered, the program should stop running
	while(command.name() != "exit") {
		
		//if the command entered is "cd"
		if(command.name() == "cd") {
			//get the intended directory from the command
			string directory = command.args()[1];
			char* charDir[1];
			charDir[0] = new char[100];
			strcpy(charDir[0], directory.c_str());
			chdir(charDir[0]);
		}
		
		//if the entered command is not "cd" or "exit"
		else {
			//create a pipe before the fork
			int pipeInfo[2];
			int myPipe = pipe(pipeInfo);
			
			//fork
			int pid = fork();
			
			if(pid != 0) { //this is the parent, keep running the program
				//if pipeFound was set to true in the last run, we need to set it to false
				if(pipeFound) 
					pipeFound = false;
				//if there was a pipe in the command, we need to save the information
				if(command.pipeOut()) {
					close(pipeInfo[1]); //close the ability to write to the pipe
					pipeSave = pipeInfo[0];
					pipeFound = true; //set the bool to true
				}
				//if there is no & in the command, wait until child is done
				if(!command.backgrounded())
					waitpid(pid, NULL, 0);
				//if there is a & in the command, continue and run a signal
				else if(command.backgrounded()) {
					savePID = pid;
					saveCommand = command;
					signal(SIGCHLD, myHandler);
				}
			}
			
			else { //this is the child, run the execvp()
				
				//if the previous command had a pipe
				if(pipeFound) {
					dup2(pipeSave, fileno(stdin));
				}
			
				//if the command has a pipe in it
				if(command.pipeOut()) {
					close(pipeInfo[0]); //close the ability to read from the pipe
					dup2(pipeInfo[1], fileno(stdout));
				}
			
				//if the command has a file to redirect into
				if(command.redirIn()) {
					const char *inFile = command.inputRedirectFile().c_str();
					FILE *fp = fopen(inFile, "r"); //open file to read
					dup2(fileno(fp), fileno(stdin));
				}
			
				//if the command has a file to redirect out to
				if(command.redirOut()) {
					const char *inFile = command.outputRedirectFile().c_str();
					FILE *fp = fopen(inFile, "w"); //open file to write
					dup2(fileno(fp), fileno(stdout));
				}
				
				//convert the array of strings into arrays of char
				int length = command.numArgs();
				char* array[length];
				for(int i = 0; i < length; i++) {
					array[i] = new char[30];
					strcpy(array[i], command.args()[i].c_str());
				}
				//make the last element in the array NULL
				array[length] = NULL;
			
				//execvp() call
				execvp(array[0], array);
			}
		}
		//if a pipe was found, don't print the prompt
		if(!pipeFound)
			cout << ">>>>"; //prompt the user to enter a command
		command.read(); //read in the command
		saveCommand = command; //save the command in the variable so the signal doesn't run on non-backgrounded commands
	}
	//once the program has ended, print a message
	cout << "Program ended." << endl;
	
	return 0;
	
}
