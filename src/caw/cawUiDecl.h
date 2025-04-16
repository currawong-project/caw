//| Copyright: (C) 2020-2024 Kevin Larke <contact AT larke DOT org> 
//| License: GNU GPL version 3.0 or above. See the accompanying LICENSE file.

namespace caw {
  namespace ui {

    enum
    {
      kPanelDivId,   
      kQuitBtnId,    
      kIoReportBtnId,
      kNetPrintBtnId,
      kReportBtnId,  
      kLatencyBtnId,

      kReloadIoBtnId,
      kReloadPgmBtnId,

      kPgmSelId,
      kPgmPresetSelId,
      kPgmLoadBtnId,
      kPgmPrintBtnId,
      kRunCheckId,

      kLogId,
    
      kRootNetPanelId,
      kNetListId,
      
      kNetPanelId,
      kNetTitleId,
      kProcListId,

      kProcPanelId,
      kProcInstLabelId,
      kProcPresetSelId,
      kProcPresetOptId,
      kChanListId,
      
      kChanPanelId,
      kVarListId,
      
      kVarPanelId,
      kWidgetListId,

      kButtonWidgetId,
      kCheckWidgetId,
      kIntWidgetId,
      kUIntWidgetId,
      kFloatWidgetId,
      kDoubleWidgetId,
      kStringWidgetId,
      kMeterWidgetId,
      kListWidgetId,
      

      kPgmBaseSelId,
      kPgmMaxSelId = kPgmBaseSelId + 1000,
      kPgmPresetBaseSelId,
      kPgmPresetMaxSelId = kPgmPresetBaseSelId + 1000
    };
    
  }
}
