#pragma once

#include "DFfile.h"

class DFcst : public DFfile
{
	using DFfile::DFfile;
public:

	std::string getHeaderInfo() {
		return std::string();
	};

	void writeAllScripts() {

		int32_t tableCount = *(int32_t*)(containers[0].data + 0x938);
		uint8_t* curr = (containers[0].data + 0x93C);
		while (tableCount--) {
			// process puppet table
			int32_t pupLogicLocation = *(int32_t*)curr;

			std::string pupDir(outPutPath);
			pupDir.push_back('/');
			pupDir.append((char*)containers[pupLogicLocation].data + 0x2B, *(containers[pupLogicLocation].data + 0x2A));
			std::filesystem::create_directory(pupDir);

			if (writeScript(containers[*(int32_t*)(containers[pupLogicLocation].data + 0x26)], pupDir + "/Script")) {
				fileCounter++;
			}
			curr += 16;
		}
	}

	void writeAllFrames() {
		namespace fs = std::filesystem;

		int32_t tableCount = *(int32_t*)(containers[0].data + 0x938);
		uint8_t* curr = (containers[0].data + 0x93C);
		while (tableCount--) {
			// process puppet table
			int32_t pupLogicLocation = *(int32_t*)curr;

			std::string pupDir(outPutPath);
			pupDir.push_back('/');
			pupDir.append((char*)containers[pupLogicLocation].data + 0x2B, *(containers[pupLogicLocation].data + 0x2A));
			fs::create_directory(pupDir);

			// each puppet has multiple sets of images
			// go though each set
			int32_t setCount = *(int32_t*)(containers[pupLogicLocation].data + 0x5A);

			uint8_t* pupTable = containers[pupLogicLocation].data + 0x5E;

			while (setCount--) {
				int32_t setInfoLocation = *(int32_t*)pupTable;
				pupTable += 16;
				//uint8_t strLength = *pupTable++;
				std::string subdir(pupDir);
				subdir.push_back('/');
				subdir.append((char*)pupTable, *pupTable++);
				fs::create_directory(subdir);

				uint8_t* setTable = containers[setInfoLocation].data + 0x72;

				int32_t setContentCount = *(int*)setTable;
				setTable += 4;
				while (setContentCount--) {
					writeTransPNGimage(subdir, *(int32_t*)setTable);
					fileCounter++;
					setTable += 44;
				}

				pupTable += 15;
			}
			curr += 16;
		}

		return;
	}

protected:

	int32_t initContainerData() {
		// dont even continue if version is higher than 4.0
		if (*(int32_t*)(containers[0].data + 0x2) > 4) {
			return ERRDFVERSION;
		}

		memcpy(colors, containers[0].data + 36, 256 * sizeof(ColorPalette));

		return 0;
	};
};