#include "cwCommon.h"
#include "cwLog.h"
#include "cwCommonImpl.h"
#include "cwText.h"
#include "cwObject.h"

using namespace cw;

void print( void* arg, const char* text )
{
  printf("%s\n",text);
}

int main( int argc, char* argv[] )
{
  rc_t rc = kOkRC;
  object_t* cfg = nullptr;
  cw::log::createGlobal();
  
  cwLogInfo("Project template: args:%i", argc);

  if( argc < 2 || textLength(argv[1])==0 )
  {
    cwLogError(kInvalidArgRC,"No cfg. file was given.");
    goto errLabel;
  }
  else
  {
    const char* val = nullptr;
    
    if((rc = objectFromFile(argv[1],cfg)) != kOkRC )
    {
      cwLogError(rc,"The file '%s'.",argv[1]);
      goto errLabel;
    }

    if((rc = cfg->getv("param",val)) != kOkRC )
    {
      cwLogError(kSyntaxErrorRC,"The 'param' cfg. field was not found.");
      goto errLabel;
    }

    cwLogInfo("param=%s",cwStringNullGuard(val));
  }

errLabel:
  cw::log::destroyGlobal();

  return 0;
}
