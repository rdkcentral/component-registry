/*
 * If not stated otherwise in this file or this component's Licenses.txt file
 * the following copyright and licenses apply:
 *
 * Copyright 2019 RDK Management
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

#include <rbus/rbus.h>
#include <rtmessage/rtVector.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <pthread.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <rtmessage/rtLog.h>

#ifndef DISABLE_RDK_LOGGER
#include "rdk_debug.h"
#define CRLOG_TRACE(format, ...)       RDK_LOG(RDK_LOG_TRACE, "LOG.RDK.CR", format"\n", ##__VA_ARGS__)
#define CRLOG_DEBUG(format, ...)       RDK_LOG(RDK_LOG_DEBUG,  "LOG.RDK.CR", format"\n", ##__VA_ARGS__)
#define CRLOG_INFO(format, ...)        RDK_LOG(RDK_LOG_INFO,   "LOG.RDK.CR", format"\n", ##__VA_ARGS__)
#define CRLOG_WARN(format, ...)        RDK_LOG(RDK_LOG_WARN,   "LOG.RDK.CR", format"\n", ##__VA_ARGS__)
#define CRLOG_ERROR(format, ...)       RDK_LOG(RDK_LOG_ERROR,  "LOG.RDK.CR", format"\n", ##__VA_ARGS__)
#define CRLOG_FATAL(format, ...)       RDK_LOG(RDK_LOG_FATAL,  "LOG.RDK.CR", format"\n", ##__VA_ARGS__)
#else
#define CRLOG_TRACE rtLog_Debug
#define CRLOG_DEBUG rtLog_Debug
#define CRLOG_INFO rtLog_Info
#define CRLOG_WARN rtLog_Warn
#define CRLOG_ERROR rtLog_Error
#define CRLOG_FATAL rtLog_Fatal
#endif

#define CR_COMPONENT_ID "com.cisco.spvtg.ccsp.CR"
#define CCSP_ETHWAN_ENABLE "/nvram/ETHWAN_ENABLE"
#define CCSP_CR_ETHWAN_DEVICE_PROFILE_XML_FILE "cr-ethwan-deviceprofile.xml"
#define CCSP_CR_DEVICE_PROFILE_XML_FILE "cr-deviceprofile.xml"

#define ERROR_CHECK(CMD) \
do \
{ \
  int err; \
  if((err=CMD) != 0) \
  { \
    CRLOG_ERROR("Error %d:%s running command " #CMD, err, strerror(err)); \
    rc = -1; \
  } \
}while(0)

typedef struct _crData
{
    int isSystemReady;
    rtVector registry;
    pthread_mutex_t readyMutex;
    pthread_cond_t readyCond;
}* crData_t;

typedef struct _ComponentDependency
{
    char* eventname;
    rtVector compRegistry;
    pthread_t monitorTID;
} ComponentDependency_t;

typedef struct _ComponentRegistration
{
    char* name;
    int version;
    int registered;
    int ready;
    ComponentDependency_t *m_depData;
} ComponentRegistration_t;

crData_t g_crData = NULL;
rbusHandle_t g_hRbus = NULL;
pthread_t g_waitThread;
int g_waitThreadStarted = 0;

#define CR_DATA_ELEMENTS_COUNT 4
#define CR_DATA_ELEMENTS \
rbusDataElement_t crDataElements[CR_DATA_ELEMENTS_COUNT] = { \
    {"Device.CR.RegisterComponent()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, methodHandler}}, \
    {"Device.CR.IsComponentRegistered()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, isRegisteredHandler}}, \
    {"Device.CR.SystemReady", RBUS_ELEMENT_TYPE_PROPERTY, {getHandler, NULL, NULL, NULL, subHandler, NULL}}, \
    {"eRT.com.cisco.spvtg.ccsp.CR.GetHealth()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, methodHandler}} \
};

static int compData_Create(ComponentDependency_t** comps)
{
    int rc = 0;
    *comps = calloc(1, sizeof(ComponentDependency_t));
    ERROR_CHECK(rtVector_Create(&(*comps)->compRegistry));
    return rc;
}

static void componentDependencyDestroyer(void* item)
{
    char* dep_comp = (char*)item;
    if(dep_comp)
        free(dep_comp);
}

static int compData_Destroy(ComponentDependency_t *comps)
{
    int rc = 0;
    if(comps)
    {
        if(comps->eventname)
            free(comps->eventname);
        if(comps->compRegistry)
            ERROR_CHECK(rtVector_Destroy(comps->compRegistry, componentDependencyDestroyer));
        if(comps->monitorTID)
            pthread_cancel(comps->monitorTID);
        free(comps);
    }
    return rc;
}

static void componentRegistrationDestroyer(void* item)
{
    ComponentRegistration_t* reg= (ComponentRegistration_t*)item;
    free(reg->name);
    free(reg);
}

static int componentRegistrationCompare(const void * item, const void * name)
{
    const ComponentRegistration_t* reg= (const ComponentRegistration_t*)item;
    /*the name being registered might have the sybsystem prefixed (e.g. eRT.)
      but the name from the xml might not have that, so we use strstr instead of strcmp*/
    return strstr((const char*)name, reg->name) == NULL;
}

static int crData_Create(crData_t* cr)
{
    int rc = 0;
    pthread_mutexattr_t mattrib;
    pthread_condattr_t cattrib;

    *cr = calloc(1, sizeof(struct _crData));

    (*cr)->isSystemReady = 0;

    ERROR_CHECK(rtVector_Create(&(*cr)->registry));

    ERROR_CHECK(pthread_mutexattr_init(&mattrib));
    ERROR_CHECK(pthread_mutexattr_settype(&mattrib, PTHREAD_MUTEX_ERRORCHECK));
    ERROR_CHECK(pthread_mutex_init(&(*cr)->readyMutex, &mattrib));  
    ERROR_CHECK(pthread_mutexattr_destroy(&mattrib));

    ERROR_CHECK(pthread_condattr_init(&cattrib));
    ERROR_CHECK(pthread_condattr_setclock(&cattrib, CLOCK_MONOTONIC));
    ERROR_CHECK(pthread_cond_init(&(*cr)->readyCond, &cattrib));
    ERROR_CHECK(pthread_condattr_destroy(&cattrib));

    return rc;
}

bool isCompNameRegistered(const char* componentName)
{
    ComponentRegistration_t* reg = rtVector_Find(g_crData->registry, componentName, componentRegistrationCompare);
    if(reg && reg->registered)
    {
        CRLOG_WARN("Component Name: %s found in registry", componentName);
        return true;
    }
    else
        return false;

}

static rbusError_t isRegisteredHandler(rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle)
{
    (void)handle;
    (void)asyncHandle;
    rbusValue_t value;
    CRLOG_WARN("isRegisteredHandler %s", methodName);
    if(strcmp(methodName, "Device.CR.IsComponentRegistered()") == 0)
    {
        rbusValue_t name = rbusObject_GetValue(inParams, "component_name");
        const char* pName = rbusValue_GetString(name, NULL);

        if(name && pName)
        {
            if(isCompNameRegistered(pName))
            {
                rbusValue_Init(&value);
                rbusValue_SetString(value, "TRUE");
                rbusObject_SetValue(outParams, "value", value);
                rbusValue_Release(value);
            }
            else
            {
                rbusValue_Init(&value);
                rbusValue_SetString(value, "FALSE");
                rbusObject_SetValue(outParams, "value", value);
                rbusValue_Release(value);
            }

            return RBUS_ERROR_SUCCESS;
        }
        else
        {
            CRLOG_WARN("isRegisteredHandler %s missing name", methodName);
            return RBUS_ERROR_INVALID_INPUT;
        }
    }
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

static int crData_Destroy(crData_t cr)
{
    int rc = 0;
    ERROR_CHECK(rtVector_Destroy(cr->registry, componentRegistrationDestroyer));
    ERROR_CHECK(pthread_mutex_destroy(&cr->readyMutex));
    ERROR_CHECK(pthread_cond_destroy(&cr->readyCond));
    free(cr);
    return rc;
}

static void publishReadyEvent(rbusHandle_t g_hRbus, const char* eventName)
{
    rbusError_t rc;

    rbusValue_t newVal, oldVal;
    rbusEvent_t event = {0};
    rbusObject_t data;

    rbusValue_Init(&newVal);
    rbusValue_Init(&oldVal);
    rbusValue_SetBoolean(newVal, true);
    rbusValue_SetBoolean(oldVal, false);

    rbusObject_Init(&data, NULL);
    rbusObject_SetValue(data, "value", newVal);
    rbusObject_SetValue(data, "oldValue", oldVal);


    event.name = eventName;
    event.data = data;
    event.type = RBUS_EVENT_VALUE_CHANGED;

    CRLOG_WARN("Publishing ready event %s", eventName);

    rc = rbusEvent_Publish(g_hRbus, &event);

    rbusValue_Release(newVal);
    rbusValue_Release(oldVal);
    rbusObject_Release(data);

    if(rc != RBUS_ERROR_SUCCESS)
    {
        CRLOG_WARN("rbusEvent_Publish %s failed with result=%d", eventName, rc);
    }
}

static rbusError_t subHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish)
{
    (void)handle;
    (void)action;
    (void)filter;
    (void)interval;
    printf("subHandler %s called", eventName);
    /*disable expensive autopublish because we can easily publish SystemReady value change on our own*/
    *autoPublish = false;
    return RBUS_ERROR_SUCCESS;
}

static rbusError_t eventGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    char const* eventName = NULL;
    bool eventReady = false;
    int i = 0, size = 0;
    rbusValue_t value = NULL;

    eventName = rbusProperty_GetName(property);
    if (!eventName)
        return RBUS_ERROR_INVALID_INPUT;
    size = (int) rtVector_Size(g_crData->registry);
    for(i = 0; i < size; i++)
    {
        ComponentRegistration_t* tempRegistry = (ComponentRegistration_t*)rtVector_At(g_crData->registry, i);
        if(tempRegistry && tempRegistry->m_depData)
        {
            char* compEventName = tempRegistry->m_depData->eventname;
            if(compEventName)
            {
                if(!strncmp(compEventName, eventName, strlen(eventName)))
                {
                    eventReady = tempRegistry->ready;
                    break;
                }
            }
        }
    }
    rbusValue_Init(&value);
    rbusValue_SetBoolean(value, eventReady);
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);

    return RBUS_ERROR_SUCCESS;
}

static int crData_LoadRegistry(crData_t cr)
{
    int rc = 0;
    xmlDocPtr doc = NULL;
    xmlNodePtr root = NULL, comps = NULL, comp = NULL, depend = NULL, field = NULL;
    xmlChar* name = NULL, * version = NULL, * Event = NULL, * dep = NULL;
    const char* fileName = NULL;

    if(access(CCSP_ETHWAN_ENABLE, F_OK) == 0)
        fileName = CCSP_CR_ETHWAN_DEVICE_PROFILE_XML_FILE;
    else
        fileName = CCSP_CR_DEVICE_PROFILE_XML_FILE;

    doc = xmlParseFile(fileName);
    
    if(doc)
    {
        CRLOG_WARN("parsed xml %s", fileName);
        root = xmlDocGetRootElement(doc);

        if(xmlStrcmp(root->name, (const xmlChar*)"deviceProfile") == 0)
        {
            for(comps = root->children; comps != NULL; comps = comps->next)
            {
                if(comps->type == XML_ELEMENT_NODE && xmlStrcmp(comps->name, (const xmlChar*)"components") == 0)
                {
                    for(comp = comps->children; comp != NULL; comp = comp->next)
                    {
                        if(comp->type == XML_ELEMENT_NODE && xmlStrcmp(comp->name, (const xmlChar*)"component") == 0)
                        {
                            bool haveDependency = false;
                            name = NULL;
                            version = NULL;
                            dep = NULL;
                            Event = NULL;
                            ComponentDependency_t *pCompDep = NULL;
                            compData_Create(&pCompDep);
                            for(field = comp->children; field != NULL; field = field->next)
                            {
                                char* dep_name = NULL;
                                if(field->type == XML_ELEMENT_NODE)
                                {
                                    if(xmlStrcmp(field->name, (const xmlChar*)"name") == 0)
                                    {
                                        name = xmlNodeGetContent(field);
                                    }
                                    else if(xmlStrcmp(field->name, (const xmlChar*)"version") == 0)
                                    {
                                        version = xmlNodeGetContent(field);
                                    }
                                    else if(xmlStrcmp(field->name, (const xmlChar*)"dependencies") == 0)
                                    {
                                        haveDependency = true;
                                        for( depend = field->children; depend != NULL; depend = depend->next)
                                        {
                                            if(depend->type == XML_ELEMENT_NODE && xmlStrcmp(depend->name, (const xmlChar*)"dependency") == 0)
                                            {
                                                dep = xmlNodeGetContent(depend);
                                                dep_name = strdup((const char*)dep);
                                                rtVector_PushBack(pCompDep->compRegistry, dep_name);

                                                CRLOG_WARN("adding component %s to dependency component registry", dep_name);
                                                if(dep)
                                                {
                                                    xmlFree(dep);
                                                    dep = NULL;
                                                }
                                            }
                                        }
                                    }
                                    else if(xmlStrcmp(field->name, (const xmlChar*)"event") == 0)
                                    {
                                        haveDependency = true;
                                        Event = xmlNodeGetContent(field);
                                        pCompDep->eventname = strdup((const char*)Event);
                                        rbusDataElement_t DataElements[1] = { \
                                            {(char*)Event, RBUS_ELEMENT_TYPE_EVENT, {eventGetHandler, NULL, NULL, NULL, subHandler, NULL}} \
                                        };
                                        rbus_regDataElements(g_hRbus, 1, DataElements);
                                    }
                                }
                            }
                            if(name)
                            {
                                ComponentRegistration_t* reg = calloc(1, sizeof(struct _ComponentRegistration));
                                reg->registered = 0;
                                reg->ready = 0;
                                reg->name = strdup((const char*)name);
                                if(version)
                                    reg->version = atoi((const char*)version);
                                else
                                    reg->version = 1;

                                if (haveDependency)
                                    reg->m_depData = pCompDep;

                                rtVector_PushBack(cr->registry, reg);
                                rc = 0;
                                CRLOG_WARN("adding component %s %d to registry", reg->name, reg->version);
                            }

                            if(name)
                                xmlFree(name);
                            if(version)
                                xmlFree(version);
                            if(Event)
                                xmlFree(Event);

                            if (!haveDependency)
                                compData_Destroy(pCompDep);
                        }
                    }
                }
            }
            if(rc != 0)
            {
                CRLOG_ERROR("no components found in xml %s", fileName);
            }
        }
        else
        {
            CRLOG_ERROR("unexpected content in xml %s", fileName);
        }
        xmlFreeDoc(doc);
        doc = NULL;
    }
    else
    {
        CRLOG_ERROR("failed to parse xml %s", fileName);
    }
    xmlCleanupParser();
    return rc;
}

static int crData_ReadyCheck(crData_t cr)
{
    int rc = 0;
    int i;

    if(cr->isSystemReady)
        return 0;

    for(i = 0; i < (int)rtVector_Size(cr->registry); ++i)
    {
        if(!((ComponentRegistration_t*)rtVector_At(cr->registry, i))->registered)
        {
            CRLOG_WARN("ReadyCheck waiting on %s", ((ComponentRegistration_t*)rtVector_At(cr->registry, i))->name);
            rc = -1;
        }
    }

    if(rc == 0)
    {
        CRLOG_WARN("ReadyCheck yes");
        cr->isSystemReady = true;
    }

    return rc;
}



static void* pollingComponentReady(void* user)
{
    int rc = 0;
    int i = 0, size = 0;
    bool itsReady = true;
    ComponentRegistration_t* reg = (ComponentRegistration_t*)user;
    ComponentDependency_t* pComp = NULL;
    CRLOG_WARN("Dependency Component wait thread enter");
  
    if ((NULL == reg))
    {
        return NULL;
    }
    else
    {
        pComp = reg->m_depData;
        if(NULL == pComp)
        {
            return NULL;
        }
    }
      
    for(;;)
    {
        itsReady = true;
        size = (int) rtVector_Size(pComp->compRegistry);
        for(i = 0; i < size; i++)
        {
            char* dep_name = rtVector_At(pComp->compRegistry, i);
            itsReady &= isCompNameRegistered(dep_name);

            if(!itsReady)
                break;
        }

        if(itsReady)
        {
            CRLOG_WARN("All Dependency Componets are ready");
            ERROR_CHECK(pthread_mutex_lock(&g_crData->readyMutex));
            reg->ready = 1;
            ERROR_CHECK(pthread_mutex_unlock(&g_crData->readyMutex));
            publishReadyEvent(g_hRbus, pComp->eventname);
            break;
        }

        sleep(1);
    }
  
    CRLOG_WARN("Dependency component wait thread exit");
    pComp->monitorTID = 0;
    (void)rc;
    return NULL;
}


static int crData_RegisterComponent(crData_t cr, const char* componentName, int version)
{
    int rc = 0;
    (void)version;
    int isSystemReady = 0;
    ComponentRegistration_t* reg = rtVector_Find(cr->registry, componentName, componentRegistrationCompare);
    if(!reg)
    {
        CRLOG_WARN("RegisterComponent: %s not found in registry", componentName);
        return -1;
    }
  
    if (reg->m_depData)
    {
        ERROR_CHECK(pthread_mutex_lock(&cr->readyMutex));
        reg->registered = 1;
        isSystemReady = crData_ReadyCheck(cr);
        ERROR_CHECK(pthread_mutex_unlock(&cr->readyMutex));
      
        if(!(reg->m_depData->monitorTID))
        {
            ERROR_CHECK(pthread_create(&(reg->m_depData->monitorTID), NULL, &pollingComponentReady, reg));
            ERROR_CHECK(pthread_detach(reg->m_depData->monitorTID));
        }
    }
    else
    {
        CRLOG_WARN("No Dependency Component for %s", componentName);

        ERROR_CHECK(pthread_mutex_lock(&cr->readyMutex));
        reg->ready = 1;
        reg->registered = 1;
        isSystemReady = crData_ReadyCheck(cr);
        ERROR_CHECK(pthread_mutex_unlock(&cr->readyMutex));
    }

    CRLOG_WARN("RegisterComponent: %s is Registered", componentName);

    if(isSystemReady == 0)
    {
        ERROR_CHECK(pthread_cond_signal(&cr->readyCond));
        return rc;
    }
    return 0;
}

static rbusError_t getCcspLegacyPropHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* options)
{
    char const* name = rbusProperty_GetName(property);
    (void)handle;
    (void)options;

    CRLOG_WARN("getCcspLegacyPropHandler %s called", name);

    if(strstr(name, ".Name"))
    {
        rbusValue_t value;
        rbusValue_Init(&value);
        rbusValue_SetString(value, CR_COMPONENT_ID);
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
        return RBUS_ERROR_SUCCESS;
    }
    else if(strstr(name, ".Health"))
    {
        rbusValue_t value;
        rbusValue_Init(&value);
        rbusValue_SetInt32(value, 3);
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
        return RBUS_ERROR_SUCCESS;
    }
    else
    {
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    return RBUS_ERROR_SUCCESS;
}
/*Register some of the base properties from Ccsp.
  Name is used by task_health_monitor.sh on some devices to determine if Cr is 'crashed' or not
  Health might be used by parodus2cssp or other processes.
 */
static rbusError_t regCcspLegacyProperties(rbusHandle_t handle, int add)
{
    rbusError_t rc;
    char name[RBUS_MAX_NAME_LENGTH];
    char health[RBUS_MAX_NAME_LENGTH];
    #define NUM_LEG_ELEMS 2

    snprintf(name, RBUS_MAX_NAME_LENGTH, "%s.Name", CR_COMPONENT_ID);
    snprintf(health, RBUS_MAX_NAME_LENGTH, "%s.Health", CR_COMPONENT_ID);

    rbusDataElement_t elems[NUM_LEG_ELEMS] = {
        {name, RBUS_ELEMENT_TYPE_PROPERTY, {getCcspLegacyPropHandler, NULL, NULL, NULL, NULL, NULL}},
        {health, RBUS_ELEMENT_TYPE_PROPERTY, {getCcspLegacyPropHandler, NULL, NULL, NULL, NULL, NULL}}
    };

    if(add)
    {
        rc = rbus_regDataElements(handle, NUM_LEG_ELEMS, elems);
    }
    else
    {
        rc = rbus_unregDataElements(handle, NUM_LEG_ELEMS, elems);
    }

    if(rc != RBUS_ERROR_SUCCESS)
    {
        CRLOG_ERROR("regCcspLegacyProperties rbus_%sDataElements failed: %d", add ? "reg" : "unreg", rc);
    }
    return rc;
}

static rbusError_t methodHandler(rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle)
{
    (void)handle;
    (void)outParams;
    (void)asyncHandle;

    CRLOG_WARN("methodHandler %s", methodName);

    if(strcmp(methodName, "Device.CR.RegisterComponent()") == 0)
    {
        rbusValue_t name = rbusObject_GetValue(inParams, "name");
        rbusValue_t version = rbusObject_GetValue(inParams, "version");
        if(name && rbusValue_GetType(name) == RBUS_STRING && rbusValue_GetString(name, NULL))
        {
            int nver = 1;
            if(version && rbusValue_GetType(name) == RBUS_INT32)
                nver = rbusValue_GetInt32(version);
            crData_RegisterComponent(g_crData, rbusValue_GetString(name, NULL), nver);
            return RBUS_ERROR_SUCCESS;
        }
        else
        {
            CRLOG_WARN("methodHandler %s missing name", methodName);
            return RBUS_ERROR_INVALID_INPUT;
        }
    }
    else if(strcmp(methodName, "eRT.com.cisco.spvtg.ccsp.CR.GetHealth()") == 0)
    {
        rbusObject_SetPropertyInt32(outParams, "status", 3);
        return RBUS_ERROR_SUCCESS;
    }
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

static rbusError_t getHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* options)
{
    char const* name = rbusProperty_GetName(property);
    rbusValue_t value;
    (void)handle;
    (void)options;

    CRLOG_WARN("getHandler %s called when systemReady=%s", name, g_crData->isSystemReady ? "true" : "false");

    if(strcmp(name, "Device.CR.SystemReady") == 0)
    {
        rbusValue_Init(&value);
        rbusValue_SetBoolean(value, g_crData->isSystemReady);
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
        return RBUS_ERROR_SUCCESS;
    }
    return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
}

static void* waitForSystemReady(void* user)
{
    int rc = 0;
    crData_t cr = (crData_t)user;
    CRLOG_WARN("wait thread enter");
    ERROR_CHECK(pthread_mutex_lock(&cr->readyMutex));
    for(;;)
    {
        if(crData_ReadyCheck(cr) == 0)
        {
            CR_DATA_ELEMENTS;
            ERROR_CHECK(pthread_mutex_unlock(&cr->readyMutex));
            CRLOG_WARN("System Ready");
            publishReadyEvent(g_hRbus, crDataElements[2].name);
            break;
        }
        if(g_waitThreadStarted == 0)/*if user enter 'q' key*/
        {
            ERROR_CHECK(pthread_mutex_unlock(&cr->readyMutex));
            break;
        }
        ERROR_CHECK(pthread_cond_wait(&cr->readyCond, &cr->readyMutex));
        CRLOG_WARN("Received Signal to Post SystemReady");
      
    }
    CRLOG_WARN("wait thread exit");
    (void)rc;
    return NULL;
}

static void crRbusLogHandler(rbusLogLevel_t level, const char* file, int line, int threadId, char* message)
{
    char buff[256];
    snprintf(buff, 256, "CR %5s %s:%d -- Thread-%" RT_THREADID_FMT ": %s", rtLogLevelToString(level), file, line, threadId, message);
#ifndef DISABLE_RDK_LOGGER
    switch(level)
    {
    case RT_LOG_DEBUG: CRLOG_DEBUG("%s", buff); break;
    case RT_LOG_INFO: CRLOG_INFO("%s", buff); break;
    case RT_LOG_WARN: CRLOG_WARN("%s", buff); break;
    case RT_LOG_ERROR: CRLOG_ERROR("%s", buff); break;
    case RT_LOG_FATAL: CRLOG_FATAL("%s", buff); break;
    }
#else
    printf("%s stdout\n", buff);
#endif
}

static void initLogger()
{
    /*to debug: 
        echo DEBUG > /nvram/rbus_log_level
        then restart component */
    FILE* file = fopen("/nvram/rbus_log_level", "r");
    if(file)
    {
        int len, ret;
        char name[10];
        ret = fscanf(file, "%6s %n", name, &len);
        if(ret == 1)
        {
            rtLogLevel level = rtLogLevelFromString(name);
            if(strcasecmp(name, rtLogLevelToString(level)) == 0)
            {
                rbus_setLogLevel((rbusLogLevel_t)level);
                CRLOG_WARN("enabling %s rbus logs\n", name);
            }
        }
        fclose(file);
    }
    
    rbus_registerLogHandler(crRbusLogHandler);
}

int CRRbusOpen()
{
    int rc;
    CR_DATA_ELEMENTS;

    initLogger();

    CRLOG_WARN("CRRbusOpen");

    crData_Create(&g_crData);

    rc = rbus_open(&g_hRbus, CR_COMPONENT_ID);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        CRLOG_ERROR("rbus_open failed: %d", rc);
        goto exit2;
    }

    if(crData_LoadRegistry(g_crData) != 0)
    {
        CRLOG_ERROR("Failed to read component registry xml");
        goto exit2;
    }

    initLogger();/*init again after rbus_open which might override*/

    rc = rbus_regDataElements(g_hRbus, CR_DATA_ELEMENTS_COUNT, crDataElements);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        CRLOG_ERROR("rbus_regDataElements failed: %d", rc);
        goto exit1;
    }

    rc = regCcspLegacyProperties(g_hRbus, 1);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        CRLOG_ERROR("regCcspLegacyProperties failed: %d", rc);
        goto exit1;
    }

    /*register self before starting wait thread, just in case CR is the only component in xml list*/
    crData_RegisterComponent(g_crData, CR_COMPONENT_ID, 1);

    ERROR_CHECK(pthread_create(&g_waitThread, NULL, &waitForSystemReady, g_crData));
    if(rc == 0)
        g_waitThreadStarted = 1;
    ERROR_CHECK(pthread_detach(g_waitThread));

    return rc;

exit1:
    if(g_hRbus)
    {
        rbus_close(g_hRbus);
        g_hRbus = NULL;
    }
exit2:
    if(g_crData)
    {
        crData_Destroy(g_crData);
        g_crData = NULL;
    }
    return -1;    
}

void CRRbusClose()
{
    CRLOG_WARN("CRRbusAppClose");

    if(g_waitThreadStarted)
    {
        int rc;
        g_waitThreadStarted = 0;
        ERROR_CHECK(pthread_cond_signal(&g_crData->readyCond));
        (void)rc;
    }

    if(g_hRbus)
    {
        int rc;
        CR_DATA_ELEMENTS;

        rc = regCcspLegacyProperties(g_hRbus, 0);
        if(rc != RBUS_ERROR_SUCCESS)
            CRLOG_ERROR("unregCcspLegacyProperties failed: %d", rc);

        rc = rbus_unregDataElements(g_hRbus, CR_DATA_ELEMENTS_COUNT, crDataElements);
        if(rc != RBUS_ERROR_SUCCESS)
            CRLOG_ERROR("rbus_unregDataElements failed: %d", rc);

        rc = rbus_close(g_hRbus);
        if(rc != RBUS_ERROR_SUCCESS)
            CRLOG_ERROR("rbus_close failed: %d", rc);

        g_hRbus = NULL;
    }

    if(g_crData)
    {
        crData_Destroy(g_crData);
        g_crData = NULL;
    }
}

#ifdef RBUS_MAIN_ENABLED
int main(int argc, char* argv[])
{
    int ch;
    (void)(argc);(void)(argv);
    if(CRRbusOpen() == -1)
        return -1;
    while((ch = getchar()) != 'q')
    {
    }
    CRRbusClose();
    return 0;
}
#endif
