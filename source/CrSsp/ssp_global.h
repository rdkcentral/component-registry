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

    module:	ssp_global.h

        For Common Component Software Platform (CCSP) Development

    ---------------------------------------------------------------

    description:

        This header file defines header files needed for CCSP CR SSP.

    ---------------------------------------------------------------

    environment:

        platform independent

    ---------------------------------------------------------------

    author:

        Bin Zhu 

    ---------------------------------------------------------------

    revision:

        07/12/2011    initial revision.

**********************************************************************/

#ifndef  _CCSP_CR_GLOBAL_H_
#define  _CCSP_CR_GLOBAL_H_

#include "ansc_platform.h"
#include "ccsp_cr_definitions.h"
#include "ccsp_cr_interface.h"
#include "ccsp_base_api.h"
#include "ccsp_namespace_mgr.h"

#include "ccsp_trace.h"

/*
 *  Define custom trace module ID
 */
#ifdef   ANSC_TRACE_MODULE_ID
    #undef  ANSC_TRACE_MODULE_ID
#endif

#define  ANSC_TRACE_MODULE_ID                       ANSC_TRACE_ID_SSP


void CRSessionTest();

void CRRegisterTest();

void CRCheckDataTypeTest();

void CRDiscoverTest();

void CRUnregisterTest();

void CRComponentTest();

void BaseApiTest(char* pRootObj, BOOL bGetName);

void CRBatchTest();

void InitDbus();

void ExitDbus();

int CRRbusOpen();

void CRRbusClose();

/*
 *  External functions
 */
extern void GenerateDataModelXml(void);

#endif
