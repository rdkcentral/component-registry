##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
# file the following copyright and licenses apply:
#
# Copyright 2015 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
if WITH_GTEST_SUPPORT
SUBDIRS = test
endif
AM_CFLAGS =

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = CcspCrSsp

CcspCrSsp_CFLAGS = $(SYSTEMD_CFLAGS)
CcspCrSsp_LDFLAGS = -lpthread

if RBUS_BUILD_ONLY
CcspCrSsp_CFLAGS += -DRBUS_MAIN_ENABLED
CcspCrSsp_SOURCES = CrSsp/ssp_rbus.c
CcspCrSsp_LDFLAGS += -lrbus -lrtMessage -lxml2
else
CcspCrSsp_SOURCES = \
	CrSsp/ssp_cmd.c \
	CrSsp/ssp_dbus.c \
	CrSsp/ssp_main.c \
	CCSP_CR/ccsp_cr_base.c \
	CCSP_CR/ccsp_cr_exportDM.c \
	CCSP_CR/ccsp_cr_operation.c \
	CCSP_CR/ccsp_cr_profile.c \
	CCSP_CR/ccsp_cr_session.c \
	CCSP_CR/ccsp_cr_utility.c
CcspCrSsp_LDFLAGS += -lccsp_common -lsyscfg -ltelemetry_msgsender -lprivilege $(SYSTEMD_LDFLAGS) -lpthread
if RBUS_BUILD_INTEGRATED
CcspCrSsp_SOURCES += CrSsp/ssp_rbus.c
CcspCrSsp_LDFLAGS += -lrbus -lrtMessage -lxml2
endif
endif

