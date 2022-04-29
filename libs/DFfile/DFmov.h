#pragma once
#include "DFfile.h"

class DFmov :
    public DFfile
{
	using DFfile::DFfile;
public:

	~DFmov();

	std::string getHeaderInfo();
	std::string getFrameData();
	static DFmov* getDFmov(DFfile* ptr);

	void writeAllAudio();
	void writeAllFrames();

	void updateContainerInfo();

protected:

	int32_t initContainerData();

	struct MovHeader {
		struct Frame {
			struct FrameInteraction {
				int16_t unknownShort1;
				int32_t unknownInt1;
				int16_t unknownShort2;
				int16_t clickRegionStartX;
				int16_t clickRegionStartY;
				int16_t clickRegionEndX;
				int16_t clickRegionEndY;

				int32_t unknownInt2;	// usually zero
				int32_t unknownInt3;	// -1 mostly
				int32_t unknownInts[10];
			};

			int32_t frameCondition;
			int16_t unknownWord1;
			int16_t unknownWord2;
			int16_t height;
			int16_t width;
			int32_t locationFrame;
			int32_t locationClickRegion;
			int16_t unknownWord3;	// either 0 or 1: 1 = end frame?
			int32_t frameContainerSize;
			char frameName[15];

			// this is in another container at "locationClickRegion"
			int32_t frameHeadInformation[5]; // I dont know nothing of these values...
			int32_t frameLogicSize;	// number of clickable things
			std::vector<FrameInteraction> frameLogic;
		};


		int32_t version;
		int32_t unknown[18];
		int32_t audioLocation;
		int32_t memoryAllocInfo;
		int32_t unknownRef2;

		int16_t globalHeight;	// applies for all frames (I Think)
		int16_t globalWidth;	// applies for all frames
		int32_t unknwonInt;		// seems important
		
		int32_t frameCount;

		std::vector<Frame> frames;
	};

public:
	MovHeader movHeader;
};

