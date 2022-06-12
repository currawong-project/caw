#include "cwCommon.h"
#include "cwLog.h"
#include "cwCommonImpl.h"

void print( void* arg, const char* text )
{
  printf("%s\n",text);
}

int main( int argc, char* argv[] )
{
  cw::log::createGlobal();
  
  cwLogInfo("Project template");

  cw::log::destroyGlobal();

  return 0;
}
