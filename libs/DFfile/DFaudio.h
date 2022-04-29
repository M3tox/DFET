#pragma once

#include "DFfile.h"

class DFaudio : public DFfile
{
	using DFfile::DFfile;
public:
	std::string getHeaderInfo() {
		return std::string();
	};

	void writeAllAudio() {
		std::string out(outPutPath);
		out.append("/AUDIO");
		std::filesystem::create_directory(out);

		writeAllAudioC(out);
	}

protected:
	int32_t initContainerData() {
		audioPtr = new AudioBlockInfos(this);
		return 0;
	};
};