#include "DFmov.h"

DFmov::~DFmov() {
	delete audioPtr;
}

std::string DFmov::getHeaderInfo() {
	return getFrameData();
};

std::string DFmov::getFrameData() {
	std::string out("FRAME information:\n");
	for (int32_t i = 0; i < movHeader.frameCount; i++) {
		out.append("\nStart frame?:   ").append(std::to_string(movHeader.frames[i].frameCondition));	// 1 = yes, 0 = No
		out.append("\nunknown word1:  ").append(std::to_string(movHeader.frames[i].unknownWord1));
		out.append("\nunknown word2:  ").append(std::to_string(movHeader.frames[i].unknownWord2));
		out.append("\nheight?:        ").append(std::to_string(movHeader.frames[i].height));
		out.append("\nwidth?:         ").append(std::to_string(movHeader.frames[i].width));
		out.append("\nframe location: ").append(std::to_string(movHeader.frames[i].locationFrame));
		out.append("\nsecond location:").append(std::to_string(movHeader.frames[i].locationClickRegion));
		out.append("\nunknown word3:  ").append(std::to_string(movHeader.frames[i].unknownWord3));
		out.append("\nframe size:     ").append(std::to_string(movHeader.frames[i].frameContainerSize));
		out.append("\nframe name:     ").append(movHeader.frames[i].frameName);
		// TODO: ADD INFORMATION FROM CLICK REGIONS
		out.push_back('\n');
	}
	return out;
}

DFmov* DFmov::getDFmov(DFfile* ptr) {
	if (ptr->fileType != DFMOV)
		return nullptr;
	return (DFmov*)ptr;
}

void DFmov::writeAllAudio() {
	std::string out(outPutPath);
	out.append("/AUDIO");
	std::filesystem::create_directory(out);

	writeAllAudioC(out);
}

void DFmov::writeAllFrames() {
	std::string out(outPutPath);
	out.append("/FRAMES");
	std::filesystem::create_directory(out);

	for (int32_t frame = 0; frame < movHeader.frameCount; frame++) {
		writeBMPimage(containers[movHeader.frames[frame].locationFrame], out, movHeader.frames[frame].frameName);
		fileCounter++;
	}
}

void DFmov::updateContainerInfo() {
	containers[0].info.assign("color palette and frame register");
	containers[movHeader.audioLocation].info.assign("information about audio containers");

	containers[movHeader.memoryAllocInfo].info.assign("Memory allocation information");

	for (auto& frame : movHeader.frames) {
		containers[frame.locationClickRegion].info.assign("click region definition for frame in container ");
		containers[frame.locationClickRegion].info.append(std::to_string(frame.locationFrame));

		containers[frame.locationFrame].info.assign("frame \"");
		containers[frame.locationFrame].info.append(frame.frameName);
		containers[frame.locationFrame].info.push_back('"');
		containers[frame.locationFrame].type = Container::IMAGE;
	}
	

	return;
}

int32_t DFmov::initContainerData() {
	// start with data from first container, head information
	movHeader.version = *(int32_t*)(containers[0].data + 0x2);
	// dont even continue if version does not match with 4.0
	if (movHeader.version != 4) {
		return ERRDFVERSION;
	}

	// TODO: AUDIO INFORMATION MISSING
	// TODO: THIS IS INCOMPLETE! CHECK UNKNWON ARRAY
	memcpy(movHeader.unknown, containers[0].data + 24, 72);
	movHeader.audioLocation = *(int32_t*)(containers[0].data + 0x60);
	movHeader.memoryAllocInfo = *(int32_t*)(containers[0].data + 0x64);
	movHeader.unknownRef2 = *(int32_t*)(containers[0].data + 0x68);

	memcpy(colors, containers[0].data + 0x6C, sizeof(ColorPalette) * 256);
	// it keeps 256 colors, no change need

	// read frame data in bottom part of header
	uint8_t* curr = containers[0].data + 0x870;
	movHeader.globalHeight = *(int16_t*)curr;
	curr += 2;
	movHeader.globalWidth = *(int16_t*)curr;
	curr += 2;
	movHeader.unknwonInt = *(int16_t*)curr;
	curr += 4;
	movHeader.frameCount = *(int32_t*)curr;
	curr += 4;

	movHeader.frames.reserve(movHeader.frameCount);

	for (int32_t i = 0; i < movHeader.frameCount; i++) {
		movHeader.frames.emplace_back();
		movHeader.frames[i].frameCondition = *(int32_t*)curr;	// 1 = yes, 0 = No
		curr += 4;
		movHeader.frames[i].unknownWord1 = *(int16_t*)curr;
		curr += 2;
		movHeader.frames[i].unknownWord2 = *(int16_t*)curr;
		curr += 2;
		movHeader.frames[i].height = *(int16_t*)curr;
		curr += 2;
		movHeader.frames[i].width = *(int16_t*)curr;
		curr += 2;
		movHeader.frames[i].locationFrame = *(int32_t*)curr;
		curr += 4;
		movHeader.frames[i].locationClickRegion = *(int32_t*)curr;
		curr += 4;
		movHeader.frames[i].unknownWord3 = *(int16_t*)curr;
		curr += 2;
		movHeader.frames[i].frameContainerSize = *(int32_t*)curr;
		curr += 4;
		memcpy(movHeader.frames[i].frameName, curr + 1, *curr);
		movHeader.frames[i].frameName[*curr] = '\0';
		curr += 16;

		// read corresponding region information for this specific frame
		uint8_t* currRegion = containers[movHeader.frames[i].locationClickRegion].data;
		memcpy(movHeader.frames[i].frameHeadInformation, currRegion, 5 * 4);

		// jump to part where regional info is written
		currRegion += 1090;
		movHeader.frames[i].frameLogicSize = *(int32_t*)currRegion;
		currRegion += 4;
		movHeader.frames[i].frameLogic.reserve(movHeader.frames[i].frameLogicSize);
		for (int32_t fi = 0; fi < movHeader.frames[i].frameLogicSize; fi++) {
			movHeader.frames[i].frameLogic.emplace_back();
			movHeader.frames[i].frameLogic[fi].unknownShort1 = *(int16_t*)currRegion;
			currRegion += 2;
			movHeader.frames[i].frameLogic[fi].unknownInt1 = *(int32_t*)currRegion;
			currRegion += 4;
			movHeader.frames[i].frameLogic[fi].unknownShort2 = *(int32_t*)currRegion;
			currRegion += 2;
			movHeader.frames[i].frameLogic[fi].clickRegionStartX = *(int32_t*)currRegion;
			currRegion += 2;
			movHeader.frames[i].frameLogic[fi].clickRegionStartY = *(int32_t*)currRegion;
			currRegion += 2;
			movHeader.frames[i].frameLogic[fi].clickRegionEndX = *(int32_t*)currRegion;
			currRegion += 2;
			movHeader.frames[i].frameLogic[fi].clickRegionEndY = *(int32_t*)currRegion;
			currRegion += 2;
			movHeader.frames[i].frameLogic[fi].unknownInt2 = *(int32_t*)currRegion;
			currRegion += 4;
			movHeader.frames[i].frameLogic[fi].unknownInt3 = *(int32_t*)currRegion;
			currRegion += 4;
			memcpy(movHeader.frames[i].frameLogic[fi].unknownInts, currRegion, 4 * 10);
			currRegion += 40;
		}
	}

	audioPtr = new AudioBlockInfos(this);

	return 0;
};