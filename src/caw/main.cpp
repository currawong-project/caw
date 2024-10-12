#include "cwCommon.h"
#include "cwLog.h"
#include "cwCommonImpl.h"
#include "cwTest.h"
#include "cwMem.h"
#include "cwText.h"
#include "cwObject.h"
#include "cwFileSys.h"
#include "cwIo.h"
#include "cwVectOps.h"

#include "cwFlowDecl.h"
#include "cwIoFlowCtl.h"

#include "cawUiDecl.h"
#include "cawUi.h"

#include "cwTest.h"

using namespace cw;
using namespace caw::ui;

enum {
  kUiSelId,
  kExecSelId,
  kTestSelId,
  kHwReportSelId,
  kTestStubSelId,
  kHelpSelId,
};


idLabelPair_t appSelA[] = {
  { kUiSelId,       "ui" },
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
  unsigned              cmd_line_action_id; // kUiSelId | kExecSelId | kTestSelId ....
  const char*           cmd_line_pgm_fname; // pgm file passed from the comman dline
  const char*           cmd_line_pgm_label; // pgm label passed from the command line 
  unsigned              pgm_preset_idx;     // currently selected pgm preset
  
  bool                  run_fl;             // true if the program is running (and the 'run' check is checked)
  
  io::handle_t          ioH;
  io_flow_ctl::handle_t ioFlowH;
  caw::ui::handle_t     uiH;

} app_t;

ui::appIdMap_t  appIdMapA[] = {
  
  { ui::kRootAppId,  kPanelDivId,     "panelDivId" },
  { kPanelDivId,     kQuitBtnId,      "quitBtnId" },
  { kPanelDivId,     kIoReportBtnId,  "ioReportBtnId" },
  { kPanelDivId,     kNetPrintBtnId,  "netPrintBtnId" },
  { kPanelDivId,     kReportBtnId,    "reportBtnId" },
  { kPanelDivId,     kLatencyBtnId,   "latencyBtnId" },
  
  { kPanelDivId,     kPgmSelId,       "pgmSelId" },
  { kPanelDivId,     kPgmPresetSelId, "pgmPresetSelId" },
  { kPanelDivId,     kPgmLoadBtnId,   "pgmLoadBtnId" },
  { kPanelDivId,     kRunCheckId,     "runCheckId" },

  { kPanelDivId,     kRootNetPanelId, "rootNetPanelId" },

  { kRootNetPanelId, kNetPanelId,    "netPanelId" },
  
  { kNetPanelId,     kProcListId,    "procListId" },
  { kProcListId,     kProcPanelId,   "procPanelId" },

  { kProcPanelId,    kProcInstLabelId,   "procInstLabel" },
  { kProcPanelId,    kProcInstSfxId,     "procInstSfxId" },
  { kProcPanelId,    kChanListId,        "chanListId" },

  { kChanListId,     kChanPanelId,   "chanPanelId" },

  { kChanPanelId,    kVarListId,     "varListId" },
  { kVarListId,      kVarPanelId,    "varPanelId" },

  { kVarPanelId,    kWidgetListId,  "widgetListId" },

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


rc_t _load_init_pgm_no_gui( app_t& app, const char* pgm_label, bool& exec_complete_fl_ref )
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

  if((rc = program_initialize(app.ioFlowH)) != kOkRC )
  {
    rc = cwLogError(rc,"Program initialize failed on '%s'.",cwStringNullGuard(pgm_label));
    goto errLabel;
  }

  // if the program is in NRT mode then run it
  if( is_program_nrt(app.ioFlowH) )
  {
    exec_complete_fl_ref = true;
    if((rc = exec_nrt(app.ioFlowH)) != kOkRC )
      goto errLabel;
  }
  

errLabel:
  return rc;
}


rc_t _on_pgm_select(app_t* app, unsigned pgmSelOptId )
{
  rc_t rc = kOkRC;

  unsigned pgm_idx          = kInvalidIdx;
  unsigned pgmPresetSelUuId = io::uiFindElementUuId( app->ioH, kPgmPresetSelId );
  unsigned pgmLoadBtnUuId   = io::uiFindElementUuId( app->ioH, kPgmLoadBtnId );
  unsigned preset_cnt       = 0;

  if( !(kPgmBaseSelId <= pgmSelOptId && pgmSelOptId <= kPgmMaxSelId ) )
  {
    rc = cwLogError(kInvalidStateRC,"An invalid pgm select id was encountered.");
    goto errLabel;
  }

  // empty the contents of the preset select menu
  if((rc = uiEmptyParent(app->ioH,pgmPresetSelUuId)) != kOkRC )
  {
    rc = cwLogError(rc,"The program preset menu clear failed.");
    goto errLabel;
  }

  uiSetEnable( app->ioH, pgmPresetSelUuId, false );  // Disable the preset menu and load btn in anticipation of error.
  uiSetEnable( app->ioH, pgmLoadBtnUuId,   false );  //
  app->pgm_preset_idx = kInvalidIdx;                 // The preset menu is empty and so there can be no valid preset selected.
  pgm_idx             = pgmSelOptId - kPgmBaseSelId; // Calc the ioFlowCtl preset index of the selected preset.

  // load the program
  if((rc = program_load(app->ioFlowH, pgm_idx )) != kOkRC )
  {
    rc = cwLogError(rc,"Program load failed.");
    goto errLabel;
  }

  preset_cnt = program_preset_count(app->ioFlowH);
  
  // populate the preset menu
  for(unsigned i=0; i<preset_cnt; ++i)
  {
    unsigned uuId;
    if((rc = uiCreateOption( app->ioH, uuId, pgmPresetSelUuId, nullptr, kPgmPresetBaseSelId+i, kInvalidId, "optClass", program_preset_title(app->ioFlowH,i) )) != kOkRC )
    {
      rc = cwLogError(rc,"Error on populating the program preset select widget.");
      goto errLabel;
    }
  }

  if( preset_cnt > 0 )
  {
    uiSetEnable( app->ioH, pgmPresetSelUuId, true );
    app->pgm_preset_idx = 0; // since it is showing - select the first preset as the default preset
  }
  
  uiSetEnable( app->ioH, pgmLoadBtnUuId, true );
  
errLabel:
  return rc;
}

rc_t _on_pgm_preset_select( app_t* app, unsigned pgmPresetSelOptId )
{
  rc_t rc = kOkRC;
  
  if( !(kPgmPresetBaseSelId <= pgmPresetSelOptId && pgmPresetSelOptId <= kPgmPresetMaxSelId ) )
  {
    rc = cwLogError(kInvalidStateRC,"An invalid pgm preset select id was encountered.");
    goto errLabel;
  }

  app->pgm_preset_idx = pgmPresetSelOptId - kPgmPresetBaseSelId;


errLabel:

  return rc;
}

rc_t _on_pgm_load(app_t* app )
{
  rc_t rc = kOkRC;
  
  if( !program_is_initialized( app->ioFlowH) )
  {
    const flow::ui_net_t* ui_net = nullptr;
  
    if((rc = program_initialize(app->ioFlowH, app->pgm_preset_idx )) != kOkRC )
    {
      rc = cwLogError(rc,"Network initialization failed.");
      goto errLabel;
    }

    if((ui_net = program_ui_net(app->ioFlowH)) == nullptr )
    {
      rc = cwLogError(rc,"Network UI description initialization failed.");
      goto errLabel;      
    }

    if((rc = caw::ui::create(app->uiH, app->ioH, app->ioFlowH, ui_net )) != kOkRC )
    {
      rc = cwLogError(rc,"Network UI create failed.");
      goto errLabel;            
    }

    uiSetEnable(app->ioH, io::uiFindElementUuId( app->ioH, kRunCheckId ), true );

  }
  
errLabel:

  return rc;
}

rc_t _on_pgm_run( app_t* app, bool run_check_fl )
{
  rc_t rc = kOkRC;

  app->run_fl = run_check_fl;

  return rc;
}

rc_t _on_ui_init( app_t* app )
{
  rc_t rc = kOkRC;
  unsigned pgmSelUuId = io::uiFindElementUuId( app->ioH, kPgmSelId );
  unsigned pgm_cnt = program_count(app->ioFlowH);
  
  // create pgm menu
  for(unsigned i=0; i<pgm_cnt; ++i)
  {
    unsigned uuId;
    if((rc = uiCreateOption( app->ioH, uuId, pgmSelUuId, nullptr, kPgmBaseSelId+i, kInvalidId, "optClass", program_title(app->ioFlowH,i) )) != kOkRC )
    {
      rc = cwLogError(rc,"Error on populating the program select widget.");
      goto errLabel;
    }

  }

  if( pgm_cnt )
    _on_pgm_select(app, kPgmBaseSelId );
  
errLabel:
  return rc;
}

template< typename T >
rc_t _on_variable_value( app_t* app, const io::ui_msg_t& m, T value )
{
  rc_t            rc     = kOkRC;
  flow::ui_var_t* ui_var = nullptr;
  unsigned        byteN  = sizeof(ui_var);
  
  if((rc = uiGetBlob(app->ioH,m.uuId, &ui_var,byteN)) != kOkRC )
  {
    rc = cwLogError(rc,"UI Blob access failed.");
    goto errLabel;
  }
  
  assert( byteN == sizeof(ui_var));

  if((rc = set_variable_value( app->ioFlowH, ui_var, value )) != kOkRC )
  {
    rc = cwLogError(rc,"Unable to access variable value.");
    goto errLabel;
  }
  
errLabel:
  if( rc != kOkRC )
  {
    rc = cwLogError(rc,"Echo failed for '%s:%i'-'%s:%i'.",cwStringNullGuard(ui_var->ui_proc->label),ui_var->ui_proc->label_sfx_id,cwStringNullGuard(ui_var->label),ui_var->label_sfx_id);
  }
  
  return rc;
}

rc_t _ui_value_callback(app_t* app, const io::ui_msg_t& m )
{
  rc_t rc = kOkRC;
  
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
      
    case kPgmSelId:
      _on_pgm_select(app,m.value->u.u);
      break;
      
    case kPgmPresetSelId:
      _on_pgm_preset_select(app,m.value->u.u);
      break;

    case kPgmLoadBtnId:
      _on_pgm_load(app);
      break;
      
    case kRunCheckId:
      io::uiSetTitle(app->ioH,m.uuId,m.value->u.b ? "Off" : "On" );
      _on_pgm_run(app,m.value->u.b);
      break;

    case kCheckWidgetId:
      rc = _on_variable_value(app,m,m.value->u.b);
      break;
      
    case kIntWidgetId:
      rc = _on_variable_value(app,m,(int)m.value->u.d);
      break;

    case kUIntWidgetId:
      rc = _on_variable_value(app,m,(unsigned)m.value->u.d);
      break;
      
    case kFloatWidgetId:
      rc = _on_variable_value(app,m,(float)m.value->u.d);
      break;
      
    case kDoubleWidgetId:
      rc = _on_variable_value(app,m,m.value->u.d);
      break;

    case kStringWidgetId:
      rc = _on_variable_value(app,m,m.value->u.s);
      break;
  }
  return rc;
}

template< typename T >
rc_t _on_variable_echo( app_t* app, const io::ui_msg_t& m )
{
  rc_t            rc     = kOkRC;
  T               value  = 0;
  flow::ui_var_t* ui_var = nullptr;
  unsigned        byteN  = sizeof(ui_var);
  
  if((rc = uiGetBlob(app->ioH,m.uuId, &ui_var,byteN)) != kOkRC )
  {
    rc = cwLogError(rc,"UI Blob access failed.");
    goto errLabel;
  }
  
  assert( byteN == sizeof(ui_var));

  if((rc = get_variable_value( app->ioFlowH, ui_var, value )) != kOkRC )
  {
    rc = cwLogError(rc,"Unable to access variable value.");
    goto errLabel;
  }

  if((rc = uiSendValue( app->ioH, m.uuId, value )) != kOkRC )
  {
    rc = cwLogError(rc,"Send value to UI failed.");
    goto errLabel;
  }
  
errLabel:
  if( rc != kOkRC )
  {
    rc = cwLogError(rc,"Echo failed for '%s:%i'-'%s:%i'.",cwStringNullGuard(ui_var->ui_proc->label),ui_var->ui_proc->label_sfx_id,cwStringNullGuard(ui_var->label),ui_var->label_sfx_id);
  }
  
  return rc;
}

rc_t _ui_echo_callback(app_t* app, const io::ui_msg_t& m )
{
  rc_t rc = kOkRC;
  
  switch( m.appId )
  {
    case kCheckWidgetId:
      rc = _on_variable_echo<bool>(app,m);
      break;
      
    case kIntWidgetId:
      rc = _on_variable_echo<int>(app,m);
      break;
      
    case kUIntWidgetId:
      rc = _on_variable_echo<unsigned>(app,m);
      break;
      
    case kFloatWidgetId:
      rc = _on_variable_echo<float>(app,m);
      break;

    case kDoubleWidgetId:
      rc = _on_variable_echo<double>(app,m);
      break;
      
    case kStringWidgetId:
      rc = _on_variable_echo<const char*>(app,m);
      break;
  }
  
  return rc;
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
      if( app->cmd_line_action_id == kUiSelId )
        _on_ui_init(app);
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
      {
        bool executable_fl = is_executable(app->ioFlowH);

        // if the app is executable and we are in 'run' mode
        if(app->run_fl && executable_fl  && m != nullptr )
        {
          io_flow_ctl::exec(app->ioFlowH,*m);
        }
        else
        {
          if( m != nullptr )
            for(unsigned i=0; i<m->u.audio->oBufChCnt; ++i)
              vop::zero(m->u.audio->oBufArray[i],m->u.audio->dspFrameCnt);
        }
        
        // if the app is not executable then we should exit run mode
        if(app->run_fl && !executable_fl )
        {
          app->run_fl = false;
          uiSendValue(app->ioH, io::uiFindElementUuId( app->ioH, kRunCheckId ), false );
        }
        
      }
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
    "       caw ui        <program_cfg_fname> {<program_label>} : Run with a GUI.\n"
    "       caw exec      <program_cfg_fname> <program_label>   : Run without a GUI.\n"
    "       caw hw_report <program_cfg_fname>                   : Print the hardware details and exit.\n"
    "       caw test      <test_cfg_fname> (<module_label> | all) (<test_label> | all) (compare | echo | gen_report )* {args ...}\n"
    "       caw test_stub ...\n";
    
  cwLogPrint(usage);
  
}

rc_t _parse_main_cfg( app_t& app, int argc, char* argv[] )
{
  rc_t rc = kOkRC;
  const char* io_cfg_fn = nullptr;
  
  if( argc < 2 )
  {
    rc = kInvalidArgRC;
    _print_command_line_help();
    goto errLabel;
  }

  // Get the selector-id associated with the first command line arg.  (e.g. exec, hw_report, ...)
  if((app.cmd_line_action_id = labelToId( appSelA, argv[1], kInvalidId )) == kInvalidId )
  {
    rc = cwLogError(kInvalidArgRC,"The command line action '%s' is not valid.",argv[1]);
    _print_command_line_help();
  }

  // if 'ui,'exec' or 'hw_report' was selected
  if( app.cmd_line_action_id==kUiSelId || app.cmd_line_action_id == kExecSelId || app.cmd_line_action_id == kHwReportSelId )
  {
    if( argc < 3 )
    {
      rc = cwLogError(kInvalidArgRC,"The command line is missing required arguments.");
      _print_command_line_help();      
    }

    // store the pgm cfg fname
    app.cmd_line_pgm_fname  = argv[2];
  
    if( app.cmd_line_pgm_fname == nullptr )
    {
      rc = cwLogError(kInvalidArgRC,"No 'caw' program cfg file was provided.");
      goto errLabel;
    }
      
    // parse the cfg. file
    if((rc = objectFromFile(app.cmd_line_pgm_fname,app.flow_cfg)) != kOkRC )
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

    // get the fourth cmd line arg (pgm label of the program to run from the cfg file in arg[2])
    if( argc >= 4 )
      app.cmd_line_pgm_label = argv[3];
    
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
  app_t app = { .pgm_preset_idx=kInvalidIdx }; // all zero except ...
  bool exec_complete_fl = false;
  
  cw::log::createGlobal();

  unsigned appIdMapN = sizeof(appIdMapA)/sizeof(appIdMapA[0]);

  if((rc= _parse_main_cfg(app, argc, argv )) != kOkRC )
    goto errLabel;

  switch( app.cmd_line_action_id )
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

  switch( app.cmd_line_action_id )
  {
    case kHwReportSelId:
      hardwareReport(app.ioH);
      goto errLabel;
      break;

    case kExecSelId:
      // the pgm to exec must have given
      if( app.cmd_line_pgm_label == nullptr )
      {
        rc = cwLogError(kInvalidArgRC,"The label of the program to execute from %s was not given.",cwStringNullGuard(app.cmd_line_pgm_fname));
        goto errLabel;
      }
      
      // load the requested program - and execute it if it is in or non-GUI non-real-time mode
      if((rc = _load_init_pgm_no_gui(app, app.cmd_line_pgm_label, exec_complete_fl )) != kOkRC )
      {
        goto errLabel;
      }
      break;

  }

  // if we are here then then program 'ui' or 'exec' was requested
  assert( app.cmd_line_action_id==kExecSelId || app.cmd_line_action_id==kUiSelId );
    
  // non-real-time programs will have already executed
  if( !exec_complete_fl )
    _io_main(app);


errLabel:

  if( app.uiH.isValid() )
    if((rc = caw::ui::destroy(app.uiH)) != kOkRC )
      rc = cwLogError(rc,"UI destroy failed.");
  
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


