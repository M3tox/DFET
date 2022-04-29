#include "DFset.h"

void DFset::TransitionContainer::FrameInformation::readFrameInformation(uint8_t* dataEntry) {

	dataEntry += swapEndians(posX, dataEntry);
	dataEntry += swapEndians(posZ, dataEntry);
	dataEntry += swapEndians(posY, dataEntry);

	dataEntry += swapEndians(axisX, dataEntry);

	posX16 = *(int16_t*)dataEntry;
	dataEntry += 2;
	posZ16 = *(int16_t*)dataEntry;
	dataEntry += 2;
	posY16 = *(int16_t*)dataEntry;
	dataEntry += 2;
	axisX8 = *(int16_t*)dataEntry;
	dataEntry += 2;

	motionInfo = *(int32_t*)dataEntry;
	dataEntry += 4;
	frameContainerLoc = *(int32_t*)dataEntry;
	dataEntry += 4;
	framePairID = *(int32_t*)dataEntry;
	dataEntry += 4;
	transitionLog = *(int32_t*)dataEntry;
	dataEntry += 4;
	viewID = *(int32_t*)dataEntry;

	return;
}

void DFset::TransitionContainer::FrameInformation::writeFrameInformation(uint8_t* dataOut) {

	dataOut += swapEndians(*(double*)dataOut, (uint8_t*)&posX);
	dataOut += swapEndians(*(double*)dataOut, (uint8_t*)&posZ);
	dataOut += swapEndians(*(double*)dataOut, (uint8_t*)&posY);

	dataOut += swapEndians(*(double*)dataOut, (uint8_t*)&axisX);

	*(int16_t*)dataOut = posX16;
	dataOut += 2;
	*(int16_t*)dataOut = posZ16;
	dataOut += 2;
	*(int16_t*)dataOut = posY16;
	dataOut += 2;
	*(int16_t*)dataOut = axisX8;
	dataOut += 2;

	*(int32_t*)dataOut = motionInfo;
	dataOut += 4;
	*(int32_t*)dataOut = frameContainerLoc;
	dataOut += 4;
	*(int32_t*)dataOut = framePairID;
	dataOut += 4;
	*(int32_t*)dataOut = transitionLog;
	dataOut += 4;
	*(int32_t*)dataOut = viewID;
	return;
}

std::string DFset::TransitionContainer::FrameInformation::getFrameInfo() {
	static const char* frameTypes[3]{
		{" = in motion"},
		{" = low frame"},
		{" = high frame"}
	};

	std::string out("General information:\n");

	out.append(" - Frame Type:          ").append(std::to_string(motionInfo)).append(frameTypes[motionInfo]).push_back('\n');
	out.append(" - transition logic:    ").append(std::to_string(transitionLog)).push_back('\n');
	out.append(" - index in View Table: ").append(std::to_string(viewID)).push_back('\n');
	out.append(" - frame container:     ").append(std::to_string(frameContainerLoc)).push_back('\n');
	out.append(" - frame ID?:           ").append(std::to_string(framePairID)).push_back('\n');
	out.append("\nCamera location in floats:\n");
	out.append(" - Position X:          ").append(std::to_string(posX)).push_back('\n');
	out.append(" - Position Z:          ").append(std::to_string(posZ)).push_back('\n');
	out.append(" - Position Y (height): ").append(std::to_string(posY)).push_back('\n');
	out.append(" - horizontal rotation: ").append(std::to_string(axisX)).append(" / ").append(std::to_string(axisX * 180 / PI)).append(" degrees\n");
	out.append("\nCamera location as integer:\n");
	out.append(" - Position X (short):  ").append(std::to_string(posX16)).push_back('\n');
	out.append(" - Position Z (short):  ").append(std::to_string(posZ16)).push_back('\n');
	out.append(" - Position Y (short):  ").append(std::to_string(posY16)).push_back('\n');
	// show it also as percentage, might be neat
	float temp = 100;
	temp /= 256;
	temp *= axisX8;
	out.append(" - Rotation as 1 Byte:  ").append(std::to_string(axisX8)).append(" = ").append(std::to_string(temp)).append("%\n");
	return out;
}

// OTHER STRUCT SceneTransitionTable!
void DFset::SceneTransitionTable::readTransitions(DFset* setRef) {
	unknownInt = *(int32_t*)setRef->containers[setRef->setHeader.transitionRegister].data;
	transitionCount = *(int32_t*)(setRef->containers[setRef->setHeader.transitionRegister].data + 4);
	uint8_t* curr = setRef->containers[setRef->setHeader.transitionRegister].data + 8;

	transitions.reserve(transitionCount);
	for (uint32_t road = 0; road < transitionCount; road++) {
		transitions.emplace_back();
		transitions[road].locationTransitionInfo = *(int32_t*)curr;
		curr += 4;
		transitions[road].locationSceneA = *(int32_t*)curr;
		curr += 4;
		transitions[road].locationSceneB = *(int32_t*)curr;
		curr += 4;
		transitions[road].unknownShort1 = *(int16_t*)curr;
		curr += 2;
		transitions[road].unknownShort2 = *(int16_t*)curr;
		curr += 2;

		// DATA FROM TABLE READ. NOW DIRECTLY TAKE ROAD DATA FROM OTHER CONTAINER AS WELL
		uint8_t* currRoad = setRef->containers[transitions[road].locationTransitionInfo].data;

		transitions[road].unknownInt = *(int32_t*)currRoad;
		currRoad += 4;
		transitions[road].rotation8 = *(int16_t*)currRoad;
		currRoad += 2;
		transitions[road].viewIDstart = *(int32_t*)currRoad;
		currRoad += 4;
		transitions[road].viewIDend = *(int32_t*)currRoad;
		currRoad += 4;
		currRoad += swapEndians(transitions[road].xAxisStart, currRoad);
		currRoad += swapEndians(transitions[road].zAxisStart, currRoad);
		currRoad += swapEndians(transitions[road].yAxisStart, currRoad);

		currRoad += swapEndians(transitions[road].xAxisEnd, currRoad);
		currRoad += swapEndians(transitions[road].zAxisEnd, currRoad);
		currRoad += swapEndians(transitions[road].yAxisEnd, currRoad);

		transitions[road].transitionName.assign((char*)currRoad, *currRoad++);
		currRoad += 15;
		transitions[road].entriesCount = *(int32_t*)currRoad;
		currRoad += 4;
		transitions[road].entries.reserve(transitions[road].entriesCount);

		for (int32_t e = 0; e < transitions[road].entriesCount; e++) {
			transitions[road].entries.emplace_back();
			currRoad += swapEndians(transitions[road].entries[e].xAxis, currRoad);
			currRoad += swapEndians(transitions[road].entries[e].zAxis, currRoad);
			currRoad += swapEndians(transitions[road].entries[e].yAxis, currRoad);
		}

		// NOW UPDATE TRANSITION TABLES THAT HOLD INFORMATION ABOUT THE FRAME CONTAINERS BEING USED
		transitions[road].frameRegister[TransitionContainer::SCENE_A].readTransitionContainer(setRef->containers[transitions[road].locationSceneA].data, setRef);
		transitions[road].frameRegister[TransitionContainer::SCENE_B].readTransitionContainer(setRef->containers[transitions[road].locationSceneB].data, setRef);
	}
}

std::string DFset::SceneTransitionTable::getTransitionData() {
	if (!transitionCount)
		return "NO TRANSITIONS/ROADS GIVEN!\n";
	std::string out("\nTRANSITION or ROAD TABLE\n");
	for (int32_t trans = 0; trans < transitionCount; trans++) {
		out.append("Transition index:           ").append(std::to_string(trans)).push_back('\n');
		out.append(getTransitionData(trans));
		out.push_back('\n');
		out.append(transitions[trans].getAllTransitions());
		out.push_back('\n');
	}

	return out;
}

std::string DFset::SceneTransitionTable::getTransitionData(int32_t rot) {
	std::string out;
	out.append("Location of road info:      ").append(std::to_string(transitions[rot].locationTransitionInfo)).push_back('\n');
	out.append("Location scene A:           ").append(std::to_string(transitions[rot].locationSceneA)).push_back('\n');
	out.append("Location scene B:           ").append(std::to_string(transitions[rot].locationSceneB)).push_back('\n');
	out.append("Unknown short1:             ").append(std::to_string(transitions[rot].unknownShort1)).push_back('\n');
	out.append("Unknown short2:             ").append(std::to_string(transitions[rot].unknownShort2)).push_back('\n');
	out.append("\nFROM ROAD DATA:\n");
	out.append("unknownInt:                 ").append(std::to_string(transitions[rot].unknownInt)).push_back('\n');
	out.append("unknownShort:               ").append(std::to_string(transitions[rot].rotation8)).push_back('\n');
	out.append("View ID start (A):          ").append(std::to_string(transitions[rot].viewIDstart)).push_back('\n');
	out.append("View ID end   (B):          ").append(std::to_string(transitions[rot].viewIDend)).push_back('\n');

	out.append("X axis start position       ").append(std::to_string(transitions[rot].xAxisStart)).push_back('\n');
	out.append("Z axis start position       ").append(std::to_string(transitions[rot].zAxisStart)).push_back('\n');
	out.append("Y axis start position       ").append(std::to_string(transitions[rot].yAxisStart)).push_back('\n');

	out.append("X axis end position         ").append(std::to_string(transitions[rot].xAxisEnd)).push_back('\n');
	out.append("Z axis end position         ").append(std::to_string(transitions[rot].zAxisEnd)).push_back('\n');
	out.append("Y axis end position         ").append(std::to_string(transitions[rot].yAxisEnd)).push_back('\n');

	out.append("transition name:            ").append(transitions[rot].transitionName).push_back('\n');
	out.append("sub entries:                ").append(std::to_string(transitions[rot].entriesCount)).push_back('\n');
	for (uint32_t e = 0; e < transitions[rot].entriesCount; e++) {
		out.append(" - X axis:                  ").append(std::to_string(transitions[rot].entries[e].xAxis)).push_back('\n');
		out.append(" - Z axis:                  ").append(std::to_string(transitions[rot].entries[e].zAxis)).push_back('\n');
		out.append(" - Y axis:                  ").append(std::to_string(transitions[rot].entries[e].yAxis)).append("\n\n");
	}


	return out;
}

// NEXT STRUCT: ROTATION TABLE
/*
void DFset::SceneRotationTable::readRotations(uint8_t* sceneContainer) {
	unknownInt = *(int32_t*)sceneContainer;
	locationHiRes = *(int32_t*)(sceneContainer + 4);
	sceneCount = *(int32_t*)(sceneContainer + 8);
	uint8_t* curr = sceneContainer + 12;
	scenes = new SceneRotationEntries[sceneCount];
	for (uint32_t scene = 0; scene < sceneCount; scene++) {
		scenes[scene].locationViewTable = *(int32_t*)curr;
		curr += 4;
		scenes[scene].locationTurnRIGHT = *(int32_t*)curr;
		curr += 4;
		scenes[scene].locationTurnLEFT = *(int32_t*)curr;
		curr += 4;
		scenes[scene].mainScene = *(int16_t*)curr;
		curr += 2;
		scenes[scene].rotation8 = *(int16_t*)curr;
		curr += 2;
	}
}

std::string DFset::SceneRotationTable::getRotationData() {
	std::string out("\nROTATION TABLE small\n");
	for (int32_t rot = 0; rot < sceneCount; rot++) {
		out.append("Scene index:               ").append(std::to_string(rot)).push_back('\n');
		out.append(getRotationData(rot));
		out.push_back('\n');
	}


	return out;
}

std::string DFset::SceneRotationTable::getRotationData(int32_t rot) {
	std::string out;
	out.append("Location of view table:    ").append(std::to_string(scenes[rot].locationViewTable)).push_back('\n');
	out.append("Location turn table RIGHT: ").append(std::to_string(scenes[rot].locationTurnRIGHT)).push_back('\n');
	out.append("Location turn table LEFT:  ").append(std::to_string(scenes[rot].locationTurnLEFT)).push_back('\n');
	out.append("Main scene yes or not?:    ").append(std::to_string(scenes[rot].mainScene)).push_back('\n');
	out.append("unknown short, usually 2:  ").append(std::to_string(scenes[rot].rotation8)).push_back('\n');

	return out;
}
*/
// TRANSITION TABLE FUNCTIONS
std::string DFset::SceneTransitionTable::SceneTransitionEntries::getAllTransitions() {

	static const char* directionstr[2]{ {"\"Scene A\"\n"},{"\"Scene B\"\n"} };
	int32_t locations[2]{ locationSceneA , locationSceneB };
	// check all blocks that have transition correlations
	// iterate though both directions
	std::string out("\nTRANSITION TABLES:           ");
	for (uint8_t direction = 0; direction < 2; direction++) {
		out.append(directionstr[direction]);
		out.append("Container:      ");
		out.append(std::to_string(locations[direction]));
		out.push_back('\n');

		out.append("\nUnknownInt:     ").append(std::to_string(frameRegister[direction].unknownInt));
		out.append("\nDestination:    ").append(std::to_string(frameRegister[direction].destination));
		out.append("\ntotal frames:   ").append(std::to_string(frameRegister[direction].frameCount));
		out.push_back('\n');

		for (uint32_t i = 0; i < frameRegister[direction].frameCount; i++) {
			out.append("FRAME ");
			out.append(std::to_string(i+1));
			out.append(" / ");
			out.append(std::to_string(frameRegister[direction].frameCount));
			out.push_back('\n');
			out.append(frameRegister[direction].frameInfos[i].getFrameInfo());
			out.push_back('\n');
		}
	}
	

	return out;
}