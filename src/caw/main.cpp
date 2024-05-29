#include "cwCommon.h"
#include "cwLog.h"
#include "cwCommonImpl.h"
#include "cwTest.h"
#include "cwMem.h"
#include "cwText.h"
#include "cwObject.h"
#include "cwFileSys.h"
#include "cwTime.h"
#include "cwMidiDecls.h"
#include "cwFlowDecl.h"
#include "cwFlow.h"

#include "cwIo.h"


using namespace cw;

rc_t test( const object_t* cfg, int argc, char* argv[] );

typedef struct app_str
{
  object_t* cfg;           // complete cfg.
  
  bool            real_time_fl;  // Execute in non-real-time mode
  const char*     pgm_label;     // 
  const object_t* pgm_cfg;       //
  const char*     base_dir;      //  
  char*           proj_dir;      //  Project directory <base_dir>/<pgm_label>
  object_t*       io_cfg;        //  IO lib cfg.
  object_t*       proc_class_dict_cfg; //
  object_t*       subnet_dict_cfg; 
  
  unsigned        value;
  io::handle_t    ioH;
  
} app_t;

enum
{
  kPanelDivId,   
  kQuitBtnId,    
  kIoReportBtnId,
  kNetPrintBtnId,
  kReportBtnId,  
  kLatencyBtnId, 
  kValueNumbId
};

ui::appIdMap_t  appIdMapA[] = {
  
  { ui::kRootAppId,  kPanelDivId,     "panelDivId" },
  { kPanelDivId,     kQuitBtnId,      "quitBtnId" },
  { kPanelDivId,     kIoReportBtnId,  "ioReportBtnId" },
  { kPanelDivId,     kNetPrintBtnId,  "netPrintBtnId" },
  { kPanelDivId,     kReportBtnId,    "reportBtnId" },
  { kPanelDivId,     kLatencyBtnId,   "latencyBtnId" },
  { kPanelDivId,     kValueNumbId,    "valueNumbId" }
};

const unsigned appIdMapN = sizeof(appIdMapA)/sizeof(appIdMapA[0]);
  
void print( void* arg, const char* text )
{
  printf("%s\n",text);
}


rc_t _ui_value_callback(app_t* app, const io::ui_msg_t& m )
{
  switch( m.appId )
  {
    case kQuitBtnId:
      io::stop( app->ioH );
      break;
      
    case kIoReportBtnId:
      io::report(app->ioH);
      break;
      
    case kNetPrintBtnId:
      break;
      
    case kReportBtnId:
      break;
      
    case kLatencyBtnId:
      latency_measure_report(app->ioH);
      latency_measure_setup(app->ioH);
      break;
      
    case kValueNumbId:
      app->value = m.value->u.u;
      cwLogInfo("Setting value:%i",app->value);
      break;
    
  }
  return kOkRC;
}

rc_t _ui_echo_callback(app_t* app, const io::ui_msg_t& m )
{
  switch( m.appId )
  {
    case kValueNumbId:
      {
        uiSendValue( app->ioH, io::uiFindElementUuId( app->ioH, kValueNumbId ), app->value );
      }
      break;
    
  }
  return kOkRC;
}

rc_t _ui_callback( app_t* app, const io::ui_msg_t& m )
{
  rc_t rc = kOkRC;
      
  switch( m.opId )
  {
    case ui::kConnectOpId:
      cwLogInfo("UI Connected: wsSessId:%i.",m.wsSessId);
      break;
          
    case ui::kDisconnectOpId:
      cwLogInfo("UI Disconnected: wsSessId:%i.",m.wsSessId);          
      break;
          
    case ui::kInitOpId:
      cwLogInfo("UI Init.");
      break;

    case ui::kValueOpId:
      _ui_value_callback( app, m );
      break;

    case ui::kCorruptOpId:
      cwLogInfo("UI Corrupt.");
      break;

    case ui::kClickOpId:
      cwLogInfo("UI Click.");
      break;

    case ui::kSelectOpId:
      cwLogInfo("UI Select.");
      break;
        
    case ui::kEchoOpId:
      _ui_echo_callback( app, m );
      break;

    case ui::kIdleOpId:
      break;
          
    case ui::kInvalidOpId:
      // fall through
    default:
      assert(0);
      break;
        
  }

  return rc;
}


rc_t _io_callback( void* arg, const io::msg_t* m )
{
  app_t* app = (app_t*)arg;
  
  switch( m->tid )
  {
    case io::kThreadTId:
      break;
      
    case io::kTimerTId:
      break;
      
    case io::kSerialTId:
      break;
      
    case io::kMidiTId:
      break;
      
    case io::kAudioTId:
      break;
      
    case io::kAudioMeterTId:
      break;
      
    case io::kSockTId:
      break;
      
    case io::kWebSockTId:
      break;
      
    case io::kUiTId:
      _ui_callback(app,m->u.ui);
      break;
      
    case io::kExecTId:
      break;
      
    default:
      assert(0);
  }
  
  return kOkRC;
}

rc_t _parse_cfg( app_t& app, int argc, char* argv[] )
{
  rc_t rc = kOkRC;
  const char* io_cfg_fn = nullptr;
  const char* mode_label = nullptr;
  const object_t* pgmL = nullptr;
  const char* proc_cfg_fname = nullptr;
  const char* subnet_cfg_fname = nullptr;
  if( argc < 3 )
  {
    cwLogPrint("Usage: caw <cfg_fname> <pgm_label>");
    rc = kInvalidArgRC;
    goto errLabel;
  }
  else
  {
    // parse the cfg. file
    if((rc = objectFromFile(argv[1],app.cfg)) != kOkRC )
    {
      rc = cwLogError(rc,"Parsing failed on the cfg. file '%s'.",argv[1]);
      goto errLabel;
    }
    
    // parse the cfg parameters
    if((rc = app.cfg->readv("param",  0, app.value,
                            "io_cfg", kOptFl, io_cfg_fn,
                            "base_dir", 0, app.base_dir,
                            "proc_dict",0,proc_cfg_fname,
                            "subnet_dict",0,subnet_cfg_fname,                            
                            "mode", 0, mode_label,
                            "programs", kDictTId, pgmL)) != kOkRC )
    {
      rc = cwLogError(rc,"'caw' system parameter processing failed.");
      goto errLabel;
    }

    // set the real-time mode flag
    app.real_time_fl = !textIsEqual(mode_label,"non_real_time");

    // if we are in real-time mode then the io_cfg file must be given
    if( app.real_time_fl && io_cfg_fn == nullptr )
    {
      rc = cwLogError(kSyntaxErrorRC,"To operation in real-time mode an 'io_cfg' file must be provided.");
      goto errLabel;
    }
    
    // parse the 'io_cfg' file
    if( app.real_time_fl )
      if((rc = objectFromFile(io_cfg_fn,app.io_cfg)) != kOkRC )
      {
        rc = cwLogError(rc,"'caw' IO cfg. file parsing failed on '%s'.",cwStringNullGuard(io_cfg_fn));
        goto errLabel;
      }


    // parse the proc dict. file
    if((rc = objectFromFile(proc_cfg_fname,app.proc_class_dict_cfg)) != kOkRC )
    {
      rc = cwLogError(rc,"The flow proc dictionary could not be read from '%s'.",cwStringNullGuard(proc_cfg_fname));
      goto errLabel;
    }

    // parse the subnet dict file
    if((rc = objectFromFile(subnet_cfg_fname,app.subnet_dict_cfg)) != kOkRC )
    {
      rc = cwLogError(rc,"The flow subnet dictionary could not be read from '%s'.",cwStringNullGuard(subnet_cfg_fname));
      goto errLabel;
    }
        
    // get the pgm label
    if( textLength(argv[2]) == 0 )
    {
      rc = cwLogError(kSyntaxErrorRC,"No 'caw' program label was given.");
      goto errLabel;
    }
    
    app.pgm_label = argv[2];

    // find the parameters for the requested program
    for(unsigned i=0; i<pgmL->child_count(); i++)      
    {
      const object_t* pgm = pgmL->child_ele(i);
      if( textIsEqual( pgm->pair_label(), app.pgm_label ) )
      {
        if( pgm->pair_value() == nullptr || !pgm->pair_value()->is_dict() )
        {
          rc = cwLogError(kSyntaxErrorRC,"The parameters for the program '%s' is not a dictionary.",cwStringNullGuard(app.pgm_label));
          goto errLabel;
        }
        
        app.pgm_cfg = pgm->pair_value();
        break;
      }      
    }

    if( app.pgm_cfg == nullptr )
      rc =cwLogError(kEleNotFoundRC,"The program '%s' was not found in the cfg. program list.",cwStringNullGuard(app.pgm_label));
    else
    {
      if((app.proj_dir = filesys::makeFn(app.base_dir,nullptr,nullptr,app.pgm_label,nullptr)) == nullptr )
      {
        rc = cwLogError(kOpFailRC,"An error occurred while forming the the project directory name for '%s' / '%s'.",cwStringNullGuard(app.base_dir),cwStringNullGuard(app.pgm_label));
        goto errLabel;
      }

      if( !filesys::isDir(app.proj_dir))
      {
        if((rc = filesys::makeDir(app.proj_dir)) != kOkRC )
        {
          rc = cwLogError(kOpFailRC,"Project directory create failed.");
          goto errLabel;
        }
      }
    }

    
  }
errLabel:
  if( rc != kOkRC )
    rc = cwLogError(rc,"App. cfg. parse failed.");
  return rc;
}

rc_t _main_gui( app_t& app )
{
  rc_t rc = kOkRC;
  
  if((rc = create( app.ioH, app.io_cfg, _io_callback, &app, appIdMapA, appIdMapN ) ) != kOkRC )
  {
    rc = cwLogError(rc,"IO create failed.");
    goto errLabel;
  }

  
  // start the IO framework instance
  if((rc = io::start(app.ioH)) != kOkRC )
  {
    rc = cwLogError(rc,"Preset-select app start failed.");
    goto errLabel;    
  }

  
  // execute the io framework
  while( !io::isShuttingDown(app.ioH))
  {
    // This call will block on the websocket handle
    // for up to io_cfg->ui.websockTimeOutMs milliseconds
    io::exec(app.ioH,50);


  }

  // stop the io framework
  if((rc = io::stop(app.ioH)) != kOkRC )
  {
    rc = cwLogError(rc,"IO API stop failed.");
    goto errLabel;    
  }

errLabel:
  return rc;
}



int main( int argc, char* argv[] )
{
  rc_t  rc  = kOkRC;
  app_t app = {};
  flow::handle_t flowH;
  
  cw::log::createGlobal();
  
  cwLogInfo("caw: args:%i", argc);

  if((rc = _parse_cfg(app,argc,argv)) != kOkRC )
    goto errLabel;

    // create the flow object
  if((rc = create( flowH,
                   app.proc_class_dict_cfg,
                   app.pgm_cfg,
                   app.subnet_dict_cfg,
                   app.proj_dir)) != kOkRC )
  {
    //rc = cwLogError(rc,"Flow object create failed.");
    goto errLabel;
  }

  // run the network
  if((rc = exec( flowH )) != kOkRC )
    rc = cwLogError(rc,"Execution failed.");
    
errLabel:
  // destroy the flow object
  if((rc = destroy(flowH)) != kOkRC )
  {
    rc = cwLogError(rc,"Close the flow object.");
    goto errLabel;
  }

  destroy(app.ioH);
  
  mem::release(app.proj_dir);

  if( app.proc_class_dict_cfg != nullptr )
    app.proc_class_dict_cfg->free();

  if( app.subnet_dict_cfg != nullptr )
    app.subnet_dict_cfg->free();
  
  if( app.io_cfg != nullptr )
    app.io_cfg->free();
  
  if( app.cfg != nullptr )
    app.cfg->free();
  
  cw::log::destroyGlobal();

  return 0;
}
