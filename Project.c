/*
    Simple Shell 
    Author: Shuvradeb Barman Srijon
*/

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include 	<dirent.h>
#include 	<sys/types.h>
#include 	<sys/stat.h>


#define MAX_ARGS		64
#define MAX_ARG_LEN		16
#define	MAX_LINE_LEN	80
#define WHITESPACE		" .,\t\n"
#define PATHDELIM		":"
#define NAMELEN			256
#define MAX_CMD_COUNT	10000
#define MAX_CMD_LEN		256
#define PROMPT_TEXT		"Simple_shell"

//Structure that saves command name and complete path 
struct cmdMap
{
	char name[MAX_LINE_LEN];
	char path[MAX_CMD_LEN];
};

//Structure that saves command name and arguments
struct command_t 
{
	char *name;
	int argc;
	char *argv[MAX_ARGS];
};

char *pathDirs[NAMELEN];
int cmdCount, pathCount;
struct cmdMap commandList[MAX_CMD_COUNT];

/**
 * @brief This function parses the complete command with arguments.
 * @param cLine the input command text.
 * @param cmd the output command structure.
 * @param cmdIdx the output command index in the command map.
 * @return the number of arguments, negative if parsing failed.
 */
int parseCommand(char *cLine, struct command_t *cmd, int *cmdIdx);

/**
 * @brief This function loads all accessible commands from all paths defined.
 */
void loadCommands();

/**
 * @brief This function validates the command whether its available for the user or not.
 * @param command_name the input command name.
 * @return command index if a valid executable command, -1 if a terminating command, -2 if invalid.
 */
int validateCommand(char *command_name);

/**
 * @brief This function trims all white space and tabs from a string.
 * @param str the input string.
 * @return length of trimmed string.
 */
int trimString(char *str);

int main(int argc, char *argv[]) {
	int i;
	int cpid, numChildren;
	int status, cmdIndex;
	char cmdLine[MAX_LINE_LEN];
	struct command_t command;
	cmdCount = 0;
	
	/* Read PATH variable */
	const char* pa = getenv("PATH");
	char *c = (char*)pa, *result = NULL;
	pathCount = 0;
    
    //Parse all the path directories and store them into pathDirs
	result = strtok(c, PATHDELIM);  
	while( result != NULL )
	{
		int path_len = strlen(result) + 1;
		pathDirs[pathCount] = (char *) malloc(sizeof(char) * path_len);
		snprintf(pathDirs[pathCount], path_len, "%s", result);		
		pathCount++;
		result = strtok(NULL, PATHDELIM);   
	}
    
    //Search all the pathDirs for available commands for the user
	loadCommands();
	
	numChildren  = 0;
	
    /* Launch Shell prompt */
	while (1)
	{
        //Display a prompt
        if(argc > 1 && *argv[1])
            printf("\n%s%% ", argv[1]);
        else
            printf("\n%s%% ", PROMPT_TEXT);
        
        //Read the user command 
		fgets(cmdLine, MAX_LINE_LEN, stdin);
		
        char *c = cmdLine;
		while(*c == ' ' || *c == '\t') c++;
        
        //Parse it 
		if(parseCommand(c, &command, &cmdIndex) < 1)
		{
			continue;
		}
		command.argv[command.argc + 1] = NULL;
		if(cmdIndex == -1) //Terminating command issued by user
			break;

        /* Create a child process to execute the command */
		if((cpid = fork()) == 0) 
		{
			execvp(commandList[cmdIndex].path, command.argv);
		}
		else
		{
			wait(cpid);
		}
        
		//Parent continuing to the command executed
		numChildren++;
	}
	printf("Executed %d commands\n", numChildren);
	printf("Terminating successfully\n");
	return 0;
}

void loadCommands()
{
	int i;
	struct dirent *de = NULL;
	for(i = 0; i < pathCount; i++)
	{
		DIR *dr = opendir(pathDirs[i]); 
		if (dr == NULL)
		{ 
			continue; 
		} 
		while ((de = readdir(dr)) != NULL) 
		{
			//use stat here to determine the execution mode of a file
			if((de->d_type == DT_REG || de->d_type == DT_LNK))
			{
				struct stat sb;
				snprintf(commandList[cmdCount].name, MAX_LINE_LEN, "%s", de->d_name);
				snprintf(commandList[cmdCount].path, MAX_CMD_LEN, "%s/%s", pathDirs[i], de->d_name);
				if (stat(commandList[cmdCount].path, &sb) == 0 && sb.st_mode & S_IXUSR)
				{
					cmdCount++;
				}
				else
				{
					commandList[cmdCount].name[0] = '\0';
					commandList[cmdCount].path[0] = '\0';
				}
			}			
		}
		closedir(dr);
	}
}

int validateCommand(char *command_name)
{
	int i=0;
	if(0 == strcmp(command_name, "exit") || 0 == strcmp(command_name, "quit"))
		return -1;
	while(i < cmdCount)
	{
		if(0 == strcmp(command_name, commandList[i].name))
		{
			return i;
		}
		i++;
	}
	return -2;
}

int trimString(char *str)
{ 
    int i, count = 0;
    for(i = 0; str[i]; i++)
    {
		if(str[i] != ' ' && str[i] != '\t' && str[i] != '|') 
        {
			str[count++] = str[i];
		}
	}
    str[count] = '\0';
	return count;
}

int parseCommand(char *cLine, struct command_t *cmd, int *cmdIdx)
{
    int argc;
    char **clPtr;
    clPtr = &cLine;
    argc = 0;
    cmd->argv[argc] = (char *) malloc(MAX_ARG_LEN);
	if(cmd->argv[argc])
		memset(cmd->argv[argc], 0, MAX_ARG_LEN);
	char *res = strsep(clPtr, WHITESPACE);
    while(res != NULL) 
	{
		char *s = strdup(res);
		if(trimString(s) > 0)
        {
			cmd->argv[argc] = s;
			cmd->argv[++argc] = (char *) malloc(MAX_ARG_LEN);
		}
		res = strsep(clPtr, WHITESPACE);
    }
	if(strcmp(cmd->argv[0],"") == 0)
		return -1;
	*cmdIdx = validateCommand(cmd->argv[0]);
	if(*cmdIdx < -1 || *cmdIdx >= cmdCount)
	{
		printf("command not exists!!\n");
		int i;
		for(i = 0; i < argc; i++)
		{
			if(cmd->argv[i])
				free(cmd->argv[i]);
			cmd->argv[i] = '\0';
		}
		return -1;
	}

    cmd->argc = argc-1;
    cmd->name = (char *) malloc(sizeof(cmd->argv[0]));
    strcpy(cmd->name, cmd->argv[0]);
	return argc;
}