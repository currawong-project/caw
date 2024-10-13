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
#include "cwFlowTypes.h"
#include "cwFlow.h"
#include "cwIoFlowCtl.h"

#include "cawUiDecl.h"
#include "cawUi.h"

using namespace cw;

namespace caw {

  namespace ui  {


    typedef struct ui_var_str
    {
      
    } ui_var_t;
     
    
    typedef struct ui_str
    {
      io::handle_t              ioH;
      io_flow_ctl::handle_t     ioFlowH;
      const flow::ui_net_t*     ui_net;
      
      
    } ui_t;


    ui_t* _handleToPtr( handle_t h )
    { return handleToPtr<handle_t,ui_t>(h); }

    rc_t _destroy( ui_t* p )
    {
      rc_t rc = kOkRC;
      mem::release(p);
      return rc;
    }

    rc_t _create_bool_widgets( ui_t* p, unsigned widgetListUuId, const flow::ui_var_t* ui_var, unsigned& uuid_ref )
    {
      rc_t rc = kOkRC;
      if((rc = uiCreateCheck(p->ioH, uuid_ref, widgetListUuId, nullptr, kCheckWidgetId, kInvalidId, nullptr, nullptr )) != kOkRC )
      {
        rc = cwLogError(rc,"Check box widget create failed on '%s'.",cwStringNullGuard(ui_var->label));
      }
    
      return rc;
    }

    rc_t _create_number_widget( ui_t* p, unsigned widgetListUuId, const flow::ui_var_t* ui_var, unsigned appId, unsigned value_type_flag, unsigned& uuid_ref, double min_val, double max_val, double step,  unsigned dec_pl )
    {
      rc_t rc;
      
      if((rc = uiCreateNumb( p->ioH, uuid_ref, widgetListUuId, nullptr, appId, kInvalidId, nullptr, nullptr, min_val, max_val, step, dec_pl )) != kOkRC )
      {
        rc = cwLogError(rc,"Integer widget create failed on '%s'.",cwStringNullGuard(ui_var->label));
        goto errLabel;
      }

    errLabel:
      return rc;
    }

    rc_t _create_string_widgets( ui_t* p, unsigned widgetListUuId, const flow::ui_var_t* ui_var, unsigned& uuid_ref )
    {
      rc_t rc = kOkRC;
      if((rc = uiCreateStr( p->ioH, uuid_ref, widgetListUuId, nullptr, kStringWidgetId, kInvalidId, nullptr, nullptr )) != kOkRC )
      {
        rc = cwLogError(rc,"String widget create failed on '%s'.",cwStringNullGuard(ui_var->label));
        goto errLabel;        
      }

    errLabel:
      return rc;
    }

    rc_t _create_var_label( ui_t* p, unsigned parentListUuId, const flow::ui_var_t* ui_var, const char* label, unsigned var_idx )
    {
      rc_t rc;
      unsigned uuId;
      if((rc = uiCreateStrDisplay( p->ioH, uuId, parentListUuId, nullptr, kInvalidId, kInvalidId, nullptr, label )) != kOkRC )
      {
        rc = cwLogError(rc,"Label widget create failed on '%s'.",cwStringNullGuard(ui_var->label));
        goto errLabel;        
        
      }

    errLabel:
      return rc;
      
    }
    
    rc_t _create_var_ui( ui_t* p, unsigned parentListUuId, const flow::ui_var_t* ui_var, unsigned var_idx )
    {

      rc_t rc = kOkRC;
      unsigned widgetListUuId  = kInvalidId;
      unsigned varUuId         = kInvalidId;
      unsigned value_type_flag = 0;
      unsigned widget_uuId     = kInvalidId;
      
      // insert the var panel
      if((rc = uiCreateFromRsrc(   p->ioH, "var", parentListUuId, var_idx )) != kOkRC )
      {
        goto errLabel;
      }

      // if this is a blank panel 
      if( ui_var == nullptr )
        goto errLabel;
        
      value_type_flag = ui_var->value_tid & flow::kTypeMask;
      
      varUuId        = uiFindElementUuId( p->ioH, parentListUuId, kVarPanelId, var_idx );
      widgetListUuId = uiFindElementUuId( p->ioH, varUuId, kWidgetListId, kInvalidId );

      //printf("%i %i %i\n",var_idx, varUuId,widgetListUuId);
      
      switch( value_type_flag )
      {
        case flow::kBoolTFl:
          rc = _create_bool_widgets(p,widgetListUuId,ui_var,widget_uuId);
          break;
          
        case flow::kIntTFl:
          {
            int min_val = std::numeric_limits<int>::min();
            int max_val = std::numeric_limits<int>::max();
            rc = _create_number_widget(p,widgetListUuId,ui_var, kIntWidgetId, flow::kIntTFl, widget_uuId, min_val, max_val, 1, 1 );
          }
          break;          
          
        case flow::kUIntTFl:
          {
            unsigned min_val = 0;
            unsigned max_val = std::numeric_limits<unsigned>::max();
            rc = _create_number_widget(p,widgetListUuId,ui_var, kUIntWidgetId, flow::kUIntTFl, widget_uuId, min_val, max_val, 1, 1 );
          }
          break;

        case flow::kFloatTFl:
          {
            float min_val = std::numeric_limits<float>::min();
            float max_val = std::numeric_limits<float>::max();
            rc = _create_number_widget(p,widgetListUuId,ui_var, kFloatWidgetId, flow::kFloatTFl, widget_uuId, min_val, max_val, 0.1, 2 );
          }
          break;
          
        case flow::kDoubleTFl:
          {
            double min_val = std::numeric_limits<float>::min();
            double max_val = std::numeric_limits<float>::max();
            rc = _create_number_widget(p,widgetListUuId,ui_var, kDoubleWidgetId, flow::kDoubleTFl, widget_uuId, min_val, max_val, 0.1, 2 );
          }
          break;

        case flow::kStringTFl:
          rc = _create_string_widgets(p,widgetListUuId,ui_var,widget_uuId);
          break;

        default:
          {
            const char* type_label;
            if((type_label = flow::value_type_flag_to_label(value_type_flag)) != nullptr )
              cwLogWarning("No UI widgets for type:%s",type_label);
            else
            {
              cwLogError(kInvalidArgRC,"The value type flag: 0x%x is not known. No UI widget was created.",value_type_flag);
            }

            goto errLabel;

          }
      }

      if( widget_uuId != kInvalidId )
      {
        uiSetBlob(p->ioH,widget_uuId, &ui_var, sizeof(&ui_var));

        // if this is a 'init' variable then disable it
        if( ui_var->desc_flags & flow::kInitVarDescFl )
          uiClearEnable(p->ioH, widget_uuId );

        
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
        if( ui_proc->varA[i].ch_cnt != kInvalidCnt && ui_proc->varA[i].ch_cnt > n )
          n = ui_proc->varA[i].ch_cnt;

      return n==0 ? 1 : n;
    }
    
    rc_t _create_proc_ui( ui_t* p, unsigned parentListUuId, const flow::ui_proc_t* ui_proc, unsigned proc_idx )
    {
      rc_t           rc              = kOkRC;
      unsigned       procPanelUuId   = kInvalidId;
      unsigned       chanPanelUuId   = kInvalidId;
      unsigned       chanListUuId    = kInvalidId;
      unsigned       chN             = _calc_max_chan_count(ui_proc);
      const unsigned label_buf_charN = 127;
      char           label_buf[ label_buf_charN+1 ];
      
      if((rc = uiCreateFromRsrc(   p->ioH, "proc", parentListUuId, proc_idx )) != kOkRC )
      {
        goto errLabel;
      }

      procPanelUuId  =  uiFindElementUuId( p->ioH, parentListUuId, kProcPanelId,   proc_idx );
      chanPanelUuId  =  uiFindElementUuId( p->ioH, procPanelUuId, kChanPanelId,   kInvalidId );
      chanListUuId   =  uiFindElementUuId( p->ioH, chanPanelUuId, kChanListId,   kInvalidId );
      //printf("proc_idx: %i %i %i : chN:%i\n",proc_idx, parentListUuId, procPanelUuId,chN);

      snprintf(label_buf,label_buf_charN,"%s:%i",ui_proc->label,ui_proc->label_sfx_id);
      
      uiSendValue( p->ioH, uiFindElementUuId(p->ioH, procPanelUuId, kProcInstLabelId, kInvalidId), label_buf );


      // for each channel (add one to the channel count to account for the leading 'label' column)
      for(unsigned ui_ch_idx=0; ui_ch_idx<chN+1; ++ui_ch_idx)
      {
        unsigned varListUuId   = kInvalidIdx;
        
        // add a varList to the chanList
        if((rc = uiCreateFromRsrc( p->ioH, "chan", chanListUuId, ui_ch_idx )) != kOkRC )
        {
          goto errLabel;
        }
      
        varListUuId = uiFindElementUuId(p->ioH, chanListUuId, kVarListId, ui_ch_idx );

        // for each var
        for(unsigned j=0; j<ui_proc->varN; ++j)
        {
          const flow::ui_var_t* ui_var = ui_proc->varA + j;

          // only create controls for certain value types
          if( !(ui_var->value_tid & (flow::kBoolTFl | flow::kIntTFl | flow::kUIntTFl | flow::kFloatTFl | flow::kDoubleTFl | flow::kStringTFl )))
            continue;
          

          if( ui_ch_idx == 0 )
          {
            if( ui_var->ch_idx == flow::kAnyChIdx )
            {
              snprintf(label_buf,label_buf_charN,"%s:%i",ui_var->label,ui_var->label_sfx_id);
              if((rc = _create_var_label(p,varListUuId, ui_var, label_buf, j)) != kOkRC )
              {
                goto errLabel;
              }
            }
            
          }
          else
          {

            // subtract one from ui_ch_idx to account for the leading label column with is associated with ui_ch_idx==0
            unsigned uiChIdx = ui_ch_idx - 1;
            bool create_fl = false;
            
            // if this is the 'any' ch. var
            if( ui_var->ch_idx == flow::kAnyChIdx  )
            {
              // if this 'any' ch. var has no other channels
              if( ui_var->ch_cnt == 0 )
              {
                create_fl = true; // always create a control or a blank var
                
                // if the current ch. == 0 then create then control ...
                if( ui_var->ch_cnt==0 && uiChIdx!=0 )
                  ui_var = nullptr; // ... otherwise create a blank 
              }
                
            }
            else // otherwise create the var if it is on chan 'uiChIdx'
            {
              create_fl = ui_var->ch_idx == uiChIdx;
            }

            if( create_fl )
            {              
              if((rc = _create_var_ui(p, varListUuId, ui_var, j)) != kOkRC )
              {
                goto errLabel;
              }
            }
          }
        }
      }

    errLabel:
      if(rc != kOkRC )
        rc = cwLogError(rc,"Processor UI creation failed on %s:%i.",cwStringNullGuard(ui_proc->label),ui_proc->label_sfx_id);
      
      return rc;
      
    }
    
    rc_t _create_net_ui( ui_t* p, unsigned parentUuId, const flow::ui_net_t* ui_net )
    {
      rc_t rc = kOkRC;

      unsigned netPanelUuId = kInvalidId;
      unsigned procListUuId = kInvalidId;

      if((rc = uiCreateFromRsrc(   p->ioH, "network", parentUuId, ui_net->poly_idx )) != kOkRC )
      {
        goto errLabel;
      }

      netPanelUuId =  uiFindElementUuId( p->ioH, parentUuId,   kNetPanelId,   ui_net->poly_idx );
      procListUuId =  uiFindElementUuId( p->ioH, netPanelUuId, "procListId" );

      //printf("net:%i %i plist:%i\n",parentUuId, netPanelUuId,procListUuId);
      
      for(unsigned i=0; i<ui_net->procN; ++i)
      {
        if((rc = _create_proc_ui( p, procListUuId, ui_net->procA+i, i )) != kOkRC )
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
    p->ioH = ioH;
    p->ioFlowH = ioFlowH;
    p->ui_net = ui_net;
    
    unsigned netPanelUuId   = io::uiFindElementUuId( ioH, kRootNetPanelId );

    if((rc = _create_net_ui( p, netPanelUuId, ui_net )) != kOkRC )
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
