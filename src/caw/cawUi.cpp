//| Copyright: (C) 2020-2024 Kevin Larke <contact AT larke DOT org> 
//| License: GNU GPL version 3.0 or above. See the accompanying LICENSE file.
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
#include "cwMtx.h"
#include "cwDspTypes.h" // real_t, sample_t
#include "cwTime.h"
#include "cwMidiDecls.h"

#include "cwFlowDecl.h"
#include "cwFlowValue.h"
#include "cwFlowTypes.h"
#include "cwFlow.h"
#include "cwIoFlowCtl.h"

#include "cawUiDecl.h"
#include "cawUi.h"

using namespace cw;

namespace caw {

  namespace ui  {

    
    typedef struct ui_str
    {
      io::handle_t              ioH;
      io_flow_ctl::handle_t     ioFlowH;
      const flow::ui_net_t*     ui_net;
      unsigned                  ui_net_idx;
      
    } ui_t;


    ui_t* _handleToPtr( handle_t h )
    { return handleToPtr<handle_t,ui_t>(h); }

    rc_t _destroy( ui_t*& p )
    {
      rc_t rc = kOkRC;

      if( p != nullptr && p->ioH.isValid() )
      {
        unsigned netPanelUuId   = io::uiFindElementUuId( p->ioH, kRootNetPanelId );
        unsigned netListUuId    = io::uiFindElementUuId( p->ioH, netPanelUuId, kNetListId, kInvalidId );

        if( netListUuId != kInvalidId )
          uiEmptyParent( p->ioH, netListUuId);

      }
      
      mem::release(p);
      return rc;
    }

    rc_t _create_bool_widget( ui_t* p, unsigned widgetListUuId, const flow::ui_var_t* ui_var, const char* title, unsigned& uuid_ref )
    {
      rc_t rc = kOkRC;
      if((rc = uiCreateCheck(p->ioH, uuid_ref, widgetListUuId, nullptr, kCheckWidgetId, kInvalidId, nullptr, title )) != kOkRC )
      {
        rc = cwLogError(rc,"Check box widget create failed on '%s:%i'.",cwStringNullGuard(ui_var->label),ui_var->label_sfx_id);
      }
    
      return rc;
    }

    rc_t _create_button_widget( ui_t* p, unsigned widgetListUuId, const flow::ui_var_t* ui_var, unsigned& uuid_ref )
    {
      rc_t rc = kOkRC;
      
      if((rc = uiCreateButton(p->ioH, uuid_ref, widgetListUuId, nullptr, kButtonWidgetId, kInvalidId, nullptr, ui_var->label )) != kOkRC )
      {
        rc = cwLogError(rc,"Check box widget create failed on '%s:%i'.",cwStringNullGuard(ui_var->label),ui_var->label_sfx_id);
      }
    
      return rc;
    }
    
    rc_t _create_number_widget( ui_t* p, unsigned widgetListUuId, const flow::ui_var_t* ui_var, unsigned appId, unsigned value_type_flag, const char* title, unsigned& uuid_ref, double min_val, double max_val, double step,  unsigned dec_pl )
    {
      rc_t rc;
      
      if((rc = uiCreateNumb( p->ioH, uuid_ref, widgetListUuId, nullptr, appId, kInvalidId, nullptr, title, min_val, max_val, step, dec_pl )) != kOkRC )
      {
        rc = cwLogError(rc,"Integer widget create failed on '%s:%i'.",cwStringNullGuard(ui_var->label),ui_var->label_sfx_id);
        goto errLabel;
      }

    errLabel:
      return rc;
    }

    rc_t _create_string_widget( ui_t* p, unsigned widgetListUuId, const flow::ui_var_t* ui_var, const char* title, unsigned& uuid_ref )
    {
      rc_t rc = kOkRC;
      if((rc = uiCreateStr( p->ioH, uuid_ref, widgetListUuId, nullptr, kStringWidgetId, kInvalidId, nullptr, title )) != kOkRC )
      {
        rc = cwLogError(rc,"String widget create failed on '%s:%i'.",cwStringNullGuard(ui_var->label),ui_var->label_sfx_id);
        goto errLabel;        
      }

    errLabel:
      return rc;
    }

    rc_t _create_meter_widget( ui_t* p, unsigned widgetListUuId, const flow::ui_var_t* ui_var, const char* title, unsigned& uuid_ref, double min_val, double max_val )
    {
      rc_t rc = kOkRC;

      if((rc = uiCreateProg(p->ioH, uuid_ref, widgetListUuId, nullptr, kMeterWidgetId, kInvalidId, nullptr, title, min_val, max_val )) != kOkRC )
      {
        rc = cwLogError(rc,"Meter widget create failed on '%s:%i'.",cwStringNullGuard(ui_var->label),ui_var->label_sfx_id);
        goto errLabel;
      }

    errLabel:
      return rc;
    }

    
    rc_t _create_list_widget( ui_t* p, unsigned widgetListUuId, const flow::ui_var_t* ui_var, const char* title, unsigned& uuid_ref )
    {
      rc_t rc = kOkRC;

      if(ui_var->list == nullptr )
      {
        rc = cwLogError(rc,"There is no list data for the list widget '%s:%i'.",cwStringNullGuard(ui_var->label),ui_var->label_sfx_id);
        goto errLabel;
      }

      if((rc = uiCreateSelect(  p->ioH, uuid_ref, widgetListUuId, nullptr, kListWidgetId, kInvalidId, nullptr, title )) != kOkRC )
      {
        rc = cwLogError(rc,"List widget create failed on '%s:%i'.",cwStringNullGuard(ui_var->label),ui_var->label_sfx_id);
        goto errLabel;
      }

      for(unsigned i=0; i<ui_var->list->eleN; ++i)
      {
        unsigned uuid   = 0;
        unsigned chanId = i;
        unsigned appId  = i;
        
        if((rc = uiCreateOption( p->ioH, uuid, uuid_ref, NULL, appId, chanId, nullptr, list_ele_label(ui_var->list,i) )) != kOkRC )
        {
          rc = cwLogError(rc,"List option create failed on '%s:%i' index '%i'.",cwStringNullGuard(ui_var->label),ui_var->label_sfx_id,i);
          goto errLabel;
        }
      }
      
    errLabel:
      return rc;
    }
    
    rc_t _create_var_label( ui_t* p, unsigned varLabelUuId, const flow::ui_var_t* ui_var, const char* label, unsigned var_idx )
    {
      rc_t rc = kOkRC;
      
      if((rc = uiSetTitle( p->ioH, varLabelUuId, label)) != kOkRC )
      {
        rc = cwLogError(rc,"Label widget create failed on '%s'.",cwStringNullGuard(label));
        goto errLabel;                
      }

    errLabel:
      return rc;      
    }

    rc_t _get_widget_type_id( const flow::ui_var_t* ui_var, unsigned& widget_type_id_ref )
    {
      rc_t            rc                = kOkRC;
      unsigned        value_type_flag   = ui_var->value_tid & flow::kTypeMask;
      const char*     widget_type_label = nullptr;
      
      idLabelPair_t   typeA[] = {
        { kMeterWidgetId, "meter" },
        { kListWidgetId,  "list" },
        { kInvalidId, nullptr }
      };

      widget_type_id_ref  = kInvalidId;
      
      switch( value_type_flag  )
      {
        case flow::kAllTFl:    widget_type_id_ref = kButtonWidgetId; break;
        case flow::kBoolTFl:   widget_type_id_ref = kCheckWidgetId;  break;
        case flow::kIntTFl:    widget_type_id_ref = kIntWidgetId;    break;
        case flow::kUIntTFl:   widget_type_id_ref = kUIntWidgetId;   break;
        case flow::kFloatTFl:  widget_type_id_ref = kFloatWidgetId;  break;
        case flow::kDoubleTFl: widget_type_id_ref = kDoubleWidgetId; break;
        case flow::kStringTFl: widget_type_id_ref = kStringWidgetId; break;          
      }

      if( ui_var->ui_cfg != nullptr )
      {
        // get the type of this 'ui' widget
        if((rc = ui_var->ui_cfg->getv("type",widget_type_label)) != kOkRC )
        {
          rc = cwLogError(rc,"Error parsing variable 'ui' type cfg.");
          goto errLabel;
        }

        // 
        if((widget_type_id_ref = labelToId(typeA,widget_type_label,kInvalidId)) == kInvalidId )
        {
          rc = cwLogError(rc,"An unknown widget type '%s' was encountered.",widget_type_label);
          goto errLabel;
        }
      }

      if( widget_type_id_ref == kInvalidId )
      {
        rc = cwLogError(kInvalidArgRC,"Unknown widget type for variable %s:%i.",cwStringNullGuard(ui_var->label),ui_var->label_sfx_id);
        goto errLabel;
      }

      
    errLabel:
      return rc;
    }
    
    
    rc_t _create_var_ui( ui_t* p, unsigned widgetListUuId, const flow::ui_var_t* ui_var, unsigned var_idx, unsigned container_uuId, unsigned var_label_uuId )
    {

      rc_t                       rc             = kOkRC;
      const char*                title          = nullptr;
      unsigned                   widget_type_id = kInvalidId;
      unsigned                   widget_uuId    = kInvalidId;
      io_flow_ctl::io_var_arg_t* io_var_arg     = nullptr;
      
      
      // if this is a blank panel 
      if( ui_var == nullptr )
        goto errLabel;

      // get the type of this widget
      if((rc= _get_widget_type_id( ui_var, widget_type_id )) != kOkRC )
      {
        goto errLabel;
      }

      switch( widget_type_id )
      {
        case kCheckWidgetId:
          rc = _create_bool_widget(p,widgetListUuId,ui_var,title,widget_uuId);
          break;

        case kButtonWidgetId:
          rc = _create_button_widget(p,widgetListUuId,ui_var,widget_uuId);
          break;
          
        case kIntWidgetId:
          {
            int min_val = std::numeric_limits<int>::min();
            int max_val = std::numeric_limits<int>::max();
            rc = _create_number_widget(p,widgetListUuId,ui_var, kIntWidgetId, flow::kIntTFl, title, widget_uuId, min_val, max_val, 1, 1 );
          }
          break;          
          
        case kUIntWidgetId:
          {
            unsigned min_val = 0;
            unsigned max_val = std::numeric_limits<unsigned>::max();
            rc = _create_number_widget(p,widgetListUuId,ui_var, kUIntWidgetId, flow::kUIntTFl, title,widget_uuId, min_val, max_val, 1, 1 );
          }
          break;

        case kFloatWidgetId:
          {
            float min_val = std::numeric_limits<float>::min();
            float max_val = std::numeric_limits<float>::max();
            rc = _create_number_widget(p,widgetListUuId,ui_var, kFloatWidgetId, flow::kFloatTFl, title, widget_uuId, min_val, max_val, 0.1, 2 );
          }
          break;
          
        case kDoubleWidgetId:
          {
            double min_val = std::numeric_limits<float>::min();
            double max_val = std::numeric_limits<float>::max();
            rc = _create_number_widget(p,widgetListUuId,ui_var, kDoubleWidgetId, flow::kDoubleTFl, title, widget_uuId, min_val, max_val, 0.1, 2 );
          }
          break;

        case kStringWidgetId:
          rc = _create_string_widget(p,widgetListUuId,ui_var,title, widget_uuId);
          break;          

        case kMeterWidgetId:
          {
            double min_val = -100.0;
            double max_val = 0;
            rc = _create_meter_widget(p,widgetListUuId,ui_var,title,widget_uuId,min_val,max_val);
          }
          break;

        case kListWidgetId:
          rc = _create_list_widget(p,widgetListUuId,ui_var,title,widget_uuId);
          break;          
          
        default:
          {            
            rc = cwLogError(rc,"Invalid widget type for variable %s:%i.",cwStringNullGuard(ui_var->label),ui_var->label_sfx_id);
            goto errLabel;

          }
      }

      if( widget_uuId != kInvalidId )
      {
        uiSetBlob(p->ioH,widget_uuId, &ui_var, sizeof(&ui_var));

        // if this is a 'init' variable or connected to a source variable then disable it
        // (The UI should not be able to change the value of a var. that is being set by a source in the network.)
        if( ui_var->disable_fl)
        {
          uiClearEnable(p->ioH, widget_uuId );
        }

        // Set the user-arg value in 
        io_var_arg = mem::allocZ<io_flow_ctl::io_var_arg_t>();
        io_var_arg->container_uuid = container_uuId;
        io_var_arg->label_uuid     = var_label_uuId;
        io_var_arg->widget_uuid    = widget_uuId;
          
        if((rc = set_variable_user_arg( p->ioFlowH, ui_var, io_var_arg )) != kOkRC )
          goto errLabel;

        // if the var is iniitally hidden
        if( ui_var->hide_fl )          
          uiSetVisible(p->ioH, container_uuId, false );
        
        // if the var is initially disabled
        if( ui_var->disable_fl )
        {
          uiSetEnable(p->ioH, widget_uuId, false );
          uiSetEnable(p->ioH, var_label_uuId, false );
        }
        
      }

    errLabel:
      if(rc != kOkRC )
        rc = cwLogError(rc,"Variable UI creation failed on %s:%i.",cwStringNullGuard(ui_var->label),ui_var->label_sfx_id);
      
      return rc;
      
    }

    // Returns 0 if all var's on on ch_idx == kInvalidId
    // otherwise returns channel count (1=mono,2=stereo, ...)
    unsigned _calc_max_chan_count( const flow::ui_proc_t* ui_proc )
    {
      unsigned n = 0;
      for(unsigned i=0; i<ui_proc->varN; ++i)
        if( !(ui_proc->varA[i].desc_flags & flow::kUiCreateVarDescFl) && ui_proc->varA[i].ch_cnt != kInvalidCnt && ui_proc->varA[i].ch_cnt > n )
          n = ui_proc->varA[i].ch_cnt;

      return n==0 ? 1 : n;
    }

    rc_t _load_proc_presets(ui_t* p, const flow::ui_proc_t* ui_proc, unsigned procPanelUuId)
    {
      rc_t rc = kOkRC;

      unsigned selUuId =  uiFindElementUuId( p->ioH, procPanelUuId, kProcPresetSelId, kInvalidId );

      if( ui_proc->desc->presetN == 0 )
      {
        uiClearEnable(p->ioH, selUuId );
        uiClearVisible(p->ioH, selUuId  );        
      }
        
      for(unsigned i=0; i<ui_proc->desc->presetN; ++i)
      {
        unsigned uuId;
        if((rc = uiCreateOption(p->ioH, uuId, selUuId, nullptr, kProcPresetOptId, i, nullptr, ui_proc->desc->presetA[i].label )) != kOkRC )
        {
          rc = cwLogError(rc,"UI option '%s' for a preset list create failed.", cwStringNullGuard(ui_proc->desc->presetA[i].label));
          break;
        }

      }
      return rc;
    }

    rc_t _create_var_list( ui_t* p, unsigned varListUuId, const flow::ui_proc_t* ui_proc )
    {
      rc_t           rc              = kOkRC;
      const unsigned label_buf_charN = 127;
      char           label_buf[ label_buf_charN+1 ];

      // for each var
      for(unsigned i=0; i<ui_proc->varN; ++i)
      {
        flow::ui_var_t* ui_var         = ui_proc->varA + i;
        unsigned        var_mult_cnt   = var_mult_count( ui_proc->proc, ui_var->label);
        unsigned        ch_cnt         = 0;
        unsigned        varUuId        = kInvalidId;
        unsigned        widgetListUuId = kInvalidId;
        unsigned        varLabelUuId   = kInvalidId;
        
        // if the ui for this var is never created via the 'no_ui' attribute in the var class description
        if( ui_var->desc_flags & flow::kUiCreateVarDescFl )
          continue;

        // only create controls for certain value types
        if( !(ui_var->value_tid & (flow::kBoolTFl | flow::kIntTFl | flow::kUIntTFl | flow::kFloatTFl | flow::kDoubleTFl | flow::kStringTFl )))
          continue;

        switch( ui_var->ch_cnt )
        {
          case 0:           ch_cnt=1; break; // only contains one channel where ui_var->ch_idx==kAnyChIdx
          case kInvalidCnt: ch_cnt=0; break; // no-channels
          default:
            ch_cnt = ui_var->ch_cnt; // 1=mono, 2=stereo ..
           
        }
                
        // insert the var panel
        if((rc = uiCreateFromRsrc( p->ioH, "var", varListUuId, i )) != kOkRC )
        {
          goto errLabel;
        }

        varUuId = uiFindElementUuId(p->ioH, varListUuId, kVarPanelId, i );

        // Get the var label and the widget list UUid's
        varLabelUuId   = uiFindElementUuId(p->ioH, varUuId, kVarLabelId,   kInvalidId );
        widgetListUuId = uiFindElementUuId(p->ioH, varUuId, kWidgetListId, kInvalidId );

        label_buf[0] = '\0';
        
        for(unsigned ch_idx = 0; ch_idx<ch_cnt; ++ch_idx)
        {
          if( ch_idx == 0 )
          {
            if( var_mult_cnt <= 1 )
              snprintf(label_buf,label_buf_charN,"%s",ui_var->title);
            else
              snprintf(label_buf,label_buf_charN,"%s:%i",ui_var->title,ui_var->label_sfx_id);

            // create the var label
            if((rc = _create_var_label(p, varLabelUuId, ui_var, label_buf, i)) != kOkRC )
            {
              goto errLabel;
            }
          }

          // create the var widget
          if((rc = _create_var_ui(p, widgetListUuId, ui_var, i, varUuId, varLabelUuId)) != kOkRC )
          {
            goto errLabel;
          }
        }

      }
      
      errLabel:
        return rc;
    }
    
    
    rc_t _create_net_ui( ui_t* p, unsigned netParentUuId, const flow::ui_net_t* ui_net, const char* label_prefix );

    rc_t _create_proc_ui( ui_t* p, unsigned netParentUuId, unsigned parentListUuId, const flow::ui_proc_t* ui_proc, unsigned proc_idx )
    {
      rc_t           rc              = kOkRC;
      unsigned       procPanelUuId   = kInvalidId;
      unsigned       varListPanelUuId = kInvalidId;
      const unsigned label_buf_charN = 127;
      char           label_buf[ label_buf_charN+1 ];

      if( !p->ui_net->ui_create_fl && !(ui_proc->proc->flags & flow::kUiCreateProcFl) )
        goto errLabel;
      
      if((rc = uiCreateFromRsrc(   p->ioH, "proc", parentListUuId, proc_idx )) != kOkRC )
      {
        goto errLabel;
      }

      procPanelUuId    = uiFindElementUuId( p->ioH, parentListUuId, kProcPanelId,   proc_idx );
      varListPanelUuId = uiFindElementUuId( p->ioH, procPanelUuId, kVarListPanelId,   kInvalidId );
      
      // set the proc title
      snprintf(label_buf,label_buf_charN,"%s %s:%i",ui_proc->desc->label,ui_proc->label,ui_proc->label_sfx_id);      
      uiSendValue( p->ioH, uiFindElementUuId(p->ioH, procPanelUuId, kProcInstLabelId, kInvalidId), label_buf );

      //if((rc = _load_proc_presets(p,ui_proc,procPanelUuId)) != kOkRC )
      //{
      //}

      if((rc = _create_var_list(p, varListPanelUuId, ui_proc )) != kOkRC )
        goto errLabel;

      if( ui_proc->internal_net )
      {
        flow::ui_net_t* ui_net = ui_proc->internal_net;
        for(; ui_net!=nullptr; ui_net=ui_net->poly_link)
          if((rc = _create_net_ui(p,netParentUuId,ui_net,label_buf)) != kOkRC )
          {
            rc = cwLogError(rc,"Internal net UI create failed.");
            goto errLabel;
          }
      }

    errLabel:
      if(rc != kOkRC )
        rc = cwLogError(rc,"Processor UI creation failed on %s:%i.",cwStringNullGuard(ui_proc->label),ui_proc->label_sfx_id);
      
      return rc;
      
    }

    rc_t _create_net_ui( ui_t* p, unsigned netListUuId, const flow::ui_net_t* ui_net, const char* title=nullptr )
    {
      rc_t rc = kOkRC;

      unsigned       netPanelUuId    = kInvalidId;
      unsigned       netTitleUuId    = kInvalidId;
      unsigned       procListUuId    = kInvalidId;

      if((rc = uiCreateFromRsrc(   p->ioH, "network", netListUuId, p->ui_net_idx )) != kOkRC )
      {
        goto errLabel;
      }

      netPanelUuId =  uiFindElementUuId( p->ioH, netListUuId, kNetPanelId, p->ui_net_idx++ );
      netTitleUuId =  uiFindElementUuId( p->ioH, netPanelUuId, kNetTitleId, kInvalidId );
      procListUuId =  uiFindElementUuId( p->ioH, netPanelUuId, kProcListId, kInvalidId );

      //printf("netlist:%i title:%i netpanel:%i proclist:%i : poly_idx:%i\n",netListUuId, netTitleUuId, netPanelUuId, procListUuId, ui_net->poly_idx);

      if( title != nullptr )
      {
        
        const unsigned label_buf_charN = 127;
        char           label_buf[ label_buf_charN+1 ];
        snprintf(label_buf,label_buf_charN,"Network: %s:%i",title,ui_net->poly_idx);
        uiSendValue( p->ioH, netTitleUuId, label_buf);
        
      }
      
      for(unsigned i=0; i<ui_net->procN; ++i)
      {
        if((rc = _create_proc_ui( p, netListUuId, procListUuId, ui_net->procA+i, i )) != kOkRC )
        {
          goto errLabel;
        }
      }
      
    errLabel:

      if(rc != kOkRC )
        rc = cwLogError(rc,"Network UI creation failed.");
      
      return rc;
    }
    
    
  }
}


cw::rc_t caw::ui::create( handle_t&             hRef,
                          io::handle_t          ioH,
                          io_flow_ctl::handle_t ioFlowH,
                          const flow::ui_net_t* ui_net)
{
  rc_t rc = kOkRC;
  if((rc = destroy(hRef)) != kOkRC )
    return rc;
  else
  {
  
    ui_t* p = mem::allocZ<ui_t>();
    p->ioH     = ioH;
    p->ioFlowH = ioFlowH;
    p->ui_net  = ui_net;
    
    unsigned netPanelUuId   = io::uiFindElementUuId( ioH, kRootNetPanelId );
    unsigned netListUuId    = io::uiFindElementUuId( ioH, netPanelUuId, kNetListId, kInvalidId );

    if((rc = _create_net_ui( p, netListUuId, ui_net )) != kOkRC )
    {
      rc = cwLogError(rc,"UI create failed.");
      goto errLabel;
    }

    hRef.set(p);
  }

  

errLabel:  
  return rc;
}

cw::rc_t caw::ui::destroy( handle_t& hRef )
{
  rc_t rc = kOkRC;
  ui_t* p = nullptr;
  
  if(!hRef.isValid())
    return rc;

  p = _handleToPtr(hRef);
  
  if((rc = _destroy(p)) != kOkRC )
  {
    rc = cwLogError(rc,"UI destroy failed.");
  }

  hRef.clear();
  
  return rc;  
}
