#include "libc.h"

void notFound(char* cmd) {
    puts(cmd);
    puts(": command not found\n");
}

int main() {
    //int firstRun = 1;
    while (1) {

        puts("shell> ");
        char* in = gets();
        //char* in = firstRun ? "cat panic" : gets();
        //firstRun = 0;
        int ind = 0;
        int argc = 0;
        char** args;

        while(in[ind] != 0 && (in[ind] == ' ' || in[ind] == '\r'))
        {
        	++in;
        }

        int runInBackground = 0;
        if(in[0] == '&')
        {
        	runInBackground = 1;
        	++in;
        }

        while(in[ind] != 0)
        {
        	while(in[ind] != 0 && in[ind] != ' ')
			{
				++ind;
			}

        	while(in[ind] == ' ')
        	{
            	++ind;
        	}

        	++argc;
        }


        if(argc == 0)
        {
        	continue;
        }

        args = malloc(4 * (argc + 1));

        int ind2 = 0;
        for(int a = 0; a < argc && in[ind2] != 0; ++a)
        {
        	int argumentLength = 0;
        	while(in[ind2] != ' ' && in[ind2] != 0)
        	{
        		++argumentLength;
        		++ind2;
        	}

        	args[a] = malloc(argumentLength + 1);
        	memcpy(args[a], in + ind2 - argumentLength, argumentLength);
        	args[a][argumentLength] = 0;

        	while(in[ind2] == ' ' && in[ind2] != 0)
        	{
        		++ind2;
        	}
        }

        args[argc] = 0;

        const long id = fork();

        if(id != 0)
        {
        	if(!runInBackground)
        	{
        		join(id);
        	}
        	else
        	{
        		puts("Running new process in background.\n");
        	}
        }
        else
        {
            const long err = execv(args[0], args);
            if (err == -1006) {
                puts(args[0]);
                puts(": access denied\n");
            }
        	else if(err < 0) {
                puts(args[0]);
                puts(": command not found\n");
        	}

            if (in) free(in);
            for(int a = 0; a < argc; ++a)
            {
            	free(args[a]);
            }

            free(args);
        	exit(5);
        }

        if (in) free(in);
    }
    return 0;
}
