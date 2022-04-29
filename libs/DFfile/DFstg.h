#pragma once

#include "DFfile.h"

class DFstg : public DFfile
{
	using DFfile::DFfile;
public:
	~DFstg() {
		delete[] frames;
	}

	std::string getHeaderInfo() {
		return std::string();
	};

	std::string getFrameInfo() {
		std::string out;
		for (uint32_t i = 0; i < frameCount; i++) {
			out.append("Identifier:           ").append(frames[i].frameName).push_back('\n');
			out.append("Condition:            ").append(std::to_string(frames[i].frameCondition)).push_back('\n');
			out.append("unknown Word:         ").append(std::to_string(frames[i].unknownWord1)).push_back('\n');
			out.append("Location Script:      ").append(std::to_string(frames[i].locationScript)).push_back('\n');
			out.append("Location Frame:       ").append(std::to_string(frames[i].locationFrame)).push_back('\n');
			out.append("Location click Logic: ").append(std::to_string(frames[i].locationClickLogic)).push_back('\n');
			out.append("Height:               ").append(std::to_string(frames[i].height)).push_back('\n');
			out.append("Width:                ").append(std::to_string(frames[i].width)).push_back('\n');
			out.append("Size:                 ").append(std::to_string(frames[i].containerSize)).push_back('\n');
			out.push_back('\n');
		}
		return out;
	}

	void writeAllScripts() {
		std::string out(outPutPath);
		out.append("/SCRIPTS");
		std::filesystem::create_directory(out);

		if (writeScript(containers[1], out + "/Main"))
			fileCounter++;

		for (uint32_t i = 0; i < frameCount; i++) {
			std::string scrName(out);
			scrName.push_back('/');
			scrName.append(frames[i].frameName);
			if (writeScript(containers[frames[i].locationScript], scrName))
				fileCounter++;
		}
	}
	void writeAllFrames() {
		std::string framePath(outPutPath);
		framePath.append("/FRAMES");
		std::filesystem::create_directory(framePath);

		for (uint32_t i = 0; i < frameCount; i++) {
			writeBMPimage(containers[frames[i].locationFrame], framePath, frames[i].frameName);
			fileCounter++;
		}
	}

protected:

	struct Frame {
		int32_t frameCondition;
		int16_t unknownWord1;
		int32_t locationScript;
		int32_t locationFrame;
		int32_t locationClickLogic;
		int32_t unknownDword;

		int16_t height;
		int16_t width;

		int32_t containerSize;
		char frameName[15];
	};

	int32_t frameCount;
	Frame* frames;

	int32_t initContainerData() {
		memcpy(colors, containers[0].data + 56, 256 * sizeof(ColorPalette));

		frameCount = *(int32_t*)(containers[0].data + 2120);

		frames = new Frame[frameCount];

		uint8_t* curr = containers[0].data + 2124;

		for (uint32_t i = 0; i < frameCount; i++) {
			frames[i].frameCondition = *(int32_t*)curr;
			curr += 4;
			frames[i].unknownWord1 = *(int16_t*)curr;
			curr += 2;
			frames[i].locationScript = *(int32_t*)curr;
			curr += 4;
			frames[i].locationFrame = *(int32_t*)curr;
			curr += 4;
			frames[i].locationClickLogic = *(int32_t*)curr;
			curr += 4;
			frames[i].unknownDword = *(int32_t*)curr;
			curr += 4;
			frames[i].height = *(int16_t*)curr;
			curr += 2;
			frames[i].width = *(int16_t*)curr;
			curr += 2;
			frames[i].containerSize = *(int32_t*)curr;
			curr += 4;

			uint8_t strSize = *curr++;
			memcpy(frames[i].frameName, curr, strSize);
			frames[i].frameName[strSize] = '\0';
			curr += 15;
		}

		return 0;
	};
};