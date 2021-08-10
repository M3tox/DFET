#pragma once

inline static bool printLastError(DFextractor& file) {
	if (file.lastError) {
		std::cout << "Error " << file.lastError << ": " << file.GetDFerrorString(file.lastError) << '\n';
		return true;
	}
	return false;
}

void mountFile(DFextractor& DFfileIn, const std::string& outputPath, const std::string& subfolder) {

	while (true) {
		std::cout << "\nWhat would you like to do with this file?\n"
			"1) analyze file\n"
			"2) split and write file into its individual containers\n\n"
			"3) print container info\n"
			"4) print color palette\n"
			"5) print puppet dialouges\n\n"
			"6) write scripts\n"
			"7) write audio\n"
			"8) write frames\n"
			"9) write all\n\n"
			"X) unmount file & exit to main menue\n";

		std::string input;
		getline(std::cin, input);
		//std::cin >> input[0];
		if (input.length() != 1) {
			std::cout << "invalid input! Type the number corresponding to the options or X to exit\n";
			continue;
		}

		switch (input[0]) {
		case '1':
			DFfileIn.analyzeContainer();
			printLastError(DFfileIn);
			break;
		case '2':
			CreateDirectoryA(outputPath.c_str(), NULL);
			DFfileIn.writeBlockFiles(outputPath);
			break;
		case '3':
			DFfileIn.printFileHeaderInfo();
			break;
		case '4':
			DFfileIn.printColorPalette();
			break;
		case '5':
			DFfileIn.printAllDialougTexts();
			break;
		case '6':
		case '7':
		case '8':
		case '9':
		{
			// create outputfolder
			CreateDirectoryA(outputPath.c_str(), NULL);

			// and subfolder
			std::string subfolderFull(outputPath);
			subfolderFull.push_back('/');
			subfolderFull.append(subfolder);
			//DFfileIn.printCurrentOutputPath();
			DFfileIn.changeCurrentOutputPath(subfolderFull);
			CreateDirectoryA(subfolderFull.c_str(), NULL);

			switch (input[0]) {
			case '6':
				DFfileIn.writeAllScripts();	// TODO
				printLastError(DFfileIn);
				break;
			case '7':
				DFfileIn.writeAllAudio();
				printLastError(DFfileIn);
				break;
			case '8':
				DFfileIn.writeAllFrames();
				printLastError(DFfileIn);
				break;
			case '9':
				DFfileIn.writeAllScripts();
				printLastError(DFfileIn);
				DFfileIn.writeAllAudio();
				printLastError(DFfileIn);
				DFfileIn.writeAllFrames();
				printLastError(DFfileIn);
				break;
			}
		}
		break;
		case 'x':
		case 'X':
			return;
		default:
			std::cout << "invalid input! Type the number corresponding to the options\n";
		}
	}
	
}

void devMode() {

	std::cout << "MeToX's DreamFactory Extraction Tool \"DFET\" version " << MDEFTVERSION << "\n\n";

	while (true) {
		std::cout << "What would you like to do?\n"
			"1) mount file\n"
			"2) place holder\n"
			"3) print DF commands\n\n"
			"X) exit application\n";

		std::string input;
		getline(std::cin, input);
		if (input.length() != 1) {
			std::cout << "invalid input! Type the number corresponding to the options\n";
			continue;
		}

		switch (input[0]) {
		case '1':
		{
			std::cout << "specify file location\n";
			getline(std::cin, input);

			DFextractor DFfileIn(input);
			if (printLastError(DFfileIn)) break;
			
			std::string outputPath("");
			std::string::size_type pos = input.rfind('/');
			if (pos != std::string::npos) {
				outputPath.assign(input.substr(0, pos+1));
				input.assign(input.substr(pos+1, input.length()));
			}
			outputPath.push_back('_');
			outputPath.append(input);

			std::string subfolder("");
			pos = outputPath.rfind('.');
			if (pos != std::string::npos) {
				subfolder.assign(outputPath.substr(pos+1, outputPath.length()));
				outputPath.assign(outputPath.substr(0, pos));
			}

			std::cout << "file mounting successful\n";
			mountFile(DFfileIn, outputPath, subfolder);
		}
		break;
		case '2':
			std::cout << "currently unavailable\n";
			// TEST
			break;
		case '3':
			DFextractor::printLookUpTable();
			break;
		case 'x':
		case 'X':
			return;
			break;
		default:
			std::cout << "invalid input! Type the number corresponding to the options or X to exit\n";
		}
	}
	
}
