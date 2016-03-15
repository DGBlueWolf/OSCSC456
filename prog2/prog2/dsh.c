#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

//Make some twidly fucntions more clear/ readable
#define dref(x)     (*(x))
#define is_even(x)  (~(x))&1
#define is_odd(x)   (x)&1
#define is_set(x,i) ( (x) & (i) )
#define setb(x,i)   (x) |= (i) 

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: dsh_prompt
//
// DESCRIPTION:
//
// This function recieves command line input for the shell.
// The storage is dynamically allocated for the input stream in
// blocks of 256 bytes
//
// INPUT:  none
//
// OUTPUT:
//
////////////////////////////////////////////////////////////////////////////////

int dsh_prompt( char** input )
{
  int  buffer_size = 256;
  int  num_read = 0;
  char *curr = NULL;
  char *line = NULL;
  line = (char*) calloc( buffer_size , sizeof(char) );
  if( line == NULL )
    return -1;

  //Display prompt
  printf( "dsh> " );

  //Read first 256 characters or until newline reached
  scanf( "%256[^\n]%n" , line , &num_read );

  if( num_read == 0 )
  {
    free(line);
    return 1;
  }

  //While 256 characters are read, allocate 256 more bytes and keep reading
  while( num_read == 256 )
  {
    buffer_size += 256;

    //Reallocate space
    line = realloc( line , buffer_size );
    if( line == NULL )
      return -1;

    //Set pointer to end of last read
    curr = line + buffer_size - 256;
    scanf( "%256[^\n]%n" , curr  , &num_read );
  }

  //CHANGEME!
  //This will temporarily set return to 1 if user enters exit
  //Needs to be moved to function that handles commands

  if( !strncmp( line , "exit" , 4 ) )
  {
    free( line );
    return 2;
  }

  //Return input by reference
  dref(input) = line;

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: parse_input
//
// DESCRIPTION:
//
// This function parses input from the commandline.
//
// INPUT:
//
//   char * input: The input string returned by prompt
//
// OUTPUT:
//
//   int      argc:  The number of arguments in the input string.
//   char *** argv:  A pointer to the argument list passed by reference
//
////////////////////////////////////////////////////////////////////////////////

int parse_input( int * argc , char* input, char*** argv )
{
  dref(argc) = 0; //Count of arguments
  int count = 0; //Counter to store arguments
  int done = 0;
  int before = 0;
  int found = 0;
  int quotes = 0;
  int status = 0;
  
  char** args = NULL;
  char* argin = input;
  char* iptin = input;

  //Tokenize and count number of arguments in input
  while( !done )
  {
    before = found;
    switch( dref(iptin) )
    {
      case 0:
        done = 1;
        break;
        
      case '\"':
        quotes++;
        found = 0;
        break;
        
      case ' ':
        if( is_even(quotes) )
        {
          found = 0;
          break;
        }
        
      case '\t':
      default:
        found = 1;
        if( !before  )
          dref(argc)++;
    }
    if( !done && !found )
      dref(iptin) = 0;
    iptin++;
  }

  //Check if there are an odd number of ""
  if( is_odd(quotes) )
  {
    printf( "Parse error: Unable to match \"\" delimiters.\n" );
    return -1;
  }

  //Allocate Exactly Enough Memory
  if( dref(argc) > 0 )
    args = (char**) malloc( (dref(argc)+1)*sizeof(char*) );
  else
    return 0;

  //Store Arguments
  found = 0;
  argin = input;
  while( argin < iptin )
  {
    if( dref(argin) != 0 )
    {
      if( !found )
      {
        //printf( "Argument %d: %s\n" , count , argin );
        args[count++] = argin;
      }
      found = 1;
    }
    else
      found = 0;
    argin++;
  }
  args[dref(argc)] = NULL;

  //For testing purposes
  if( dref(argc) > 0 && argv != NULL )
  {
    dref(argv) = args;
  }
  else
    free(args);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: run_command
//
// DESCRIPTION:
//
// This function takes the argument list from Main and directs it to either the
// fork/exec code for single functions or to the instrinsic commands.
//
// The first argument is expected to be the command_name. If the command is
// signal then the order of the remaining arguments doesn't matter.
//
// The intrinsic commands are:
//   cmdnm
//   signal
//   systat
//   exit
//   cd
//   pwd
//
// INPUT:
//
// OUTPUT:
//
////////////////////////////////////////////////////////////////////////////////

int run_command( int args, char** arg_list )
{
  int i = 0;
  char intrinsic[7][20] = {"cmdnm", "signal", "systat", "exit", "cd", "pwd",
                           "hb"};
  int res = 0;
  //Check if command is intrinsic
  for( i = 0 ; i < 7 ; i++ )
  {
    res = strcmp( arg_list[0] , intrinsic[i] );
    if( !res )
      return Run( i , args , arg_list );
  }

  //Try to do a fork and exec
  return New_Process( arg_list );
}

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: main
//
// DESCRIPTION:
//
// This function implements the main event loop for the shell. It waits for
// the exit command to terminate.
//
////////////////////////////////////////////////////////////////////////////////

int main()
{
  char* input = NULL;
  char** arg_list = NULL;
  int status = 0;
  int args = 0;
  int mode = 0;
  char * spec_res = NULL;
  do
  {
    args = 0;
    input = NULL;
    arg_list = NULL;
    status = dsh_prompt( &input );
    scanf( "%*1c" );
    
    if( !status )
    {
      status = parse_input( &args, input, &arg_list );
      mode = parse_special( arg_list , &spec_res );
      fflush(stdout);
      if( mode == -1 )
        fprintf( stderr , "Parse error: Verify command and order of operators\n" );
      else if( mode == 0 )
        status = run_command( args , arg_list ); 
      else
        status = run_special( spec_res , mode );   
    }
    
    if( input != NULL )
      free( input );
    if( arg_list != NULL )
      free( arg_list );
  }while( status < 2 );

  return 0;
}
