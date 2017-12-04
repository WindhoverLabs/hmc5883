/************************************************************************
** Includes
*************************************************************************/
#include <string.h>

#include "cfe.h"

#include "hmc5883_custom.h"
#include "hmc5883_app.h"
#include "hmc5883_msg.h"
#include "hmc5883_version.h"


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Instantiate the application object.                             */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
HMC5883 oHMC5883;



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Default constructor.                                            */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
HMC5883::HMC5883()
{

}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Destructor constructor.                                         */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
HMC5883::~HMC5883()
{

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Initialize event tables.                                        */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 HMC5883::InitEvent()
{
    int32  iStatus=CFE_SUCCESS;
    int32  ind = 0;
    int32 customEventCount = 0;
    
    CFE_EVS_BinFilter_t   EventTbl[CFE_EVS_MAX_EVENT_FILTERS];

    /* Initialize the event filter table.
     * Note: 0 is the CFE_EVS_NO_FILTER mask and event 0 is reserved (not used) */
    memset(EventTbl, 0x00, sizeof(EventTbl));
    
    /* TODO: Choose the events you want to filter.  CFE_EVS_MAX_EVENT_FILTERS
     * limits the number of filters per app.  An explicit CFE_EVS_NO_FILTER 
     * (the default) has been provided as an example. */
    EventTbl[  ind].EventID = HMC5883_RESERVED_EID;
    EventTbl[ind++].Mask    = CFE_EVS_NO_FILTER;
    EventTbl[  ind].EventID = HMC5883_READ_ERR_EID;
    EventTbl[ind++].Mask    = CFE_EVS_FIRST_16_STOP;
    
    
    /* Add custom events to the filter table */
    customEventCount = HMC5883_Custom_Init_EventFilters(ind, EventTbl);
    
    if(-1 == customEventCount)
    {
        iStatus = CFE_EVS_APP_FILTER_OVERLOAD;
        (void) CFE_ES_WriteToSysLog("Failed to init custom event filters (0x%08X)\n", (unsigned int)iStatus);
        goto end_of_function;
    }

    /* Register the table with CFE */
    iStatus = CFE_EVS_Register(EventTbl, (ind + customEventCount), CFE_EVS_BINARY_FILTER);
    if (iStatus != CFE_SUCCESS)
    {
        (void) CFE_ES_WriteToSysLog("HMC5883 - Failed to register with EVS (0x%08lX)\n", iStatus);
    }

end_of_function:

    return iStatus;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Initialize Message Pipes                                        */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 HMC5883::InitPipe()
{
    int32  iStatus=CFE_SUCCESS;

    /* Init schedule pipe and subscribe to wakeup messages */
    iStatus = CFE_SB_CreatePipe(&SchPipeId,
    		HMC5883_SCH_PIPE_DEPTH,
			HMC5883_SCH_PIPE_NAME);
    if (iStatus == CFE_SUCCESS)
    {
        iStatus = CFE_SB_SubscribeEx(HMC5883_WAKEUP_MID, SchPipeId, CFE_SB_Default_Qos, HMC5883_WAKEUP_MID_MAX_MSG_COUNT);
        if (iStatus != CFE_SUCCESS)
        {
            (void) CFE_EVS_SendEvent(HMC5883_SUBSCRIBE_ERR_EID, CFE_EVS_ERROR,
            		"Sch Pipe failed to subscribe to HMC5883_WAKEUP_MID. (0x%08lX)",
                    iStatus);
            goto HMC5883_InitPipe_Exit_Tag;
        }

        iStatus = CFE_SB_SubscribeEx(HMC5883_SEND_HK_MID, SchPipeId, CFE_SB_Default_Qos, HMC5883_SEND_HK_MID_MAX_MSG_COUNT);
        if (iStatus != CFE_SUCCESS)
        {
            (void) CFE_EVS_SendEvent(HMC5883_SUBSCRIBE_ERR_EID, CFE_EVS_ERROR,
					 "CMD Pipe failed to subscribe to HMC5883_SEND_HK_MID. (0x%08X)",
					 (unsigned int)iStatus);
            goto HMC5883_InitPipe_Exit_Tag;
        }
    }
    else
    {
        (void) CFE_EVS_SendEvent(HMC5883_PIPE_INIT_ERR_EID, CFE_EVS_ERROR,
			 "Failed to create SCH pipe (0x%08lX)",
			 iStatus);
        goto HMC5883_InitPipe_Exit_Tag;
    }

    /* Init command pipe and subscribe to command messages */
    iStatus = CFE_SB_CreatePipe(&CmdPipeId,
    		HMC5883_CMD_PIPE_DEPTH,
			HMC5883_CMD_PIPE_NAME);
    if (iStatus == CFE_SUCCESS)
    {
        /* Subscribe to command messages */
        iStatus = CFE_SB_Subscribe(HMC5883_CMD_MID, CmdPipeId);

        if (iStatus != CFE_SUCCESS)
        {
            (void) CFE_EVS_SendEvent(HMC5883_SUBSCRIBE_ERR_EID, CFE_EVS_ERROR,
				 "CMD Pipe failed to subscribe to HMC5883_CMD_MID. (0x%08lX)",
				 iStatus);
            goto HMC5883_InitPipe_Exit_Tag;
        }
    }
    else
    {
        (void) CFE_EVS_SendEvent(HMC5883_PIPE_INIT_ERR_EID, CFE_EVS_ERROR,
			 "Failed to create CMD pipe (0x%08lX)",
			 iStatus);
        goto HMC5883_InitPipe_Exit_Tag;
    }

HMC5883_InitPipe_Exit_Tag:
    return iStatus;
}
    

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Initialize Global Variables                                     */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void HMC5883::InitData()
{
    /* Init housekeeping message. */
    CFE_SB_InitMsg(&HkTlm, HMC5883_HK_TLM_MID, sizeof(HkTlm), TRUE);
    /* Init output messages */
    CFE_SB_InitMsg(&SensorMagMsg, PX4_SENSOR_MAG_MID, 
            sizeof(PX4_SensorMagMsg_t), TRUE);
    /* Init custom data */
    HMC5883_Custom_InitData();
    /* Set initial values for calibration */
    HkTlm.Calibration.x_scale  = 1.0f;
    HkTlm.Calibration.y_scale  = 1.0f;
    HkTlm.Calibration.z_scale  = 1.0f;
    HkTlm.Calibration.x_offset = 0.0f;
    HkTlm.Calibration.y_offset = 0.0f;
    HkTlm.Calibration.z_offset = 0.0f;
    /* Set range and scale */
    HkTlm.Range                = 1.3f;
    HkTlm.Scaling              = 1.0f / 1090.0f;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* HMC5883 initialization                                          */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 HMC5883::InitApp()
{
    int32  iStatus   = CFE_SUCCESS;
    int8   hasEvents = 0;
    boolean returnBool = TRUE;

    iStatus = InitEvent();
    if (iStatus != CFE_SUCCESS)
    {
        (void) CFE_ES_WriteToSysLog("HMC5883 - Failed to init events (0x%08lX)\n", iStatus);
        goto HMC5883_InitApp_Exit_Tag;
    }
    else
    {
        hasEvents = 1;
    }

    iStatus = InitPipe();
    if (iStatus != CFE_SUCCESS)
    {
        goto HMC5883_InitApp_Exit_Tag;
    }

    InitData();
    returnBool = HMC5883_Custom_Init();
    if (FALSE == returnBool)
    {
        iStatus = -1;
        CFE_EVS_SendEvent(HMC5883_INIT_ERR_EID, CFE_EVS_ERROR,
                "Custom init failed");
        goto HMC5883_InitApp_Exit_Tag;
    }
    returnBool = HMC5883_Custom_ValidateID();
    if (FALSE == returnBool)
    {
        iStatus = -1;
        CFE_EVS_SendEvent(HMC5883_INIT_ERR_EID, CFE_EVS_ERROR,
            "HMC5883 Device failed validate ID");
        goto HMC5883_InitApp_Exit_Tag;
    }
    
    /* TODO add self test*/

    /* TODO self calibration routine */
    returnBool = HMC5883_Custom_Calibration(&HkTlm.Calibration);
    if (FALSE == returnBool)
    {
        iStatus = -1;
        CFE_EVS_SendEvent(HMC5883_INIT_ERR_EID, CFE_EVS_ERROR,
            "HMC5883 Device failed calibration");
        goto HMC5883_InitApp_Exit_Tag;
    }
    returnBool = HMC5883_Custom_Set_Range(HMC5883_BITS_CONFIG_B_RANGE_1GA3);
    if (FALSE == returnBool)
    {
        iStatus = -1;
        CFE_EVS_SendEvent(HMC5883_INIT_ERR_EID, CFE_EVS_ERROR,
            "HMC5883 Device failed set range");
        goto HMC5883_InitApp_Exit_Tag;
    }
    returnBool = HMC5883_Custom_Check_Range(HMC5883_BITS_CONFIG_B_RANGE_1GA3);
    if (FALSE == returnBool)
    {
        iStatus = -1;
        CFE_EVS_SendEvent(HMC5883_INIT_ERR_EID, CFE_EVS_ERROR,
            "HMC5883 Device failed check range");
        goto HMC5883_InitApp_Exit_Tag;
    }
    
    /* Register the cleanup callback */
    iStatus = OS_TaskInstallDeleteHandler(&HMC5883_CleanupCallback);
    if (iStatus != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(HMC5883_INIT_ERR_EID, CFE_EVS_ERROR,
                                 "Failed to init register cleanup callback (0x%08X)",
                                 (unsigned int)iStatus);
        goto HMC5883_InitApp_Exit_Tag;
    }

HMC5883_InitApp_Exit_Tag:
    if (iStatus == CFE_SUCCESS)
    {
        (void) CFE_EVS_SendEvent(HMC5883_INIT_INF_EID, CFE_EVS_INFORMATION,
                                "Initialized.  Version %d.%d.%d.%d",
                                HMC5883_MAJOR_VERSION,
                                HMC5883_MINOR_VERSION,
                                HMC5883_REVISION,
                                HMC5883_MISSION_REV);
        /* Set the app state to initialized */
        oHMC5883.HkTlm.State = HMC5883_INITIALIZED;
    }
    else
    {
        if (hasEvents == 1)
        {
            (void) CFE_ES_WriteToSysLog("HMC5883 - Application failed to initialize\n");
        }
    }

    return iStatus;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Receive and Process Messages                                    */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int32 HMC5883::RcvSchPipeMsg(int32 iBlocking)
{
    int32           iStatus=CFE_SUCCESS;
    CFE_SB_Msg_t*   MsgPtr=NULL;
    CFE_SB_MsgId_t  MsgId;

    /* Stop Performance Log entry */
    CFE_ES_PerfLogExit(HMC5883_MAIN_TASK_PERF_ID);

    /* Wait for WakeUp messages from scheduler */
    iStatus = CFE_SB_RcvMsg(&MsgPtr, SchPipeId, iBlocking);

    /* Start Performance Log entry */
    CFE_ES_PerfLogEntry(HMC5883_MAIN_TASK_PERF_ID);

    if (iStatus == CFE_SUCCESS)
    {
        MsgId = CFE_SB_GetMsgId(MsgPtr);
        switch (MsgId)
        {
            case HMC5883_WAKEUP_MID:
                ReadDevice();
                SendSensorMagMsg();
                /* TODO:  Do something here. */
                break;

            case HMC5883_SEND_HK_MID:
                ProcessCmdPipe();
                ReportHousekeeping();
                break;

            default:
                (void) CFE_EVS_SendEvent(HMC5883_MSGID_ERR_EID, CFE_EVS_ERROR,
                    "Recvd invalid SCH msgId (0x%04X)", MsgId);
        }
    }
    else if (iStatus == CFE_SB_NO_MESSAGE)
    {
        /* TODO: If there's no incoming message, you can do something here, or 
         * nothing.  Note, this section is dead code only if the iBlocking arg
         * is CFE_SB_PEND_FOREVER. */
        iStatus = CFE_SUCCESS;
    }
    else if (iStatus == CFE_SB_TIME_OUT)
    {
        /* TODO: If there's no incoming message within a specified time (via the
         * iBlocking arg, you can do something here, or nothing.  
         * Note, this section is dead code only if the iBlocking arg
         * is CFE_SB_PEND_FOREVER. */
        iStatus = CFE_SUCCESS;
    }
    else
    {
        (void) CFE_EVS_SendEvent(HMC5883_RCVMSG_ERR_EID, CFE_EVS_ERROR,
            "SCH pipe read error (0x%08lX).", iStatus);
    }

    return iStatus;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Process Incoming Commands                                       */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void HMC5883::ProcessCmdPipe()
{
    int32 iStatus = CFE_SUCCESS;
    CFE_SB_Msg_t*   CmdMsgPtr=NULL;
    CFE_SB_MsgId_t  CmdMsgId;

    /* Process command messages until the pipe is empty */
    while (1)
    {
        iStatus = CFE_SB_RcvMsg(&CmdMsgPtr, CmdPipeId, CFE_SB_POLL);
        if(iStatus == CFE_SUCCESS)
        {
            CmdMsgId = CFE_SB_GetMsgId(CmdMsgPtr);
            switch (CmdMsgId)
            {
                case HMC5883_CMD_MID:
                    ProcessAppCmds(CmdMsgPtr);
                    break;

                default:
                    /* Bump the command error counter for an unknown command.
                     * (This should only occur if it was subscribed to with this
                     *  pipe, but not handled in this switch-case.) */
                    HkTlm.usCmdErrCnt++;
                    (void) CFE_EVS_SendEvent(HMC5883_MSGID_ERR_EID, CFE_EVS_ERROR,
                                      "Recvd invalid CMD msgId (0x%04X)", (unsigned short)CmdMsgId);
                    break;
            }
        }
        else if (iStatus == CFE_SB_NO_MESSAGE)
        {
            break;
        }
        else
        {
            (void) CFE_EVS_SendEvent(HMC5883_RCVMSG_ERR_EID, CFE_EVS_ERROR,
                  "CMD pipe read error (0x%08lX)", iStatus);
            break;
        }
    }
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Process HMC5883 Commands                                        */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void HMC5883::ProcessAppCmds(CFE_SB_Msg_t* MsgPtr)
{
    uint32  uiCmdCode=0;

    if (MsgPtr != NULL)
    {
        uiCmdCode = CFE_SB_GetCmdCode(MsgPtr);
        switch (uiCmdCode)
        {
            case HMC5883_NOOP_CC:
                HkTlm.usCmdCnt++;
                (void) CFE_EVS_SendEvent(HMC5883_CMD_NOOP_EID, CFE_EVS_INFORMATION,
                    "Recvd NOOP. Version %d.%d.%d.%d",
                    HMC5883_MAJOR_VERSION,
                    HMC5883_MINOR_VERSION,
                    HMC5883_REVISION,
                    HMC5883_MISSION_REV);
                break;

            case HMC5883_RESET_CC:
                HkTlm.usCmdCnt = 0;
                HkTlm.usCmdErrCnt = 0;
                break;

            default:
                HkTlm.usCmdErrCnt++;
                (void) CFE_EVS_SendEvent(HMC5883_CC_ERR_EID, CFE_EVS_ERROR,
                                  "Recvd invalid command code (%u)", (unsigned int)uiCmdCode);
                break;
        }
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Send HMC5883 Housekeeping                                       */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void HMC5883::ReportHousekeeping()
{
    CFE_SB_TimeStampMsg((CFE_SB_Msg_t*)&HkTlm);
    CFE_SB_SendMsg((CFE_SB_Msg_t*)&HkTlm);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Publish Output Data                                             */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void HMC5883::SendSensorMagMsg()
{
    CFE_SB_TimeStampMsg((CFE_SB_Msg_t*)&SensorMagMsg);
    CFE_SB_SendMsg((CFE_SB_Msg_t*)&SensorMagMsg);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Verify Command Length                                           */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
boolean HMC5883::VerifyCmdLength(CFE_SB_Msg_t* MsgPtr,
                           uint16 usExpectedLen)
{
    boolean bResult  = TRUE;
    uint16  usMsgLen = 0;

    if (MsgPtr != NULL)
    {
        usMsgLen = CFE_SB_GetTotalMsgLength(MsgPtr);

        if (usExpectedLen != usMsgLen)
        {
            bResult = FALSE;
            CFE_SB_MsgId_t MsgId = CFE_SB_GetMsgId(MsgPtr);
            uint16 usCmdCode = CFE_SB_GetCmdCode(MsgPtr);

            (void) CFE_EVS_SendEvent(HMC5883_MSGLEN_ERR_EID, CFE_EVS_ERROR,
                              "Rcvd invalid msgLen: msgId=0x%08X, cmdCode=%d, "
                              "msgLen=%d, expectedLen=%d",
                              MsgId, usCmdCode, usMsgLen, usExpectedLen);
            HkTlm.usCmdErrCnt++;
        }
    }

    return bResult;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* HMC5883 Application C style main entry point.                   */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
extern "C" void HMC5883_AppMain()
{
    oHMC5883.AppMain();
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* HMC5883 Application C++ style main entry point.                 */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void HMC5883::AppMain()
{
    /* Register the application with Executive Services */
    uiRunStatus = CFE_ES_APP_RUN;

    int32 iStatus = CFE_ES_RegisterApp();
    if (iStatus != CFE_SUCCESS)
    {
        (void) CFE_ES_WriteToSysLog("HMC5883 - Failed to register the app (0x%08lX)\n", iStatus);
    }

    /* Start Performance Log entry */
    CFE_ES_PerfLogEntry(HMC5883_MAIN_TASK_PERF_ID);

    /* Perform application initializations */
    if (iStatus == CFE_SUCCESS)
    {
        iStatus = InitApp();
    }

    if (iStatus == CFE_SUCCESS)
    {
        /* Do not perform performance monitoring on startup sync */
        CFE_ES_PerfLogExit(HMC5883_MAIN_TASK_PERF_ID);
        CFE_ES_WaitForStartupSync(HMC5883_STARTUP_TIMEOUT_MSEC);
        CFE_ES_PerfLogEntry(HMC5883_MAIN_TASK_PERF_ID);
    }
    else
    {
        uiRunStatus = CFE_ES_APP_ERROR;
    }

    /* Application main loop */
    while (CFE_ES_RunLoop(&uiRunStatus) == TRUE)
    {
        RcvSchPipeMsg(HMC5883_SCH_PIPE_PEND_TIME);
    }

    /* Stop Performance Log entry */
    CFE_ES_PerfLogExit(HMC5883_MAIN_TASK_PERF_ID);

    /* Exit the application */
    CFE_ES_ExitApp(uiRunStatus);
}


void HMC5883::ReadDevice(void)
{
    boolean returnBool = FALSE;
    float rawX_f = 0;
    float rawY_f = 0;
    float rawZ_f = 0;
    CFE_TIME_SysTime_t cfeTimeStamp = {0, 0};

    cfeTimeStamp = HMC5883_Custom_Get_Time();
    
    /* Timestamp */
    SensorMagMsg.Timestamp.Seconds = cfeTimeStamp.Seconds;
    SensorMagMsg.Timestamp.Subseconds = cfeTimeStamp.Subseconds;

    /* Mag */
    returnBool = HMC5883_Custom_Measure(&SensorMagMsg.XRaw, &SensorMagMsg.YRaw, &SensorMagMsg.ZRaw);
    if(FALSE == returnBool)
    {
        goto end_of_function;
    }

    /* Apply any rotation */
    returnBool = HMC5883_Apply_Platform_Rotation(&SensorMagMsg.XRaw, &SensorMagMsg.YRaw, &SensorMagMsg.ZRaw);
    if(FALSE == returnBool)
    {
        goto end_of_function;
    }

    /* Set calibrated values */
    SensorMagMsg.X = (SensorMagMsg.XRaw - HkTlm.Calibration.x_offset) * HkTlm.Calibration.x_scale;
    SensorMagMsg.Y = (SensorMagMsg.YRaw - HkTlm.Calibration.y_offset) * HkTlm.Calibration.y_scale;
    SensorMagMsg.Z = (SensorMagMsg.ZRaw - HkTlm.Calibration.z_offset) * HkTlm.Calibration.z_scale;
    
    /* Set range */
    SensorMagMsg.Range = HkTlm.Range;
    
    /* Set scale */
    SensorMagMsg.Scaling = HkTlm.Scaling;
    
end_of_function:
;
}


void HMC5883_CleanupCallback(void)
{
    oHMC5883.HkTlm.State = HMC5883_UNINITIALIZED;
    if(HMC5883_Custom_Uninit() != TRUE)
    {
        CFE_EVS_SendEvent(HMC5883_UNINIT_ERR_EID, CFE_EVS_ERROR,"HMC5883_Uninit failed");
        oHMC5883.HkTlm.State = HMC5883_INITIALIZED;
    }
}
/************************/
/*  End of File Comment */
/************************/
