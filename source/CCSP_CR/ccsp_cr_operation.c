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

    module: ccsp_cr_operation.c

        For Common Component Software Platform (CCSP) Development

    ---------------------------------------------------------------

    description:

        This module implements CR external apis.

        *   CcspCrRegisterCapabilities
        *   CcspCrUnregisterNamespace
        *   CcspCrUnregisterComponent
        *   CcspCrDiscoverComponentSupportingNamespace
        *   CcspCrDiscoverComponentSupportingDynamicTbl
        *   CcspCrCheckNamespaceDataType
        *   CcspCrGetRegisteredComponents
        *   CcspCrGetNamespaceByComponent
        *   CcspCrAfterComponentLost

    ---------------------------------------------------------------

    environment:

        platform independent

    ---------------------------------------------------------------

    author:

        Bin Zhu 

    ---------------------------------------------------------------

    revision:

        06/21/2011    initial revision.

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

extern BOOLEAN g_exportAllDM; 
extern void GenerateDataModelXml(void);

/**********************************************************************

    prototype:

        int
        CcspCrRegisterCapabilities
            (
                ANSC_HANDLE                 hCcspCr,
                const char*                 pCompName,
                ULONG                       compVersion,
                const char*                 pDbusPath,
                const char*                 pPrefix,
                PVOID*                      pRegNamespace,
                ULONG                       ulSize
            );

    description:

        This function is called to by every CCSP component to register its 
        own name space to CR.

    argument:   ANSC_HANDLE                                 hCcspCr
                the handle of CCSP CR component;

                const char*                 pCompName,
                The component name;

                ULONG                       compVersion,
                The component version;

                const char*                 pDbusPath,
                The component D-Bus path;

                const char*                 pPrefix,
                The subsystem prefix;

                PVOID*                      pRegNamespace,
                The array of name space information;

                ULONG                       ulSize
                The size of the array;

    return:     The status of the operation

**********************************************************************/
int
CcspCrRegisterCapabilities
    (
        ANSC_HANDLE                 hCcspCr,
        const char*                 pCompName,
        ULONG                       compVersion,
        const char*                 pDbusPath,
        const char*                 pPrefix,
        PVOID*                      pRegNamespace,
        ULONG                       ulSize
    )
{
    PCCSP_CR_MANAGER_OBJECT         pMyObject         = (PCCSP_CR_MANAGER_OBJECT)hCcspCr;
    PCCSP_NAMESPACE_MGR_OBJECT      pNSMgr            = (PCCSP_NAMESPACE_MGR_OBJECT)pMyObject->hCcspNamespaceMgr;
    PCCSP_COMPONENT_INFO            pCompInfo         = (PCCSP_COMPONENT_INFO)NULL;
    PSINGLE_LINK_ENTRY              pSLinkEntry       = (PSINGLE_LINK_ENTRY)NULL;
    int                             iStatus           = CCSP_SUCCESS;

    if( pCompName == NULL || pDbusPath == NULL)
    {
        return CCSP_CR_ERR_INVALID_PARAM;
    }

    if( pNSMgr == NULL)
    {
        return CCSP_ERR_MEMORY_ALLOC_FAIL;
    }

    /* check whether this component is in the profile or not */
    pCompInfo = (PCCSP_COMPONENT_INFO)
        CcspCrLookforComponent( hCcspCr, pCompName);

    if( pCompInfo == NULL)
    {
        AnscTraceWarning(("Unknown Registered Component:  %s\n", pCompName));

#if 0   /* still register it though */
        return CCSP_CR_ERR_UNKNOWN_COMPONENT;
#endif

    }
    else if( pCompInfo->uVersion != compVersion)
    {
        AnscTraceWarning(("Mismatched Version of component '%s'. Expect version %lu not %lu. \n", pCompName, pCompInfo->uVersion, compVersion));

        return CCSP_CR_ERR_UNKNOWN_COMPONENT;
    }


    /* register the component in NamespaceMgr object */
    if( pPrefix == NULL || AnscSizeOfString(pPrefix) == 0)
    {
        iStatus = pNSMgr->RegisterNamespaces(pNSMgr, pCompName, pDbusPath, pMyObject->pPrefix, pRegNamespace, ulSize);
    	AnscTraceWarning(("RegisterNamespaces %s/%s for NULL prefix - returned %d\n",
    			pCompName, pDbusPath, iStatus));
    }
    else
    {
        iStatus = pNSMgr->RegisterNamespaces(pNSMgr, pCompName, pDbusPath, pPrefix, pRegNamespace, ulSize);
    	AnscTraceWarning(("RegisterNamespaces %s/%s for prefix %s - returned %d\n",
    			pCompName, pDbusPath, pPrefix, iStatus));
    }

    if( pCompInfo != NULL)
    {
        if(pCompInfo->uStatus != CCSP_Component_RegSuccessful && iStatus != CCSP_SUCCESS)
        {
            pCompInfo->uStatus = CCSP_Component_RegFailed;
        	AnscTraceWarning(("CcspCrRegisterCapabilities - component registration failed %s\n", pCompInfo->pComponentName));

            return iStatus;
        }

        pCompInfo->uStatus = CCSP_Component_RegSuccessful;

        /* check whether system is ready or not */
        if(!pMyObject->bSystemReady)
        {
            pSLinkEntry = AnscQueueGetFirstEntry(&pMyObject->CompInfoQueue);

            while ( pSLinkEntry )
            {
                pCompInfo       = ACCESS_CCSP_COMPONENT_INFO(pSLinkEntry);
                pSLinkEntry     = AnscQueueGetNextEntry(pSLinkEntry);

                AnscTraceWarning(("CcspCrRegisterCapabilities - print component %s with status %lu\n",
            			pCompInfo->pComponentName, pCompInfo->uStatus));
            }

            pSLinkEntry = AnscQueueGetFirstEntry(&pMyObject->CompInfoQueue);

            while ( pSLinkEntry )
            {
                pCompInfo       = ACCESS_CCSP_COMPONENT_INFO(pSLinkEntry);
                pSLinkEntry     = AnscQueueGetNextEntry(pSLinkEntry);

                AnscTraceWarning(("CcspCrRegisterCapabilities - component %s status %lu\n",
            			pCompInfo->pComponentName, pCompInfo->uStatus));

                if( pCompInfo->uStatus != CCSP_Component_RegSuccessful)
                {
                    break;
                }
            }

            if( pSLinkEntry == NULL)
            {
                pMyObject->bSystemReady = TRUE;
                
                /*Collect all Data model and generate /tmp/datamodel.xml*/                
                if(g_exportAllDM) 
                    GenerateDataModelXml();

                /* send out System Ready event */
                AnscTraceWarning(("From CR: System is ready...\n"));

                if( pMyObject->SignalProc.SignalSystemReadyProc != NULL)
                {
                    pMyObject->SignalProc.SignalSystemReadyProc(pMyObject->hDbusHandle);
                }

            }
        }
    }

    return iStatus;
}


/**********************************************************************

    prototype:

        int
        CcspCrUnregisterNamespace
            (
                ANSC_HANDLE                 hCcspCr,
                const char*                 pCompName,
                const char*                 pNamespace
            );

    description:

        This function is called to Unregister specified name space.

    argument:   ANSC_HANDLE                                 hCcspCr
                the handle of CCSP CR component;

                const char*                 pCompName,
                The component name;

                const char*                 pNamespace
                The specified name space;

    return:     The status of the operation

**********************************************************************/
int
CcspCrUnregisterNamespace
    (
        ANSC_HANDLE                 hCcspCr,
        const char*                 pCompName,
        const char*                 pNamespace
    )
{
    PCCSP_CR_MANAGER_OBJECT         pMyObject         = (PCCSP_CR_MANAGER_OBJECT)hCcspCr;
    PCCSP_NAMESPACE_MGR_OBJECT      pNSMgr            = (PCCSP_NAMESPACE_MGR_OBJECT)pMyObject->hCcspNamespaceMgr;
    PCCSP_COMPONENT_INFO            pCompInfo         = (PCCSP_COMPONENT_INFO)NULL;

    if( pCompName == NULL || pNamespace == NULL)
    {
        return CCSP_CR_ERR_INVALID_PARAM;
    }

    /* check whether this component is in the profile or not */
    pCompInfo = (PCCSP_COMPONENT_INFO)
        CcspCrLookforComponent( hCcspCr, pCompName);

    if( pCompInfo == NULL)
    {
        return CCSP_CR_ERR_UNKNOWN_COMPONENT;
    }

    return pNSMgr->UnregisterNamespace(pNSMgr, pCompName, pNamespace);
}

/**********************************************************************

    prototype:

        int
        CcspCrUnregisterComponent
            (
                ANSC_HANDLE                 hCcspCr,
                const char*                 pCompName
            );

    description:

        This function is called to Unregister component specified by name;

    argument:   ANSC_HANDLE                                 hCcspCr
                the handle of CCSP CR component;

                const char*                 pCompName,
                The component name;

    return:     The status of the operation

**********************************************************************/
int
CcspCrUnregisterComponent
    (
        ANSC_HANDLE                 hCcspCr,
        const char*                 pCompName
    )
{
    PCCSP_CR_MANAGER_OBJECT         pMyObject         = (PCCSP_CR_MANAGER_OBJECT)hCcspCr;
    PCCSP_NAMESPACE_MGR_OBJECT      pNSMgr            = (PCCSP_NAMESPACE_MGR_OBJECT)pMyObject->hCcspNamespaceMgr;
    PCCSP_COMPONENT_INFO            pCompInfo         = (PCCSP_COMPONENT_INFO)NULL;
    int                             iStatus           = CCSP_SUCCESS;

    if( pCompName == NULL)
    {
        return CCSP_CR_ERR_INVALID_PARAM;
    }

    /* check whether this component is in the profile or not */
    pCompInfo = (PCCSP_COMPONENT_INFO)
        CcspCrLookforComponent( hCcspCr, pCompName);

    iStatus = pNSMgr->UnregisterComponent(pNSMgr, pCompName);

    if( pCompInfo != NULL)
    {
        if( iStatus == CCSP_SUCCESS)
        {
            /* Don't pop it yet. It may re-register */
            AnscTrace("Component '%s' is unregistered successfully.\n", pCompName);
            pCompInfo->uStatus = CCSP_Component_NotRegistered;

    #if 0
            AnscQueuePopEntryByLink(&pMyObject->CompInfoQueue, &pCompInfo->Linkage);
            AnscFreeMemory(pCompInfo->pComponentName);
            AnscFreeMemory(pCompInfo);
    #endif
        }
    }

    return iStatus;
}


/**********************************************************************

    prototype:

        int
        CcspCrDiscoverComponentSupportingNamespace
            (
                ANSC_HANDLE                 hThisObject,        
                const char*                 pNamespace,
                const char*                 pPrefix,
                BOOL                        bNextLevel,
                PVOID**                     ppComponent,
                ULONG*                      pulSize
            );

    description:

        This function is called to discover components supporting the
        specified name space;

    argument:   ANSC_HANDLE                 hCcspCr
                the handle of CCSP CR component;

                const char*                 pNamespace,
                The specified name space;

                const char*                 pPrefix,
                The specified prefix;

                BOOL                        bNextLevel,
                next level is true or not;

                PVOID**                     ppComponent,
                The output component array;

                ULONG*                      pulSize
                The output buffer of the size of the array;

    return:     The status of the operation

**********************************************************************/
int
CcspCrDiscoverComponentSupportingNamespace
    (
        ANSC_HANDLE                 hCcspCr,        
        const char*                 pNamespace,
        const char*                 pPrefix,
        BOOL                        bNextLevel,
        PVOID**                     ppComponent,
        ULONG*                      pulSize
    )
{
    PCCSP_CR_MANAGER_OBJECT         pMyObject         = (PCCSP_CR_MANAGER_OBJECT)hCcspCr;
    PCCSP_NAMESPACE_MGR_OBJECT      pNSMgr            = (PCCSP_NAMESPACE_MGR_OBJECT)pMyObject->hCcspNamespaceMgr;
    int                             iStatus           = CCSP_SUCCESS;
    char*                           remoteCR          = NULL;
    componentStruct_t**             ppCompStruct      = (componentStruct_t**)NULL;

    /* Too many trace. Disable this -- Yan*/
    /* AnscTrace("CR- discoverComponentSupportingNamespace.\n"); */
	
    if( pNamespace == NULL || ppComponent == NULL)
    {
        return CCSP_CR_ERR_INVALID_PARAM;
    }

    if( pPrefix == NULL || AnscSizeOfString(pPrefix) == 0)
    {
	    iStatus = pNSMgr->DiscoverNamespace(pNSMgr, pNamespace, pMyObject->pPrefix, bNextLevel, ppComponent, pulSize);
	}
	else
	{
	    iStatus = pNSMgr->DiscoverNamespace(pNSMgr, pNamespace, pPrefix, bNextLevel, ppComponent, pulSize);	
	}    

    if( iStatus != CCSP_SUCCESS)
    {
        /* we try with remote CR */
        remoteCR = CcspCrLookforRemoteCR(hCcspCr, (char*)pPrefix);

        if( remoteCR != NULL)
        {

            iStatus = 
                CcspBaseIf_discComponentSupportingNamespace 
                (
                    pMyObject->hDbusHandle,
                    remoteCR,
                    pNamespace,
                    pPrefix,
                    &ppCompStruct,
                    (int*)pulSize
                );

            *ppComponent = (PVOID*)ppCompStruct;

        }
    }

    return iStatus;
}

/**********************************************************************

    prototype:

        int
        CcspCrDiscoverComponentSupportingDynamicTbl
            (
                ANSC_HANDLE                 hThisObject,        
                const char*                 pNamespace,
                const char*                 pPrefix,
                BOOL                        bNextLevel,
                PVOID*                      pComponent
            );

    description:

        This function is called to discover components supporting the
        specified dynamic table name space;

    argument:   ANSC_HANDLE                 hCcspCr
                the handle of CCSP CR component;

                const char*                 pNamespace,
                The specified name space;

                const char*                 pPrefix,
                The specified prefix;

                BOOL                        bNextLevel,
                next level is true or not;

                PVOID*                      pComponent
                The output component struct;


    return:     The status of the operation

**********************************************************************/
int
CcspCrDiscoverComponentSupportingDynamicTbl
    (
        ANSC_HANDLE                 hCcspCr,        
        const char*                 pNamespace,
        const char*                 pPrefix,
        BOOL                        bNextLevel,
        PVOID*                      pOutComponent
    )
{
    PCCSP_CR_MANAGER_OBJECT         pMyObject         = (PCCSP_CR_MANAGER_OBJECT)hCcspCr;
    PCCSP_NAMESPACE_MGR_OBJECT      pNSMgr            = (PCCSP_NAMESPACE_MGR_OBJECT)pMyObject->hCcspNamespaceMgr;
    PVOID*                          pComponent        = NULL;
    ULONG                           ulSize            = 0;
    int                             iStatus           = CCSP_SUCCESS;
    ULONG                           uLength           = 0;
    componentStruct_t*              pCompStruct       = (componentStruct_t*)NULL;    

    if( pNamespace == NULL || pOutComponent == NULL)
    {
        return CCSP_CR_ERR_INVALID_PARAM;
    }

    uLength = AnscSizeOfString(pNamespace);

    if( _ansc_strstr(pNamespace, TR69_NAME_TABLE_END) != (char*)(pNamespace + uLength - AnscSizeOfString(TR69_NAME_TABLE_END))) /* end with an entry */
    {
        return CCSP_CR_ERR_INVALID_PARAM;        
    }

    if( pPrefix == NULL || AnscSizeOfString(pPrefix) == 0)
    {
	    iStatus = pMyObject->DiscoverComponentSupportingNamespace(pNSMgr, pNamespace, pMyObject->pPrefix,bNextLevel, &pComponent, &ulSize);	    
	}
	else
	{
	    iStatus = pMyObject->DiscoverComponentSupportingNamespace(pNSMgr, pNamespace, pPrefix,bNextLevel, &pComponent, &ulSize);
	}    


    if( iStatus != CCSP_SUCCESS)
    {
        return iStatus;
    }

    if( ulSize == 0 || pComponent == NULL || pComponent[0] == NULL)
    {
        *pOutComponent  = NULL;

        return iStatus;
    }

    if( ulSize > 1)
    {
        AnscTraceWarning(("WARNING: There're multiple component support a table object: %s\n", pNamespace));
    }

    pCompStruct     = (componentStruct_t*)pComponent[0];
    *pOutComponent  = pCompStruct;

    CcspNsMgrFreeMemory(pNSMgr->pContainerName, pComponent);

    return iStatus;
}

/**********************************************************************

    prototype:

        int
        CcspCrCheckNamespaceDataType
            (
                ANSC_HANDLE                 hCcspCr,        
                const char*                 pNamespace,
                ULONG                       uType,
                BOOL*                       pbMatch
            );

    description:

        This function is called to check the data type of specified name space;

    argument:   ANSC_HANDLE                                 hCcspCr
                the handle of CCSP CR component;

                const char*                 pNamespace,
                The specified name space;

                ULONG                       uType
                The data type assumed;

                BOOL*                       pbMatch
                The output buffer of match flag;

    return:     The status of the operation

**********************************************************************/
int
CcspCrCheckNamespaceDataType
    (
        ANSC_HANDLE                 hCcspCr,        
        const char*                 pNamespace,
        ULONG                       uType,
        BOOL*                       pbMatch
    )
{
    PCCSP_CR_MANAGER_OBJECT         pMyObject         = (PCCSP_CR_MANAGER_OBJECT)hCcspCr;
    PCCSP_NAMESPACE_MGR_OBJECT      pNSMgr            = (PCCSP_NAMESPACE_MGR_OBJECT)pMyObject->hCcspNamespaceMgr;

    return pNSMgr->CheckNamespaceDataType(pNSMgr, pNamespace, uType, pbMatch);
}


/**********************************************************************

    prototype:

        int
        CcspCrGetRegisteredComponents
            (
                ANSC_HANDLE                 hCcspCr,
                PVOID**                     ppComponent,
                ULONG*                      pulSize
            );

    description:

        This function is called to retrieve the name array of all 
        the registered components.

    argument:   ANSC_HANDLE                 hCcspCr
                the handle of CCSP CR component;

                PVOID**                     ppComponent,
                The output component name array;

                ULONG*                      pulSize
                The output buffer of size of array;

    return:     The status of the operation

**********************************************************************/
int
CcspCrGetRegisteredComponents
    (
        ANSC_HANDLE                 hCcspCr,        
        PVOID**                     ppComponent,
        ULONG*                      pulSize
    )
{
    PCCSP_CR_MANAGER_OBJECT         pMyObject         = (PCCSP_CR_MANAGER_OBJECT)hCcspCr;
    PCCSP_NAMESPACE_MGR_OBJECT      pNSMgr            = (PCCSP_NAMESPACE_MGR_OBJECT)pMyObject->hCcspNamespaceMgr;
    ULONG                           uCount            = pMyObject->CompInfoQueue.Depth;
    
    if( uCount == 0)
    {
       *pulSize      = 0;
       *ppComponent  = NULL;

       return CCSP_SUCCESS;
    }

    return pNSMgr->GetRegisteredComponents(pNSMgr, ppComponent, pulSize);
}

/**********************************************************************

    prototype:

        int
        CcspCrGetNamespaceByComponent
            (
                ANSC_HANDLE                 hCcspCr,
                const char*                 pCompName,                
                PVOID**                     ppNamespace,
                ULONG*                      pulSize
            );

    description:

        This function is called to retrieve the supported name space specified
        by the component name.

    argument:   ANSC_HANDLE                                 hCcspCr
                the handle of CCSP CR component;

                const char*                 pCompName,                
                The specified component name.

                PVOID**                     ppNamespace,
                The output name space array;

                ULONG*                      pulSize
                The output buffer of size of array;

    return:     The status of the operation

**********************************************************************/
int
CcspCrGetNamespaceByComponent
    (
        ANSC_HANDLE                 hCcspCr,
        const char*                 pCompName,                
        PVOID**                     ppNamespace,
        ULONG*                      pulSize
    )
{
    PCCSP_CR_MANAGER_OBJECT         pMyObject         = (PCCSP_CR_MANAGER_OBJECT)hCcspCr;
    PCCSP_NAMESPACE_MGR_OBJECT      pNSMgr            = (PCCSP_NAMESPACE_MGR_OBJECT)pMyObject->hCcspNamespaceMgr;

    return pNSMgr->GetNamespaceByComponent(pNSMgr, pCompName, ppNamespace, pulSize);
}
/**********************************************************************

    prototype:

        int
        CcspCrAfterComponentLost
            (
                ANSC_HANDLE                 hCcspCr,
                const char*                 pComponentName
            );

    description:

        This function is called after CR detected the component lost connection
        with D-Bus.

    argument:   ANSC_HANDLE                                 hCcspCr
                the handle of CCSP CR component;

                const char*                 pComponentName
                The specified component name whose lost connection.

    return:     The status of the operation

**********************************************************************/
int
CcspCrAfterComponentLost
    (
        ANSC_HANDLE                 hCcspCr,
        const char*                 pComponentName
    )
{
    PCCSP_CR_MANAGER_OBJECT         pMyObject         = (PCCSP_CR_MANAGER_OBJECT)hCcspCr;
    PCCSP_NAMESPACE_MGR_OBJECT      pNSMgr            = (PCCSP_NAMESPACE_MGR_OBJECT)pMyObject->hCcspNamespaceMgr;
    PCCSP_NAMESPACE_COMP_OBJECT     pCompInfo         = (PCCSP_NAMESPACE_COMP_OBJECT)NULL;

    /* check whether this component is in the profile or not */
    pCompInfo = (PCCSP_NAMESPACE_COMP_OBJECT)pNSMgr->LookforComponent(pNSMgr, pComponentName);

    if( pCompInfo == NULL)
    {
        return CCSP_SUCCESS;
    }

    /* send out the deviceProfileChangeSignal() with the specified Component Name */
    if( pMyObject->SignalProc.SignalProfileChangeProc != NULL)
    {
        pMyObject->SignalProc.SignalProfileChangeProc
            (
                pMyObject->hDbusHandle,
                (char*)pComponentName,
                pCompInfo->pDbusPath,
                FALSE
            );
    }

    return CCSP_SUCCESS;
}
