/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]
 
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
 
       http://www.apache.org/licenses/LICENSE-2.0
 
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/


/**********************************************************************

    module: ccsp_cr_profile.c

        For Common Component Software Platform (CCSP) Development

    ---------------------------------------------------------------

    description:

        This module implements transaction session related functions for 
        CR (Component Registrar) development.

        *   CcspCrRequestSessionID
        *   CcspCrGetCurrentSessionID
        *   CcspCrInformEndOfSession

    ---------------------------------------------------------------

    environment:

        platform independent

    ---------------------------------------------------------------

    author:

        Bin Zhu 

    ---------------------------------------------------------------

    revision:

        06/16/2011    initial revision.

**********************************************************************/

#include "ansc_platform.h"

#include "ansc_ato_interface.h"
#include "ansc_ato_external_api.h"

#include "ccsp_base_api.h"

#include "ccsp_cr_definitions.h"
#include "ccsp_cr_interface.h"
#include "ccsp_cr_profile.h"
#include "ccsp_cr_internal_api.h"


/**********************************************************************

    prototype:

        int
        CcspCrRequestSessionID
            (
                ANSC_HANDLE                 hThisObject,
                ULONG                       uPriority,
                ULONG*                      pulSessionID
            );

    description:

        This function is called to request a session ID;

    argument:   
                ANSC_HANDLE                 hThisObject,
                The object handle;

                ULONG                       uPriority,
                The input priority level;

                ULONG*                      pulSessionID
                The output buffer of session ID;

    return:     status of the operation;

**********************************************************************/
int
CcspCrRequestSessionID
    (
        ANSC_HANDLE                 hThisObject,
        ULONG                       uPriority,
        ULONG*                      pulSessionID
    )
{
    PCCSP_CR_MANAGER_OBJECT         pThisObject = (PCCSP_CR_MANAGER_OBJECT)hThisObject;

    if( pThisObject->bInSession) /* if it's in session already */
    {
        if( uPriority <= pThisObject->uPriority)
        {
            return CCSP_CR_ERR_SESSION_IN_PROGRESS;
        }
    }    
        
    /* create new session and return */
    pThisObject->uSessionID ++;

    pThisObject->uPriority = uPriority;

    if( pulSessionID != NULL)
    {
        *pulSessionID = pThisObject->uSessionID;
    }

    pThisObject->bInSession = TRUE;

#if 0  /* Commenting this part as sessionID is taken care of session_mgr of rbus */
    /* signal D-Bus about the new session */
    if( pThisObject->SignalProc.SignalSessionChangeProc != NULL)
    {
        pThisObject->SignalProc.SignalSessionChangeProc
            (
                pThisObject->hDbusHandle,
                pThisObject->uPriority,
                pThisObject->uSessionID
            );
    }
#endif

#ifdef   _DEBUG
    AnscTrace("CcspCrRequestSessionID - priority %lu, session id = %lu.\n", uPriority, pThisObject->uSessionID);
#endif

    return CCSP_SUCCESS;
}

/**********************************************************************

    prototype:

        int
        CcspCrGetCurrentSessionID
            (
                ANSC_HANDLE                 hThisObject,
                ULONG*                      pulPriority,
                ULONG*                      pulSessionID
            );

    description:

        This function is called to get current session ID;

    argument:   
                ANSC_HANDLE                 hThisObject,
                The object handle;

                ULONG*                      pulPriority,
                The output buffer of current priority;

                ULONG*                      pulSessionID
                The output buffer of current session ID;

    return:     status of the operation;

**********************************************************************/
int
CcspCrGetCurrentSessionID
    (
        ANSC_HANDLE                 hThisObject,
        ULONG*                      pulPriority,
        ULONG*                      pulSessionID
    )
{
    PCCSP_CR_MANAGER_OBJECT         pThisObject = (PCCSP_CR_MANAGER_OBJECT)hThisObject;

    if( pThisObject->bInSession) /* if it's in session already */
    {
        if( pulPriority != NULL)
        {
            *pulPriority = pThisObject->uPriority;
        }

        if( pulSessionID != NULL)
        {
            *pulSessionID = pThisObject->uSessionID;
        }
    }
    else
    {
        if( pulPriority != NULL)
        {
            *pulPriority = 0;
        }

        if( pulSessionID != NULL)
        {
            *pulSessionID = 0;
        }
    }

#ifdef   _DEBUG
    AnscTrace("CcspCrGetCurrentSessionID - current session id = %lu.\n", pThisObject->uSessionID);
#endif

    return CCSP_SUCCESS;
}


/**********************************************************************

    prototype:

        int
        CcspCrInformEndOfSession
            (
                ANSC_HANDLE                 hThisObject,
                ULONG                       ulSessionID
            );

    description:

        This function is called to end current session ID;

    argument:   
                ANSC_HANDLE                 hThisObject,
                The object handle;

                ULONG                       ulSessionID
                The input session ID
    return:     status of the operation;

**********************************************************************/
int
CcspCrInformEndOfSession
    (
        ANSC_HANDLE                 hThisObject,
        ULONG                       ulSessionID
    )
{
    PCCSP_CR_MANAGER_OBJECT         pThisObject = (PCCSP_CR_MANAGER_OBJECT)hThisObject;

    if( pThisObject->bInSession) /* if it's in session already */
    {
        if( ulSessionID == pThisObject->uSessionID)
        {
            pThisObject->bInSession = FALSE;

#if 0  /* Commenting this part as sessionID is taken care of session_mgr of rbus */
            /* signal D-Bus about the new session */
            if( pThisObject->SignalProc.SignalSessionChangeProc != NULL)
            {
                pThisObject->SignalProc.SignalSessionChangeProc
                    (
                        pThisObject->hDbusHandle,
                        0,
                        0
                    );
            }
#endif
        }
    }

#ifdef   _DEBUG
    AnscTrace("CcspCrInformEndOfSession - session id = %lu.\n", ulSessionID);
#endif

    return CCSP_SUCCESS;
}

