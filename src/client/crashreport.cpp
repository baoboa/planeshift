/*
 * crashreport.cpp by Andrew Dai
 *
 * Copyright (C) 2010 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License)
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
// Code for wrapping around Google Breakpad.

#include <psconfig.h>
#include "util/log.h"
#include <csutil/cfgfile.h>
#include <iutil/vfs.h>

// Mac uses a different breakpad library. Disabled in Debug.
#if defined(CS_DEBUG) || defined(CS_PLATFORM_MACOSX)
#undef USE_BREAKPAD
#endif

// Force breakpad usage
#define USE_BREAKPAD


#ifdef USE_BREAKPAD
#ifdef WIN32
#include "resource.h"
#include "client/windows/handler/exception_handler.h"
#include "client/windows/sender/crash_report_sender.h"
#elif !defined(CS_PLATFORM_MACOSX) && defined(CS_PLATFORM_UNIX) // Mac uses a separate lib
#include "client/linux/handler/exception_handler.h"
#include "common/linux/libcurl_wrapper.h"
#endif

#include <map>
#include <string>
#include "globals.h"
#include "psengine.h"
#include "util/pscssetup.h"

#ifdef WIN32
typedef wchar_t PS_CHAR;
typedef std::wstring BpString;
#define PS_PATH_MAX 4096 // Real limit is 32k but that won't fit on the stack.
#define PS_PATH_SEP L"\\"
#define PS_STRNCAT wcsncat
#define PS_STRCAT wcscat
#define PS_STRLEN wcslen
/// Helper macro for string literals
#define STR(s)		CS_STRING_TO_WIDE(s)
#else
typedef char PS_CHAR;
typedef std::string BpString;
#define PS_PATH_MAX PATH_MAX
#define PS_PATH_SEP "/"
#define PS_STRNCAT strncat
#define PS_STRCAT strcat
#define PS_STRLEN strlen
/// Helper macro for string literals
#define STR(s)          s
#endif

static const PS_CHAR crash_post_url[] = STR("http://194.116.72.94/crash-reports/submit");
#define DUMP_EXTENSION STR(".dmp")

#define QUOTE_(X)   #X
#define QUOTE(X)    QUOTE_(X)

/// Set a dump parameter to a plain old char string (takes care of conversion)
static void SetParameter (BpString& param, const char* value)
{
#ifdef WIN32
  PS_CHAR paramBuffer[512];
  mbstowcs (paramBuffer, value, (sizeof(paramBuffer)/sizeof(PS_CHAR))-1);
  param = paramBuffer;
#else
  param = value;
#endif
}

using namespace google_breakpad;

#ifdef WIN32
bool UploadDump(const PS_CHAR* dump_path,
                     const PS_CHAR* minidump_id,
                     void* context,
                     EXCEPTION_POINTERS* exinfo,
                     MDRawAssertionInfo* assertion,
                     bool succeeded);
#else
static bool UploadDump( const google_breakpad::MinidumpDescriptor& descriptor,
                     void* context,
                     bool succeeded);
#endif

PS_CHAR szComments[1500];
static const PS_CHAR* tempPath;

#ifdef CS_PLATFORM_UNIX
	   google_breakpad::MinidumpDescriptor descriptor;
	   char uploadBuffer[1024];
#endif


// Initialise the crash dumper.
class BreakPadWrapper
{
public:
		BreakPadWrapper() {

#ifdef WIN32
		int pathLen = GetTempPathW(0, NULL);
		CS_ALLOC_STACK_ARRAY(PS_CHAR, tempPath, pathLen);
		GetTempPathW(pathLen, tempPath);
		szComments[0] = '\0';

        wrapperCrash_handler = new ExceptionHandler(tempPath,
                                                              NULL,
                                                              UploadDump,
                                                              NULL /*context*/,
                                                              ExceptionHandler::HANDLER_ALL);

		wrapperCrash_sender = new CrashReportSender(L"");

#else
	// find PlaneShift app dir ??? psengine not valid yet???
	//csRef<iVFS> vfs = psengine->GetVFS();

    static const PS_CHAR* tempPath = "/tmp";
	descriptor = MinidumpDescriptor(tempPath);

	wrapperCrash_handler = new ExceptionHandler(descriptor,
					NULL,
					UploadDump,
					NULL,
					true,
	-1
					);
#endif

				// Set up parameters

				parameters[STR("BuildDate")] = STR(__DATE__) STR(" ") STR(__TIME__);

			// Set process starttime parameter
			time_t start_time = time(NULL);
			// Reserve space for player name, gfx card info etc because we can't use the heap
			// when handling the crash.
				parameters[STR("PlayerName")] = STR("");
				parameters[STR("PlayerName")].reserve(256);
				parameters[STR("Platform")] = STR(CS_PLATFORM_NAME);
				parameters[STR("Processor")] =
				STR(CS_PROCESSOR_NAME) STR("(") STR(QUOTE(CS_PROCESSOR_SIZE)) STR(")");
				parameters[STR("Compiler")] = STR(CS_COMPILER_NAME);
				parameters[STR("Renderer")] = STR("");
				parameters[STR("Renderer")].reserve(256);
				parameters[STR("RendererVersion")] = STR("");
				parameters[STR("RendererVersion")].reserve(256);
				parameters[STR("CrashTime")] = STR("");
				parameters[STR("CrashTime")].reserve(32);
				parameters[STR("Comments")] = STR("");
				parameters[STR("Comments")].reserve(1500);
				PS_CHAR timeBuffer[128];

#ifdef WIN32
		swprintf(timeBuffer, L"%I64u", start_time);
#else
		sprintf(timeBuffer, "%lu", start_time);
#endif
                // MANDATORY ATTRIBUTES, without those the parsing server side will not work
				parameters[STR("StartupTime")] = timeBuffer;
				parameters[STR("ProductName")] = STR("PlaneShift");
                parameters[STR("ReleaseChannel")] = STR("release");
				parameters[STR("Version")] = STR(PS_VERSION);
				report_code.reserve(512);

		}
		~BreakPadWrapper() {
				delete wrapperCrash_handler;
				wrapperCrash_handler = NULL;

#ifdef WIN32
		delete wrapperCrash_sender;
		wrapperCrash_sender = NULL;
#endif

		}

#ifdef WIN32
	static CrashReportSender* wrapperCrash_sender;
#endif

		std::map<BpString, BpString> parameters;
		BpString report_code;

//private:
		static google_breakpad::ExceptionHandler* wrapperCrash_handler;

//        google_breakpad::MinidumpDescriptor descriptor;
};

ExceptionHandler* BreakPadWrapper::wrapperCrash_handler = NULL;

#ifdef WIN32
CrashReportSender* BreakPadWrapper::wrapperCrash_sender = NULL;
#endif

// At global scope to ensure we hook in as early as possible.
BreakPadWrapper BPwrapper;

#ifdef WIN32
BOOL CALLBACK CrashReportProc(HWND hwndDlg, 
                             UINT message, 
                             WPARAM wParam, 
                             LPARAM lParam) 
{ 
    switch (message) 
    { 
        case WM_COMMAND: 
            switch (LOWORD(wParam)) 
            {
                case IDOK: 
                    GetDlgItemTextW(hwndDlg, IDC_COMMENTS, szComments, 1500); 
                    EndDialog(hwndDlg, wParam);
                    return TRUE;
                case IDCANCEL:
                    EndDialog(hwndDlg, wParam);
                    return FALSE;
            }
    } 
    return FALSE; 
} 
#endif


#ifdef WIN32

// This function should not modify the heap!
bool UploadDump(const PS_CHAR* dump_path,
                     const PS_CHAR* minidump_id,
                     void* context,
                     EXCEPTION_POINTERS* exinfo,
                     MDRawAssertionInfo* assertion,
                     bool succeeded) 
{
#else
static bool UploadDump( const google_breakpad::MinidumpDescriptor& descriptor,
					 void* context,
					 bool succeeded)
{

	const char* dump_path = descriptor.path();

#endif

#ifdef WIN32
	PS_CHAR path_file[PS_PATH_MAX + 1];
		path_file[0] = '\0';
	PS_CHAR* p_path_end = path_file + PS_PATH_MAX;

	PS_STRNCAT(path_file, dump_path, PS_PATH_MAX);

    PS_CHAR* p_path = path_file;
    p_path += PS_STRLEN(path_file);
    PS_STRCAT(path_file, PS_PATH_SEP);
    p_path += PS_STRLEN(PS_PATH_SEP);
    PS_STRNCAT(path_file, minidump_id, p_path_end - p_path);
    p_path += PS_STRLEN(minidump_id);
    PS_STRNCAT(path_file, DUMP_EXTENSION, p_path_end - p_path);

	INT_PTR dialogResult = DialogBox(GetModuleHandle(0), 
              MAKEINTRESOURCE(IDD_CRASH), 
			  psEngine::hwnd, 
              (DLGPROC)CrashReportProc);

    // exit if user clicked on cancel
    if (dialogResult == IDCANCEL) {
       return true;
    }

	// Comments are available only on Windows at the moment
	BPwrapper.parameters[STR("Comments")] = szComments;
#endif

	time_t crash_time = time(NULL);
	PS_CHAR timeBuffer[128];
#ifdef WIN32
    swprintf(timeBuffer, L"%I64u", crash_time);
#else
    sprintf(timeBuffer, "%lu", crash_time);
#endif

	SetParameter (BPwrapper.parameters[STR("Renderer")], psEngine::hwRenderer);
	SetParameter (BPwrapper.parameters[STR("RendererVersion")], psEngine::hwVersion);
	SetParameter (BPwrapper.parameters[STR("PlayerName")], psEngine::playerName);
	BPwrapper.parameters[STR("CrashTime")] = timeBuffer;

	fprintf(stderr, "PlaneShift has quit unexpectedly!\n\nA report containing only information strictly necessary to identify this problem will be sent to the PlaneShift developers.\nPlease consult the PlaneShift forums for more details.\nAttempting to upload crash report.\n");

#ifdef WIN32
#else
	// on linux at the moment we just call pslaunch to upload the dump
	printf( "****UploadDump sending file %s \n",descriptor.path());

	// write file with parameters in the .PlaneShift app dir
	csRef<iVFS> vfs = psengine->GetVFS();
	csRef<iConfigFile> configFile;
	configFile.AttachNew(new csConfigFile("/planeshift/userdata/crash/crash.params",vfs));

	for (std::map<BpString,BpString>::iterator pos = BPwrapper.parameters.begin(); pos != BPwrapper.parameters.end(); ++pos) {
		configFile->SetStr((*pos).first.c_str(),(*pos).second.c_str());
	}
	configFile->Save();

	// call pslaunch for upload
	sprintf( uploadBuffer, "./pslaunch --uploaddump=%-512.512s", descriptor.path() );
	system (uploadBuffer);
	printf( "done\n");
	return false;

#endif

    bool result = false;

#ifdef WIN32
	wprintf(L"WIN32 SendReport url %s\n",crash_post_url);
	wprintf(L"WIN32 SendReport path_file %s\n",path_file);
	wprintf(L"WIN32 SendReport dump_path %s\n",dump_path);
    ReportResult reportResult = BreakPadWrapper::wrapperCrash_sender->SendCrashReport(crash_post_url,
		    BPwrapper.parameters,
		    path_file,
		    &BPwrapper.report_code);
    if(reportResult == RESULT_SUCCEEDED)
	    result = true;
#endif
    if(result && !BPwrapper.report_code.empty())
    {
	    printf("Upload successful.\n");
#ifdef WIN32
	    ::MessageBoxW( NULL, BPwrapper.report_code.c_str(), L"Report upload successful. Thanks.", MB_OK );
#else
	    printf("%s\n", BPwrapper.report_code.c_str());
#endif
			return succeeded;
	}
	else if(!result)
	{
			printf("Report upload failed. ");
#ifdef WIN32
	    if (reportResult == RESULT_FAILED) {
		    printf("Could not reach server.\n");
		    ::MessageBoxA( NULL, "Report upload failed: Could not reach server.", "PlaneShift", MB_OK + MB_ICONERROR );
	    }
	    else
	    {
		    printf("Unknown reason.\n");
		    ::MessageBoxA( NULL, "Report upload failed: Unknown reason.", "PlaneShift", MB_OK + MB_ICONERROR );
	    }
#endif
			return false;
	}
	else // result is true but report code is empty.
	{
			printf("Report upload failed: Unknown reason.\n");
#ifdef WIN32
	    ::MessageBoxA( NULL, "Report upload failed: Unknown reason.", "PlaneShift", MB_OK + MB_ICONERROR );
#endif
			return false;
	}
	return false;
}


#endif // USE_BREAKPAD
