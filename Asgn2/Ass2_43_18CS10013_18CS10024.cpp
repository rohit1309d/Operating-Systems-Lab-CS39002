#include<bits/stdc++.h>
#include<unistd.h>
#include<fcntl.h> 
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdio>
using namespace std;
#define MAXLEN 1000

int main(int argc, char const *argv[])
{
	cout << "Welcome to virtual shell" << endl;
	char cmd[MAXLEN] = {'\0'};

	bool flag;

	int status = 0;
	pid_t wpid;

	while(1){  //loop runs untill "exit" command is entered

		printf(">>> ");
		cin.getline(cmd,sizeof(cmd));  //scanning string to get the entire line

		if (strcmp("exit",cmd) == 0)   // if entered command is "exit" then loop is breaked
		{
			cout << "Exiting the shell " << endl;
			break;
		}

		if (strlen(cmd) == 0)
		{
			continue;
		}

		flag = 0;    //to check whether command has '&' at the end
		bool back_flag = 0;

		for (int i = 0; i < strlen(cmd); ++i)
		{
			if(cmd[i] == '&')
			{
				int j = i + 1;
				while(cmd[j] == ' '){
					j++;
				}
				if (cmd[j] == '\0')
				{
					flag = 1;
					cmd[i] = '\0';
				}
				else{
					printf("Invalid Command\n");  // if '&'is not at the end then it is an invalid command
					back_flag = 1;
					break;
				}

			}
		}

		if (back_flag)
		{
			continue;
		}
		int stat_pipe;
        if((stat_pipe=fork()) == 0) //forking the child and executing the command
        {

        	char* piped[MAXLEN]; 
        	char* tok;
        	tok = strtok(cmd,"|");
        	int r=0;
        	while(tok!=NULL)
        	{
                 piped[r]=tok;   //storing all strings separated by '|'
                 r++;
                 tok= strtok(NULL,"|");
        	}

        	int pip[r-1][2]; //r-1 pipes for r commands

        	for(int j = 0; j < r-1; j++)
				if(pipe(pip[j]) < 0)
				{
					printf("Pipe Creation has failed\n");
					exit(0);
				}

            for(int k=0;k<r;k++)
            {
            
				if ((status=fork()) == 0)
				{
					//duplicate STDIN
					if(k>0){
						close(pip[k-1][1]);
						dup2(pip[k-1][0], 0);
						close(pip[k-1][0]);
					}
					// Duplicate STDOUT 
					if(k != r-1){
						close(pip[k][0]);
						dup2(pip[k][1], 1);
						close(pip[k][1]);
					}

					char* argmt[MAXLEN];
					int i = 0;
					char* token = strtok(piped[k]," ");

					char *infile, *outfile;
					bool in = 0, out = 0;

					while(token){
						// if there is a '<' denotes the next entered word in input file
						if (strcmp(token, "<") == 0)
						{
							in = 1;
							token = strtok(NULL, " ");
							infile = token;
						}
						else if (strcmp(token, ">") == 0)// if there is a '>' denotes the next entered word in output file
						{
							out = 1;
							token = strtok(NULL, " ");
							outfile = token;
						}
						else{
							argmt[i] = token;
							i++;
							token = strtok(NULL, " ");
						}			
					}
					

					if (in)
					{
						int fin = open(infile, O_RDONLY);
						if (fin < 0)
						{
							printf("File error\n");
							exit(0);
						}

						dup2(fin,0);//duplicate file with stdin
						close(fin);
					}

					if (out)
					{
						int fout = open(outfile, O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU);
						if (fout < 0)
						{
							printf("File error\n");
							exit(0);
						}

						dup2(fout,1);//duplicate file with stdout
						close(fout);
					}


					execvp(argmt[0], argmt);//executing command
					printf("Invalid command\n"); 
					exit(0);
				}
				else{
					
					for(int i=0; i<k; i++){  //close the used pipes
						close(pip[i][0]);
						close(pip[i][1]);
					}
				}				
			}
			while((wpid = wait(&status)) > 0);
				exit(0);
		}
		else{
					
			if(!flag){ // if there is no '&' at the end then wait for all processes to complete
				while((wpid = wait(&stat_pipe)) > 0);
			}

			usleep(10000);
		}
		
	}

	return 0;
}