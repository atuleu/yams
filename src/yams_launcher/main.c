#include <shlobj.h>  // For SHGetFolderPathA
#include <shlwapi.h> // For PathRemoveFileSpec
#include <stdio.h>

#include <windows.h>

void getYamsInstallDir(char *installDir) {
	char exePath[MAX_PATH];
	// Get the full path to the running EXE ("C:\Program Files\YAMS
	// 0.1\bin\yams_launcher.exe")
	GetModuleFileNameA(NULL, exePath, MAX_PATH);

	// Make a modifiable copy of the path to get the directory
	strcpy(installDir, exePath);
	PathRemoveFileSpecA(installDir
	); // Now exeDir: "C:\Program Files\YAMS 0.1\bin"
	PathRemoveFileSpecA(installDir); // Now exeDir: "C:\Program Files\YAMS 0.1"
};

void updateEnvironment(const char *yamsInstallDir) {
	// Compute the GStreamer directory based on the known relative location
	char gstreamerPath[MAX_PATH];
	snprintf(
	    gstreamerPath,
	    MAX_PATH,
	    "%s\\gstreamer\\1.0\\msvc_x86_64",
	    yamsInstallDir
	);

	// Normalize the result (abs path)
	char fullGstreamerPath[MAX_PATH];
	GetFullPathNameA(gstreamerPath, MAX_PATH, fullGstreamerPath, NULL);

	// Prepend this to the existing PATH environment variable
	char oldPath[4096];
	GetEnvironmentVariableA("PATH", oldPath, sizeof(oldPath));
	char newPath[4096];
	snprintf(
	    newPath,
	    sizeof(newPath),
	    "%s\\bin;%s",
	    fullGstreamerPath,
	    oldPath
	);
	SetEnvironmentVariableA("PATH", newPath);

	char gstPluginScannerPath[MAX_PATH];
	snprintf(
	    gstPluginScannerPath,
	    MAX_PATH,
	    "%s\\libexec\\gstreamer-1.0\\gst-plugin-scanner",
	    fullGstreamerPath
	);
	SetEnvironmentVariableA("GST_PLUGIN_SCANNER", gstPluginScannerPath);
}

HANDLE openLogFile() {

	char logPath[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, logPath)
	    )) {
		strcat(logPath, "\\YAMS");
		CreateDirectoryA(logPath, NULL); // Ensure dir exists
		strcat(logPath, "\\yams.log");
		// ... now create/open the log file here!
	}

	// Create log file handle
	SECURITY_ATTRIBUTES sa =
	    {sizeof(sa), NULL, TRUE}; // Make handle inheritable
	return CreateFileA(
	    logPath,
	    GENERIC_WRITE,
	    FILE_SHARE_READ,
	    &sa,
	    CREATE_ALWAYS,
	    FILE_ATTRIBUTE_NORMAL,
	    NULL
	);
}

int WINAPI WinMain(
    HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow
) {

	char yamsInstallDir[MAX_PATH];
	getYamsInstallDir(yamsInstallDir);

	updateEnvironment(yamsInstallDir);

	HANDLE hLog = openLogFile();
	if (hLog == INVALID_HANDLE_VALUE) {
		MessageBoxA(
		    NULL,
		    "Failed to create log file",
		    "Error",
		    MB_OK | MB_ICONERROR
		);
		return 1;
	}

	// Build command line for yams.exe (assume it's in the same directory as the
	// launcher)
	char yamsPath[MAX_PATH];
	snprintf(yamsPath, MAX_PATH, "%s\\bin\\yams.exe", yamsInstallDir);

	// Set up process startup info
	STARTUPINFOA si = {sizeof(si)};
	si.dwFlags |= STARTF_USESTDHANDLES;
	si.hStdOutput = hLog;
	si.hStdError  = hLog;
	si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE); // Or NULL

	PROCESS_INFORMATION pi = {0};

	// Build command line (optional: pass args)
	char cmdLine[MAX_PATH + 10];
	snprintf(
	    cmdLine,
	    sizeof(cmdLine),
	    "\"%s\" %s",
	    yamsPath,
	    GetCommandLineA()
	);

	if (!CreateProcessA(
	        yamsPath,
	        cmdLine, // pass args
	        NULL,
	        NULL,
	        TRUE,
	        CREATE_NO_WINDOW,
	        NULL,
	        NULL,
	        &si,
	        &pi
	    )) {
		MessageBoxA(
		    NULL,
		    "Failed to launch yams.exe",
		    "Error",
		    MB_OK | MB_ICONERROR
		);
		CloseHandle(hLog);
		return 1;
	}

	WaitForSingleObject(pi.hProcess, INFINITE); // Wait for yams.exe to finish

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseHandle(hLog);
	return 0;
}
