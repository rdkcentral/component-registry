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

    module: ccsp_cr_internal_api.h

        For Common Component Software Platform (CCSP) Development

    ---------------------------------------------------------------

    description:

        This header file contains the prototype definition for all
        the internal functions provided by CCSP_CR object.

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

#ifndef  _CCSP_CR_INTERNAL_API_H
#define  _CCSP_CR_INTERNAL_API_H

/***********************************************************
         FUNCTIONS IMPLEMENTED IN CCSP_CR_PROFILE.C
***********************************************************/
BOOL
CcspCrLoadDeviceProfile
    (
        ANSC_HANDLE                                 hCcspCr
    );

BOOL
CcspCrLoadComponentProfile
    (
        ANSC_HANDLE                                 hCcspCr,
        ANSC_HANDLE                                 hXmlHandle
    );

BOOL
CcspCrLoadRemoteCRInfo
    (
        ANSC_HANDLE                                 hCcspCr,
        ANSC_HANDLE                                 hXmlHandle
    );

/***********************************************************
         FUNCTIONS IMPLEMENTED IN CCSP_CR_OPERATION.C
***********************************************************/
int
CcspCrRegisterCapabilities
    (
        ANSC_HANDLE                 hThisObject,
        const char*                 pCompName,
        ULONG                       compVersion,
        const char*                 pDbusPath,
        const char*                 pPrefix,
        PVOID*                      pRegNamespace,
        ULONG                       ulSize
    );

int
CcspCrUnregisterNamespace
    (
        ANSC_HANDLE                 hThisObject,
        const char*                 pCompName,
        const char*                 pNamespace
    );

int
CcspCrUnregisterComponent
    (
        ANSC_HANDLE                 hThisObject,
        const char*                 pCompName
    );


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

int
CcspCrDiscoverComponentSupportingDynamicTbl
    (
        ANSC_HANDLE                 hThisObject,        
        const char*                 pNamespace,
        const char*                 pPrefix,
        BOOL                        bNextLevel,
        PVOID*                      pComponent
    );

int
CcspCrCheckNamespaceDataType
    (
        ANSC_HANDLE                 hThisObject,        
        const char*                 pNamespace,
        ULONG                       uType,
        BOOL*                       pbMatch
    );

int
CcspCrGetRegisteredComponents
    (
        ANSC_HANDLE                 hThisObject,        
        PVOID**                     ppComponent,
        ULONG*                      pulSize
    );

int
CcspCrGetNamespaceByComponent
    (
        ANSC_HANDLE                 hThisObject,
        const char*                 pCompName,                
        PVOID**                     ppNamespace,
        ULONG*                      pulSize
    );

int
CcspCrAfterComponentLost
    (
        ANSC_HANDLE                 hThisObject,
        const char*                 pComponentName
    );

/***********************************************************
         FUNCTIONS IMPLEMENTED IN CCSP_CR_SESSION.C
***********************************************************/
int
CcspCrRequestSessionID
    (
        ANSC_HANDLE                 hThisObject,
        ULONG                       uPriority,
        ULONG*                      pulSessionID
    );

int
CcspCrGetCurrentSessionID
    (
        ANSC_HANDLE                 hThisObject,
        ULONG*                      pulPriority,
        ULONG*                      pulSessionID
    );

int
CcspCrInformEndOfSession
    (
        ANSC_HANDLE                 hThisObject,
        ULONG                       ulSessionID
    );

/***********************************************************
         FUNCTIONS IMPLEMENTED IN CCSP_CR_UTILITY.C
***********************************************************/
char*
CcspCrLookforRemoteCR
    (
        ANSC_HANDLE                 hThisObject,
        char*                       pPrefix
    );

ANSC_HANDLE
CcspCrLookforComponent
    (
        ANSC_HANDLE                 hThisObject,
        const char*                 pCompName
    );

BOOL
CcspCrCleanAll
    (
        ANSC_HANDLE                 hThisObject
    );

BOOL
CcspCrDumpObject
    (
        ANSC_HANDLE                 hThisObject
    );

BOOL
CcspCrIsSystemReady
    (
        ANSC_HANDLE                 hThisObject
    );

void
CcspCrSetPrefix
    (
        ANSC_HANDLE                 hThisObject,
        char*                       pPrefix
    );

#endif
