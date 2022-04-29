#define MDEFTVERSION "0.89"	// make sure its always 4 characters!

#include <Windows.h>

#include "resource.h"
#include "Shlobj.h"
#include "libs/DFfile/DFlib.h";
//#pragma warning(disable: 4996)


INT_PTR CALLBACK    mainProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

static HINSTANCE hInst;
static std::string OpenFile(HWND hWnd, const char* filter);
static std::string browseDir(HWND hWnd);

// callback proc for browsing folder window
INT CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData)
{
	if (uMsg == BFFM_INITIALIZED) {
		SendMessageA(hwnd, BFFM_SETSELECTIONA, TRUE, (LPARAM)pData);
	}
	return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	std::wstring cmdParams(lpCmdLine);

	hInst = hInstance;
	DialogBox(hInst, MAKEINTRESOURCE(IDD_PROPPAGE_LARGE), NULL, mainProc);

	return 0;
}

INT_PTR CALLBACK mainProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

	static DFfile* DFCurrInst = nullptr;
	static uint8_t extrOptions{ 7 };
	static std::string customOutput = std::string();

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		SendMessage(GetDlgItem(hDlg, IDC_CHECK1), BM_SETCHECK, BST_CHECKED, 0);
		SendMessage(GetDlgItem(hDlg, IDC_CHECK2), BM_SETCHECK, BST_CHECKED, 0);
		SendMessage(GetDlgItem(hDlg, IDC_CHECK3), BM_SETCHECK, BST_CHECKED, 0);
	}
	return (INT_PTR)TRUE;

	case WM_COMMAND:
		switch (wParam) {
		case IDC_CHECK1:
			if (extrOptions & 1)
				extrOptions &= ~1;
			else
				extrOptions |= 1;
			break;
		case IDC_CHECK2:
			if ((extrOptions >> 1) & 1)
				extrOptions &= ~(1 << 1);
			else
				extrOptions |= (1 << 1);
			break;
		case IDC_CHECK3:
			if ((extrOptions >> 2) & 1)
				extrOptions &= ~(1 << 2);
			else
				extrOptions |= (1 << 2);
			break;
		case IDC_CHECK4:
			if ((extrOptions >> 3) & 1) {
				extrOptions &= ~(1 << 3);
				EnableWindow(GetDlgItem(hDlg, IDC_STATIC1), false);
				EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), false);
				EnableWindow(GetDlgItem(hDlg, IDC_BUTTON2), false);
			}
			else {
				extrOptions |= (1 << 3);
				EnableWindow(GetDlgItem(hDlg, IDC_STATIC1), true);
				EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), true);
				EnableWindow(GetDlgItem(hDlg, IDC_BUTTON2), true);
			}

			break;
		case ID_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hDlg, About);
			break;
		case IDC_BUTTON1:
		case ID_FILE_OPENFILE:
		{

			if (!(extrOptions & (~(1 << 3)))) {
				MessageBoxA(hDlg, "You must enable at least one of the three extraction categories!", "Info", MB_OK | MB_ICONWARNING);
				break;
			}


			std::string filePath = OpenFile(hDlg, "DF file\0*\0");
			if (!filePath.empty()) {
				delete DFCurrInst;

				if (int32_t err = readDFfile(DFCurrInst, filePath)) {
					MessageBoxA(hDlg, DFCurrInst->getErrorMsg(err), "Error", MB_OK | MB_ICONWARNING);
					break;
				}

				if (DFCurrInst) {
					DFCurrInst->versionSig.assign(MDEFTVERSION);

					std::string outputPath("");
					std::string::size_type pos = filePath.rfind('\\');
					if (pos != std::string::npos) {
						outputPath.assign(filePath.substr(0, pos + 1));
						filePath.assign(filePath.substr(pos + 1, filePath.length()));
					}
					if ((extrOptions >> 3) & 1) {
						if (customOutput.empty()) {
							MessageBoxA(hDlg, "No valid path was set. Please select a valid path via the \"Change Path\" Button!", "Info", MB_OK | MB_ICONWARNING);
							break;
						}
						outputPath.assign(customOutput);
						outputPath.push_back('\\');
					}
					outputPath.push_back('_');
					outputPath.append(filePath);

					std::string subfolder("");
					pos = outputPath.rfind('.');
					if (pos != std::string::npos) {
						subfolder.assign(outputPath.substr(pos + 1, outputPath.length()));
						outputPath.assign(outputPath.substr(0, pos));
					}

					// create outputfolder
					CreateDirectoryA(outputPath.c_str(), NULL);

					// and subfolder
					if (!subfolder.empty()) {
						std::string subfolderFull(outputPath);
						subfolderFull.push_back('\\');
						subfolderFull.append(subfolder);
						//DFfileIn.printCurrentOutputPath();
						DFCurrInst->outPutPath.assign(subfolderFull);
						CreateDirectoryA(subfolderFull.c_str(), NULL);

						// might wanna improve this with move semantics:
						subfolder.assign(subfolderFull);
					}

					// prepare status message
					std::string extStatus("Extraction report:\n\n");
					// reset countr
					uint32_t totalFileCounter{ 0 };

					DFCurrInst->fileCounter = 0;
					if (extrOptions & 1) {
						DFCurrInst->writeAllScripts();
						if (DFCurrInst->fileCounter) {
							totalFileCounter += DFCurrInst->fileCounter;
							extStatus.append("Scripts: " + std::to_string(DFCurrInst->fileCounter) + " files\n");
						}
						else {
							extStatus.append("No scripts detected\n");
						}
						DFCurrInst->fileCounter = 0;
					}

					if ((extrOptions >> 1) & 1) {
						DFCurrInst->writeAllAudio();
						if (DFCurrInst->fileCounter) {
							totalFileCounter += DFCurrInst->fileCounter;
							extStatus.append("Audio: " + std::to_string(DFCurrInst->fileCounter) + " files\n");
						}
						else {
							extStatus.append("No audio detected\n");
						}
						DFCurrInst->fileCounter = 0;
					}

					if ((extrOptions >> 2) & 1) {
						DFCurrInst->writeAllFrames();
						if (DFCurrInst->fileCounter) {
							totalFileCounter += DFCurrInst->fileCounter;
							extStatus.append("Frames: " + std::to_string(DFCurrInst->fileCounter) + " files\n");
						}
						else {
							extStatus.append("No frames detected\n");
						}
						DFCurrInst->fileCounter = 0;
					}

					if (!totalFileCounter) {
						extStatus.append("Nothing was extracted!");
						// clean up empty folders
						RemoveDirectoryA(subfolder.c_str());
						RemoveDirectoryA(outputPath.c_str());
					}
					MessageBoxA(hDlg, extStatus.c_str(), "Status", MB_OK | MB_ICONINFORMATION);
				}
				else {
					MessageBoxA(hDlg, "File not initiliazed!", "Error", MB_OK | MB_ICONERROR);
				}
			}
		}
		break;
		case IDC_BUTTON2:
		{
			customOutput = browseDir(hDlg);
			//MessageBoxA(hDlg, test.c_str() , "Error", MB_OK | MB_ICONERROR);
			::SetWindowTextA(GetDlgItem(hDlg, IDC_EDIT1), customOutput.c_str());
		}
		break;
		case ID_FILE_CLOSE:
		case IDCANCEL:
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		}
		break;
	}

	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		// update dialoug text with current version
		::SetWindowTextA(GetDlgItem(hDlg, IDC_STATIC1), std::string("DF extraction tool, version ").append(MDEFTVERSION).c_str());

		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


static std::string OpenFile(HWND hWnd, const char* filter) {

	OPENFILENAMEA ofn;
	CHAR szFile[MAX_PATH] = { 0 };
	ZeroMemory(&ofn, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	if (GetOpenFileNameA(&ofn) == TRUE) {
		return ofn.lpstrFile;
	}
	return std::string();
}

static std::string browseDir(HWND hWnd) {

	CHAR path[MAX_PATH];
	GetModuleFileNameA(NULL, path, MAX_PATH);
	std::string browseStart(path);
	size_t find = browseStart.rfind('\\');
	if (find != std::string::npos) {
		browseStart = browseStart.substr(0, find + 1);
	}
	else {
		browseStart = browseStart;
	}

	BROWSEINFOA bwi;
	ZeroMemory(&bwi, sizeof(BROWSEINFOA));
	bwi.lpfn = BrowseCallbackProc;
	bwi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
	bwi.hwndOwner = hWnd;
	bwi.lpszTitle = "Please select the folder you want to extract to.";
	bwi.lParam = (LPARAM)browseStart.c_str();

	LPCITEMIDLIST pidl = NULL;
	if (pidl = SHBrowseForFolderA(&bwi)) {
		CHAR szFolder[MAX_PATH]{ 0 };
		if (SHGetPathFromIDListA(pidl, szFolder)) {
			return szFolder;
		}
	}

	return std::string();
}
