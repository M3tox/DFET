#pragma once

#include "DFfile.h"

class DFpup : public DFfile
{
	using DFfile::DFfile;
public:
	std::string getHeaderInfo();

	void writeAllAudio();
	void writeAllScripts();
	void writeAllFrames();

protected:
	struct PuppetData {
		// these string blocks are within Block 0:
		// they are all exactly 312 bytes big and split as follows:
		int32_t unknownInt1;
		int16_t unknownShort1;
		int16_t unknownShort2;
		int32_t audioLocation;
		int32_t animLogic;
		int32_t unknownInt2;
		int32_t unknownInt3;
		std::string text;
		std::string ident;
	};

	int32_t initContainerData();
	bool readPuppetStrings();
	void writeAllDialougTexts(const std::string& path);

	std::vector<PuppetData>pupData;

};

