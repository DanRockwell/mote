/*
Copyright (c) 2013, Dust Networks.  All rights reserved.
*/

#include "dn_common.h"
#include "loc_task.h"
#include "dnm_local.h"
#include "dn_api_param.h"

//=========================== defines =========================================

#define DFLT_EVENTMASK       0xFFFFFFFF

//=========================== variables =======================================

/// Variables local to the loc_task module.
typedef struct {
   // configuration
   INT8U                fJoin;                   ///< Flag to indicate whether module should should (1) or not (0)
   dn_netid_t           netId;                   ///< Network ID. No effect when set to 0.
   INT16U               udpPort;                 ///< UDP port to bind to.
   INT32U               bandwidth;               ///< Bandwidth to request (inter-packet duration, in ms)
   // stack communication
   INT8U      notifBuf[DN_API_LOC_MAXMSG_SIZE];  ///< Buffer to receive stack notifications in.
   CH_DESC              notifChDesc;             ///< Notification channel.
   OS_FLAG_GRP*         locEventsFlag;           ///< Event flags.
   // tasks
   OS_STK locNotifTaskStack[TASK_APP_LOCNOTIF_STK_SIZE];   ///< Notification task stack.
   OS_STK locCtrlTaskStack[TASK_APP_LOCCTRL_STK_SIZE];     ///< Control task stack.
   // status
   OS_EVENT*            joinedSem;               ///< Semaphore to post when mote done joining.
   OS_EVENT*            serviceSem;              ///< Semaphore to post when service granted.
   INT8U                socketId;                ///< Socket ID.
   // flags
   INT8U                fConfigured;             ///< Flag to indicate whether mote has already be configured.
   INT8U                fHandledOperational;     ///< Flag to indicate whether mote has handled the operational event yet.
   INT8U                fHandledSvcChanged;      ///< Flag to indicate whether mote has handled the "service changed" event yet.
} notif_task_vars_t;

notif_task_vars_t notif_task_v;

//=========================== prototypes ======================================

// tasks
static void        locCtrlTask(void* arg);
static void        locNotifTask(void* unused);
// dnm_local callbacks
static dn_error_t  appEventNotifCb(dn_api_loc_notif_events_t* notifEvents, INT8U* rsp);

//=========================== public ==========================================

/**
\brief Start this module.

This function sets up the notification channel to receive events generated by
the stack, and creates a notification and an event task.
*/
void loc_task_init(INT8U           fJoin,
                   dn_netid_t      netId,
                   INT16U          udpPort,
                   OS_EVENT*       joinedSem,
                   INT32U          bandwidth,
                   OS_EVENT*       serviceSem) {
   dn_error_t      dnErr;
   INT8U           osErr;
   
   //===== store params
   notif_task_v.fJoin             = fJoin;
   notif_task_v.netId             = netId;
   notif_task_v.udpPort           = udpPort;
   notif_task_v.joinedSem         = joinedSem;
   notif_task_v.bandwidth         = bandwidth;
   notif_task_v.serviceSem        = serviceSem;
   
   //===== initialize dnm_local module
   
   // create a synchronous channel for the dnm_local module to receive notifications from the stack
   dnErr = dn_createSyncChannel(&notif_task_v.notifChDesc);
   ASSERT(dnErr==DN_ERR_NONE);
   
   // register that channel to DN_MSG_TYPE_NET_NOTIF notifications
   dnErr = dn_registerChannel(notif_task_v.notifChDesc, DN_MSG_TYPE_NET_NOTIF);
   ASSERT(dnErr==DN_ERR_NONE);
   
   // initialize the local interface
   dnErr = dnm_loc_init(
      PASSTHROUGH_OFF,                 // mode
      notif_task_v.notifBuf,           // pBuffer
      sizeof(notif_task_v.notifBuf)    // buffLen
   );
   ASSERT(dnErr==DN_ERR_NONE);

   // create a flag for appEventNotifCb() to signal an event to locCtrlTask
   notif_task_v.locEventsFlag = OSFlagCreate(0x0000, &osErr);
   ASSERT(osErr==OS_ERR_NONE);

   // ask the dnm_local module to call appEventNotifCb() when an event is reported by the stack
   dnErr = dnm_loc_registerEventNotifCallback(appEventNotifCb);
   ASSERT(dnErr==DN_ERR_NONE);
   
   // create the local control task
   osErr = OSTaskCreateExt(
      locCtrlTask,
      (void *)0,
      (OS_STK*)(&notif_task_v.locCtrlTaskStack[TASK_APP_LOCCTRL_STK_SIZE - 1]),
      TASK_APP_LOCCTRL_PRIORITY,
      TASK_APP_LOCCTRL_PRIORITY,
      (OS_STK*)notif_task_v.locCtrlTaskStack,
      TASK_APP_LOCCTRL_STK_SIZE,
      (void *)0,
      OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR
   );
   ASSERT(osErr == OS_ERR_NONE);
   OSTaskNameSet(TASK_APP_LOCCTRL_PRIORITY, (INT8U*) TASK_APP_LOCCTRL_NAME, &osErr);
   ASSERT(osErr == OS_ERR_NONE);

   // create the local notifications task
   osErr = OSTaskCreateExt(
      locNotifTask,
      (void *) 0,
      (OS_STK*) (&notif_task_v.locNotifTaskStack[TASK_APP_LOCNOTIF_STK_SIZE - 1]),
      TASK_APP_LOCNOTIF_PRIORITY,
      TASK_APP_LOCNOTIF_PRIORITY,
      (OS_STK*) notif_task_v.locNotifTaskStack,
      TASK_APP_LOCNOTIF_STK_SIZE,
      (void *) 0,
      OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR
   );
   ASSERT(osErr == OS_ERR_NONE);
   OSTaskNameSet(TASK_APP_LOCNOTIF_PRIORITY, (INT8U*) TASK_APP_LOCNOTIF_NAME, &osErr);
   ASSERT(osErr == OS_ERR_NONE);
}

/**
\brief Retrieve the socket ID opened when connecting to the network.

Only call this function when the mote has passe the LOC_STATE_CONFIG state.
Calling it before will return <tt>0</tt>.

\return The socket ID that was created.
*/
INT8U loc_getSocketId() {
   return notif_task_v.socketId;
}

//=========================== private =========================================

//===== tasks

/**
\brief uC/OS-II task which reacts to events generated by the dnm_local module.  This task is a Stack Controller.

\note This task is started when loc_task_init() is called.

Local events are signaled by the dnm_local module which calls the appEventNotifCb()
function. appEventNotifCb() sets one or more flags in the
notif_task_v.locEventsFlag. This tasks waits for those flags to be set. When
one of more flags are set, this tasks updates the mote's state, reacts to the
event, and waits again.
*/
static void locCtrlTask(void* arg) {
   dn_error_t                     dnErr;
   INT8U                          osErr;
   INT8U                          rc;
   INT8U                          moteInfoBuf[2+sizeof(dn_api_rsp_get_moteinfo_t)];
   dn_api_rsp_get_moteinfo_t*     moteInfo;
   INT8U                          respLen;
   OS_FLAGS                       eventFlags;
   INT16U                         netId;
   INT32U                         eventMask;
      
   
   while (1) { // this is a task, it executes forever
      
      //===== wait for an event from the stack
     
      // This actual event is received by appEventNotifCb(), which posts flags
      eventFlags = OSFlagPend(
         notif_task_v.locEventsFlag,             // event flag group
         DN_API_LOC_EV_BOOT        | \
         DN_API_LOC_EV_ALARMS_CHG  | \
         DN_API_LOC_EV_TIME_CHG    | \
         DN_API_LOC_EV_JOINFAIL    | \
         DN_API_LOC_EV_DISCON      | \
         DN_API_LOC_EV_OPERATIONAL | \
         DN_API_LOC_EV_SVC_CHG     | \
         DN_API_LOC_EV_JOINSTART,                // which flags to check
         OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME, // wait for any flag, clear flag when happens
         0,                                      // timeout (0=wait forever)
         &osErr                                  // error code
      );
      ASSERT(osErr==OS_ERR_NONE);

      // this delay is only needed to prevent overlap in traces; traces are not
      // mutexed for performance reasons; the callback function will ack the
      // notification, so this delay will not affect the stack.
      OSTimeDly(SECOND/50);    
      
      // when you get here, eventsFlags is a bitmap of the events that just happened
      
      //===== handle the event
      
      //===
      
      if (eventFlags & DN_API_LOC_EV_BOOT) {
         // mote just booted
         
         if (notif_task_v.fConfigured==0x00) {
            // not configured yet
            
            // retrieve the stack version
            dnErr = dnm_loc_getParameterCmd(
               DN_API_PARAM_MOTEINFO,                 // paramId
               moteInfoBuf,                           // payload
               0,                                     // txPayloadLen
               &respLen,                              // rxPayloadLen
               &rc                                    // rc
            );
            ASSERT(dnErr==DN_ERR_NONE);
            ASSERT(rc==DN_ERR_NONE);
            
            // print the stack version
            moteInfo = (dn_api_rsp_get_moteinfo_t*)(&moteInfoBuf[0]);
            dnm_ucli_printf(
               "SmartMeshIP stack, ver %d.%d.%d.%d\r\n",
               moteInfo->swVer.major,
               moteInfo->swVer.minor,
               moteInfo->swVer.patch,
               htons(moteInfo->swVer.build)
            );   
            
            // overwrite the mote's netid (if specified)
            if (notif_task_v.netId!=NETID_NONE) {
               netId = htons(notif_task_v.netId);
               dnErr = dnm_loc_setParameterCmd(
                  DN_API_PARAM_NETID,                 // paramId
                  (INT8U*)(&netId),                   // payload
                  sizeof(notif_task_v.netId),         // payload length
                  &rc                                 // return code
               );
               ASSERT(dnErr==DN_ERR_NONE);
               ASSERT(rc==DN_ERR_NONE);
            }
            
            // configure the events you want to receive from the stack
            if (notif_task_v.fJoin) {
                eventMask = htonl(DFLT_EVENTMASK);
                dnErr = dnm_loc_setParameterCmd(
                   DN_API_PARAM_EVENTMASK,            // paramId
                   (INT8U*)(&eventMask),              // payload
                   4,                                 // payload length
                   &rc                                // return code
                );
                ASSERT(dnErr==DN_ERR_NONE);
              ASSERT(rc==DN_ERR_NONE);
            }
            
            // open and bind socket (if specified)
            if (notif_task_v.udpPort!=UDPPORT_NONE) {
               
               // open a socket
               dnErr = dnm_loc_openSocketCmd(
                  DN_API_PROTO_UDP,                   // protocol
                  &notif_task_v.socketId,             // socketId
                  &rc                                 // return code
               );
               ASSERT(dnErr==DN_ERR_NONE);
               ASSERT(rc==DN_ERR_NONE);
               
               // bind that socket to a UDP port
               dnErr = dnm_loc_bindSocketCmd(
                  notif_task_v.socketId,              // socketId
                  notif_task_v.udpPort,               // UDP port to bind to
                  &rc                                 // return code
               );
               ASSERT(dnErr==DN_ERR_NONE);
               ASSERT(rc==DN_ERR_NONE);
            }
            
            // join network (if specified)
            if (notif_task_v.fJoin) {
               dnErr = dnm_loc_joinCmd(&rc);
               ASSERT(dnErr==DN_ERR_NONE);
               ASSERT(rc==DN_ERR_NONE);
            }
            
            // remember it's configured
            notif_task_v.fConfigured = 0x01;
         }
      }
      
      //===
      
      if (eventFlags & DN_API_LOC_EV_OPERATIONAL) {
         // mote finished joining a network
         
         if (notif_task_v.fHandledOperational==0x00) {
            // I haven't reacted to the operational event yet
            
            // post the joinedSem (if any)
            if (notif_task_v.joinedSem) {
               osErr = OSSemPost(notif_task_v.joinedSem);
               ASSERT(osErr==OS_ERR_NONE);
            }
            
            // request service (if specified)
            if (notif_task_v.bandwidth) {
               dnErr = dnm_loc_requestServiceCmd(
                  DN_MGR_SHORT_ADDR,        // destAddr
                  DN_API_SERVICE_TYPE_BW,   // svcType
                  notif_task_v.bandwidth,   // svcInfo
                  &rc                       // rc
               );
               ASSERT(dnErr==DN_ERR_NONE);
               ASSERT(rc==DN_ERR_NONE);
            }
            
            // remember I've reacted to the operational event
            notif_task_v.fHandledOperational = 0x01;
         }
      }
      
      //===
      
      if (eventFlags & DN_API_LOC_EV_SVC_CHG) {
         
         if (notif_task_v.fHandledSvcChanged==0x00) {
         
            // post the serviceSem (if any)
            if (notif_task_v.serviceSem ) {
               osErr = OSSemPost(notif_task_v.serviceSem);
               ASSERT(osErr==OS_ERR_NONE);
            }
            
            // remember I've reacted to the "service changed" event
            notif_task_v.fHandledSvcChanged = 0x01;
         }
      }
   }
}

/**
\brief uC/OS-II task which drives the notification processing at the
   dnm_local module.

\note This task is started when loc_task_init() is called.

The dnm_local module does not define a task which calls the
dnm_loc_processNotifications() continuously.
*/
static void locNotifTask(void* unused) {
   
   while (1) { // this is a task, it executes forever
      
      dnm_loc_processNotifications();
   }
}

//===== dnm_local callbacks

/**
\brief Function called by the dnm_local module when it has received an event.

When the stack has an event to signal, it calls this function. This function
releases the locEventsFlag to signal the event to the locCtrlTask(), which is
pending on locEventsFlag.

\param[in]  notifEvents  Pointer to a variable which will holds event bit masks.
\param[out] rsp          Response value for stack this function needs to fill in.

\return DN_ERR_NONE if all went well, or another value if the event
    notification callback is defined.
*/
static dn_error_t appEventNotifCb(dn_api_loc_notif_events_t* notifEvents, INT8U* rsp) {
   INT8U      osErr;
   dn_error_t dnErr;
   INT32U     events;
   
   // by default, all went well
   dnErr  = DN_ERR_NONE;
   
   // change endianness of event signaled in the notification
   events = ntohl(notifEvents->events);
   
   if (events) {
      *rsp = DN_API_RC_OK;
      
      // post the flags corresponding to the events just received, unlocking locCtrlTask
      OSFlagPost(
         notif_task_v.locEventsFlag,   // the event flag group
         events,                       // flag to change
         OS_FLAG_SET,                  // change = set the flags
         &osErr                        // return code
      );
      ASSERT(osErr==OS_ERR_NONE);
   }
   
   return dnErr;
}

