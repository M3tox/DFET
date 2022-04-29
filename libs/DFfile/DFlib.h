#pragma once

#include "DFboot.h"
#include "DFset/DFset.h"
#include "DFpup.h"
#include "DFmov.h"
#include "DFaudio.h"
#include "DFstg.h"
#include "DFcst.h"
#include "DFshp.h"
#include "DFsnd.h"

// first function to call to determine correct class
static int32_t readDFfile(DFfile* &fileInst, const std::string& filePath) {

	std::ifstream file(filePath);
	if (!file.good()) {
		fileInst = nullptr;
		return DFfile::ERRFILENOTFOUND;
	}

	// sanity check:
	file.seekg(0, file.end);
	const std::streamoff fileSize = file.tellg();

	// valid DF file can't be smaller than this
	if (fileSize < 1024) {
		fileInst = nullptr;
		return DFfile::ERRFILEFORMAT;
	}

	// read the first 40 bytes and make sure the LPPALPPA format is written
	char FileData[8];
	file.seekg(32, file.beg);
	file.read(FileData, sizeof FileData);


	if (memcmp(FileData, "LPPALPPA", 8)) {
		fileInst = nullptr;
		return DFfile::ERRFILEFORMAT;
	}

	// initial checks done

	// cut off file name and its location
	std::string fileName(filePath);
	std::string fileLocation;
	std::string::size_type pos = filePath.rfind('\\');
	if (pos != std::string::npos) {
		fileName.assign(filePath.substr(pos + 1, filePath.length()));
		fileLocation.assign(filePath.substr(0, pos));
	}
	else {
		pos = filePath.rfind('/');
		if (pos != std::string::npos) {
			fileName.assign(filePath.substr(pos + 1, filePath.length()));
			fileLocation.assign(filePath.substr(0, pos));
		}
	}

	// capitalize file name if its written in small letters for some reason
	for (uint8_t i = 0; i < fileName.length(); i++)
		fileName.at(i) = toupper(fileName.at(i));

	// check file type by looking its suffix
	pos = fileName.rfind('.');
	if (pos == std::string::npos) {
		if (fileName.compare("BOOTFILE"))
		{
			// If it has no suffix and is no bootfile, then I dont know...
			fileInst = nullptr;
			return DFfile::ERRNOTSUPPORTED;
		}

		// process BOOTFILE, should be the only one without suffix. Comparision makes sure
		fileInst = new DFboot(fileName, DFfile::DFBOOTFILE);
	}
	else {
		// if here, it must be a regular file, get the files suffix
		std::string suffix = fileName.substr(pos + 1, fileName.length());
		//fileName = fileName.substr(0, pos); // cut off suffix

		if (!suffix.compare("PUP")) {
			// PUPPET FILE
			fileInst = new DFpup(fileName, DFfile::DFPUP);
		}
		else if (!suffix.compare("SET")) {
			// SET FILE
			fileInst = new DFset(fileName, DFfile::DFSET);
		}
		else if (!suffix.compare("MOV")) {
			// MOVIE FILE
			fileInst = new DFmov(fileName, DFfile::DFMOV);
		}
		else if (!suffix.compare("TRK")) {
			// TRACK FILE
			fileInst = new DFaudio(fileName, DFfile::DFTRK);
		}
		else if (!suffix.compare("SND")) {
			// SOUND FILE
			fileInst = new DFsnd(fileName, DFfile::DFSND);
		}
		else if (!suffix.compare("SFX")) {
			// SFX FILE (sound effects)
			fileInst = new DFaudio(fileName, DFfile::DFSFX);
		}
		else if (!suffix.compare("11K")) {
			// 11K (shorter track files)
			fileInst = new DFaudio(fileName, DFfile::DF11K);
		}
		else if (!suffix.compare("STG")) {
			// STAGE FILE
			fileInst = new DFstg(fileName, DFfile::DFSTG);
		}
		else if (!suffix.compare("CST")) {
			// CAST FILE
			fileInst = new DFcst(fileName, DFfile::DFCST);
		}
		else if (!suffix.compare("SHP")) {
			// SHOP FILE
			fileInst = new DFshp(fileName, DFfile::DFSHP);
		}

		else {
			fileInst = nullptr;
			return DFfile::ERRNOTSUPPORTED;
		}
	}

	// set default output path for possible later operations
	fileInst->outPutPath.assign(fileLocation);
	// read whole file splitten up in containers already into RAM
	// and and initialize the data
	return fileInst->readFileIntoMemory(filePath);
}