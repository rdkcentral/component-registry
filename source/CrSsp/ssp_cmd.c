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

#include <stdio.h>
#include <string.h>

#ifdef __GNUC__
#if (!defined _BUILD_ANDROID) && (!defined _NO_EXECINFO_H_)
#include <execinfo.h>
#endif
#endif

#include <sys/types.h>
#include <sys/ipc.h>

#include "ssp_global.h"

extern PCCSP_CR_MANAGER_OBJECT                     g_pCcspCrMgr;

static void PrintoutStatus( int iCode, BOOL bReturn)
{
    switch( iCode)
    {
        case CCSP_SUCCESS :   
            AnscTrace("\"Success\""); break;
        case CCSP_ERR_NOT_CONNECT:
            AnscTrace("\"Network Error\""); break;
        case CCSP_ERR_MEMORY_ALLOC_FAIL :   
            AnscTrace("\"Memory Failure\""); break;
        case CCSP_CR_ERR_NAMESPACE_OVERLAP:
            AnscTrace("\"Namespace overlap\""); break;
        case CCSP_CR_ERR_UNKNOWN_COMPONENT :   
            AnscTrace("\"Unknown Component\""); break;
        case CCSP_CR_ERR_NAMESPACE_MISMATCH:
            AnscTrace("\"Namespace mismatch\""); break;
        case CCSP_CR_ERR_UNSUPPORTED_NAMESPACE :   
            AnscTrace("\"Unsupported Namespace\""); break;
        case CCSP_CR_ERR_DP_COMPONENT_VERSION_MISMATCH:
            AnscTrace("\"Mismatch Version\""); break;
        case CCSP_CR_ERR_INVALID_PARAM:
            AnscTrace("\"Invalid Param\""); break;
        case CCSP_CR_ERR_UNSUPPORTED_DATATYPE :   
            AnscTrace("\"Unsupported data type\""); break;
        case CCSP_CR_ERR_SESSION_IN_PROGRESS:
            AnscTrace("\"Session In Progress\""); break;
        case CCSP_FAILURE:     
        	AnscTrace("\"CCSP Failure\""); break;
        default:
            AnscTrace("\"unknown error %d\"", iCode);
            break;    
    }

    if( bReturn )
    {
        AnscTrace("\n");
    }
}

void CRSessionTest()
{
    ULONG               uSession = 0;
    ULONG               uPriority= 0;
    int                 iStatus  = 0;

    g_pCcspCrMgr->GetCurrentSessionID(g_pCcspCrMgr, &uPriority,&uSession);
    AnscTrace("Current CR Session id= %lu and Priority = %lu\n", uSession, uPriority);

    AnscTrace("Try to request a session ID with Priority 5...\n");
    iStatus = g_pCcspCrMgr->RequestSessionID(g_pCcspCrMgr, 5, &uSession);
    AnscTrace("The status = %i ", iStatus);
    PrintoutStatus(iStatus, FALSE);
    AnscTrace(" and Session ID = %lu\n", uSession);

    g_pCcspCrMgr->GetCurrentSessionID(g_pCcspCrMgr, &uPriority,&uSession);
    AnscTrace("Current CR Session id= %lu and Priority = %lu\n", uSession, uPriority);

    AnscTrace("Try to request a session ID with Priority 3...\n");
    iStatus = g_pCcspCrMgr->RequestSessionID(g_pCcspCrMgr, 3, &uSession);
    AnscTrace("The status = %i ", iStatus);
    PrintoutStatus(iStatus, FALSE);
    AnscTrace(" and Session ID = %lu\n", uSession);
    g_pCcspCrMgr->GetCurrentSessionID(g_pCcspCrMgr, &uPriority,&uSession);
    AnscTrace("Current CR Session id= %lu and Priority = %lu\n", uSession, uPriority);

    AnscTrace("Try to request a session ID with Priority 8...\n");
    iStatus = g_pCcspCrMgr->RequestSessionID(g_pCcspCrMgr, 8, &uSession);
    AnscTrace("The status = %i ", iStatus);
    PrintoutStatus(iStatus, FALSE);
    AnscTrace(" and Session ID = %lu\n", uSession);

    g_pCcspCrMgr->GetCurrentSessionID(g_pCcspCrMgr, &uPriority,&uSession);
    AnscTrace("Current CR Session id= %lu and Priority = %lu\n", uSession, uPriority);

    AnscTrace("Release the session.\n");
    g_pCcspCrMgr->InformEndOfSession(g_pCcspCrMgr, uSession);
    g_pCcspCrMgr->GetCurrentSessionID(g_pCcspCrMgr, &uPriority,&uSession);
    AnscTrace("Current CR Session id= %lu and Priority = %lu\n", uSession, uPriority);
}

void CRFreeComponent(name_spaceType_t***   component, ULONG  count)
{
    name_spaceType_t**           ppSpaceType       = *component;
    ULONG                        i                 = 0;            

    if (ppSpaceType)
    {
       for( i = 0; i < count; i ++)
       {
          if (ppSpaceType[i])
          {
             if (ppSpaceType[i]->name_space)
                AnscFreeMemory((char*)ppSpaceType[i]->name_space);

             AnscFreeMemory(ppSpaceType[i]);
          }
       }
       AnscFreeMemory(ppSpaceType);
    }
}

void CRRegisterTest()
{
     name_spaceType_t**           ppSpaceType       = NULL;
     ULONG                        uCount            = rand() % 16 + 10;
     ULONG                        i                 = 0;            
     char                         buffer[256]       = { 0 };
     int                          iStatus           = 0;

     ppSpaceType       = (name_spaceType_t**)AnscAllocateMemory( uCount * sizeof(name_spaceType_t*));

     if (!ppSpaceType)
     {
        AnscTrace("Memory allocation failed");
        return;
     }
     for( i = 0; i < uCount; i ++)
     {
        ppSpaceType[i] = (name_spaceType_t*)AnscAllocateMemory(sizeof(name_spaceType_t));
        if (!ppSpaceType[i])
        {
           AnscTrace("Memory allocation failed");
           CRFreeComponent(&ppSpaceType, i);
           return;
        }

        if( i < uCount - 1)
        {
            sprintf(buffer, "Device.DevInfo.param%lu", i + 1);
        }
        else
        {
            sprintf(buffer, "Device.DevInfo.object%lu.", i + 1);
        }

        ppSpaceType[i]->name_space = AnscCloneString(buffer);
        ppSpaceType[i]->dataType   = rand() % 9;
     }

     AnscTrace("Register a component with an unknown object...\n");
     iStatus = g_pCcspCrMgr->RegisterCapabilities(g_pCcspCrMgr, "CCSP_XXXX", 1, "/com/cisco/spvtg/ccsp/DevInfo", "", (PVOID*)ppSpaceType, uCount);
     AnscTrace("The result = %d ", iStatus);
     PrintoutStatus(iStatus, TRUE);

     AnscTrace("Register a component with an object level name space...\n");
     iStatus = g_pCcspCrMgr->RegisterCapabilities(g_pCcspCrMgr, "CCSP_DevInfo", 1, "/com/cisco/spvtg/ccsp/DevInfo", "", (PVOID*)ppSpaceType, uCount);
     AnscTrace("The result = %d ", iStatus);
     PrintoutStatus(iStatus, TRUE);

     AnscTrace("Register a component with mismatched version...\n");
     iStatus = g_pCcspCrMgr->RegisterCapabilities(g_pCcspCrMgr, "CCSP_DevInfo", 2, "/com/cisco/spvtg/ccsp/DevInfo", "", (PVOID*)ppSpaceType, uCount - 1);
     AnscTrace("The result = %d ", iStatus);
     PrintoutStatus(iStatus, TRUE);

     AnscTrace("Register a component with correct version...\n");
     iStatus = g_pCcspCrMgr->RegisterCapabilities(g_pCcspCrMgr, "CCSP_DevInfo", 1, "/com/cisco/spvtg/ccsp/DevInfo", "", (PVOID*)ppSpaceType, uCount - 1);
     AnscTrace("The result = %d ", iStatus);
     PrintoutStatus(iStatus, TRUE);

     AnscTrace("Register again...\n");
     iStatus = g_pCcspCrMgr->RegisterCapabilities(g_pCcspCrMgr, "CCSP_DevInfo", 1, "/com/cisco/spvtg/ccsp/DevInfo", "", (PVOID*)ppSpaceType, uCount - 1);
     AnscTrace("The result = %d ", iStatus);
     PrintoutStatus(iStatus, TRUE);

     /* free the memory */
     CRFreeComponent(&ppSpaceType, uCount);

     /* register another component "CCSP_ObjectSample" */
     ppSpaceType       = (name_spaceType_t**)AnscAllocateMemory( uCount * sizeof(name_spaceType_t*));

     if (!ppSpaceType)
     {
        AnscTrace("Memory allocation failed");
        return;
     }
     for( i = 0; i < uCount; i ++)
     {
        ppSpaceType[i] = (name_spaceType_t*)AnscAllocateMemory(sizeof(name_spaceType_t));
        if (!ppSpaceType[i])
        {
           AnscTrace("Memory allocation failed");
           CRFreeComponent(&ppSpaceType, i);
           return;
        }

        if( i < uCount/2)
        {
            sprintf(buffer, "Device.DevInfo.DevXXX.param%lu", i + 1);
        }
        else
        {
            sprintf(buffer, "Device.DevInfo.DevXXX.DevYYY.param%lu", i + 1);
        }

        ppSpaceType[i]->name_space = AnscCloneString(buffer);
        ppSpaceType[i]->dataType   = rand() % 9;
     }

     AnscTrace("Register component 'CCSP_ObjectSample'...\n");
     iStatus = g_pCcspCrMgr->RegisterCapabilities(g_pCcspCrMgr, "CCSP_ObjectSample", 1, "/com/cisco/spvtg/ccsp/ObjSample", "", (PVOID*)ppSpaceType, uCount);
     AnscTrace("The result = %d ", iStatus);
     PrintoutStatus(iStatus, TRUE);

     /* free the memory */
     CRFreeComponent(&ppSpaceType, uCount);

     /* register another component "CCSP_TableSample" */
     ppSpaceType       = (name_spaceType_t**)AnscAllocateMemory( uCount * sizeof(name_spaceType_t*));

     if (!ppSpaceType)
     {
        AnscTrace("Memory allocation failed");
        return;
     }    
     for( i = 0; i < uCount; i ++)
     {
        ppSpaceType[i] = (name_spaceType_t*)AnscAllocateMemory(sizeof(name_spaceType_t));
        if (!ppSpaceType[i])
        {
           AnscTrace("Memory allocation failed");
           CRFreeComponent(&ppSpaceType, i);
           return;
        }

        if( i == 0)
        {
            sprintf(buffer, "Device.DevInfo.DevXXX.DevTableNumberOfEntries");
            ppSpaceType[i]->dataType   = ccsp_unsignedLong;
            ppSpaceType[i]->name_space = AnscCloneString(buffer);
        }
        else
        {
            sprintf(buffer, "Device.DevInfo.DevXXX.DevTable.{i}.param%lu", i + 1);
            ppSpaceType[i]->name_space = AnscCloneString(buffer);
            ppSpaceType[i]->dataType   = rand() % 9;
        }
     }

     AnscTrace("Register component 'CCSP_TableSample'...\n");
     iStatus = g_pCcspCrMgr->RegisterCapabilities(g_pCcspCrMgr, "CCSP_TableSample", 1, "/com/cisco/spvtg/ccsp/TableSample", "", (PVOID*)ppSpaceType, uCount);
     AnscTrace("The result = %d ", iStatus);
     PrintoutStatus(iStatus, TRUE);

     /* free the memory */
     CRFreeComponent(&ppSpaceType, uCount); 
}

void CRCheckDataTypeTest()
{
    BOOL                             bMatch       = FALSE;
    int                              iStatus      = CCSP_SUCCESS;

    AnscTrace("Check data type with an unknown name space...\n");
    iStatus = g_pCcspCrMgr->CheckNamespaceDataType(g_pCcspCrMgr, "Device.DevInfo.DevXXX.DevTableNumber", ccsp_unsignedLong, &bMatch);
    AnscTrace("The result = %d ", iStatus);
     PrintoutStatus(iStatus, TRUE);
    AnscTrace("bMatch = %d\n", bMatch);

    AnscTrace("Check data type with wrong type...\n");
    iStatus = g_pCcspCrMgr->CheckNamespaceDataType(g_pCcspCrMgr, "Device.DevInfo.DevXXX.DevTableNumberOfEntries", ccsp_string, &bMatch);
    AnscTrace("The result = %d ", iStatus);
     PrintoutStatus(iStatus, TRUE);
    AnscTrace("bMatch = %d\n", bMatch);

    AnscTrace("Check data type with correct data type...\n");
    iStatus = g_pCcspCrMgr->CheckNamespaceDataType(g_pCcspCrMgr, "Device.DevInfo.DevXXX.DevTableNumberOfEntries", ccsp_unsignedLong, &bMatch);
    AnscTrace("The result = %d ", iStatus);
     PrintoutStatus(iStatus, TRUE);
    AnscTrace("bMatch = %d\n", bMatch);

    if( TRUE)
    {
        char*                       pNewName = NULL;
        BOOL                        bChange  = FALSE;

        AnscTrace("Test 'CcspNsMgrChangeToRegNamespaceName'...\n");

        AnscTrace("Input = 'Device.DevInfo.'\n");
        pNewName = CcspNsMgrChangeToRegNamespaceName("CCSP_CR", "Device.DevInfo.", &bChange);
        if( pNewName == NULL)
        {
            AnscTrace("Output = NULL\n");
        }
        else
        {
            AnscTrace("Output = '%s'\n", pNewName);
            CcspNsMgrFreeMemory("CCSP_CR", pNewName);
        }

        AnscTrace("Input = 'Device.DevInfo.Param1'\n");
        pNewName = CcspNsMgrChangeToRegNamespaceName("CCSP_CR", "Device.DevInfo.Param1", &bChange);
        if( pNewName == NULL)
        {
            AnscTrace("Output = NULL\n");
        }
        else
        {
            AnscTrace("Output = '%s'\n", pNewName);
            CcspNsMgrFreeMemory("CCSP_CR", pNewName);
        }

        AnscTrace("Input = 'Device.DevInfo.DevTable.3.Param1'\n");
        pNewName = CcspNsMgrChangeToRegNamespaceName("CCSP_CR", "Device.DevInfo.DevTable.3.Param1", &bChange);
        if( pNewName == NULL)
        {
            AnscTrace("Output = NULL\n");
        }
        else
        {
            AnscTrace("Output = '%s'\n", pNewName);
            CcspNsMgrFreeMemory("CCSP_CR", pNewName);
        }

        AnscTrace("Input = 'Device.DevInfo.DevTable.3.SubTable.1.'\n");
        pNewName = CcspNsMgrChangeToRegNamespaceName("CCSP_CR", "Device.DevInfo.DevTable.3.SubTable.1.", &bChange);
        if( pNewName == NULL)
        {
            AnscTrace("Output = NULL\n");
        }
        else
        {
            AnscTrace("Output = '%s'\n", pNewName);
            CcspNsMgrFreeMemory("CCSP_CR", pNewName);
        }

        AnscTrace("Input = 'Device.DevInfo.DevTable.3.SubObject.'\n");
        pNewName = CcspNsMgrChangeToRegNamespaceName("CCSP_CR", "Device.DevInfo.DevTable.3.SubObject.", &bChange);
        if( pNewName == NULL)
        {
            AnscTrace("Output = NULL\n");
        }
        else
        {
            AnscTrace("Output = '%s'\n", pNewName);
            CcspNsMgrFreeMemory("CCSP_CR", pNewName);
        }

    }
}

#ifdef   _DEBUG

#define  ssp_remove_trailing_CRLF(ss)                                               \
    do {                                                                            \
        int                         size, i;                                        \
        size = AnscSizeOfString(ss);                                                \
        for ( i = size - 1; i >= 0; i -- )                                          \
        {                                                                           \
            if ( ss[i] == '\r' || ss[i] == '\n' ) ss[i] = 0;                        \
                else break;                                                         \
        }                                                                           \
    } while (0)

#endif

void CRDiscoverTest()
{
     int                          iStatus            = 0;
     ULONG                        ulSize             = 0;
     componentStruct_t**          ppDbuspath         = NULL;
     ULONG                        i                  = 0;

     AnscTrace("Try to discover from 'Device.' with nextLevel = FALSE ...\n");
     iStatus = g_pCcspCrMgr->DiscoverComponentSupportingNamespace(g_pCcspCrMgr,"Device.", "", FALSE, (PVOID**)&ppDbuspath, &ulSize);

     if( iStatus != CCSP_SUCCESS)
     {
        AnscTrace("Failed with error ");
        PrintoutStatus(iStatus, TRUE);
     }
     else
     {
         if( ulSize == 0)
         {
            AnscTrace("There's no component supporting the namespace.\n");
         }
         else
         {
             for( i = 0; i < ulSize; i ++)
             {
                AnscTrace("#%.2lu D-Bus Path: %s\n", (i + 1), ppDbuspath[i]->dbusPath);

                AnscFreeMemory(ppDbuspath[i]->componentName);
                AnscFreeMemory(ppDbuspath[i]->dbusPath);
                AnscFreeMemory(ppDbuspath[i]);
             }
        
             AnscFreeMemory(ppDbuspath);
         }
     }

     AnscTrace("Try to discover from 'Device.' with nextLevel = TRUE ...\n");
     iStatus = g_pCcspCrMgr->DiscoverComponentSupportingNamespace(g_pCcspCrMgr,"Device.", "", TRUE, (PVOID**)&ppDbuspath, &ulSize);

     if( iStatus != CCSP_SUCCESS)
     {
        AnscTrace("Failed with error ");
        PrintoutStatus(iStatus, TRUE);
     }
     else
     {
         if( ulSize == 0)
         {
            AnscTrace("There's no component supporting the namespace.\n");
         }
         else
         {
             for( i = 0; i < ulSize; i ++)
             {
                AnscTrace("#%.2lu D-Bus Path: %s\n", (i + 1), ppDbuspath[i]->dbusPath);

                AnscFreeMemory(ppDbuspath[i]->componentName);
                AnscFreeMemory(ppDbuspath[i]->dbusPath);
                AnscFreeMemory(ppDbuspath[i]);
             }
        
             AnscFreeMemory(ppDbuspath);
         }
     }

     AnscTrace("Try to discover from unknown object 'Device.Next.' with nextLevel = TRUE ...\n");
     iStatus = g_pCcspCrMgr->DiscoverComponentSupportingNamespace(g_pCcspCrMgr,"Device.Next.", "", FALSE, (PVOID**)&ppDbuspath, &ulSize);

     if( iStatus != CCSP_SUCCESS)
     {
        AnscTrace("Failed with error ");
        PrintoutStatus(iStatus, TRUE);
     }
     else
     {
         if( ulSize == 0)
         {
            AnscTrace("There's no component supporting the namespace.\n");
         }
         else
         {
             for( i = 0; i < ulSize; i ++)
             {
                AnscTrace("#%.2lu D-Bus Path: %s\n", (i + 1), ppDbuspath[i]->dbusPath);

                AnscFreeMemory(ppDbuspath[i]->componentName);
                AnscFreeMemory(ppDbuspath[i]->dbusPath);
                AnscFreeMemory(ppDbuspath[i]);
             }
        
             AnscFreeMemory(ppDbuspath);
         }
     }

     AnscTrace("Try to discover from a table object 'Device.DevInfo.DevXXX.DevTable.3.' with nextLevel = TRUE ...\n");
     iStatus = g_pCcspCrMgr->DiscoverComponentSupportingNamespace(g_pCcspCrMgr,"Device.DevInfo.DevXXX.DevTable.3.", "", FALSE, (PVOID**)&ppDbuspath, &ulSize);

     if( iStatus != CCSP_SUCCESS)
     {
        AnscTrace("Failed with error ");
        PrintoutStatus(iStatus, TRUE);
     }
     else
     {
         if( ulSize == 0)
         {
            AnscTrace("There's no component supporting the namespace.\n");
         }
         else
         {
             for( i = 0; i < ulSize; i ++)
             {
                AnscTrace("#%.2lu D-Bus Path: %s\n", (i + 1), ppDbuspath[i]->dbusPath);

                AnscFreeMemory(ppDbuspath[i]->componentName);
                AnscFreeMemory(ppDbuspath[i]->dbusPath);
                AnscFreeMemory(ppDbuspath[i]);
             }
        
             AnscFreeMemory(ppDbuspath);
         }
     }

     AnscTrace("Try to discover from another object with nextLevel = TRUE ...\n");
     iStatus = g_pCcspCrMgr->DiscoverComponentSupportingNamespace(g_pCcspCrMgr,"Device.DevInfo.DevXXX.", "", FALSE, (PVOID**)&ppDbuspath, &ulSize);

     if( iStatus != CCSP_SUCCESS)
     {
        AnscTrace("Failed with error ");
        PrintoutStatus(iStatus, TRUE);
     }
     else
     {
         if( ulSize == 0)
         {
            AnscTrace("There's no component supporting the namespace.\n");
         }
         else
         {
             for( i = 0; i < ulSize; i ++)
             {
                AnscTrace("#%.2lu D-Bus Path: %s\n", (i + 1), ppDbuspath[i]->dbusPath);

                AnscFreeMemory(ppDbuspath[i]->componentName);
                AnscFreeMemory(ppDbuspath[i]->dbusPath);
                AnscFreeMemory(ppDbuspath[i]);
             }
        
             AnscFreeMemory(ppDbuspath);
         }
     }

     AnscTrace("Try to discover a param 'Device.DevInfo.DevXXX.param2'...\n");
     iStatus = g_pCcspCrMgr->DiscoverComponentSupportingNamespace(g_pCcspCrMgr,"Device.DevInfo.DevXXX.param2", "", FALSE, (PVOID**)&ppDbuspath, &ulSize);

     if( iStatus != CCSP_SUCCESS)
     {
        AnscTrace("Failed with error ");
        PrintoutStatus(iStatus, TRUE);
     }
     else
     {
         if( ulSize == 0)
         {
            AnscTrace("There's no component supporting the namespace.\n");
         }
         else
         {
             for( i = 0; i < ulSize; i ++)
             {
                AnscTrace("#%.2lu D-Bus Path: %s\n", (i + 1), ppDbuspath[i]->dbusPath);

                AnscFreeMemory(ppDbuspath[i]->componentName);
                AnscFreeMemory(ppDbuspath[i]->dbusPath);
                AnscFreeMemory(ppDbuspath[i]);
             }
        
             AnscFreeMemory(ppDbuspath);
         }
     }

     AnscTrace("Try to discover a param under a table 'Device.DevInfo.DevXXX.DevTable.6.param2'...\n");
     iStatus = g_pCcspCrMgr->DiscoverComponentSupportingNamespace(g_pCcspCrMgr,"Device.DevInfo.DevXXX.DevTable.6.param2", "", FALSE, (PVOID**)&ppDbuspath, &ulSize);

     if( iStatus != CCSP_SUCCESS)
     {
        AnscTrace("Failed with error ");
        PrintoutStatus(iStatus, TRUE);
     }
     else
     {
         if( ulSize == 0)
         {
            AnscTrace("There's no component supporting the namespace.\n");
         }
         else
         {
             for( i = 0; i < ulSize; i ++)
             {
                AnscTrace("#%.2lu D-Bus Path: %s\n", (i + 1), ppDbuspath[i]->dbusPath);

                AnscFreeMemory(ppDbuspath[i]->componentName);
                AnscFreeMemory(ppDbuspath[i]->dbusPath);
                AnscFreeMemory(ppDbuspath[i]);
             }
        
             AnscFreeMemory(ppDbuspath);
         }
     }

    {
        char                            ns[256];
        BOOL                            bNextLevel         = TRUE;

        AnscCopyString(ns, "Device.DevInfo.param1");

         iStatus = g_pCcspCrMgr->DiscoverComponentSupportingNamespace(g_pCcspCrMgr,ns, "", bNextLevel, (PVOID**)&ppDbuspath, &ulSize);

         if( bNextLevel)
         {
            AnscTrace("Try to discover '%s' with NextLevel = TRUE\n", ns);
         }
         else
         {
            AnscTrace("Try to discover '%s' with NextLevel = FALSE\n", ns);
         }

         if( iStatus != CCSP_SUCCESS)
         {
            AnscTrace("Failed with error ");
            PrintoutStatus(iStatus, TRUE);
         }
         else
         {
             if( ulSize == 0)
             {
                AnscTrace("There's no component supporting the namespace.\n");
             }
             else
             {
                 for( i = 0; i < ulSize; i ++)
                 {
                    AnscTrace("#%.2lu D-Bus Path: %s\n", (i + 1), ppDbuspath[i]->dbusPath);

                    AnscFreeMemory(ppDbuspath[i]->componentName);
                    AnscFreeMemory(ppDbuspath[i]->dbusPath);
                    AnscFreeMemory(ppDbuspath[i]);
                 }
        
                 AnscFreeMemory(ppDbuspath);
             }
         }

    }

    {
        char                            ns[256];
        BOOL                            bNextLevel         = FALSE;

        AnscCopyString(ns, "Device.DevInfo.");

         iStatus = g_pCcspCrMgr->DiscoverComponentSupportingNamespace(g_pCcspCrMgr,ns, "", bNextLevel, (PVOID**)&ppDbuspath, &ulSize);

         if( bNextLevel)
         {
            AnscTrace("Try to discover '%s' with NextLevel = TRUE\n", ns);
         }
         else
         {
            AnscTrace("Try to discover '%s' with NextLevel = FALSE\n", ns);
         }

         if( iStatus != CCSP_SUCCESS)
         {
            AnscTrace("Failed with error ");
            PrintoutStatus(iStatus, TRUE);
         }
         else
         {
             if( ulSize == 0)
             {
                AnscTrace("There's no component supporting the namespace.\n");
             }
             else
             {
                 for( i = 0; i < ulSize; i ++)
                 {
                    AnscTrace("#%.2lu D-Bus Path: %s\n", (i + 1), ppDbuspath[i]->dbusPath);

                    AnscFreeMemory(ppDbuspath[i]->componentName);
                    AnscFreeMemory(ppDbuspath[i]->dbusPath);
                    AnscFreeMemory(ppDbuspath[i]);
                 }
        
                 AnscFreeMemory(ppDbuspath);
             }
         }

    }

}

void CRUnregisterTest()
{
     int                          iStatus            = 0;
     name_spaceType_t**           ppSpaceType       = NULL;
     ULONG                        uCount            = rand() % 16 + 10;
     ULONG                        i                 = 0;            
     char                         buffer[256]       = { 0 };

    AnscTrace("Try to unregister a namespace with an unknown component...\n");
    iStatus = g_pCcspCrMgr->UnregisterNamespace(g_pCcspCrMgr, "CCSP_XXX", "Device.DevInfo.param1");
    AnscTrace("The result = %d ", iStatus);
    PrintoutStatus(iStatus, TRUE);

    AnscTrace("Try to unregister an unknown namespace...\n");
    iStatus = g_pCcspCrMgr->UnregisterNamespace(g_pCcspCrMgr, "CCSP_DevInfo", "Device.DevInfo.pppp1");
    AnscTrace("The result = %d ", iStatus);
    PrintoutStatus(iStatus, TRUE);

    AnscTrace("Try to unregister a correct name space...\n");
    iStatus = g_pCcspCrMgr->UnregisterNamespace(g_pCcspCrMgr, "CCSP_DevInfo", "Device.DevInfo.param1");
    AnscTrace("The result = %d ", iStatus);
    PrintoutStatus(iStatus, TRUE);

    AnscTrace("Try to unregister an unknown component...\n");
    iStatus = g_pCcspCrMgr->UnregisterComponent(g_pCcspCrMgr, "CCSP_XXX");
    AnscTrace("The result = %d ", iStatus);
    PrintoutStatus(iStatus, TRUE);

    AnscTrace("Try to unregister a correct registered component...\n");
    iStatus = g_pCcspCrMgr->UnregisterComponent(g_pCcspCrMgr, "CCSP_DevInfo");
    AnscTrace("The result = %d ", iStatus);
    PrintoutStatus(iStatus, TRUE);


     AnscTrace("Register back the component...\n");
     ppSpaceType       = (name_spaceType_t**)AnscAllocateMemory( uCount * sizeof(name_spaceType_t*));

     if (!ppSpaceType)
     {
        AnscTrace("Memory allocation failed");
        return;
     }
     for( i = 0; i < uCount; i ++)
     {
        ppSpaceType[i] = (name_spaceType_t*)AnscAllocateMemory(sizeof(name_spaceType_t));
        if (!ppSpaceType[i])
        {
           AnscTrace("Memory allocation failed");
           CRFreeComponent(&ppSpaceType, i);
           return;
        }

        sprintf(buffer, "Device.DevInfo.param%lu", i + 1);

        ppSpaceType[i]->name_space = AnscCloneString(buffer);
        ppSpaceType[i]->dataType   = rand() % 9;
     }

     AnscTrace("Register a component with correct version...\n");
     iStatus = g_pCcspCrMgr->RegisterCapabilities(g_pCcspCrMgr, "CCSP_DevInfo", 1, "/com/cisco/spvtg/ccsp/DevInfo", "", (PVOID*)ppSpaceType, uCount - 1);
     AnscTrace("The result = %d ", iStatus);
     PrintoutStatus(iStatus, TRUE);

     /* free the memory */
     CRFreeComponent(&ppSpaceType, uCount);
}

void CRComponentTest()
{
    componentStruct_t**              ppComponentArray  = NULL;
    ULONG                            ulArraySize       = 0;
    name_spaceType_t**               ppStringArray     = NULL;

    ULONG                            ulSize            = 0;
    ULONG                            i                 = 0;
    ULONG                            j                 = 0;

    g_pCcspCrMgr->GetRegisteredComponents(g_pCcspCrMgr, (PVOID**)&ppComponentArray, &ulArraySize);

    AnscTrace("The count of Registered component: %lu\n", ulArraySize);
    for( i = 0 ; i < ulArraySize; i ++)
    {
        AnscTrace("#%.2lu  %s -- %s\n", (i+1), ppComponentArray[i]->componentName, ppComponentArray[i]->dbusPath);

        ulSize = 0;
        g_pCcspCrMgr->GetNamespaceByComponent(g_pCcspCrMgr, (const char*)ppComponentArray[i]->componentName, (PVOID**)&ppStringArray, &ulSize);

        if( ulSize == 0)
        {
            AnscTrace(" No namespace registered.\n");
        }
        else
        {
            for( j = 0; j < ulSize; j ++)
            {
                if( ppStringArray[j] != NULL)
                {
                    AnscTrace(" @%.3lu  %s %d\n", (j+1), ppStringArray[j]->name_space, ppStringArray[j]->dataType);

                    CcspNsMgrFreeMemory(g_pCcspCrMgr->pDeviceName, ppStringArray[j]->name_space);
                    CcspNsMgrFreeMemory(g_pCcspCrMgr->pDeviceName, ppStringArray[j]);
                }
            }

            CcspNsMgrFreeMemory(g_pCcspCrMgr->pDeviceName, ppStringArray);
        }

        /* free the memory */
        CcspNsMgrFreeMemory( g_pCcspCrMgr->pDeviceName, ppComponentArray[i]->componentName);
        CcspNsMgrFreeMemory( g_pCcspCrMgr->pDeviceName, ppComponentArray[i]->dbusPath);
        CcspNsMgrFreeMemory( g_pCcspCrMgr->pDeviceName, (PVOID)ppComponentArray[i]);
    }

    CcspNsMgrFreeMemory(g_pCcspCrMgr->pDeviceName, (PVOID)ppComponentArray);

}

void
AnscTraceMemoryUsage
    (
        void
    )
{
}

void CRBatchTest()
{
    AnscTrace("====================== CR Batch Test ============================\n");
    AnscTrace("@@@@Memory Usage:\n");
    AnscTraceMemoryUsage();
    AnscTrace("\n@@@@Session Test:\n");
    CRSessionTest();
    AnscTrace("\n@@@@Dump Object:\n");
    g_pCcspCrMgr->DumpObject(g_pCcspCrMgr);
    AnscTrace("\n@@@@Registration Test:\n");
    CRRegisterTest();
    AnscTrace("\n@@@@Dump Object:\n");
    g_pCcspCrMgr->DumpObject(g_pCcspCrMgr);
    AnscTrace("@@@@Memory Usage:\n");
    AnscTraceMemoryUsage();
    AnscTrace("\n@@@@Check DataType Test:\n");
    CRCheckDataTypeTest();
    AnscTrace("\n@@@@Discover Test:\n");
    CRDiscoverTest();
    AnscTrace("\n@@@@Components Test:\n");
    CRComponentTest();
    AnscTrace("\n@@@@Unregistration Test:\n");
    CRUnregisterTest();
    AnscTrace("@@@@Memory Usage:\n");
    AnscTraceMemoryUsage();
    AnscTrace("@@@@Clean All ...\n");
    CcspFreeCR((ANSC_HANDLE)g_pCcspCrMgr);
    g_pCcspCrMgr = NULL;
    AnscTrace("@@@@Memory Usage:\n");
    AnscTraceMemoryUsage();
    AnscTrace("@@@@Memory Track:\n");
    AnscTraceMemoryTable();

    AnscTrace("@@@@GetParmeterNames from 'Device.'\n");
    BaseApiTest("Device.", TRUE);
    AnscTrace("@@@@GetParmeterValues from 'Device.'\n");
    BaseApiTest("Device.", FALSE);
    AnscTrace("@@@@GetParmeterNames from 'com.cisco.'\n");
    BaseApiTest("com.cisco.", TRUE);
    AnscTrace("@@@@GetParmeterValues from 'com.cisco.'\n");
    BaseApiTest("com.cisco.", FALSE);
}

void BaseApiTest
    (
        char*                   pRootName,
        BOOL                    bGetName
    )
{
    PCCSP_CR_MANAGER_OBJECT         pMyObject         = (PCCSP_CR_MANAGER_OBJECT)g_pCcspCrMgr;
    PSINGLE_LINK_ENTRY              pSLinkEntry       = (PSINGLE_LINK_ENTRY)NULL;
    PCCSP_NAMESPACE_MGR_OBJECT      pNSMgr            = (PCCSP_NAMESPACE_MGR_OBJECT)pMyObject->hCcspNamespaceMgr;
    PCCSP_NAMESPACE_COMP_OBJECT     pCompInfo         = (PCCSP_NAMESPACE_COMP_OBJECT)NULL;
    int                             i                 = 0;
    parameterInfoStruct_t**         ParamInfoArray    = NULL;
    int                             ParamInfoArraySize= 0;
    parameterValStruct_t**          pParamValues      = NULL;
    int                             nParamCount       = 0;
    int                             nRet              = CCSP_SUCCESS;

    pSLinkEntry = AnscQueueGetFirstEntry(&pNSMgr->ComponentQueue);

    AnscTrace("From object: %s\n", pRootName);

    while ( pSLinkEntry )
    {
        pCompInfo       = ACCESS_CCSP_NAMESPACE_COMP_OBJECT(pSLinkEntry);
        pSLinkEntry     = AnscQueueGetNextEntry(pSLinkEntry);

        if( bGetName)
        {
            AnscTrace("GetParameterNames from component - '%s'\n", pCompInfo->pCompName);

            nRet = 
                CcspBaseIf_getParameterNames
                    (
                        pMyObject->hDbusHandle,
                        pCompInfo->pCompName,
                        pCompInfo->pDbusPath,
                        pRootName,
                        FALSE,
                        &ParamInfoArraySize,
                        &ParamInfoArray
                    );

            if( nRet == CCSP_SUCCESS)
            {
                for ( i = 0; i < ParamInfoArraySize; i ++ )
                {
                    AnscTrace("#%.3d %s\n", (i + 1), ParamInfoArray[i]->parameterName);
                }
            }
            else
            {
                AnscTrace("Operation Failed with error:");
                PrintoutStatus(nRet, TRUE);
            }

            free_parameterInfoStruct_t(pMyObject->hDbusHandle, ParamInfoArraySize, ParamInfoArray);
            ParamInfoArraySize = 0;
            ParamInfoArray     = NULL;
        }
        else /* Get Value */
        {
            AnscTrace("GetParameterValues from component - '%s'\n", pCompInfo->pCompName);

            nRet = 
                CcspBaseIf_getParameterValues
                    (
                        pMyObject->hDbusHandle,
                        pCompInfo->pCompName,
                        pCompInfo->pDbusPath,
                        &pRootName,
                        1,
                        &nParamCount,
                        &pParamValues
                    );

            if( nRet == CCSP_SUCCESS)
            {
                for ( i = 0; i < nParamCount; i ++ )
                {
                    if( pParamValues[i]->parameterValue)
                    {
                        AnscTrace("#%.3d %s = %s \n", (i + 1), pParamValues[i]->parameterName, pParamValues[i]->parameterValue);
                    }
                    else
                    {
                        AnscTrace("#%.3d %s = (EMPTY) \n", (i + 1), pParamValues[i]->parameterName);
                    }
                }
            }
            else
            {
                AnscTrace("Operation Fialed with error:");
                PrintoutStatus(nRet, TRUE);
            }

            free_parameterValStruct_t(pMyObject->hDbusHandle, nParamCount, pParamValues);
            nParamCount   = 0;
            pParamValues  = NULL;
        }
    }
}
