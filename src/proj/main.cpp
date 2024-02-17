#include "cwCommon.h"
#include "cwLog.h"
#include "cwCommonImpl.h"
#include "cwText.h"
#include "cwObject.h"

#include "cwIo.h"

using namespace cw;

typedef struct app_str
{
  object_t*       cfg;
  unsigned        value;
  const object_t* io_cfg;
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
  
  if( argc < 2 || textLength(argv[1])==0 )
  {
    rc = cwLogError(kInvalidArgRC,"No cfg. file was given.");
    goto errLabel;
  }
  else
  {
    if((rc = objectFromFile(argv[1],app.cfg)) != kOkRC )
    {
      rc = cwLogError(rc,"The file '%s'.",argv[1]);
      goto errLabel;
    }

    if((rc = app.cfg->getv("param", app.value,
                           "libcw", app.io_cfg)) != kOkRC )
    {
      rc = cwLogError(kSyntaxErrorRC,"The 'param' cfg. field was not found.");
      goto errLabel;
    }

  }
errLabel:
  if( rc != kOkRC )
    rc = cwLogError(rc,"App. cfg. parse failed.");
  return rc;
}

int main( int argc, char* argv[] )
{
  rc_t  rc  = kOkRC;
  app_t app = {};
  cw::log::createGlobal();
  
  cwLogInfo("Project template: args:%i", argc);

  if((rc = _parse_cfg(app,argc,argv)) != kOkRC )
    goto errLabel;
  
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

  //io::uiReport(app.ioH);

  
  // execute the io framework
  while( !io::isShuttingDown(app.ioH))
  {
    // This call will block on the websocket handle
    // for up to io_cfg->ui.websockTimeOutMs milliseconds
    io::exec(app.ioH);


  }

  // stop the io framework
  if((rc = io::stop(app.ioH)) != kOkRC )
  {
    rc = cwLogError(rc,"IO API stop failed.");
    goto errLabel;    
  }


errLabel:
  destroy(app.ioH);
  if( app.cfg != nullptr )
    app.cfg->free();
  cw::log::destroyGlobal();

  return 0;
}
