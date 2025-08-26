#include "DFpup.h"

// public functions
std::string DFpup::getHeaderInfo() {
	return std::string();
};

void DFpup::writeAllAudio() {

	for (auto& p : pupData) {
		std::string out(outPutPath);
		out.append("/AUDIO");
		std::filesystem::create_directory(out);

		if (!writeWavFromContainer(out, p.ident, p.audioLocation)) {
			return;
		}
	}

	// little extra: output dialoug as textfile as well
	writeAllDialougTexts(outPutPath);
}

void DFpup::writeAllScripts() {
	struct PupScripts {
		int32_t location;
		char scriptName[31];
	};

	int16_t pupScriptscount = *(int16_t*)(containers[2].data + 22);
	//PupScripts* pupscripts = new PupScripts[pupScriptscount];

	uint8_t* curr = containers[2].data + 24;

	for (int32_t i = 0; i < pupScriptscount; i++) {
		PupScripts ps;	// no reason to keep this in memory
		ps.location = *(int32_t*)(curr);
		curr += 4;
		curr += 4; // another unknown value here, mostly zero
		uint8_t strSize = *curr++;
		memcpy(ps.scriptName, curr, strSize);
		ps.scriptName[strSize] = '\0';
		curr += 31;

		std::string outName(outPutPath);
		outName.push_back('/');
		outName.append(ps.scriptName);
		if (writeScript(containers[ps.location], outName))
			fileCounter++;
	}
}

void DFpup::writeAllFrames() {


	namespace fs = std::filesystem;

	struct FrameRegister {
		int16_t count;
		int16_t unknown;
		int16_t totalEntries; // maybe??
		int32_t locations[64];
	};

	constexpr uint8_t tableCount{ 11 };
	static const char* faceNames[tableCount]{
		"Background",	// 0
		"Body",			// 1
		"Head",			// 2
		"Eyes",			// 3
		"Eyebrows",		// 4
		"Nose",			// 5
		"Jaw",			// 6
		"Left",			// 7
		"Hands 1",		// 8
		"Right",		// 9
		"Hands 2"		// 10
	};


	std::string framePath(outPutPath);
	framePath.append("/FRAMES");
	fs::create_directory(framePath);

	if (*(int32_t*)(containers[0].data + 0x2) == 1) {
		// DUST
		// in Dust was only one stance defined
		FrameRegister frameLocations[tableCount];
		uint8_t* curr = containers[3].data + 22;
		for (uint8_t table = 0; table < tableCount; table++) {

			frameLocations[table].count = *(int16_t*)curr;
			curr += 2;
			frameLocations[table].unknown = *(int16_t*)curr;
			curr += 2;
			frameLocations[table].totalEntries = *(int16_t*)curr;
			curr += 2;

			memcpy(frameLocations[table].locations, curr, 64 * 4);
			curr += 256;

			// only create folder if there is actually something
			if (frameLocations[table].count) {
				std::string tableName(framePath);
				tableName.push_back('/');
				tableName.append(faceNames[table]);
				fs::create_directory(tableName);

				while (frameLocations[table].count--) {
					writeTransPNGimage(tableName, frameLocations[table].locations[frameLocations[table].count]);
					fileCounter++;
				}
			}
		}
	}
	else {
		// TAOOT
		// go to main frame register
		uint32_t tableLocations[64];
		memcpy(tableLocations, containers[3].data + 22, 64 * 4);

		for (int32_t table = 0; table < 64; table++) {
			// not very elegant solution... should be fixed at some point
			if (tableLocations[table] > fileHeader.containerCount || tableLocations[table] == 0)
				break;

			std::string newFolder(framePath);
			newFolder.append("/Stance ");
			newFolder.append(std::to_string(table + 1));
			fs::create_directory(newFolder);

			FrameRegister frameLocations[tableCount];

			uint8_t* curr = containers[tableLocations[table]].data + 22;
			for (int32_t subTable = 0; subTable < tableCount; subTable++) {

				frameLocations[subTable].count = *(int16_t*)curr;
				curr += 2;
				frameLocations[subTable].unknown = *(int16_t*)curr;
				curr += 2;
				frameLocations[subTable].totalEntries = *(int16_t*)curr;
				curr += 2;

				memcpy(frameLocations[subTable].locations, curr, 64 * 4);
				curr += 256;


				// only create folder if there is actually something
				if (frameLocations[subTable].count) {
					std::string tableName(newFolder);
					tableName.push_back('/');
					tableName.append(faceNames[subTable]);
					fs::create_directory(tableName);

					while (frameLocations[subTable].count--) {
						writeTransPNGimage(tableName, frameLocations[subTable].locations[frameLocations[subTable].count]);
						fileCounter++;
					}
				}
			}
		}
	}


	return;
}

// protected functions:
int32_t DFpup::initContainerData() {

	memcpy(colors, containers[0].data + 58, 256 * sizeof(ColorPalette));
	readPuppetStrings();
	return 0;
};

bool DFpup::readPuppetStrings() {

	// calculate number of text blocks, block size always 312 bytes
	// thanks god they always start at 2184, in DF and TI

	int16_t audioTextCount = *(int16_t*)(containers[0].data + 2158);

	uint8_t* curr = containers[0].data + 2160;
	pupData.reserve(audioTextCount);
	// append zeros to strings
	for (int16_t i = 0; i < audioTextCount; i++) {
		pupData.emplace_back();

		pupData[i].unknownInt1 = *(int32_t*)curr;
		curr += 4;
		pupData[i].unknownShort1 = *(int16_t*)curr;
		curr += 2;
		pupData[i].unknownShort2 = *(int16_t*)curr;
		curr += 2;
		pupData[i].audioLocation = *(int32_t*)curr;
		curr += 4;
		pupData[i].animLogic = *(int32_t*)curr;
		curr += 4;
		pupData[i].unknownInt2 = *(int32_t*)curr;
		curr += 4;
		pupData[i].unknownInt3 = *(int32_t*)curr;
		curr += 4;
		pupData[i].text.assign((char*)(++curr), *curr);
		curr += 255;
		pupData[i].ident.assign((char*)(++curr), *curr);
		curr += 31;
	}

	return true;
}

void DFpup::writeAllDialougTexts(const std::string& path) {

	std::string fileName(path);
	fileName.append("\\AUDIO\\texts.csv");
	std::ofstream output(fileName, std::ios::binary, std::ios::trunc);
	// header:
	output << "\"ID\", " << "\"container\", " << "\"Identifier\", " << "\"Text\"\n";

	for (short i = 0; i < pupData.size(); i++) {
		output << i + 1 << ", " << pupData[i].audioLocation << ", \"" << pupData[i].ident << "\", \"" << pupData[i].text << "\"\n";
		//output <<  i+1 << "\tAudioLoc: "<< strBlocks0[i].audioLocation << "\tFile name: " << strBlocks0[i].ident << ".wav\tText: " << strBlocks0[i].text << '\n';
	}
	output.close();
	fileCounter++;
}