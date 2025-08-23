#define MDEFTVERSION "0.89"	// make sure its always 4 characters!

#if defined(_WIN32) && defined(DFET_GUI_MODE)

// ===================================================================================
//
// WINDOWS GUI MODE
//
// ===================================================================================

#include <Windows.h>

#include "resource.h"
#include "Shlobj.h"
#include "libs/DFfile/DFlib.h";


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

#elif defined(DFET_CLI_MODE)

// ===================================================================================
//
// CLI MODE
//
// ===================================================================================

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>
#include "libs/DFfile/DFlib.h"

#if defined(_WIN32)
#include <Windows.h>
#endif

void print_help() {
    std::cout << "DFET - DreamFactory Extraction Tool v" << MDEFTVERSION << "\n";
    std::cout << "Usage: dfet <input_file> [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -o <output_dir>  Specify custom output directory (default: same as input file).\n";
    std::cout << "  -s               Extract scripts (default: on).\n";
    std::cout << "  -a               Extract audio (default: on).\n";
    std::cout << "  -f               Extract frames (default: on).\n";
    std::cout << "  -d               Extract raw container data (default: off).\n";
    std::cout << "  -v               Print verbose file information.\n";
    std::cout << "  -h, --help       Show this help message.\n\n";
    std::cout << "Example: dfet mygame.set -o extracted_files -a -f\n";
    std::cout << "Supported file types: BOOTFILE, PUP, SET, MOV, TRK, SND, SFX, 11K, STG, CST, SHP.\n";
}

int main(int argc, char* argv[]) {

#if defined(_WIN32)

    if (GetConsoleWindow() == NULL) {
        if (AllocConsole()) {
            FILE* fp;
            // Redirect standard streams to the new console
            freopen_s(&fp, "CONOUT$", "w", stdout);
            freopen_s(&fp, "CONOUT$", "w", stderr);
            freopen_s(&fp, "CONIN$", "r", stdin);
        }
    }

#endif

    if (argc < 2) {
        print_help();
        return 0;
    }

    std::string input_file_path;
    std::string output_dir_param;
    bool extract_scripts = true;
    bool extract_audio = true;
    bool extract_frames = true;
    bool extract_raw_data = false;
    bool verbose_info = false;

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") { print_help(); return 0; }
        else if (arg == "-o") { if (i + 1 < argc) { output_dir_param = argv[++i]; } else { std::cerr << "Error: -o option requires an argument.\n"; return 1; } }
        else if (arg == "-s") { extract_scripts = true; }
        else if (arg == "-a") { extract_audio = true; }
        else if (arg == "-f") { extract_frames = true; }
        else if (arg == "-d") { extract_raw_data = true; }
        else if (arg == "-v") { verbose_info = true; }
        else if (input_file_path.empty()) { input_file_path = arg; }
        else { std::cerr << "Error: Unknown argument or duplicate input file: " << arg << "\n"; return 1; }
    }

    if (input_file_path.empty()) { std::cerr << "Error: No input file specified.\n"; print_help(); return 1; }

    DFfile* df_instance = nullptr;
    int32_t error_code = readDFfile(df_instance, input_file_path);
    if (error_code != 0 || !df_instance) { // Use 0 for NOERRORS
        std::cerr << "Error reading file: " << DFfile::getErrorMsg(error_code) << "\n";
        if (df_instance) delete df_instance;
        return 1;
    }

    try {
        std::string final_output_path = output_dir_param.empty() ? std::filesystem::path(input_file_path).parent_path().string() : output_dir_param;
        std::string base_filename = std::filesystem::path(input_file_path).stem().string();
        std::string specific_output_folder = final_output_path + "/_" + base_filename;
        std::filesystem::create_directories(specific_output_folder);
        df_instance->outPutPath.assign(specific_output_folder);

        std::cout << "Processing file: " << input_file_path << "\n";
        std::cout << "Output directory: " << specific_output_folder << "\n";

        if (verbose_info) {
            std::cout << "\n--- Header Info ---\n" << df_instance->getHeaderInfo() << "\n";
            std::cout << "--- Container Info ---\n" << df_instance->getContainerInfo() << "\n";
        }

        uint32_t total_files_extracted = 0;

        if (extract_scripts) {
            df_instance->fileCounter = 0;
            df_instance->writeAllScripts();
            if (df_instance->fileCounter > 0) {
                std::cout << "Extracted " << df_instance->fileCounter << " scripts.\n";
                total_files_extracted += df_instance->fileCounter;
            }
            else {
                std::cout << "No scripts detected or extracted.\n";
            }
        }

        if (extract_audio) {
            df_instance->fileCounter = 0;
            df_instance->writeAllAudio();
            if (df_instance->fileCounter > 0) {
                std::cout << "Extracted " << df_instance->fileCounter << " audio files.\n";
                total_files_extracted += df_instance->fileCounter;
            }
            else {
                std::cout << "No audio detected or extracted.\n";
            }
        }

        if (extract_frames) {
            df_instance->fileCounter = 0;
            df_instance->writeAllFrames();
            if (df_instance->fileCounter > 0) {
                std::cout << "Extracted " << df_instance->fileCounter << " frames.\n";
                total_files_extracted += df_instance->fileCounter;
            }
            else {
                std::cout << "No frames detected or extracted.\n";
            }
        }

        if (extract_raw_data) {
            std::string raw_output_path = specific_output_folder + "/_RAW_CONTAINERS";
            std::filesystem::create_directories(raw_output_path);
            df_instance->writeContainersToFiles(raw_output_path);
            std::cout << "Extracted " << df_instance->containers.size() << " raw containers.\n";
        }

        // Final summary
        if (total_files_extracted == 0 && !extract_raw_data) {
            std::cout << "Extraction complete. Nothing was extracted based on the selected options.\n";
        }
        else {
            std::cout << "Extraction complete. Total files: " << total_files_extracted << "\n";
        }

    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << "\n";
        delete df_instance;
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "A general runtime error occurred: " << e.what() << "\n";
        delete df_instance;
        return 1;
    }

    delete df_instance;
    return 0;
}

#endif
