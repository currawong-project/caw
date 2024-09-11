#include "cwCommon.h"
#include "cwLog.h"
#include "cwCommonImpl.h"
#include "cwTest.h"
#include "cwMem.h"
#include "cwText.h"
#include "cwObject.h"
#include "cwFileSys.h"
#include "cwIo.h"

#include "cwIoFlowCtl.h"


#include "cwTest.h"

using namespace cw;

enum {
  kExecSelId,
  kTestSelId,
  kHwReportSelId,
  kTestStubSelId,
  kHelpSelId
};

idLabelPair_t appSelA[] = {
  { kExecSelId,     "exec" },
  { kTestSelId,     "test" },
  { kHwReportSelId, "hw_report" },
  { kTestStubSelId, "test_stub" },
  { kHelpSelId,     "help" },
  { kInvalidId, nullptr }
};

rc_t test( const object_t* cfg, int argc, char* argv[] );

typedef struct app_str
{
  object_t*             flow_cfg;           // flow program cfg
  object_t*             io_cfg;             //  IO lib cfg.
  const char*           cmd_line_pgm_label; //

  unsigned appSelId;
  
  io::handle_t          ioH;
  io_flow_ctl::handle_t ioFlowH;

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

rc_t _run_test_suite(int argc, const char** argv)
{
  rc_t rc = kOkRC;

  if( argc < 1 )
  {
    rc = cwLogError(kInvalidArgRC,"The command line is invalid for running the test suite.");
    goto errLabel;
  }
  
  rc = test::test(argv[0],argc,argv);

errLabel:
  return rc;
}


rc_t _load_init_pgm( app_t& app, const char* pgm_label, bool& exec_complete_fl_ref )
{
  rc_t     rc = kOkRC;
  unsigned pgm_idx;

  exec_complete_fl_ref = false;
  
  if((pgm_idx = program_index(app.ioFlowH,pgm_label)) == kInvalidIdx )
  {
    rc = cwLogError(kInvalidArgRC,"A program named '%s' was not be found.",cwStringNullGuard(pgm_label));
    goto errLabel;
  }

  if((rc = program_load(app.ioFlowH,pgm_idx)) != kOkRC )
  {
    rc = cwLogError(rc,"Program load failed on '%s'.",cwStringNullGuard(pgm_label));
    goto errLabel;
  }

  if( is_program_nrt(app.ioFlowH) )
  {
    exec_complete_fl_ref = true;
    if((rc = exec_nrt(app.ioFlowH)) != kOkRC )
      goto errLabel;
  }
  

errLabel:
  return rc;
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
      //app->value = m.value->u.u;
      //cwLogInfo("Setting value:%i",app->value);
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
        //uiSendValue( app->ioH, io::uiFindElementUuId( app->ioH, kValueNumbId ), app->value );
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
      if( is_executable(app->ioFlowH) && m != nullptr )
        io_flow_ctl::exec(app->ioFlowH,*m);
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

void _print_command_line_help()
{
  const char* usage =
    "Usage:\n"
    "       caw exec      <program_cfg_fname> <program_label>\n"
    "       caw hw_report <program_cfg_fname> <program_label>\n"
    "       caw test <test_cfg_fname> (<module_label> | all) (<test_label> | all) (compare | echo | gen_report )* {args ...}\n"
    "       caw test_stub ...\n";
    
  cwLogPrint(usage);
  
}

rc_t _parse_main_cfg( app_t& app, int argc, char* argv[] )
{
  rc_t rc = kOkRC;
  const char* io_cfg_fn = nullptr;
  
  if( argc < 1 )
  {
    rc = kInvalidArgRC;
    _print_command_line_help();
    goto errLabel;
  }

  if((app.appSelId = labelToId( appSelA, argv[1], kInvalidId )) == kInvalidId )
  {
    rc = cwLogError(kInvalidArgRC,"The command line action '%s' is not valid.",argv[1]);
    _print_command_line_help();
  }

  if( app.appSelId == kExecSelId || app.appSelId == kHwReportSelId )
  {
    if( argc <= 3 )
    {
      rc = cwLogError(kInvalidArgRC,"The command line is missing required arguments.");
      _print_command_line_help();      
    }
    
    app.cmd_line_pgm_label = argv[3];
      
    // parse the cfg. file
    if((rc = objectFromFile(argv[2],app.flow_cfg)) != kOkRC )
    {
      rc = cwLogError(rc,"Parsing failed on the cfg. file '%s'.",argv[1]);
      goto errLabel;
    }

    // read the io cfg filename
    if((rc = app.flow_cfg->getv("io_dict", io_cfg_fn)) != kOkRC )
    {
      goto errLabel;
    }

    // parse the IO cfg file
    if((rc = objectFromFile(io_cfg_fn,app.io_cfg)) != kOkRC )
    {
      rc = cwLogError(rc,"Parsing failed on '%s'.",cwStringNullGuard(io_cfg_fn));
      goto errLabel;
    }

  }

errLabel:
  return rc;
}

rc_t _io_main( app_t& app )
{
  rc_t rc = kOkRC;
  
  // start the IO framework instance
  if((rc = io::start(app.ioH)) != kOkRC )
  {
    rc = cwLogError(rc,"Preset-select app start failed.");
    goto errLabel;    
  }

  
  // execute the IO framework
  while( !io::isShuttingDown(app.ioH))
  {
    // This call will block on the websocket handle
    // for up to io_cfg->ui.websockTimeOutMs milliseconds
    io::exec(app.ioH,50);

    if( is_exec_complete(app.ioFlowH) )
      break;
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


//#include "cwMidi.h"
//#include "cwMidiFile.h"
//#include "cwDspTypes.h"
//#include "cwWaveTableBank.h"

void _test_stub( app_t& app )
{

}

int main( int argc, char* argv[] )
{
  rc_t  rc  = kOkRC;
  app_t app = {};
  bool exec_complete_fl = false;
  
  cw::log::createGlobal();

  unsigned appIdMapN = sizeof(appIdMapA)/sizeof(appIdMapA[0]);

  if((rc= _parse_main_cfg(app, argc, argv )) != kOkRC )
    goto errLabel;

  switch( app.appSelId )
  {
    case kTestSelId:
      _run_test_suite(argc-2, (const char**)(argv + 2));
      goto errLabel;
      
    case kHelpSelId:
      _print_command_line_help();
      goto errLabel;

    case kTestStubSelId:
      _test_stub(app);
      goto errLabel;
      break;
      
    default:
      break;
  }
  
  // instantiate the IO framework
  if((rc = create( app.ioH, app.io_cfg, _io_callback, &app, appIdMapA, appIdMapN, nullptr )) != kOkRC )
  {
    rc = cwLogError(rc,"IO Framework instantiation failed.");
    goto errLabel;
  }

  // instantiate the IO flow controller
  if((rc = create( app.ioFlowH, app.ioH, app.flow_cfg)) != kOkRC )
  {
    rc = cwLogError(rc,"IO-Flow instantiation failed.");
    goto errLabel;
  }

  switch( app.appSelId )
  {
    case kHwReportSelId:
      hardwareReport(app.ioH);
      goto errLabel;
      break;

  }
  
  if( app.cmd_line_pgm_label != nullptr )
    if((rc = _load_init_pgm(app, app.cmd_line_pgm_label, exec_complete_fl )) != kOkRC )
      goto errLabel;
  

  if( !exec_complete_fl )
    _io_main(app);


errLabel:
  if((rc = destroy(app.ioFlowH)) != kOkRC )
    rc = cwLogError(rc,"IO Flow destroy failed.");
  
  if((rc = destroy(app.ioH)) != kOkRC )
    rc = cwLogError(rc,"IO destroy failed.");

  if( app.io_cfg != nullptr )
    app.io_cfg->free();
  
  if( app.flow_cfg != nullptr )
    app.flow_cfg->free();


  cw::log::destroyGlobal();
  
  return rc;
}


