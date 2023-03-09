#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "shlwapi")
#pragma comment(lib, "strmiids") 
#include <windows.h>
#include <dshow.h>
#include <shlwapi.h>

#define ID_LISTBOX 1000
#define ID_DELETE 1001
#define ID_RETURN 1002
#define ID_SPACE 1003
#define ID_LOOP1 1004
#define ID_LOOP2 1005
#define ID_LOOP3 1006

TCHAR szClassName[] = TEXT("Window");

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hList;
	static IGraphBuilder*pGraphBuilder;
	static IMediaControl*pMediaControl;
	static IMediaEventEx*g_pEvent;
	switch (msg) {
	case WM_CREATE:
		CoInitialize(NULL);
		hList = CreateWindow(TEXT("LISTBOX"), 0, WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT, 0, 0, 0, 0, hWnd, (HMENU)ID_LISTBOX, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		CreateWindow(TEXT("BUTTON"), TEXT("ループしない"), WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 0, 0, 128, 28, hWnd, (HMENU)ID_LOOP1, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		CreateWindow(TEXT("BUTTON"), TEXT("全曲ループ"), WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 128, 0, 128, 28, hWnd, (HMENU)ID_LOOP2, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		CreateWindow(TEXT("BUTTON"), TEXT("一曲ループ"), WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 256, 0, 128, 28, hWnd, (HMENU)ID_LOOP3, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendDlgItemMessage(hWnd, ID_LOOP1, BM_SETCHECK, BST_CHECKED, 0);
		DragAcceptFiles(hWnd, 1);
		break;
	case WM_SIZE:
		MoveWindow(hList, 0, 28, LOWORD(lParam), HIWORD(lParam) - 28, 1);
		break;
	case WM_APP:
		{
			if (g_pEvent == NULL) {
				break;
			}
			LONG evCode;
			LONG_PTR param1, param2;
			while (SUCCEEDED(g_pEvent->GetEvent(&evCode, &param1, &param2, 0))) {
				g_pEvent->FreeEventParams(evCode, param1, param2);
				switch (evCode) {
				case EC_COMPLETE:
					if (SendDlgItemMessage(hWnd, ID_LOOP1, BM_GETCHECK, 0, 0)) {
						SendMessage(hWnd, WM_COMMAND, ID_SPACE, 0);
					} else {
						const int nIndex = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
						if (nIndex != LB_ERR) {
							const int nItemCount = (int)SendMessage(hList, LB_GETCOUNT, 0, 0);
							if (SendDlgItemMessage(hWnd, ID_LOOP2, BM_GETCHECK, 0, 0)) {
								SendMessage(hList, LB_SETCURSEL, (nIndex + 1 == nItemCount) ? 0 : nIndex + 1, 0);
							}
							SendMessage(hWnd, WM_COMMAND, ID_RETURN, 0);
						}
					}
					return 0;
				}
			}
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_RETURN:
			{
				const int nIndex = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
				if (nIndex != LB_ERR) {
					TCHAR szTmp[MAX_PATH];
					SendMessage(hList, LB_GETTEXT, nIndex, (LPARAM)szTmp);
					if (pMediaControl) {
						pMediaControl->Release();
						pMediaControl = NULL;
					}
					if (g_pEvent) {
						g_pEvent->SetNotifyWindow(NULL, 0, 0);
						g_pEvent->Release();
						g_pEvent = NULL;
					}
					if (pGraphBuilder) {
						pGraphBuilder->Release();
						pGraphBuilder = NULL;
					}
					CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IGraphBuilder, (LPVOID*)&pGraphBuilder);
					pGraphBuilder->QueryInterface(IID_IMediaEventEx, (void **)&g_pEvent);
					g_pEvent->SetNotifyWindow((OAHWND)hWnd, WM_APP, 0);
					pGraphBuilder->QueryInterface(IID_IMediaControl, (LPVOID*)&pMediaControl);
					pMediaControl->RenderFile(szTmp);
					pMediaControl->Run();
				}
			}
			break;
		case ID_LISTBOX:
			if (HIWORD(wParam) == LBN_DBLCLK) {
				SendMessage(hWnd, WM_COMMAND, ID_RETURN, 0);
			}
			break;
		case ID_DELETE:
			{
				const int nIndex = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
				if (nIndex != LB_ERR) {
					SendMessage(hList, LB_DELETESTRING, nIndex, 0);
				}
			}
			break;
		case ID_SPACE:
			if (pMediaControl) {
				pMediaControl->Stop();
				SendMessage(hList, LB_SETCURSEL, -1, 0);
			}
			break;
		}
		break;
	case WM_DROPFILES:
		{
			const UINT iFileNum = DragQueryFile((HDROP)wParam, -1, NULL, 0);
			TCHAR szTmp[MAX_PATH];
			for (UINT i = 0; i<iFileNum; i++) {
				DragQueryFile((HDROP)wParam, i, szTmp, MAX_PATH);
				if (PathMatchSpec(PathFindExtension(szTmp), TEXT("*.MP3")) || PathMatchSpec(PathFindExtension(szTmp), TEXT("*.WAV"))) {
					const int nIndex = (int)SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)szTmp);
					SendMessage(hList, LB_SETCURSEL, nIndex, 0);
				}
			}
			DragFinish((HDROP)wParam);
		}
		break;
	case WM_DESTROY:
		if (pMediaControl) {
			pMediaControl->Release();
			pMediaControl = NULL;
		}
		if (g_pEvent) {
			g_pEvent->SetNotifyWindow(NULL, 0, 0);
			g_pEvent->Release();
			g_pEvent = NULL;
		}
		if (pGraphBuilder) {
			pGraphBuilder->Release();
			pGraphBuilder = NULL;
		}
		CoUninitialize();
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
	MSG msg;
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInstance,
		0,
		LoadCursor(0,IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(
		szClassName,
		L"音楽再生リスト",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	ACCEL Accel[] = { { FVIRTKEY,VK_DELETE,ID_DELETE },{ FVIRTKEY,VK_RETURN,ID_RETURN },{ FVIRTKEY,VK_SPACE,ID_SPACE } };
	HACCEL hAccel = CreateAcceleratorTable(Accel, sizeof(Accel) / sizeof(ACCEL));
	while (GetMessage(&msg, 0, 0, 0)) {
		if (!TranslateAccelerator(hWnd, hAccel, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	DestroyAcceleratorTable(hAccel);
	return (int)msg.wParam;
}
