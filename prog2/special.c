#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

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
    
struct pipe_builder * pipeBuilder( int command_count , char ** arg_list )
{
  int i;
  char ** pos = arg_list;
  struct pipe_builder * pip = (struct pipe_builder *) malloc( sizeof( struct pipe_builder ) );
  if( pip == NULL )
  {
    fprintf( stderr , "Allocation Error.\n" );
    return NULL;
  }
  
  pip->n = command_count;
  pip->arg_lists = (char *** ) malloc( (command_count+1) * sizeof( char ** ) );
  if( pip->arg_lists == NULL )
  {
    fprintf( stderr , "Allocation Error.\n" );
    free( pip );
    return NULL;
  }
  
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
  
int parse_special( char **arg_list , char **spec_res )
{
  *spec_res = NULL;
  int command_count = 1;
  int mode = 0;
  char ** pos = arg_list;
  struct parse_set * arg_set = (struct parse_set * ) malloc( sizeof( struct parse_set ) );
  if( arg_set == NULL )
  {
    fprintf( stderr , "Allocation Error.\n" );
    mode = -2;
  }
  
  while( dref(pos) != NULL && mode >= 0 )
  {
    //Look for the redirect stdin symbol and handle
    if( !strncmp( dref(pos) , "<" , 1 ) )
    {
      dref(pos) = NULL;
      
      if( is_set( mode , REDIR_STDIN ) )
      {
        fprintf( stderr , "Cannot redirect stdin more than once.\n" );
        mode = -1;
        break;
      }
      
      else if( is_set( mode , LOCAL_HOST | REMOTE_HOST ) )
      {
        fprintf( stderr , "Cannot redirect stdin when using host pipe.\n" );
        mode = -1;
        break;
      }
      
      setb( mode , REDIR_STDIN );
      pos++;
      if( dref(pos) == NULL )
      {
        fprintf( stderr , "No file provided for redirect.\n" );
        mode = -1;
        break;
      }
      
      arg_set->rdin = (struct redir_stdin * ) malloc( sizeof( struct redir_stdin ) );
      if( arg_set->rdin == NULL )
      {
        fprintf( stderr , "Allocation Error.\n" );
        mode = -2;
        break;
      }
      
      arg_set->rdin->path = dref(pos);
    }
    
    //Look for the redirect stdout symbol and handle 
    else if( !strncmp( dref(pos) , ">" , 1 ) )
    {
      dref(pos) = NULL;
      
      if( is_set( mode , REDIR_STDOUT_W | REDIR_STDOUT_A ) )
      {
        fprintf( stderr , "Cannot redirect stdout more than once.\n" );
        mode = -1;
        break;
      }
      
      setb( mode , REDIR_STDOUT_W );
      pos++;
      if( dref(pos) == NULL )
      {
        fprintf( stderr , "No file provided for redirect.\n" );
        mode = -1;
        break;
      }
      
      arg_set->rdout = (struct redir_stdout *) malloc( sizeof( struct redir_stdout ) );
      if( arg_set->rdout == NULL )
      {
        fprintf( stderr , "Allocation Error.\n" );
        mode = -2;
        break;
      }
      
      arg_set->rdout->path = dref(pos);
    }
    
    //Look for the appending redirect symbol
    else if( !strncmp( dref(pos) , ">>" , strlen(dref(pos)) ) )
    {
      dref(pos) = NULL;
      
      if( is_set( mode , REDIR_STDOUT_W | REDIR_STDOUT_A ) )
      {
        fprintf( stderr , "Cannot redirect stdout more than once.\n" );
        mode = -1;
        break;
      }
      
      setb( mode , REDIR_STDOUT_A );
      pos++;
      if( dref(pos) == NULL )
      {
        fprintf( stderr , "No file provided for redirect.\n" );
        mode = -1;
        break;
      }
      
      arg_set->rdout = (struct redir_stdout *) malloc( sizeof( struct redir_stdout ) );
      if( arg_set->rdout == NULL )
      {
        fprintf( stderr , "Allocation Error.\n" );
        mode = -2;
        break;
      }
      
      arg_set->rdout->path = dref(pos);
    }
    
    //Look for the pipe symbol
    else if( !strncmp( dref(pos) , "|" , 1 ) )
    {
      if( is_set( mode , REDIR_STDOUT_W | REDIR_STDOUT_A | REDIR_STDIN ) )
      {
        fprintf( stderr , "Piping after redirects not allowed.\n" );
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
      
      command_count++;
      setb( mode , PIPE_BUILDER );
    }
    
    else if( !strncmp( dref(pos) , "((" , strlen(dref(pos)) ) )
    {
      if( is_set( mode , REDIR_STDOUT_W | REDIR_STDOUT_A | REDIR_STDIN | PIPE_BUILDER ) )
      {
        fprintf( stderr , "Local host pipe must be first.\n" );
        mode = -1;
        break;
      }
      
      setb( mode , LOCAL_HOST );
    }
    
    else if( !strncmp( dref(pos) , "))" , strlen(dref(pos)) ) )
    {
      if( is_set( mode , REDIR_STDOUT_W | REDIR_STDOUT_A | REDIR_STDIN | PIPE_BUILDER ) ) 
      {
        fprintf( stderr , "Remote host pipe must be first.\n" );
        mode = -1;
        break;
      }
      
      setb( mode , REMOTE_HOST );
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

int run_special( char * spec_res , int mode )
{
  struct parse_set * arg_set = (struct parse_set *) spec_res;
  
  char *** cmd = arg_set->pbargs->arg_lists;
  
  int   p[2];
  pid_t pid;
  int   fd_in = 0;
  pipe(p);
  if( (pid = fork()) == -1 )
  {
    fprintf( stderr , "Failed to fork.\n" );
    free( arg_set );
    return -3;
  }
  else if( pid == 0 )
  {
    //if child
    if( is_set( mode , REDIR_STDIN ) )
      freopen( arg_set->rdin->path , "r" , stdin );
    if( dref(cmd + 1) == NULL )
    { 
      if( is_set( mode , REDIR_STDOUT_W ) )
        freopen( arg_set->rdout->path , "w" , stdout );
      else if( is_set( mode , REDIR_STDOUT_A ) )
        freopen( arg_set->rdout->path , "a" , stdout );
    }
    else
      dup2(p[1], 1);
    dup2(fd_in , 0);
    close(p[0]);
    execvp( (dref(cmd))[0] , dref(cmd) );
    exit( EXIT_FAILURE );
  }
  
  else
  {
    wait(NULL);
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
        if( is_set( mode , REDIR_STDOUT_W ) )
          freopen( arg_set->rdout->path , "w" , stdout );
        else if( is_set( mode , REDIR_STDOUT_A ) )
          freopen( arg_set->rdout->path , "a" , stdout );
      }
      close(p[0]);
      execvp((*cmd)[0], *cmd);
      exit(EXIT_FAILURE);
    }
    else
    {
      wait(NULL);
      close(p[1]);
      fd_in = p[0]; //save the input for the next command
      cmd++;
    }
  }
  free( arg_set );   
  return 0;
}
