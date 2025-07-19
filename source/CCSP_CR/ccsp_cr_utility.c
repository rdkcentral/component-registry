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

    module: ccsp_cr_utility.c

        For Common Component Software Platform (CCSP) Development

    ---------------------------------------------------------------

    description:

        This module implements utility functions used by the object.

        *   CcspCrLookforRemoteCR
        *   CcspCrLookforComponent
        *   CcspCrCleanAll
        *   CcspCrDumpObject
        *   CcspCrIsSystemReady
        *   CcspCrSetPrefix

    ---------------------------------------------------------------

    environment:

        platform independent

    ---------------------------------------------------------------

    author:

        Bin Zhu 

    ---------------------------------------------------------------

    revision:

        06/21/2011    initial revision.
        09/20/2011    add api to look for remote CR path specified by prefix;

**********************************************************************/

#include "ansc_platform.h"

#include "ansc_ato_interface.h"
#include "ansc_ato_external_api.h"

#include "ccsp_cr_definitions.h"
#include "ccsp_cr_interface.h"
#include "ccsp_cr_profile.h"
#include "ccsp_cr_internal_api.h"
#include "ccsp_namespace_mgr.h"

#include "ccsp_base_api.h"

#include <telemetry_busmessage_sender.h>

/**********************************************************************

    prototype:

        char*
        CcspCrLookforRemoteCR
            (
                ANSC_HANDLE                 hCcspCr,
                char*                       pPrefix
            );

    description:

        This function is called to look for remote cr path specified by name.

    argument:   ANSC_HANDLE                                 hCcspCr
                the handle of CCSP CR component;

                char*                       pPrefix
                The specified subsystem prefix;

    return:     The dbus path if found;

**********************************************************************/
char*
CcspCrLookforRemoteCR
    (
        ANSC_HANDLE                 hThisObject,
        char*                       pPrefix
    )
{
    PCCSP_CR_MANAGER_OBJECT         pMyObject         = (PCCSP_CR_MANAGER_OBJECT)hThisObject;
    PCCSP_REMOTE_CRINFO             pCompInfo         = (PCCSP_REMOTE_CRINFO)NULL;
    PSINGLE_LINK_ENTRY              pSLinkEntry       = (PSINGLE_LINK_ENTRY)NULL;

    if( pPrefix == NULL || AnscSizeOfString(pPrefix) == 0)
    {
        return NULL;
    }

    pSLinkEntry = AnscQueueGetFirstEntry(&pMyObject->RemoteCRQueue);

    while ( pSLinkEntry )
    {
        pCompInfo       = ACCESS_CCSP_REMOTE_CRINFO(pSLinkEntry);
        pSLinkEntry     = AnscQueueGetNextEntry(pSLinkEntry);

        if (strcmp(pCompInfo->pPrefix, pPrefix) == 0)
        {
            return  pCompInfo->pID;
        }
    }

    return NULL;

}

/**********************************************************************

    prototype:

        ANSC_HANDLE
        CcspCrLookforComponent
            (
                ANSC_HANDLE                 hCcspCr,
                const char*                 pCompName
            );

    description:

        This function is called to look for component infor specified by name.

    argument:   ANSC_HANDLE                                 hCcspCr
                the handle of CCSP CR component;

                const char*                 pCompName
                The specified component name;

    return:     The handle of found component info;

**********************************************************************/
static BOOL ccspStringEndswith
    (
        char*                       pString,
        char*                       pStrEnds
    )
{
    char*                           pLooking = _ansc_strstr(pString, pStrEnds);

    if( pLooking == NULL)
    {
        return FALSE;
    }
  
    if (strcmp(pLooking, pStrEnds) == 0)
    {
        return TRUE;
    }

    return FALSE;
}

ANSC_HANDLE
CcspCrLookforComponent
    (
        ANSC_HANDLE                 hCcspCr,
        const char*                 pCompName
    )
{
    PCCSP_CR_MANAGER_OBJECT         pMyObject         = (PCCSP_CR_MANAGER_OBJECT)hCcspCr;
    PCCSP_COMPONENT_INFO            pCompInfo         = (PCCSP_COMPONENT_INFO)NULL;
    PSINGLE_LINK_ENTRY              pSLinkEntry       = (PSINGLE_LINK_ENTRY)NULL;

    pSLinkEntry = AnscQueueGetFirstEntry(&pMyObject->CompInfoQueue);

    while ( pSLinkEntry )
    {
        pCompInfo       = ACCESS_CCSP_COMPONENT_INFO(pSLinkEntry);
        pSLinkEntry     = AnscQueueGetNextEntry(pSLinkEntry);

        /* Since component in the device profile doesn't have prefix included while registerCapabilities does,
         * we don't need a perfect match here */
        /* if ( pCompName != NULL && (strcmp(pCompInfo->pComponentName, (char*)pCompName) == 0)) */
        if ( pCompName != NULL && ccspStringEndswith((char*)pCompName, pCompInfo->pComponentName))
        {
            return  (ANSC_HANDLE)pCompInfo;
        }
    }

    return NULL;
}


/**********************************************************************

    prototype:

        BOOL
        CcspCrCleanAll
            (
                ANSC_HANDLE                 hCcspCr
            );

    description:

        This function is called to clean all the memories.

    argument:   ANSC_HANDLE                                 hCcspCr
                the handle of CCSP CR component;

    return:     The status of the operation;

**********************************************************************/
BOOL
CcspCrCleanAll
    (
        ANSC_HANDLE                 hCcspCr
    )
{
    PCCSP_CR_MANAGER_OBJECT         pMyObject         = (PCCSP_CR_MANAGER_OBJECT)hCcspCr;
    PCCSP_COMPONENT_INFO            pCompInfo         = (PCCSP_COMPONENT_INFO)NULL;
    PCCSP_REMOTE_CRINFO             pCRInfo           = (PCCSP_REMOTE_CRINFO)NULL;
    PSINGLE_LINK_ENTRY              pSLinkEntry       = (PSINGLE_LINK_ENTRY)NULL;
    PCCSP_NAMESPACE_MGR_OBJECT      pNSMgr            = (PCCSP_NAMESPACE_MGR_OBJECT)pMyObject->hCcspNamespaceMgr;

    pMyObject->bSystemReady = FALSE;

    if( pNSMgr != NULL)
    {
        CcspFreeNamespaceMgr(CCSP_CR_NAME, pMyObject->hCcspNamespaceMgr);

        pMyObject->hCcspNamespaceMgr = NULL;
    }

    if( pMyObject->pDeviceName != NULL)
    {
        AnscFreeMemory(pMyObject->pDeviceName);

        pMyObject->pDeviceName = NULL;
    }

    if( pMyObject->pCRName != NULL)
    {
        AnscFreeMemory(pMyObject->pCRName);

        pMyObject->pCRName = NULL;
    }

    if( pMyObject->pPrefix != NULL)
    {
        AnscFreeMemory(pMyObject->pPrefix);

        pMyObject->pPrefix = NULL;
    }

    /* delete the component infor objects */
    if ( TRUE )
    {
        pSLinkEntry = AnscQueuePopEntry(&pMyObject->CompInfoQueue);

        while ( pSLinkEntry )
        {
            pCompInfo       = ACCESS_CCSP_COMPONENT_INFO(pSLinkEntry);
            pSLinkEntry     = AnscQueuePopEntry(&pMyObject->CompInfoQueue);

            AnscFreeMemory(pCompInfo->pComponentName);
            AnscFreeMemory(pCompInfo);
        }
    }

    if ( TRUE )
    {
        pSLinkEntry = AnscQueuePopEntry(&pMyObject->UnknowCompInfoQueue);

        while ( pSLinkEntry )
        {
            pCompInfo       = ACCESS_CCSP_COMPONENT_INFO(pSLinkEntry);
            pSLinkEntry     = AnscQueuePopEntry(&pMyObject->UnknowCompInfoQueue);

            AnscFreeMemory(pCompInfo->pComponentName);
            AnscFreeMemory(pCompInfo);
        }
    }

    if ( TRUE )
    {
        pSLinkEntry = AnscQueuePopEntry(&pMyObject->RemoteCRQueue);

        while ( pSLinkEntry )
        {
            pCRInfo         = ACCESS_CCSP_REMOTE_CRINFO(pSLinkEntry);
            pSLinkEntry     = AnscQueuePopEntry(&pMyObject->RemoteCRQueue);

            AnscFreeMemory(pCRInfo->pPrefix);
            AnscFreeMemory(pCRInfo->pID);
            AnscFreeMemory(pCRInfo);
        }
    }

    return TRUE;
}

/**********************************************************************

    prototype:

        BOOL
        CcspCrDumpObject
            (
                ANSC_HANDLE                 hCcspCr
            );

    description:

        This function is called to dump the objects information.

    argument:   ANSC_HANDLE                                 hCcspCr
                the handle of CCSP CR component;

    return:     The status of the operation;

**********************************************************************/
BOOL
CcspCrDumpObject
    (
        ANSC_HANDLE                 hCcspCr
    )
{
    PCCSP_CR_MANAGER_OBJECT         pMyObject         = (PCCSP_CR_MANAGER_OBJECT)hCcspCr;
    PCCSP_COMPONENT_INFO            pCompInfo         = (PCCSP_COMPONENT_INFO)NULL;
    PSINGLE_LINK_ENTRY              pSLinkEntry       = (PSINGLE_LINK_ENTRY)NULL;
    PCCSP_NAMESPACE_MGR_OBJECT      pNSMgr            = (PCCSP_NAMESPACE_MGR_OBJECT)pMyObject->hCcspNamespaceMgr;
    ULONG                           i                 = 0;

    AnscTrace("CR Name:         %s\n", CCSP_CR_NAME);
    AnscTrace("CR Version:      %d\n", CCSP_CR_VERSION);

    if( pMyObject->bSystemReady)
    {
        AnscTrace("System is ready.\n");
    }
    else
    {
        AnscTrace("System is not ready yet.\n");
    }

    if( pNSMgr != NULL)
    {
        AnscTrace("Total Namespace Registered:  %lu\n", pNSMgr->GetCountOfRegNamespace(pNSMgr));
    }

    pSLinkEntry = AnscQueueGetFirstEntry(&pMyObject->CompInfoQueue);

    if( pSLinkEntry != NULL)
    {
        AnscTrace("Component Information:\n");
    }

    while ( pSLinkEntry )
    {
        pCompInfo       = ACCESS_CCSP_COMPONENT_INFO(pSLinkEntry);
        pSLinkEntry     = AnscQueueGetNextEntry(pSLinkEntry);

        if( pCompInfo != NULL)
        {
            i ++;

            if( pCompInfo->uStatus == CCSP_Component_NotRegistered)
            {
		AnscTrace("#%.2lu %s v%lu NotRegistered\n", i,
			  pCompInfo->pComponentName, pCompInfo->uVersion);
		if (strstr(pCompInfo->pComponentName, "ccsp.tr069pa"))
		{
		    t2_event_d("SYS_ERROR_TR69_Not_Registered", 1);
		}
            }
            else if( pCompInfo->uStatus == CCSP_Component_RegSuccessful)
            {
                AnscTrace("#%.2lu %s v%lu RegSucceeded\n", i, pCompInfo->pComponentName, pCompInfo->uVersion);
            }
            else
            {
                AnscTrace("#%.2lu %s v%lu RegFailed\n", i, pCompInfo->pComponentName, pCompInfo->uVersion);
            }
        }
    }


    return TRUE;
}

/**********************************************************************

    prototype:

        BOOL
        CcspCrIsSystemReady
            (
                ANSC_HANDLE                 hCcspCr
            );

    description:

        This function is called to check whether the system is ready or not;

    argument:   ANSC_HANDLE                                 hCcspCr
                the handle of CCSP CR component;

    return:     Yes or No;

**********************************************************************/
BOOL
CcspCrIsSystemReady
    (
        ANSC_HANDLE                 hCcspCr
    )
{
    PCCSP_CR_MANAGER_OBJECT         pMyObject         = (PCCSP_CR_MANAGER_OBJECT)hCcspCr;

    return pMyObject->bSystemReady;
}

/**********************************************************************

    prototype:

        void
        CcspCrSetPrefix
            (
                ANSC_HANDLE                 hCcspCr,
                char*                       pPrefix
            );

    description:

        This function is called to set subsystem prefix of CR;

    argument:   ANSC_HANDLE                                 hCcspCr
                the handle of CCSP CR component;

                char*                                       pPrefix
                The prefix stirng;

    return:     none

**********************************************************************/
void
CcspCrSetPrefix
    (
        ANSC_HANDLE                 hCcspCr,
        char*                       pPrefix
    )
{
    PCCSP_CR_MANAGER_OBJECT         pMyObject         = (PCCSP_CR_MANAGER_OBJECT)hCcspCr;
    PCCSP_NAMESPACE_MGR_OBJECT      pNSMgr            = (PCCSP_NAMESPACE_MGR_OBJECT)pMyObject->hCcspNamespaceMgr;

    pMyObject->pPrefix = AnscCloneString(pPrefix);
    pNSMgr->SubsysPrefix = AnscCloneString(pPrefix);
}
