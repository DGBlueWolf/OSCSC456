#include <stdio.h>
#include <sys/types.h>
#include <time.h>

int main()
{
  time_t t,next;
  struct timespec Time;
  char buffer[26];
  int i;
  long nsec;
  struct tm * time;
  clock_gettime(CLOCK_REALTIME , &Time);
  next = Time.tv_sec+1;
  for( i = 0 ; i < 20 ; i++ )
  {
    while( t < next )
    {
    clock_gettime( CLOCK_REALTIME , &Time);
    t = Time.tv_sec;
    }
    next++;
    nsec = Time.tv_nsec;
    time = localtime( &t );
    strftime( buffer , 26 , "%H:%M:%S" , time );
    puts( buffer );
  }
  return 0;
}
