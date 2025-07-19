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


#ifdef __GNUC__
#if (!defined _BUILD_ANDROID) && (!defined _NO_EXECINFO_H_)
#include <execinfo.h>
#endif
#endif

#include <sys/types.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <fcntl.h>

#include "ssp_global.h"
#include "syscfg/syscfg.h"
#include "cap.h"
#include "telemetry_busmessage_sender.h"

static cap_user appcaps;

#ifdef INCLUDE_BREAKPAD
#include "breakpad_wrapper.h"
#endif
#define DEBUG_INI_NAME  "/etc/debug.ini"

#define SUBSYS_LEN 32

PCCSP_CR_MANAGER_OBJECT                     g_pCcspCrMgr            = NULL;
void*                                       g_pDbusHandle           = NULL;
ULONG                                       g_ulAllocatedSizeInit   = 0;
char                                        g_Subsystem[SUBSYS_LEN] = {0};

extern ULONG                                g_ulAllocatedSizeCurr;

extern char*                                pComponentName;

#define  CCSP_COMMON_COMPONENT_HEALTH_Red                   1
#define  CCSP_COMMON_COMPONENT_HEALTH_Yellow                2
#define  CCSP_COMMON_COMPONENT_HEALTH_Green                 3
int                                         g_crHealth = CCSP_COMMON_COMPONENT_HEALTH_Red;

/*
 *  For export data models in Motive compliant format
 */
BOOLEAN                                     g_exportAllDM = false;

#ifndef INCLUDE_BREAKPAD
static void _print_stack_backtrace(void)
{
#ifdef __GNUC__
#if (!defined _BUILD_ANDROID) && (!defined _NO_EXECINFO_H_)
        void* tracePtrs[100];
        char** funcNames = NULL;
        int i, count = 0;

        count = backtrace( tracePtrs, 100 );
        backtrace_symbols_fd( tracePtrs, count, 2 );

        funcNames = backtrace_symbols( tracePtrs, count );

        if ( funcNames ) {
            // Print the stack trace
            for( i = 0; i < count; i++ )
                printf("%s\n", funcNames[i] );

            // Free the string pointers
            free( funcNames );
        }
#endif
#endif
}

static void sig_handler(int sig)
{
    if ( sig == SIGINT ) {
        signal(SIGINT, sig_handler); /* reset it to this function */
        CcspTraceInfo(("SIGINT received!\n"));
        exit(0);
    }
    else if ( sig == SIGUSR1 ) {
        signal(SIGUSR1, sig_handler); /* reset it to this function */
        CcspTraceInfo(("SIGUSR1 received!\n"));
    }
    else if ( sig == SIGUSR2 ) {
        CcspTraceInfo(("SIGUSR2 received!\n"));
        /* DH  Very ugly reference of functions -- generate the complete DM XML file*/
        GenerateDataModelXml();
    }
    else if ( sig == SIGCHLD ) {
        signal(SIGCHLD, sig_handler); /* reset it to this function */
        CcspTraceInfo(("SIGCHLD received!\n"));
    }
    else if ( sig == SIGPIPE ) {
        signal(SIGPIPE, sig_handler); /* reset it to this function */
        CcspTraceInfo(("SIGPIPE received!\n"));
    }
    else if ( sig == SIGTERM )
    {
        CcspTraceInfo(("SIGTERM received!\n"));
        exit(0);
    }
    else if ( sig == SIGKILL )
    {
        CcspTraceInfo(("SIGKILL received!\n"));
        exit(0);
    }
    else {
        /* get stack trace first */
        _print_stack_backtrace();
        CcspTraceInfo(("Signal %d received, exiting!\n", sig));
        exit(0);
    }
}
#endif

static void drop_root()
{ 
  appcaps.caps = NULL;
  appcaps.user_name = NULL;
  AnscTrace("NonRoot feature is enabled, dropping root privileges for CcspCr process\n");
  init_capability();
  drop_root_caps(&appcaps);
  update_process_caps(&appcaps);
  read_capability(&appcaps);
}

int main(int argc, char* argv[])
{
    int cmdChar = 0;
    int idx     = 0;
    BOOL bRunAsDaemon = TRUE;
    char cmd[1024] = {0};
    FILE *fd = NULL;
    int rc;
    sem_t *sem = NULL;

    // Buffer characters till newline for stdout and stderr
    setlinebuf(stdout);
    setlinebuf(stderr);

    pComponentName = CCSP_DBUS_INTERFACE_CR;
#ifdef FEATURE_SUPPORT_RDKLOG
    RDK_LOGGER_INIT();
#endif

#if defined(_DEBUG) || defined(_COSA_SIM_)
    AnscSetTraceLevel(CCSP_TRACE_LEVEL_INFO);
#endif

    srand(time(0));

    for (idx = 1; idx < argc; idx++)
    {
        if ( (strcmp(argv[idx], "-subsys") == 0) )
        {
	    /* CID-137568 fix */
	    if(strlen(argv[idx+1]) < SUBSYS_LEN)
	    {
		AnscCopyString(g_Subsystem, argv[idx+1]);
	    }
	    else
	    {
	        AnscTrace("subsys length error \n");
		return 1;
	    }
        }
        else if ( strcmp(argv[idx], "-c" ) == 0 )
        {
            bRunAsDaemon = FALSE;
        }
        else if (strcmp(argv[idx], "-eDM") == 0)
        {
            g_exportAllDM = true;
        }
    }

/*demonizing*/
    if ( bRunAsDaemon )
    {

		 /* initialize semaphores for shared processes */
		 sem = sem_open ("pSemCr", O_CREAT | O_EXCL, 0644, 0);
		 if(SEM_FAILED == sem)
		 {
		 	AnscTrace("Failed to create semaphore %d - %s\n", errno, strerror(errno));
		 	_exit(1);
		 }
		 /* name of semaphore is "pSemCr", semaphore is reached using this name */
		 sem_unlink ("pSemCr");
		 /* unlink prevents the semaphore existing forever */
		 /* if a crash occurs during the execution         */
		 AnscTrace("Semaphore initialization Done!!\n");
		
		 rc = fork();
		 if(rc == 0 ){
		 	AnscTrace("Demonizing Done!!\n");
		 	}
		 else if(rc == -1){

			AnscTrace("Demonizing Error (fork)! %d - %s\n", errno, strerror(errno));
			exit(0);
		 	}
		 else
		 	{
				sem_wait (sem);
				sem_close(sem);
		 		_exit(0);
		 	}
	
		if (setsid() <	0) {
			AnscTrace("Demonizing Error (setsid)! %d - %s\n", errno, strerror(errno));
			exit(0);
		}
	
#ifndef  _DEBUG
                int FileDescriptor;
		FileDescriptor = open("/dev/null", O_RDONLY);
		if (FileDescriptor != 0) {
			dup2(FileDescriptor, 0);
			close(FileDescriptor);
		}
		FileDescriptor = open("/dev/null", O_WRONLY);
		if (FileDescriptor != 1) {
			dup2(FileDescriptor, 1);
			close(FileDescriptor);
		}
		FileDescriptor = open("/dev/null", O_WRONLY);
		if (FileDescriptor != 2) {
			dup2(FileDescriptor, 2);
			close(FileDescriptor);
		}

#endif
	}

	fd = fopen("/var/tmp/CcspCrSsp.pid", "w+");
    if ( !fd )
    {
        AnscTrace("Create /var/tmp/CcspCrSsp.pid error. \n");
        return 1;
    }
    else
    {
        sprintf(cmd, "%d", getpid());
        fputs(cmd, fd);
        fclose(fd);
    }

#ifdef INCLUDE_BREAKPAD
    breakpad_ExceptionHandler();
#else
    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);

    signal(SIGSEGV, sig_handler);
    signal(SIGBUS, sig_handler);
    signal(SIGKILL, sig_handler);
    signal(SIGFPE, sig_handler);
    signal(SIGILL, sig_handler);
    signal(SIGQUIT, sig_handler);
    signal(SIGHUP, sig_handler);
    signal(SIGPIPE, SIG_IGN);

#endif
    t2_init("ccsp-cr");

        if(CRRbusOpen() != 0)
        {
            AnscTrace("CRRbusOpen failed\n");
            return 1;
        }

	system("touch /tmp/cr_initialized");

    if ( bRunAsDaemon )
    {
		sem_post (sem);
		sem_close(sem);
		drop_root();

		while (1) {
			sleep(30);
		}
    }
    else
    {
		while ( cmdChar != 'q' )
		{
			cmdChar = getchar();
		}
    }

    CRRbusClose();

    if ( g_pCcspCrMgr )
    {
        ExitDbus();
        CcspFreeCR((ANSC_HANDLE)g_pCcspCrMgr);
        g_pCcspCrMgr = NULL;
    }

    return 0;
}

