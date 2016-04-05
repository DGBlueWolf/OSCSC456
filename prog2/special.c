#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

//Make some twidly fucntions more clear/ readable
#define dref(x)     (*(x))
#define is_even(x)  (~(x))&1
#define is_odd(x)   (x)&1
#define is_set(x,i) ( (x) & (i) )
#define setb(x,i)   (x) |= (i)

enum parse_modes
{
  REDIR_STDIN = 1,
  REDIR_STDOUT_W = 2,
  REDIR_STDOUT_A = 4,
  PIPE_BUILDER = 8,
  LOCAL_HOST = 16,
  REMOTE_HOST = 32
};

struct redir_stdin
{
  char * path;
};

struct redir_stdout
{
  char * path;
};

struct pipe_builder
{
  int n;
  char *** arg_lists;
};

struct local_host
{
  char * port_no;
};

struct remote_host
{
  char * ipv6;
  char * port_no;
};

struct parse_set
{
  struct redir_stdin * rdin;
  struct redir_stdout * rdout;
  struct pipe_builder * pbargs;
  struct local_host * lhost;
  struct remote_host * rhost;
};

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: parse_set_free
//
// DESCRIPTION:
//
// This function frees a struct parse_set pointer
//
////////////////////////////////////////////////////////////////////////////////

void parse_set_free( struct parse_set * arg_set )
{
  if( arg_set == NULL )
    return;
    
  if( arg_set->rdin != NULL )
    free( arg_set->rdin );
    
  if( arg_set->rdout != NULL )
    free( arg_set->rdout );
    
  if( arg_set->pbargs != NULL )
  {
    if( arg_set->pbargs->arg_lists )
      free( arg_set->pbargs->arg_lists );
    free( arg_set->pbargs );
  }
  
  if( arg_set->lhost != NULL )
    free( arg_set->lhost );
    
  if( arg_set->rhost != NULL )
    free( arg_set->rhost );
}
    
////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: pipeBuilder
//
// DESCRIPTION:
//
// This function parse further by "|" pipe characters. Generates a NULL
// terminated set of arglists for each command piped together.
//
////////////////////////////////////////////////////////////////////////////////

struct pipe_builder * pipeBuilder( int command_count , char ** arg_list )
{
  int i;
  char ** pos = arg_list;
  //Allocate a pipe_builder
  struct pipe_builder * pip = (struct pipe_builder *) malloc( sizeof( struct pipe_builder ) );
  if( pip == NULL )
  {
    fprintf( stderr , "Allocation Error.\n" );
    return NULL;
  }
  
  //Allocate n + 1 arg_lists
  pip->n = command_count;
  pip->arg_lists = (char *** ) malloc( (command_count+1) * sizeof( char ** ) );
  if( pip->arg_lists == NULL )
  {
    fprintf( stderr , "Allocation Error.\n" );
    free( pip );
    return NULL;
  }
  
  //Populate arg_lists
  for( i = 0 ; i < command_count ; i++ ) 
  {
    pip->arg_lists[i] = pos;
    while( dref(pos) != NULL && strncmp( dref(pos) , "|" , 1 ) ) pos++;
    dref(pos) = NULL;
    pos++;
  }
  
  (pip->arg_lists)[command_count] = NULL;
  return pip;
}

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: parse_special
//
// DESCRIPTION:
//
// Parses the initially parsed arglist, searching for reserved operators,
// allocating memory as necessary. 
//
// OUTPUT:
//    int mode;
//    char ** reference to a converted pointer (char *) from (struct parse_set *)
//
////////////////////////////////////////////////////////////////////////////////

int parse_special( char **arg_list , char **spec_res )
{
  *spec_res = NULL; //Initialize spec_res
  
  int command_count = 1;
  int mode = 0;
  char ** pos = arg_list;
  
  //Try to allocate arg_set
  struct parse_set * arg_set = (struct parse_set * ) malloc( sizeof( struct parse_set ) );
  if( arg_set == NULL )
  {
    fprintf( stderr , "Allocation Error.\n" );
    mode = -2;
  }
  
  //Searched parsed arglist for reserved operators
  while( dref(pos) != NULL && mode >= 0 )
  {
    //If '<' the redirect stdin symbol is found do this
    if( strlen( dref(pos)) == 1 && !strncmp( dref(pos) , "<" , 1 ) )
    {
      dref(pos) = NULL;
      
      if( is_set( mode , REDIR_STDIN | REMOTE_HOST) )
      {
        fprintf( stderr , "Cannot redirect stdin more than once.\n" );
        mode = -1;
        break;
      }
      //set mode to redirect stdin
      setb( mode , REDIR_STDIN );
      pos++;
      if( dref(pos) == NULL )
      {
        fprintf( stderr , "No file provided for redirect.\n" );
        mode = -1;
        break;
      }
      //allocate new instance
      arg_set->rdin = (struct redir_stdin * ) malloc( sizeof( struct redir_stdin ) );
      if( arg_set->rdin == NULL )
      {
        fprintf( stderr , "Allocation Error.\n" );
        mode = -2;
        break;
      }
      //set path to file to read from
      arg_set->rdin->path = dref(pos);
    }
    
    //Look for the redirect stdout symbol and handle 
    else if( strlen( dref(pos)) == 1 && !strncmp( dref(pos) , ">" , 1 ) )
    {
      dref(pos) = NULL;
      
      if( is_set( mode , REDIR_STDOUT_W | REDIR_STDOUT_A ) )
      {
        fprintf( stderr , "Cannot redirect stdout more than once.\n" );
        mode = -1;
        break;
      }
      //set mode to truncating redirect
      setb( mode , REDIR_STDOUT_W );
      pos++;
      if( dref(pos) == NULL )
      {
        fprintf( stderr , "No file provided for redirect.\n" );
        mode = -1;
        break;
      }
      //allocate new instance
      arg_set->rdout = (struct redir_stdout *) malloc( sizeof( struct redir_stdout ) );
      if( arg_set->rdout == NULL )
      {
        fprintf( stderr , "Allocation Error.\n" );
        mode = -2;
        break;
      }
      //set path to file to truncate and write to
      arg_set->rdout->path = dref(pos);
    }
    
    //Look for the appending redirect symbol
    else if( strlen( dref(pos)) == 2 && !strncmp( dref(pos) , ">>" , 2 ) )
    {
      dref(pos) = NULL;
      
      if( is_set( mode , REDIR_STDOUT_W | REDIR_STDOUT_A ) )
      {
        fprintf( stderr , "Cannot redirect stdout more than once.\n" );
        mode = -1;
        break;
      }
      //Set mode as appending redirect stdout
      setb( mode , REDIR_STDOUT_A );
      pos++;
      if( dref(pos) == NULL )
      {
        fprintf( stderr , "No file provided for redirect.\n" );
        mode = -1;
        break;
      }
      //Allocate a new instance
      arg_set->rdout = (struct redir_stdout *) malloc( sizeof( struct redir_stdout ) );
      if( arg_set->rdout == NULL )
      {
        fprintf( stderr , "Allocation Error.\n" );
        mode = -2;
        break;
      }
      //Set path to file to append to
      arg_set->rdout->path = dref(pos);
    }
    
    else if( strlen( dref(pos)) == 1 && !strncmp( dref(pos) , "|" , 1 ) )
    {
      if( is_set( mode , REDIR_STDOUT_W | REDIR_STDOUT_A | REDIR_STDIN ) )
      {
        fprintf( stderr , "Standard shell pipes must come before redirects.\n" );
        mode = -1;
        break;
      }
      
      pos++;
      if( dref(pos) == NULL )
      { 
        fprintf( stderr , "Missing argument for pipe.\n" );
        mode = -1;
        break;
      }
      //Count number of piped together commands
      command_count++;
      //Yay PIPES!!! (This is really unnecessary, all pipe commands must come 
      //               first anyway.)
      setb( mode , PIPE_BUILDER );
    }
    
    //Handle local host pipe
    else if( strlen( dref(pos)) == 2 &&  !strncmp( dref(pos) , "))" , 2 ) )
    {
      dref(pos) = NULL;
      if( is_set( mode , REDIR_STDOUT_W | REDIR_STDOUT_A ) )
      {
        fprintf( stderr , "Can't bother redirect and write to socket.\n" );
        mode = -1;
        break;
      }
      
      //allocate instance of struct
      arg_set->lhost = (struct local_host *) malloc( sizeof( struct local_host ) );
      if( arg_set->lhost == NULL )
      {
        fprintf( stderr , "Allocation Error.\n" );
        mode = -2;
        break;
      }
      
      pos++;
      if( dref(pos) == NULL )
      {
        fprintf( stderr , "Missing port for host pipe.\n" );
        mode = -1;
        break;
      }
      //set port
      arg_set->lhost->port_no = dref(pos);
      //set mode as local host (sending)
      setb( mode , LOCAL_HOST );
    }
    
    //Handle remote host pipe
    else if( strlen( dref(pos)) == 2 && !strncmp( dref(pos) , "((" , 2 ) )
    {
      dref(pos) = NULL;
      if( is_set( mode , REDIR_STDIN | REMOTE_HOST) )
      {
        fprintf( stderr , "Cannot redirect stdin more than once.\n" );
        mode = -1;
        break;
      }
      //set mode to remote_host (reading)
      setb( mode , REMOTE_HOST );
      //allocate new instance
      arg_set->rhost = (struct remote_host *) malloc( sizeof( struct remote_host ) );
      if( arg_set->rhost == NULL )
      {
        fprintf( stderr , "Allocation Error.\n" );
        mode = -2;
        break;
      }
      
      pos++;
      if( dref(pos) == NULL )
      {
        fprintf( stderr , "Missing port for host pipe.\n" );
        mode = -1;
        break;
      }
      //Set ip
      arg_set->rhost->ipv6 = dref(pos);
      
      pos++;
      if( dref(pos) == NULL )
      {
        fprintf( stderr , "Missing port for host pipe.\n" );
        mode = -1;
        break;
      }
      //Set port
      arg_set->rhost->port_no = dref(pos);
      
    } 
    pos++;
  }
  
  if( mode < 0 )
    parse_set_free( arg_set );
  else if( mode > 0 )
  {
    arg_set->pbargs = pipeBuilder( command_count , arg_list ); 
    if( arg_set->pbargs == NULL ) 
    {
      parse_set_free( arg_set );
      return -2;
    } 
  }
  
  *spec_res = (char *) arg_set;
  return mode;
}     

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: dclient
//
// DESCRIPTION:
//
// Connect to stuff
//
////////////////////////////////////////////////////////////////////////////////

int dclient( char * port , char * ipv6 )
{
  int socket_fd;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	
	//make socket
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0) 
		fprintf(stderr,"ERROR opening socket");
  
  //get server struct
	server = gethostbyname( ipv6 );
	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
	}
	
	//zero address
	bzero((char *) &serv_addr, sizeof(serv_addr));
	
	serv_addr.sin_family = AF_INET;
	
	//copy address to the socket
	bcopy((char *)server->h_addr, 
			(char *)&serv_addr.sin_addr.s_addr,
			server->h_length);

  //host to network	
	serv_addr.sin_port = htons( atoi(port) );
	
	//try connecting
	if (connect(socket_fd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
		fprintf(stderr,"ERROR connecting");
		
  return socket_fd;
}

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: dserver
//
// DESCRIPTION:
//
// Connect to stuff
//
////////////////////////////////////////////////////////////////////////////////

int dserver( char * port )
{
  int socket_fd,newsocket_fd;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;

  //make socket
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	
	if (socket_fd < 0) 
		fprintf(stderr,"ERROR opening socket");
	
	//zero address	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons( atoi(port) );
	
	//bind socket
	if( bind(socket_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr) ) < 0) 
		fprintf(stderr,"ERROR on binding");
	
	//wait for remote connection and accept it
	listen(socket_fd,5);
	clilen = sizeof(cli_addr);
	newsocket_fd = accept(socket_fd , (struct sockaddr *) &cli_addr, &clilen);
			
	if (newsocket_fd < 0) 
		fprintf(stderr,"ERROR on accept");
		
  return newsocket_fd;
}

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: run_special
//
// DESCRIPTION:
//
// Runs commands from structure parse_set based on mode.
//
////////////////////////////////////////////////////////////////////////////////

int run_special( char * spec_res , int mode )
{
  struct parse_set * arg_set = (struct parse_set *) spec_res;
  
  char *** cmd = arg_set->pbargs->arg_lists;
  
  int   p[2];
  int   fd_in = 0;
  int   fdout = 1;
  int   status = 0;
  pid_t pid;

  pipe(p);
  if( (pid = fork()) == -1 )
  {
    fprintf( stderr , "Failed to fork.\n" );
    free( arg_set );
    return -3;
  }
  else if( pid == 0 )
  {
    //Redirect of STDIN
    if( is_set( mode , REDIR_STDIN ) )
      freopen( arg_set->rdin->path , "r" , stdin );
    //Redirect from socket
    else if( is_set( mode , REMOTE_HOST ) )
      fd_in = dclient( arg_set->rhost->port_no , arg_set->rhost->ipv6 );
    if( dref(cmd + 1) == NULL )
    { 
      //Truncating Redirect
      if( is_set( mode , REDIR_STDOUT_W ) )
        freopen( arg_set->rdout->path , "w" , stdout );
      //Appending Redirect
      else if( is_set( mode , REDIR_STDOUT_A ) )
        freopen( arg_set->rdout->path , "a" , stdout );
      //Socket Redirect
      else if( is_set( mode , LOCAL_HOST ) )
      {
        fdout = dserver( arg_set->lhost->port_no );
        dup2( fdout , STDOUT_FILENO );
      }
    }
    //Pipe Redirect
    else
      dup2(p[1], STDOUT_FILENO );
    dup2(fd_in , STDIN_FILENO );
    close(p[0]);
    execvp( (dref(cmd))[0] , dref(cmd) );
    exit( EXIT_FAILURE );
  }
  
  else
  {
    wait(&status);
    if( status == 256 )
    {
      fprintf(stderr, "dsh: %s: command not found\n" , (*cmd)[0] );
      free(arg_set);
      return -1;
    }
    close(p[1]);
    fd_in = p[0]; //save the input for the next command
    cmd++;
  }
  
  while ( dref(cmd) != NULL)
  {
    pipe(p);
    if ((pid = fork()) == -1)
    {
      exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {      
      dup2(fd_in, 0); //change the input according to the old one 
      if ( dref(cmd + 1) != NULL)
        dup2(p[1], 1);
      else
      { 
        //Truncating redirect
        if( is_set( mode , REDIR_STDOUT_W ) )
          freopen( arg_set->rdout->path , "w" , stdout );
        //Appending redirect
        else if( is_set( mode , REDIR_STDOUT_A ) )
          freopen( arg_set->rdout->path , "a" , stdout );
        //Socket redirect
        else if( is_set( mode , LOCAL_HOST ) )
        {
          fdout = dserver( arg_set->lhost->port_no );
          dup2( fdout , STDOUT_FILENO );
        }
      }
      close(p[0]);
      execvp((*cmd)[0], *cmd);
      exit(EXIT_FAILURE);
    }
    else
    {
      wait(&status);
      if( status == 256 )
      {
        fprintf(stderr, "dsh: %s: command not found\n" , (*cmd)[0] );
        free( arg_set );
        return -1;
      }
      close(p[1]);
      fd_in = p[0]; //save the input for the next command
      cmd++;
    }
  }
  free( arg_set );   
  return 0;
}
