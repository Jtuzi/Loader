
// LoaderDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "Loader.h"
#include "LoaderDlg.h"
#include "afxdialogex.h"
#include "stdio.h"
#include "tlhelp32.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CLoaderDlg 对话框



CLoaderDlg::CLoaderDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_LOADER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CLoaderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CLoaderDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CLoaderDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// CLoaderDlg 消息处理程序

BOOL CLoaderDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CLoaderDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CLoaderDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CLoaderDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

DWORD getIDbyName(LPCSTR name) {
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);    //使用前先设置它的大小
	HANDLE hprocess = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hprocess)
	{
		return 0;
	}
	BOOL ok = Process32First(hprocess, &pe32);      //获取第一个进程
	int i = 0;
	while (ok)
	{

		if (0 == _stricmp(name, (char*)pe32.szExeFile))
		{
			//找到指定进程了，pe32.th32ProcessID 就是进程ID返回就是了
		}                                //下个进程
		ok = Process32Next(hprocess, &pe32);
	}
	CloseHandle(hprocess);
}





BOOL EnableDebugPrivilege(BOOL fEnable) {

	// Enabling the debug privilege allows the application to see
	// information about service applications
	BOOL fOk = FALSE;    // Assume function fails
	HANDLE hToken;

	// Try to open this process's access token
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES,
		&hToken)) {

		// Attempt to modify the "Debug" privilege
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid);
		tp.Privileges[0].Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0;
		AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
		fOk = (GetLastError() == ERROR_SUCCESS);
		CloseHandle(hToken);
	}
	return(fOk);
}
void inject(HANDLE hProcess) {

	char CurPath[256] = { 0 };

	strcpy(CurPath, "C:\\WINDOWS\\system32\\Hook.dll");

	PWSTR pszLibFileRemote = NULL;


	int len = (lstrlen((LPCWSTR)CurPath) + 1) * 2;
	WCHAR wCurPath[256];
	MultiByteToWideChar(CP_ACP, 0, CurPath, -1, wCurPath, 256);

	//申请内存
	pszLibFileRemote = (PWSTR)VirtualAllocEx(hProcess,
		NULL,
		len,
		MEM_COMMIT,
		PAGE_READWRITE);
	if (pszLibFileRemote == NULL) {
		MessageBoxA(0,"申请内存失败！","提示",NULL);
	}
	else {
		MessageBoxA(0, "申请内存成功！", "提示",NULL);
	}
	WriteProcessMemory(hProcess, pszLibFileRemote,(PVOID)wCurPath, len, NULL);

	PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "LoadLibraryW");

	CreateRemoteThread(hProcess,
		NULL,
		0,
		pfnThreadRtn,
		pszLibFileRemote,
		0,
		NULL);

}
void TotalInject()
{
	HANDLE         hProcessSnap = NULL;
	BOOL           bRet = FALSE;
	PROCESSENTRY32 pe32 = { 0 };

	//  Take a snapshot of all processes in the system. 
	EnableDebugPrivilege(1);

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hProcessSnap == INVALID_HANDLE_VALUE)
		return;

	//  Fill in the size of the structure before using it. 

	pe32.dwSize = sizeof(PROCESSENTRY32);

	//  Walk the snapshot of the processes, and for each process, 
	//  display information. 

	if (Process32First(hProcessSnap, &pe32))
	{

		do
		{
			HANDLE hProcess;
			// Get the actual priority class.
			hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,pe32.th32ProcessID);
			inject(hProcess);
			CloseHandle(hProcess);

		} while (Process32Next(hProcessSnap, &pe32));

	}

	// Do not forget to clean up the snapshot object. 
	EnableDebugPrivilege(0);

	CloseHandle(hProcessSnap);
	return;


}
//把dll注入到指定进程id中
BOOL injectDll(DWORD pid, const char* dllpath)
{
	HANDLE hInjectProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_ALL_ACCESS, FALSE, pid);

	if (hInjectProcess)
	{
		// 向目标进程地址空间写入DLL地址。 
		DWORD dllLength = lstrlenA(dllpath) + 1;
		SIZE_T writeLength = 0;
		LPVOID pRmtdllPathSpace = VirtualAllocEx(hInjectProcess, NULL, dllLength, MEM_COMMIT, PAGE_READWRITE);
		if (NULL == pRmtdllPathSpace)
		{
			printf("VirtualAllocEx failed! %d \n", GetLastError());
			CloseHandle(hInjectProcess);
			return FALSE;
		}
		if (WriteProcessMemory(hInjectProcess, pRmtdllPathSpace, (LPVOID)dllpath, dllLength, &writeLength))
		{
			// 需写入字节数与实际写入字节数不同，数据异常导致失败。 
			if (dllLength != writeLength)
			{
				printf("WriteProcessMemory length is err %d \n", GetLastError());
				VirtualFreeEx(hInjectProcess, pRmtdllPathSpace, writeLength, MEM_DECOMMIT);
				CloseHandle(hInjectProcess);
				return FALSE;
			}
		}
		else
		{
			printf("WriteProcessMemory failed! %d \n", GetLastError());
			CloseHandle(hInjectProcess);
			return FALSE;
		}
		// CreateRemoteThread 64的进程需要64的dll和64的注入进程
		HANDLE hRmtThread = CreateRemoteThread(hInjectProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, pRmtdllPathSpace, NULL, 0);
		if (NULL == hRmtThread)
		{
			printf("create remote thread failed! %d \n", GetLastError());
			return FALSE;
		}

		VirtualFreeEx(hInjectProcess, pRmtdllPathSpace, dllLength, MEM_RELEASE);
		CloseHandle(hInjectProcess);
		hInjectProcess = NULL;

		return TRUE;
	}
	else
	{
		printf("OpenProcess failed! %d \n", GetLastError());
		return FALSE;
	}
}

void CLoaderDlg::OnBnClickedButton1()
{
	//HANDLE p = OpenProcess(PROCESS_ALL_ACCESS, false, getIDbyName("AH-OP.exe"));
	//inject(p);


	//char Path[256] = { 0 };
	//GetCurrentDirectory(256,(LPWSTR)Path);
	//char CurPath[256] = { 0 };
	//strcpy(CurPath, "\\Hook.dll");

	//injectDll(getIDbyName("AH-OP.exe"), strcat(Path, CurPath));
	TotalInject();
	// TODO: 在此添加控件通知处理程序代码
}