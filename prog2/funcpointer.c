#include <stdio.h>

int foo( int p )
{
  return p*p;
}

int bar( int p )
{
  return p/2;
}

int main()
{
  int (*f[2])(int) = { foo , bar };
  printf( "foo(2) = %d\n" , f[0](2) );
  printf( "bar(2) = %d\n" , f[1](2) );
  return 0;
}
