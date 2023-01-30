// #include <iostream>
#include <windows.h>

#include "utils.cpp"
#include "svc.cpp"

using namespace std;

// 服务名称
string gSvcName    = "WindowsSvc";

// 可执行文件路径
string gExecPath  = "";
// 可执行文件
string gExecFile  = "";
// 执行参数
string gExecParam = "";

// 日志
string gLogName   = gSvcName + ".log";
string gLogPrefix = "\nSvc Log: ";

int main(int argc, char** argv) {
    // 获取可执行文件和参数
    if (argc >= 2) {
        gExecFile   = argv[1];
        for (int i = 2; i < argc; ++i) {
            gExecParam += string(" ") + argv[i];
        }
    } else {
        return 1;
    }

    // 解析可执行文件名称和路径
    if (gExecFile != "") {
        gSvcName = gExecFile;
        size_t index = gSvcName.rfind("\\");
        if (index != string::npos) {
            gExecPath = gSvcName.substr(0, index+1);
            gSvcName  = gSvcName.substr(index+1);
        }
        if (gSvcName.rfind(".exe") == gSvcName.length() - 4) {
            gSvcName = gSvcName.substr(0, gSvcName.length() - 4);
        }
        gLogName = gSvcName + ".log";
    }

    // 获取服务路径
    string svcExecPath = argv[0];
    size_t index = svcExecPath.rfind("\\");
    svcExecPath = svcExecPath.substr(0, index+1);

    gLogName = svcExecPath + gLogName;

    // 测试
    // DoWorkMain();

    // 开启服务
    StartSvc();
    return EXIT_SUCCESS;
}

DWORD DoWorkMain() {
    // 打开日志文件
    SECURITY_ATTRIBUTES sa{sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
    HANDLE hLog = CreateFileA(gLogName.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, &sa, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hLog == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }

    string execFileAndParam = gExecFile + gExecParam;

    // 启动目标进程
    STARTUPINFOA startupInfo{0};
    startupInfo.cb = sizeof(STARTUPINFOA);
    startupInfo.dwFlags = STARTF_USESTDHANDLES;
    startupInfo.hStdOutput = hLog;
    startupInfo.hStdError = hLog;
    PROCESS_INFORMATION processInfo{0};
    if (!CreateProcessA(NULL, (PSTR)execFileAndParam.c_str(), NULL, NULL, TRUE, NULL, NULL, gExecPath.c_str(), &startupInfo, &processInfo)) {
        DWORD err = GetLastError();
        string errMsg = gLogPrefix + "CreateProcessA Error: " + errmsg(err) + "\n";
        WriteFile(hLog, errMsg.c_str(), errMsg.length(), NULL, NULL);
        CloseHandle(hLog);
        return err;
    }

    string startMsg = gLogPrefix + "Start\n";
    WriteFile(hLog, startMsg.c_str(), startMsg.length(), NULL, NULL);

    // 检测是否手动停止服务
    while (WaitForSingleObject(processInfo.hProcess, 1000) == WAIT_TIMEOUT) {
        if (gSvcStopEvent != NULL && WaitForSingleObject(gSvcStopEvent, 1000) != WAIT_TIMEOUT) {
            CloseHandle(gSvcStopEvent);
            // 杀进程
            TerminateProcess(processInfo.hProcess, 0);
            WaitForSingleObject(processInfo.hProcess, INFINITE);
        }
    }

    // 向日志写入程序返回结果
    DWORD exitCode = 0;
    GetExitCodeProcess(processInfo.hProcess, &exitCode);

    CHAR exitMsg[64]{0};
    int exitMsgLen = sprintf(exitMsg, "%sExitCode(%d)\n", gLogPrefix.c_str(), exitCode);
    WriteFile(hLog, exitMsg, exitMsgLen, NULL, NULL);
    CloseHandle(hLog);

    CloseHandle(processInfo.hProcess);
	CloseHandle(processInfo.hThread);

    return exitCode;
}
