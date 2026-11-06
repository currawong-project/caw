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
#include "cwIdTable.h"

#include "cwVectOps.h"
#include "cwMtx.h"
#include "cwDspTypes.h" // real_t, sample_t
#include "cwTime.h"
#include "cwMidiDecls.h"

#include "cwFlowDecl.h"
#include "cwFlowValue.h"
#include "cwFlowRecd.h"
#include "cwFlowTypes.h"
#include "cwFlow.h"
#include "cwIoFlowCtl.h"

#include "cawUiDecl.h"
#include "cawUi.h"

using namespace cw;

namespace caw {

  namespace ui  {

    enum {
      kColVarLayoutId,
      kRowVarLayoutId
    };
    
    typedef struct ui_str
    {
      io::handle_t              ioH;
      io_flow_ctl::handle_t     ioFlowH;
      const flow::ui_net_t*     ui_net;
      unsigned                  ui_net_idx;
      unsigned                  ui_proc_idx;  // Next proc chanId. Unique across all networks.
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

      const char* title = ui_var->label;

      if( ui_var->btn_title != nullptr )
        title = ui_var->btn_title;
      else
      {
        if( ui_var->title != nullptr && ui_var->title_to_label_fl )
          title = ui_var->title;
      }
      
      if((rc = uiCreateButton(p->ioH, uuid_ref, widgetListUuId, nullptr, kButtonWidgetId, kInvalidId, nullptr, title )) != kOkRC )
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

    rc_t _create_status_widget( ui_t* p, unsigned widgetListUuId, const flow::ui_var_t* ui_var, const char* title, unsigned& uuid_ref )
    {
      rc_t            rc      = kOkRC;
      const object_t* statusD = nullptr;
      unsigned statusN = 0;
      
      if( ui_var->ui_cfg == nullptr )
      {
        rc = cwLogError(kInvalidArgRC,"No status array was provided in the var. desc. 'ui:' stanza.");
        goto errLabel;
      }
      
      if((rc = ui_var->ui_cfg->getv("status",statusD)) != kOkRC )
      {
        rc = cwLogError(rc,"The var. desc. 'ui:' stanza has no 'status' dictionary.");
        goto errLabel;
      }

      statusN = statusD->child_count();

      if((rc = uiCreateStatus( p->ioH, uuid_ref, widgetListUuId, nullptr, kStatusWidgetId, kInvalidId, nullptr, title )) != kOkRC )
      {
        rc = cwLogError(rc,"Status widget create failed on '%s:%i'.",cwStringNullGuard(ui_var->label),ui_var->label_sfx_id);
        goto errLabel;        
      }

      for(unsigned i=0; i<statusD->child_count(); ++i)
      {
        const char* label = nullptr;
        const char* color = nullptr;
        unsigned    value = 0;

        if((rc = statusD->child_ele(i)->getv("label",label,
                                             "color",color,
                                             "value",value)) != kOkRC )
        {
          rc = cwLogError(rc,"The 'status' array element at index '%i' could not be parsed.",i);
          goto errLabel;
        }

        if((rc = uiAttachStatusState(p->ioH, uuid_ref, label, value, color )) != kOkRC )
        {
          rc = cwLogError(rc,"'status' value update at index '%i' update failed.",i);
          goto errLabel;
        }

      }
      
    errLabel:
      if(rc != kOkRC )
        rc = cwLogError(rc,"The 'status' widget creation failed on '%s:%i'.",cwStringNullGuard(ui_var->label),ui_var->label_sfx_id);
      
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
        { kListWidgetId, "list" },
        { kStatusWidgetId, "status" },
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

    rc_t _get_var_desc_add_class_name( const flow::ui_var_t* ui_var, const char*& add_class_name_ref )
    {
      rc_t rc = kOkRC;
      
      if( ui_var->ui_cfg != nullptr )
      {
        // get the layout label from the cfg
        if((rc = ui_var->ui_cfg->getv_opt("add_class",add_class_name_ref)) != kOkRC )
        {
          rc = cwLogError(rc,"Error parsing variable 'ui' additional class name cfg.");
          goto errLabel;
        }

      }
    errLabel:
      return rc;
      
    }
    
    rc_t _get_var_layout_id( const flow::ui_var_t* ui_var, unsigned& layout_id_ref )
    {
      rc_t        rc           = kOkRC;
      const char* layout_label = nullptr;
      
      idLabelPair_t   typeA[] = {
        { kColVarLayoutId, "col" },
        { kRowVarLayoutId, "row" },
        { kInvalidId, nullptr }
      };

      // var layout will be a row by default
      layout_id_ref  = kRowVarLayoutId;
      
      if( ui_var->ui_cfg != nullptr )
      {
        // get the layout label from the cfg
        if((rc = ui_var->ui_cfg->getv_opt("layout",layout_label)) != kOkRC )
        {
          rc = cwLogError(rc,"Error parsing variable 'ui' layout cfg.");
          goto errLabel;
        }


        // if no layout was specified then use the 'row' layout
        if( layout_label != nullptr )
        {        
          //  convert the layout label to an id
          if((layout_id_ref = labelToId(typeA,layout_label,kInvalidId)) == kInvalidId )
          {
            rc = cwLogError(rc,"An unknown widget layout '%s' was encountered.",layout_label);
            goto errLabel;
          }
        }
      }

      // validate the layout label
      if( layout_id_ref == kInvalidId )
      {
        rc = cwLogError(kInvalidArgRC,"Unknown widget layout for variable %s:%i.",cwStringNullGuard(ui_var->label),ui_var->label_sfx_id);
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

        case kStatusWidgetId:
          rc = _create_status_widget(p,widgetListUuId,ui_var,title,widget_uuId);
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

    const flow::ui_var_t* _find_ui_var( const flow::ui_proc_t* ui_proc, const char* label, unsigned label_sfx_id, unsigned ch_idx )
    {
      for(unsigned i=0; i<ui_proc->varN; ++i)
      {
        const flow::ui_var_t* ui_var = ui_proc->varA + i;
        if( cw::textIsEqual(label,ui_var->label) && ui_var->label_sfx_id == label_sfx_id && ui_var->ch_idx == ch_idx )
          return ui_var;
          
      }
     
      return nullptr;
    }

    rc_t _create_var_list( ui_t* p, unsigned varListUuId, const flow::ui_proc_t* ui_proc )
    {
      rc_t           rc              = kOkRC;
      const unsigned label_buf_charN = 127;
      char           label_buf[ label_buf_charN+1 ];

      idLabelPair_t   layoutA[] = {
        { kColVarLayoutId, "uiCol" },
        { kRowVarLayoutId, "uiRow" },
        { kInvalidId, nullptr }
      };

      // for each var
      for(unsigned i=0; i<ui_proc->varN; ++i)
      {
        flow::ui_var_t* ui_var         = ui_proc->varA + i;
        unsigned        var_mult_cnt   = var_mult_count( ui_proc->proc, ui_var->label);
        unsigned        ch_cnt         = 0;
        unsigned        varUuId        = kInvalidId;
        unsigned        widgetListUuId = kInvalidId;
        unsigned        varLabelUuId   = kInvalidId;
        unsigned        varChDivUuId   = kInvalidId;
        const char*     add_class_name = nullptr;

        // 
        if( ui_var->ch_idx != flow::kAnyChIdx )
          continue;
        
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
                
        // insert the var panel which contains the label and widget list.
        if((rc = uiCreateFromRsrc( p->ioH, "var", varListUuId, i )) != kOkRC )
        {
          goto errLabel;
        }

        varUuId = uiFindElementUuId(p->ioH, varListUuId, kVarPanelId, i );

        // set the additional var desc class name for this variable
        if((rc = _get_var_desc_add_class_name( ui_var, add_class_name )) != kOkRC )
        {
          goto errLabel;
        }
        else
        {        
          if( add_class_name != nullptr )
            uiAppendClassName( p->ioH, varUuId, add_class_name );
        }

        // set the additional var instance class name for this variable
        if( textLength(ui_var->add_class) != 0 && textIsNotEqual(ui_var->add_class,add_class_name))
        {
          uiAppendClassName(p->ioH, varUuId, ui_var->add_class );
        }
        
        // Get the var label and the widget list UUid's
        varLabelUuId   = uiFindElementUuId(p->ioH, varUuId, kVarLabelId,   kInvalidId );
        widgetListUuId = uiFindElementUuId(p->ioH, varUuId, kWidgetListId, kInvalidId );

        varChDivUuId = uiPhysicalParentUuId(p->ioH, widgetListUuId );

        // if there is not var title then hide the parent title div also
        //if( cwIsFlag(ui_var->desc_flags,flow::kUiNoTitleVarDescFl ) )
        if( ui_var->no_title_fl )  
        {
          uiClearVisible( p->ioH, varLabelUuId );
        }
        else
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

          label_buf[0] = '\0';
        }
          
        for(unsigned ch_idx = 0; ch_idx<ch_cnt; ++ch_idx)
        {
          const flow::ui_var_t* ui_chan_var = ui_var;

          // if this is the var's first channel ...
          if( ch_idx == 0 )
          {
            // ... then determine if successive channels will be layed out as a column or row
            // (defaults to row, See the var desc for audio_meter for an example of a var which is layed out as a column)
            
            unsigned    layout_id = kRowVarLayoutId;
            const char* className = "uiRow";

            // get the requested layout from the var_desc->ui_cfg
            _get_var_layout_id( ui_var, layout_id);

            //  convert the layout id to a class label (e.g. uiCol,uiRow)
            if((className = idToLabel(layoutA,layout_id,kInvalidId)) == nullptr )
            {
              cwLogError(kAssertFailRC,"The lookup of layout id:%i failed.",layout_id);
              className = "uiRow";
            }

            // the var UI was created as a 'div' append an additional class to make it a row or column
            uiAppendClassName( p->ioH, varChDivUuId,  className );
            uiAppendClassName( p->ioH, widgetListUuId, className );

          }
          

          // if ui_var->ch_cnt == 0 only the kAnyChIdx channel exists (which is the var pointed to by ui_var)
          // otherwise we find a numbered channel var according to ch_idx
          if( ui_var->ch_cnt != 0 )
          {
            // Locate the ui_var associated with this channel
            if((ui_chan_var = _find_ui_var( ui_proc, ui_var->label, ui_var->label_sfx_id, ch_idx )) == nullptr )
            {
              rc = cwLogError(kInvalidStateRC,"The channel variable for '%s:%i' ch:%i could not be found.",cwStringNullGuard(ui_var->label),ui_var->label_sfx_id,ch_idx);
              goto errLabel;
            }
          }
          
          // create the var widget
          if((rc = _create_var_ui(p, widgetListUuId, ui_chan_var, i, varUuId, varLabelUuId)) != kOkRC )
          {
            goto errLabel;
          }
          
        }
      }
      
      errLabel:
        return rc;
    }
    
    
    rc_t _create_net_ui( ui_t* p, unsigned netParentUuId, const flow::ui_net_t* ui_net, const char* label_prefix );


    // Returns the name of the template element that holds child procs,
    // or nullptr if this proc is not a UI container.
    const char* _proc_ui_list_name( const flow::ui_proc_t* ui_proc )
    {
      return ui_proc->ui_list_name != nullptr ? ui_proc->ui_list_name : ui_proc->desc->ui_list_name;
    }

    bool _proc_is_container( const flow::ui_proc_t* ui_proc )
    { return _proc_ui_list_name(ui_proc)!=nullptr; }

    // Append the proc class and proc instance 'add_class' names to a UI element.
    void _append_proc_class_names( ui_t* p, const flow::ui_proc_t* ui_proc, unsigned uuId )
    {
      if( ui_proc->desc->add_class != nullptr )
        uiAppendClassName( p->ioH, uuId, ui_proc->desc->add_class );

      if( textLength(ui_proc->ui_add_class) != 0 && textIsNotEqual(ui_proc->ui_add_class,ui_proc->desc->add_class) )
        uiAppendClassName( p->ioH, uuId, ui_proc->ui_add_class );
    }
    
    // Find a UI parent in this proc's network, and then in each enclosing network.
    const flow::ui_proc_t* _find_ui_parent_proc( const flow::ui_proc_t* ui_proc )
    {
      const flow::ui_net_t* net = ui_proc->ui_net;

      while( net != nullptr )
      {
        unsigned sfx_id = ui_proc->ui_parent_sfx_id == kInvalidId ? net->poly_idx : ui_proc->ui_parent_sfx_id;

        for(unsigned i=0; i<net->procN; ++i)
          if( net->procA[i].label_sfx_id == sfx_id && textIsEqual(net->procA[i].label,ui_proc->ui_parent_label) )
            return net->procA + i;

        net = net->parent_ui_proc == nullptr ? nullptr : net->parent_ui_proc->ui_net;
      }

      return nullptr;
    }

    rc_t _create_proc_ui( ui_t* p, unsigned netParentUuId, unsigned parentListUuId, flow::ui_proc_t* ui_proc, unsigned proc_idx )
    {
      rc_t           rc               = kOkRC;
      unsigned       procPanelUuId    = kInvalidId;
      unsigned       varListPanelUuId = kInvalidId;
      const char*    ui_template      = "proc";
      const char*    proc_panel_name  = "procPanelId";
      const char*    ui_list_name     = nullptr;
      bool           is_container_fl  = false;
      unsigned       logParentUuId    = parentListUuId;
      const unsigned label_buf_charN  = 127;
      char           label_buf[ label_buf_charN+1 ];

      if( !p->ui_net->ui_create_fl && !(ui_proc->proc->flags & flow::kUiCreateProcFl) )
        goto errLabel;

      // The proc may specify an alternate ui.cfg template.
      // Proc instance values take precedence over proc class values.      
      if( ui_proc->ui_template != nullptr )
        ui_template = ui_proc->ui_template;
      else
      {
        if( ui_proc->desc->ui_template != nullptr )
          ui_template = ui_proc->desc->ui_template;
      }
      
      // If the proc specifies the name of the template element that holds child procs then this proc is a UI container.
      if( (ui_list_name  = _proc_ui_list_name(ui_proc)) != nullptr )
      {
        proc_panel_name = ui_list_name;
        is_container_fl = true;
      }

      // if the proc-inst UI specifies it's parent UI container
      if( ui_proc->ui_parent_label != nullptr )
      {
        const flow::ui_proc_t* parent_ui_proc = nullptr;

        // find the proc instance that represents the parent UI container
        if((parent_ui_proc = _find_ui_parent_proc(ui_proc)) == nullptr )
        {
          rc = cwLogError(kEleNotFoundRC,"The UI parent proc '%s' could not be found.",cwStringNullGuard(ui_proc->ui_parent_label));
          goto errLabel;
        }

        // Verify that the parent proc is a container.
        if( !_proc_is_container(parent_ui_proc) )
        {
          rc = cwLogError(kInvalidStateRC,"The UI parent proc '%s:%i' is not a UI container.",cwStringNullGuard(ui_proc->ui_parent_label), parent_ui_proc->label_sfx_id);
          goto errLabel;
        }
        
        // Verify that the parent proc already has a UI element.
        if( parent_ui_proc->ui_uuid == kInvalidId )
        {
          rc = cwLogError(kInvalidStateRC,"The UI parent proc '%s:%i' does not have a valid UI element. Perhaps it has not yet been created?",cwStringNullGuard(ui_proc->ui_parent_label), parent_ui_proc->label_sfx_id);
          goto errLabel;
        }

        parentListUuId = parent_ui_proc->ui_uuid;

        // Containers have no appId, so child panels are registered under the container's logical parent.
        if((logParentUuId = uiLogicalParentUuId( p->ioH, parentListUuId )) == kInvalidId )
        {
          rc = cwLogError(kEleNotFoundRC,"The UI logical parent UUId could not be determined for the parent '%s:%i'.",cwStringNullGuard(ui_proc->ui_parent_label), parent_ui_proc->label_sfx_id);
          goto errLabel;
        }
      }

      // create the 'proc-panel'
      if((rc = uiCreateFromRsrc(   p->ioH, ui_template, parentListUuId, proc_idx )) != kOkRC )
      {
        goto errLabel;
      }

      // get the proc panel to which a title and variable controls may be added
      if((procPanelUuId = uiFindElementUuId( p->ioH, logParentUuId, proc_panel_name, proc_idx )) == kInvalidId )
      {
        rc = cwLogError(kEleNotFoundRC,"The proc-panel-uuid could not be determined for the proc panel named '%s' proc index:%i.",cwStringNullGuard(proc_panel_name),proc_idx);
        goto errLabel;                        
      }

      // save the uuid of the new proc panel - if this is a container we will want it later
      ui_proc->ui_uuid = procPanelUuId;


      // format the default or custom title - we do this even when kUiTitleProcFl is not set to be sure it is available for _create_net_ui() below
      if( ui_proc->ui_title == nullptr )
        snprintf(label_buf,label_buf_charN,"%s %s:%i",cwStringNullGuard(ui_proc->desc->label),cwStringNullGuard(ui_proc->label),ui_proc->label_sfx_id);
      else
        snprintf(label_buf,label_buf_charN,"%s",ui_proc->ui_title);
      
      // set the proc title
      if( ui_proc->proc->flags & flow::kUiTitleProcFl )
      {
        // A container's label has no appId, so it is found by name under the same logical parent as its list.
        unsigned uuid = is_container_fl
          ? uiFindElementUuId(p->ioH, logParentUuId, "procInstLabel", proc_idx)
          : uiFindElementUuId(p->ioH, procPanelUuId, kProcInstLabelId, kInvalidId);
        
        if( uuid == kInvalidId )
        {
          rc = cwLogError(kEleNotFoundRC,"The label element for the title '%s' could not be found.",label_buf);
          goto errLabel;
        }
        
        uiSendValue( p->ioH, uuid, label_buf );
      }
      
      //if((rc = _load_proc_presets(p,ui_proc,procPanelUuId)) != kOkRC )
      //{
      //}

      if( is_container_fl )
      {
        // append the 'addClassName' to this container - procPanelUuId is the container's list element 
        _append_proc_class_names( p, ui_proc, procPanelUuId );
      }
      else
      {
        // get the var list uuid
        if((varListPanelUuId = uiFindElementUuId( p->ioH, procPanelUuId, kVarListPanelId,   kInvalidId )) == kInvalidId )
        {
          rc = cwLogError(kEleNotFoundRC,"The 'varListPanelUuid' could not be found.");
          goto errLabel;
        }

        // create th var list and fill it with var UI widgets
        if((rc = _create_var_list(p, varListPanelUuId, ui_proc )) != kOkRC )
          goto errLabel;

        // if an 'addClassName' exists for this proc then add it here
        _append_proc_class_names( p, ui_proc, varListPanelUuId );

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

      if((netPanelUuId =  uiFindElementUuId( p->ioH, netListUuId, kNetPanelId, p->ui_net_idx++ )) != kInvalidId )
      {
        cwLogError(kSyntaxErrorRC,"The net panel UI uuid lookup failed.");
        goto errLabel;
      }
      
      if((netTitleUuId =  uiFindElementUuId( p->ioH, netPanelUuId, kNetTitleId, kInvalidId )) != kInvalidId )
      {
        cwLogError(kSyntaxErrorRC,"The net title UI uuid lookup failed.");
        goto errLabel;
      }
      
      if((procListUuId =  uiFindElementUuId( p->ioH, netPanelUuId, kProcListId, kInvalidId )) != kInvalidId )
      {
        cwLogError(kSyntaxErrorRC,"The net panel proc-list uuid lookup failed.");
        goto errLabel;
      }

      //printf("netlist:%i title:%i netpanel:%i proclist:%i : poly_idx:%i\n",netListUuId, netTitleUuId, netPanelUuId, procListUuId, ui_net->poly_idx);

      if( title != nullptr )
      {
        
        const unsigned label_buf_charN = 127;
        char           label_buf[ label_buf_charN+1 ];
        snprintf(label_buf,label_buf_charN,"Network: %s:%i",title,ui_net->poly_idx);
        if((rc = uiSendValue( p->ioH, netTitleUuId, label_buf)) != kOkRC )
        {
          goto errLabel;
        }
        
      }
      
      for(unsigned i=0; i<ui_net->procN; ++i)
      {
        if((rc = _create_proc_ui( p, netListUuId, procListUuId, ui_net->procA+i, p->ui_proc_idx++ )) != kOkRC )
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
