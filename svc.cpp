#include <string>
#include <windows.h>

/*
typedef struct _SERVICE_STATUS {
    // 服务的类型, SERVICE_WIN32_OWN_PROCESS 该服务在其自己的进程中运行
    DWORD   dwServiceType;
    // 服务的当前状态
    DWORD   dwCurrentState;
    // 调整服务能接收到的控制代码
    DWORD   dwControlsAccepted;
    // 错误代码 (NO_ERROR | ERROR_SERVICE_SPECIFIC_ERROR)
    DWORD   dwWin32ExitCode;
    // 特定错误代码 (dwWin32ExitCode=ERROR_SERVICE_SPECIFIC_ERROR)
    DWORD   dwServiceSpecificExitCode;
    // 服务定期递增的检查值
    DWORD   dwCheckPoint;
    // 操作的时间(毫秒), 如果在规定时间里没有更新 dwCurrentState 或者 dwCheckPoint 则判定服务异常
    DWORD   dwWaitHint;
} SERVICE_STATUS, *LPSERVICE_STATUS;
*/

// 服务状态
SERVICE_STATUS        gSvcStatus       = { SERVICE_WIN32_OWN_PROCESS, 0, SERVICE_ACCEPT_STOP, NO_ERROR, 0, 0, 0 };
// 服务实例
SERVICE_STATUS_HANDLE gSvcStatusHandle = NULL;
// 服务事件
HANDLE                gSvcStopEvent    = NULL;
// 工作线程
HANDLE                gWorkThread      = NULL;
DWORD                 gWorkThreadId    = NULL;

// 服务启动主方法
VOID WINAPI SvcMain(DWORD argc, LPSTR *argv);
// 服务事件方法
VOID WINAPI SvcCtrlHandler(DWORD);
// 报告服务状态
VOID ReportSvcStatus(DWORD dwCurrentState, DWORD dwWaitHint);

// 工作任务
DWORD WINAPI WorkMain(LPVOID);

// 服务名称
extern std::string gSvcName;
extern DWORD DoWorkMain();

void StartSvc() {
    SERVICE_TABLE_ENTRYA ServiceTable[] =  { 
        { (PSTR)gSvcName.c_str(), SvcMain }, 
        { NULL       , NULL       } 
    };

    // 启动服务
    StartServiceCtrlDispatcherA(ServiceTable);
}

VOID WINAPI SvcMain(DWORD argc, LPSTR *argv) {
    // 注册控制器
    gSvcStatusHandle = RegisterServiceCtrlHandlerA(gSvcName.c_str(), SvcCtrlHandler);
    if (gSvcStatusHandle == NULL) {
        exit(GetLastError());
    }

    // 报告启动状态
    ReportSvcStatus(SERVICE_START_PENDING, 3000);

    // 创建服务事件
    gSvcStopEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (gSvcStopEvent == NULL) {
        exit(GetLastError());
    }

    // 启动工作
    gWorkThread = CreateThread(NULL, 0, WorkMain, NULL, 0, &gWorkThreadId);
    if (gWorkThread == NULL) {
        exit(GetLastError());
    }

    // 报告运行状态
    ReportSvcStatus(SERVICE_RUNNING, 0);

    // 等待服务关闭
    WaitForSingleObject(gWorkThread, INFINITE);

    // 获取运行返回值
    DWORD exitCode = 0;
    if (GetExitCodeThread(gWorkThread, &exitCode) == 0) {
        exit(GetLastError());
    }

    if (exitCode != 0) {
        exit(exitCode);
    }

    // 报告结束状态
    ReportSvcStatus(SERVICE_STOPPED, 0);
}

VOID WINAPI SvcCtrlHandler(DWORD dwControl) {
    switch (dwControl) {
    // 通知服务应停止 (dwControlsAccepted=SERVICE_ACCEPT_STOP)
    case SERVICE_CONTROL_STOP:
        ReportSvcStatus(SERVICE_STOP_PENDING, 30000);
        SetEvent(gSvcStopEvent);
        break;
    // 通知服务应暂停 (dwControlsAccepted=SERVICE_ACCEPT_PAUSE_CONTINUE)
    case SERVICE_CONTROL_PAUSE:
        break;
    // 通知暂停的服务应恢复 (dwControlsAccepted=SERVICE_ACCEPT_PAUSE_CONTINUE)
    case SERVICE_CONTROL_CONTINUE:
        break;
    // 通知服务应将其当前状态信息报告给服务控制管理器
    case SERVICE_CONTROL_INTERROGATE:
        break;
    // 通知系统关闭 (dwControlsAccepted=SERVICE_ACCEPT_SHUTDOWN)
    case SERVICE_CONTROL_SHUTDOWN:
        break;
    // 通知服务其启动参数已更改 (dwControlsAccepted=SERVICE_ACCEPT_PARAMCHANGE)
    case SERVICE_CONTROL_PARAMCHANGE:
        break;
    // 通知系统即将关闭 (dwControlsAccepted=SERVICE_ACCEPT_PRESHUTDOWN)
    case SERVICE_CONTROL_PRESHUTDOWN:
        break;
    default:
        break;
    }
}

VOID ReportSvcStatus(DWORD dwCurrentState, DWORD dwWaitHint) {
    static DWORD dwCheckPoint = 1;

    gSvcStatus.dwCurrentState = dwCurrentState;
    gSvcStatus.dwWaitHint = dwWaitHint;

    if ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED)) {
        gSvcStatus.dwCheckPoint = 0;
    } else {
        gSvcStatus.dwCheckPoint = dwCheckPoint++;
    }
    SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}

DWORD WINAPI WorkMain(LPVOID) {
    return DoWorkMain();
}
