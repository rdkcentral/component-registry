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

        This module implements device profile related functions for 
        CR (Component Registrar) development.

        *  CcspCrLoadDeviceProfile
        *  CcspCrLoadComponentProfile
        *  CcspCrLoadRemoteCRInfo

    ---------------------------------------------------------------

    environment:

        platform independent

    ---------------------------------------------------------------

    author:

        Bin Zhu 

    ---------------------------------------------------------------

    revision:

        06/06/2011    initial revision.

**********************************************************************/

#include "ansc_platform.h"

#include "ansc_ato_interface.h"
#include "ansc_ato_external_api.h"

#include "ccsp_cr_definitions.h"
#include "ccsp_cr_interface.h"
#include "ccsp_cr_profile.h"
#include "ccsp_cr_internal_api.h"

#include "ccsp_base_api.h"

#include "ansc_xml_dom_parser_interface.h"
#include "ansc_xml_dom_parser_external_api.h"
#include "ansc_xml_dom_parser_status.h"

#include "ccsp_namespace_mgr.h"

/* define default CR device profile name */
#define CCSP_CR_DEVICE_PROFILE_XML_FILENAME         "cr-deviceprofile.xml"
#define CCSP_CR_DEVICE_PROFILE_XML_LOCATION         "/usr/ccsp/"
#define CCSP_CR_DEVICE_PROFILE_XML_FILE             CCSP_CR_DEVICE_PROFILE_XML_LOCATION CCSP_CR_DEVICE_PROFILE_XML_FILENAME

#define CCSP_CR_ETHWAN_DEVICE_PROFILE_XML_FILENAME  "cr-ethwan-deviceprofile.xml"
#define CCSP_CR_ETHWAN_DEVICE_PROFILE_XML_FILE      CCSP_CR_DEVICE_PROFILE_XML_LOCATION CCSP_CR_ETHWAN_DEVICE_PROFILE_XML_FILENAME

#define CCSP_ETHWAN_ENABLE "/nvram/ETHWAN_ENABLE"

/**********************************************************************

    prototype:

        BOOL
        CcspCrLoadDeviceProfile
            (
                ANSC_HANDLE                                 hCcspCr
            );

    description:

        This function is called to load device profile for CR.

    argument:   ANSC_HANDLE                                 hCcspCr
                the handle of CCSP CR component;

    return:     Succeeded or not;

**********************************************************************/
BOOL
CcspCrLoadDeviceProfile
    (
        ANSC_HANDLE                                 hCcspCr
    )
{
    PCCSP_CR_MANAGER_OBJECT         pMyObject          = (PCCSP_CR_MANAGER_OBJECT)hCcspCr;
    ANSC_HANDLE                     pFileHandle        = NULL;
    char*                           pXMLContent        = NULL;
    char*                           pBackContent       = NULL;
    PANSC_XML_DOM_NODE_OBJECT       pXmlNode           = (PANSC_XML_DOM_NODE_OBJECT)NULL;
    PANSC_XML_DOM_NODE_OBJECT       pListNode          = (PANSC_XML_DOM_NODE_OBJECT)NULL;
    PANSC_XML_DOM_NODE_OBJECT       pChildNode         = (PANSC_XML_DOM_NODE_OBJECT)NULL;
    BOOL                            bStatus            = TRUE;
    ULONG                           uFileLength        = 0;
    ULONG                           uBufferSize        = 0;
    char                            buffer[512]        = { 0 };
    ULONG                           uLength            = 511;
    PCCSP_COMPONENT_INFO            pCompInfo          = (PCCSP_COMPONENT_INFO)NULL;

    /* load from the file */
    if (access(CCSP_ETHWAN_ENABLE, F_OK) == 0)
    {
        pFileHandle = AnscOpenFile
        (
            CCSP_CR_ETHWAN_DEVICE_PROFILE_XML_FILE,
            ANSC_FILE_O_BINARY | ANSC_FILE_O_RDONLY,
            ANSC_FILE_S_IREAD
        );

        if( pFileHandle == NULL)
        {
            AnscTraceWarning(("Failed to load the file : " CCSP_CR_ETHWAN_DEVICE_PROFILE_XML_FILENAME "'\n"));
            return FALSE;
        }
    }
    else
    {
        pFileHandle = AnscOpenFile
        (
            CCSP_CR_DEVICE_PROFILE_XML_FILE,
            ANSC_FILE_O_BINARY | ANSC_FILE_O_RDONLY,
            ANSC_FILE_S_IREAD
        );

        if( pFileHandle == NULL)
        {
            AnscTraceWarning(("Failed to load the file : " CCSP_CR_DEVICE_PROFILE_XML_FILENAME "'\n"));
            return FALSE;
        }
    }

    uFileLength = AnscGetFileSize( pFileHandle);

    pXMLContent = (char*)AnscAllocateMemory( uFileLength + 8);

    if( pXMLContent == NULL)
    {
        AnscCloseFile(pFileHandle); /*RDKB-6901, CID-33521, free unused resources before exit */
        AnscTraceWarning(("Failed to allocate memory for pXMLContent'\n"));
        return FALSE;
    }

    uBufferSize = uFileLength + 8;

    if( AnscReadFile( pFileHandle, pXMLContent, &uBufferSize) != ANSC_STATUS_SUCCESS)
    {
        AnscFreeMemory(pXMLContent);
        AnscCloseFile(pFileHandle); /*RDKB-6901, CID-33521, free unused resources before exit */
        AnscTraceWarning(("AnscReadFile failure for file : " CCSP_CR_DEVICE_PROFILE_XML_FILENAME "'\n"));
        return FALSE;
    }

    if( pFileHandle != NULL)
    {
        AnscCloseFile(pFileHandle);
    }

     /*CID 137743 - String not null terminated */
    pXMLContent[uBufferSize] = '\0';
    /* parse the XML content */
    pBackContent = pXMLContent;

    pXmlNode = (PANSC_XML_DOM_NODE_OBJECT)
        AnscXmlDomParseString((ANSC_HANDLE)NULL, (PCHAR*)&pXMLContent, uBufferSize);

    AnscFreeMemory(pBackContent);

    if( pXmlNode == NULL)
    {
        AnscTraceWarning(("Failed to parse the file : " CCSP_CR_DEVICE_PROFILE_XML_FILENAME "'\n"));

        return FALSE;
    }

    /* load CR name */
    pChildNode  = (PANSC_XML_DOM_NODE_OBJECT)
    AnscXmlDomNodeGetChildByName(pXmlNode, CCSP_CR_XML_NODE_crName);

    if( pChildNode != NULL && pChildNode->GetDataString(pChildNode, NULL, buffer, &uLength) == ANSC_STATUS_SUCCESS && uLength > 0)
    {
        pMyObject->pCRName = AnscCloneString(buffer);
        AnscTrace("Setting CRName: %s,based on file : '%s'\n", pMyObject->pCRName, CCSP_CR_DEVICE_PROFILE_XML_FILENAME);
    }
    else
    {
        pMyObject->pCRName = AnscCloneString(CCSP_CR_NAME);
        AnscTrace("Setting default CR Name: %s \n", pMyObject->pCRName);
    }

#if 0
    /* load prefix name */
    /* Prefix will be set from command line instead */
    PCCSP_NAMESPACE_MGR_OBJECT      pNSMgr             = (PCCSP_NAMESPACE_MGR_OBJECT)pMyObject->hCcspNamespaceMgr;
    uLength            = 511;
    AnscZeroMemory(buffer, 512);

    pChildNode  = (PANSC_XML_DOM_NODE_OBJECT)
		AnscXmlDomNodeGetChildByName(pXmlNode, CCSP_CR_XML_NODE_prefix); 

    if( pChildNode != NULL && pChildNode->GetDataString(pChildNode, NULL, buffer, &uLength) == ANSC_STATUS_SUCCESS && uLength > 0)
    {
        pMyObject->pPrefix = AnscCloneString(buffer);

        pNSMgr->SubsysPrefix = AnscCloneString(buffer);

        AnscTraceWarning(("CR Prefix: %s\n", pMyObject->pPrefix));
    }
#endif

    /* get remote cr array */
    pListNode = (PANSC_XML_DOM_NODE_OBJECT)
		AnscXmlDomNodeGetChildByName(pXmlNode, CCSP_CR_XML_NODE_remote);

    if( pListNode != NULL)
    {
        pChildNode = (PANSC_XML_DOM_NODE_OBJECT)
		    AnscXmlDomNodeGetHeadChild(pListNode);

        while(pChildNode != NULL)
        {
            /* load remote cr information */
            if(!CcspCrLoadRemoteCRInfo(hCcspCr, (ANSC_HANDLE)pChildNode))
            {
                AnscTraceWarning(("Failed to load remote cr infor.\n"));
            }

            pChildNode = (PANSC_XML_DOM_NODE_OBJECT)
		        AnscXmlDomNodeGetNextChild(pListNode, pChildNode);
        }
    }


    /* get the component array node */
    pListNode = (PANSC_XML_DOM_NODE_OBJECT)
		AnscXmlDomNodeGetChildByName(pXmlNode, CCSP_CR_XML_NODE_components);

    if( pListNode != NULL)
    {
        pChildNode = (PANSC_XML_DOM_NODE_OBJECT)
		    AnscXmlDomNodeGetHeadChild(pListNode);

        while(pChildNode != NULL)
        {
            /* load component information */
            if(!CcspCrLoadComponentProfile(hCcspCr, (ANSC_HANDLE)pChildNode))
            {
                AnscTraceWarning(("Failed to load component profile.\n"));
            }

            pChildNode = (PANSC_XML_DOM_NODE_OBJECT)
		        AnscXmlDomNodeGetNextChild(pListNode, pChildNode);
        }
    }

    if( pXmlNode != NULL)
    {
        pXmlNode->Remove(pXmlNode);
    }


    /* create a Component Info for CR itself */
    pCompInfo = (PCCSP_COMPONENT_INFO)AnscAllocateMemory(sizeof(CCSP_COMPONENT_INFO));

    if( pCompInfo != NULL)
    {
        pCompInfo->pComponentName = AnscCloneString(CCSP_CR_NAME);
        pCompInfo->uVersion       = CCSP_CR_VERSION;
        pCompInfo->uStatus        = CCSP_Component_NotRegistered;

        AnscQueuePushEntry(&pMyObject->CompInfoQueue, &pCompInfo->Linkage);
    }

    return bStatus;
}


/**********************************************************************

    prototype:

        BOOL
        CcspCrLoadComponentProfile
            (
                ANSC_HANDLE                                 hCcspCr,
                ANSC_HANDLE                                 hXmlHandle
            );

    description:

        This function is called to load single component profile.

    argument:   ANSC_HANDLE                                 hCcspCr
                the handle of CCSP CR component;

                ANSC_HANDLE                                 hXmlHandle
                the handle of component XML handle in profile;

    return:     Succeeded or not;

**********************************************************************/
BOOL
CcspCrLoadComponentProfile
    (
        ANSC_HANDLE                                 hCcspCr,
        ANSC_HANDLE                                 hXmlHandle
    )
{
    PCCSP_CR_MANAGER_OBJECT         pMyObject         = (PCCSP_CR_MANAGER_OBJECT)hCcspCr;
    PANSC_XML_DOM_NODE_OBJECT       pObjectNode       = (PANSC_XML_DOM_NODE_OBJECT)hXmlHandle;
    PANSC_XML_DOM_NODE_OBJECT       pChildNode        = (PANSC_XML_DOM_NODE_OBJECT)NULL;
    PCCSP_COMPONENT_INFO            pCompInfo         = (PCCSP_COMPONENT_INFO)NULL;
    char                            buffer[512]       = { 0 };
    ULONG                           uLength           = 512;
    ULONG                           uVersion          = 0;

    /* get the name */
    pChildNode = (PANSC_XML_DOM_NODE_OBJECT)
        pObjectNode->GetChildByName(pObjectNode, CCSP_CR_XML_NODE_component_name);
     
    if( pChildNode == NULL || pChildNode->GetDataString(pChildNode, NULL, buffer, &uLength) != ANSC_STATUS_SUCCESS || uLength == 0)
    {
        AnscTraceWarning(("Failed to load component name.\n"));
        
        return FALSE;
    }

    /* get the version */
    pChildNode = (PANSC_XML_DOM_NODE_OBJECT)
        pObjectNode->GetChildByName(pObjectNode, CCSP_CR_XML_NODE_component_version);

    if( pChildNode == NULL || pChildNode->GetDataUlong(pChildNode, NULL, &uVersion) != ANSC_STATUS_SUCCESS)
    {
        AnscTraceWarning(("Failed to load component version.\n"));
        
        return FALSE;
    }

    /* create a Component Info and add it */
    pCompInfo = (PCCSP_COMPONENT_INFO)AnscAllocateMemory(sizeof(CCSP_COMPONENT_INFO));

    if( pCompInfo != NULL)
    {
        pCompInfo->pComponentName = AnscCloneString(buffer);
        pCompInfo->uVersion       = uVersion;
        pCompInfo->uStatus        = CCSP_Component_NotRegistered;

        AnscQueuePushEntry(&pMyObject->CompInfoQueue, &pCompInfo->Linkage);
    }

    return TRUE;
}

/**********************************************************************

    prototype:

        BOOL
        CcspCrLoadRemoteCRInfo
            (
                ANSC_HANDLE                                 hCcspCr,
                ANSC_HANDLE                                 hXmlHandle
            );

    description:

        This function is called to load single remote cr information

    argument:   ANSC_HANDLE                                 hCcspCr
                the handle of CCSP CR component;

                ANSC_HANDLE                                 hXmlHandle
                the XML handle of remote CR in profile;

    return:     Succeeded or not;

**********************************************************************/
BOOL
CcspCrLoadRemoteCRInfo
    (
        ANSC_HANDLE                                 hCcspCr,
        ANSC_HANDLE                                 hXmlHandle
    )
{
    PCCSP_CR_MANAGER_OBJECT         pMyObject         = (PCCSP_CR_MANAGER_OBJECT)hCcspCr;
    PANSC_XML_DOM_NODE_OBJECT       pObjectNode       = (PANSC_XML_DOM_NODE_OBJECT)hXmlHandle;
    PANSC_XML_DOM_NODE_OBJECT       pChildNode        = (PANSC_XML_DOM_NODE_OBJECT)NULL;
    PCCSP_REMOTE_CRINFO             pCRInfo           = (PCCSP_REMOTE_CRINFO)NULL;
    char                            buffer1[512]       = { 0 };
    char                            buffer2[512]       = { 0 };
    ULONG                           uLength           = 512;

    /* get the prefix */
    pChildNode = (PANSC_XML_DOM_NODE_OBJECT)
        pObjectNode->GetChildByName(pObjectNode, CCSP_CR_XML_NODE_cr_prefix);
     
    if( pChildNode == NULL || pChildNode->GetDataString(pChildNode, NULL, buffer1, &uLength) != ANSC_STATUS_SUCCESS || uLength == 0)
    {
        AnscTraceWarning(("Empty or invalid 'prefix' name.\n"));
        
        return FALSE;
    }

    uLength = 512;
    /* get the id */
    pChildNode = (PANSC_XML_DOM_NODE_OBJECT)
        pObjectNode->GetChildByName(pObjectNode, CCSP_CR_XML_NODE_cr_id);
     
    if( pChildNode == NULL || pChildNode->GetDataString(pChildNode, NULL, buffer2, &uLength) != ANSC_STATUS_SUCCESS || uLength == 0)
    {
        AnscTraceWarning(("Empty or invalid 'id' name.\n"));
        
        return FALSE;
    }

    /* create a remote CR info */
    pCRInfo = (PCCSP_REMOTE_CRINFO)AnscAllocateMemory(sizeof(CCSP_REMOTE_CRINFO));

    if( pCRInfo != NULL)
    {
        pCRInfo->pPrefix        = AnscCloneString(buffer1);
        pCRInfo->pID            = AnscCloneString(buffer2);

        AnscQueuePushEntry(&pMyObject->RemoteCRQueue, &pCRInfo->Linkage);
    }

    return TRUE;
}
