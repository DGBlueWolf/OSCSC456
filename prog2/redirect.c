#include <stdio.h>

int main()
{
  freopen( "redirect.out" , "w" , stdout );
  printf( "Hello, World! I am good at doing redirection!\n" );
  fflush(stdout);
  freopen( "/dev/tty" , "a" , stdout );
  printf( "And we're back\n" ); 
  
  int n;
  freopen( "redirect.in" , "r" , stdin );
  scanf( "%d" , &n );
  printf("The file said: %d\n" , n );
  freopen( "/dev/tty" , "r" , stdin );
  scanf( "%d" , &n );
  printf("But I said: %d\n" , n );
  return 0;
}
