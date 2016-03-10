#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

#define CMDNM  0
#define SIGNAL 1
#define SYSTAT 2
#define EXIT   3
#define CD     4
#define PWD    5

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: cmdnm
//
// DESCRIPTION:
//
// This function gets the command the started a process by accessing
// /proc/<pid>/comm.
//
////////////////////////////////////////////////////////////////////////////////

int cmdnm( char* pid )
{
  FILE* fin = NULL;
  char name[1024] = "";
  char path[256] = "/proc/";
  strncat( path , pid , 10 );
  strncat( path , "/comm" , 5 );

  fin = fopen( path , "r" );
  if( fin == NULL )
  {
    printf( "Error: Didn't find process %s\n" , pid );
    return -1;
  }

  fgets( name , 1024 , fin );
  printf( "Process stated by: \'%s\'" , name);

  fclose(fin);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: send_signal
//
// DESCRIPTION:
//
// This function sends a signal to a process using the kill command. It checks
// if the arguments are in the proper ranges, switching them if not.
//
////////////////////////////////////////////////////////////////////////////////

int send_signal( char* arg1 , char* arg2 )
{
  int pid = atoi( arg2 );
  int sig = atoi( arg1 );
  int tmp = 0;
  if( sig > 32 )
  {
    tmp = pid;
    pid = sig;
    sig = tmp;
  }
  if ( kill(pid,sig) )
    fprintf( stderr , "Failed to send signal \'%d\' to \'%d\' \n" , sig , pid );
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: systat
//
// DESCRIPTION:
//
// This function gets some information about the system and displays it for the
// user in stdout. The specific information it provides is as follows:
//  -Linux version and system uptime
//  -Memory Usage: memtotal and memfree
//  -CPU Information: vendor id through cache size
//
////////////////////////////////////////////////////////////////////////////////

int systat()
{
  int  fails = 0;
  char line[1024];
  double running,idle;
  FILE * fin = fopen( "/proc/version" , "r" );
  if( fin == NULL )
  {
    fails--;
    fprintf( stderr , "Error: Couldn't open file /proc/version.\n" );
  }
  else
  {
    fgets( line , 1024 , fin );
    printf( "\n%s" , line );
    fclose( fin );
    clearerr( fin );
  }

  fin = fopen( "/proc/uptime" , "r" );
  if( fin == NULL )
  {
    fails--;
    fprintf( stderr , "Error: Couldn't open file /proc/uptime.\n" );
  }
  else
  {
    fscanf( fin , "%lg%lg" , &running , &idle );
    printf( "\nSystem has been up for %f seconds, and idle for %f seconds.\n"
            , running , idle );
    fclose( fin );
    clearerr( fin );
  }

  fin = fopen( "/proc/meminfo" , "r" );
  if( fin == NULL )
  {
    fails--;
    fprintf( stderr , "Error: Couldn't open file /proc/meminfo.\n" );
  }
  else
  {
    printf("\nMemory Usage:\n");
    fgets( line , 1024 , fin );
    printf( "%s" , line );
    fgets( line , 1024 , fin );
    printf( "%s" , line );
    fclose( fin );
    clearerr( fin );
  }

  fin = fopen( "/proc/cpuinfo" , "r" );
  if( fin == NULL )
  {
    fails--;
    fprintf( stderr , "Error: Couldn't open file /proc/cpuinfo.\n" );
  }
  else
  {
    printf("\nCPU Information:\n");
    fgets( line , 1024 , fin );
    int i;
    for( i = 0 ; i < 8 ; i++ )
    {
      fgets( line , 1024 , fin );
      printf( "%s" , line );
    }
    fclose( fin );
    clearerr( fin );
  }

  return fails;
}

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: cd
//
// DESCRIPTION:
//
// This function implements the change directory intrinsic command.
//
////////////////////////////////////////////////////////////////////////////////

int cd( char *path )
{
  struct stat path_info;
  stat(path , &path_info);
  if( S_ISREG(path_info.st_mode) )
  {
    fprintf( stderr, "dsh: cd: %s: Not a directory.\n" , path );
    return -2;
  }
  if( chdir( path ) )
  {
    fprintf( stderr, "dsh: cd: %s: No such directory.\n" , path );
    return -1;
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: pwd
//
// DESCRIPTION:
//
// This function implements the print working directory intrinsic command.
//
////////////////////////////////////////////////////////////////////////////////

int pwd()
{
  char path[1024] = "";
  getcwd( path , 1024 );
  puts(path);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: Run
//
// DESCRIPTION:
//
// This function directs the program to run the intrinsic commands.
//
////////////////////////////////////////////////////////////////////////////////

int Run( int cmd_num , int args, char** arg_list )
{
  switch( cmd_num )
  {
    case CMDNM:
      if( args != 2 )
      {
        fprintf(stderr, "cmdnm requires 1 argument: <process_id>\n" );
        return -1;
      }
      return cmdnm( arg_list[1] );
    case SIGNAL:
      if( args != 3 )
      {
        fprintf( stderr,
                "signal requires 2 arguments: <signal_num> <process_id>\n" );
        return -1;
      }
      return send_signal( arg_list[1] , arg_list[2] );
    case SYSTAT:
      return systat();
    case EXIT:
      return 2;
    case CD:
      if( args != 2 )
      {
        fprintf(stderr,
               "cd requires 1 argument: <relative path> or <absolute_path>\n" );
        return -1;
      }
      return cd( arg_list[1] );
    case PWD:
      return pwd();
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: New_Process
//
// DESCRIPTION:
//
// Creates a new process to run command given on the command line in the
// diagnostic shell.
//
////////////////////////////////////////////////////////////////////////////////

int New_Process( char** arg_list )
{
  pid_t child_pid;
  int status;

  //Duplicate current process to make child process
  child_pid = fork();
  if( child_pid == 0 )
  {
    //If child, try execute
    execvp( arg_list[0] , arg_list );
    //Exit if failed
    exit(-1);
  }
  else
  {
    //Struct that holds usage information
    struct rusage runtime_data;

    //Wait for child process to terminate and get usage information
    wait3( &status , 0 , &runtime_data );

    //If
    if( status == 65280 )
    {
      fprintf(stderr, "dsh: %s: command not found\n" , arg_list[0] );
      return -1;
    }

    printf( "Child_Pid: %d.\n" , (int)child_pid );
    printf( "User time: %d seconds, %d microseconds.\n" ,
      (int) runtime_data.ru_utime.tv_sec , (int) runtime_data.ru_utime.tv_usec);
    printf( "System time: %d second, %d microseconds.\n" ,
      (int) runtime_data.ru_stime.tv_sec , (int) runtime_data.ru_stime.tv_usec);
    printf( "Number of page faults: %d (soft) , %d (hard)\n" ,
      (int) runtime_data.ru_minflt , (int) runtime_data.ru_majflt );
    printf( "Number of page swaps: %d\n" , (int) runtime_data.ru_nswap );
  }
  return 0;
}
