#include <string>
#include <cstring>
#include <vector>
#include <sstream>
#include <cstdio>
#include <iostream>

#ifdef WIN32
#	include <fcntl.h>
#	include <windows.h>
#	define STDERR_FILENO 2
#	define STDIN_FILENO 0
#	define STDOUT_FILENO 1
#else
#	include <unistd.h>
#	include <sys/wait.h>
#endif

// Class to handle building command lines for both execvp and CreateProcess
// the main issue is that CreateProcess requires paths with spaces to be quoted, while execvp doesn't understand that syntax.
// https://gist.github.com/multiplemonomials/1d1806062a3809ffe26f7a232757ecb6
class CommandLine
{
	std::string _program;

	std::vector<std::string> _arguments;

	std::string quoteIfNecessary(std::string toQuote)
	{
		if(toQuote.find(" ") != std::string::npos)
		{
			toQuote = '\"' + toQuote + '\"';
		}

		return toQuote;
	}

	public:

	//construct with full path to or name of program to execute
	CommandLine(std::string program):
	_program(program),
	_arguments()
	{

	}

	// adds an argument.  This is NOT a simple string concatenation; the argument should be one element of the target program's argv array.
	// Spaces will be quoted automatically if necessary.
	CommandLine& arg(std::string arg)
	{
		_arguments.push_back(arg);

		return *this;
	}


	// Get a command line with the program and its arguments, like you'd type into a shell or pass to CreateProcess()
	// Arguments with spaces will be double quoted.
	std::string getCommandlineString()
	{
		std::stringstream cmdline;

		cmdline << quoteIfNecessary(_program);

		for(std::vector<std::string>::iterator arg = _arguments.begin(); arg != _arguments.end(); ++arg)
		{
			cmdline << " " << quoteIfNecessary(*arg);
		}

		return cmdline.str();
	}

	// Execute the command and arguments, setting its standard streams to the three provided if they are not 0.
	// Blocks until it finishes.
	// If the command is not an absolute path, it will be searched for in the system PATH.
	// Returns the exit code of the process, or -1 if the process could not be started.

	int executeAndWait(int sin, int sout, int serr)
	{

	#ifdef WIN32
		STARTUPINFO startInfo;

		ZeroMemory(&startInfo, sizeof(startInfo));
		startInfo.cb = sizeof(startInfo);
		startInfo.dwFlags = STARTF_USESTDHANDLES;

		// convert file descriptors to win32 handles
		if(sin != 0)
		{
			startInfo.hStdInput = (HANDLE)_get_osfhandle(sin);
		}
		if(sout != 0)
		{
			startInfo.hStdOutput = (HANDLE)_get_osfhandle(sout);
		}
		if(serr != 0)
		{
			startInfo.hStdError = (HANDLE)_get_osfhandle(serr);
		}

		PROCESS_INFORMATION procInfo;
		if(CreateProcessA(NULL, const_cast<char*>(getCommandlineString().c_str()), NULL, NULL, true, 0, NULL, NULL, &startInfo, &procInfo) == 0)
		{
			int lasterror = GetLastError();

			LPTSTR strErrorMessage = NULL;

			// the next line was taken from GitHub
			FormatMessage(
				FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
				NULL,
				lasterror,
				0,
				(LPTSTR)(&strErrorMessage),
				0,
				NULL);

			std::cerr << "CreateProcess(" << getCommandlineString() << ") failed with error " << std::dec << lasterror << ": " << strErrorMessage << std::endl;
			return -1;
		}

		// Wait until child process exits.
		WaitForSingleObject(procInfo.hProcess, INFINITE);

		DWORD exitCode;
		GetExitCodeProcess(procInfo.hProcess, &exitCode);

		// Close process and thread handles.
		CloseHandle(procInfo.hProcess);
		CloseHandle(procInfo.hThread);

		return exitCode;
    #else
	/*                   from David A Curry,
						 "Using C on the UNIX System" p. 105-106    */

		//create array of C strings
		std::vector<char*> execvpArguments;

		char* program_c_string = new char[_program.size() + 1];
		strcpy(program_c_string, _program.c_str());
		execvpArguments.push_back(program_c_string);

		for(std::vector<std::string>::iterator arg = _arguments.begin(); arg != _arguments.end(); ++arg)
		{
			char* c_string = new char[(*arg).size() + 1];
			strcpy(c_string, arg->c_str());
			execvpArguments.push_back(c_string);
		}

		//null-terminate the argument array
		execvpArguments.push_back(NULL);

		//we use pipes as the delimiter between logical arguments
//		std::cerr << "Invoking command: ";
//		for(char* arg : execvpArguments)
//		{
//			std::cerr << '|' << arg;
//		}
//		std::cerr << std::endl;

		int status;
		pid_t pid;

		if ((pid = fork()) < 0) {
			perror("fork");
			return -1;
		}

		if(pid == 0) {
			if( sin != 0 ) {
				close( 0 );  dup( sin );
			}
			if( sout != 1 ) {
				close( 1 );  dup( sout );
			}
			if( serr != 2 ) {
				close( 2 );  dup( serr );
			}

			execvp(_program.c_str(), &execvpArguments[0]);
			perror(("Error executing " + _program).c_str());
			exit(1);
		}

		//free arg strings
		for(std::vector<char*>::iterator arg = execvpArguments.begin(); arg != execvpArguments.end(); ++arg)
		{
			delete[] *arg;
		}

		while( wait(&status) != pid ) ;
		return( status );
	#endif
	}
};