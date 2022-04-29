#include "DFset.h"


DFset::SceneData::SceneData(int32_t sceneID, DFset* inst)
	: setRef(inst) {

	//dataEntry
	uint8_t* curr = setRef->containers[setRef->setHeader.mainSceneRegister].data + (sceneID * 42);
	unknownDWORD1 = *(int32_t*)(curr);
	curr += 4;
	XaxisMap = *(int16_t*)(curr);
	curr += 2;
	ZaxisMap = *(int16_t*)(curr);
	curr += 2;
	YaxisMap = *(int16_t*)(curr);
	curr += 2;
	locationViews = *(int32_t*)(curr);
	curr += 4;
	locationViewLogic[RIGHTTURNS] = *(int32_t*)(curr);
	curr += 4;
	locationViewLogic[LEFTTURNS] = *(int32_t*)(curr);
	curr += 4;
	locationScript = *(int32_t*)(curr);
	curr += 4;
	sceneName.assign((char*)curr, *curr++);
	//uint8_t stringSize = *curr++;
	//memcpy(sceneName, curr, stringSize);
	//sceneName[stringSize] = 0;

	containerSceneScript = inst->containers[locationScript];

	readViewTable();
	rotationRegister[RIGHTTURNS].readTransitionContainer(setRef->containers[locationViewLogic[RIGHTTURNS]].data, inst);
	rotationRegister[LEFTTURNS].readTransitionContainer(setRef->containers[locationViewLogic[LEFTTURNS]].data, inst);
}

DFset::SceneData::~SceneData() {
	//delete[] sceneViews;
}

std::string DFset::SceneData::getSceneInfo() {

	std::string out("\nSCENE HEADER\n");
	out.append("Name:                    ").append(sceneName).push_back('\n');
	out.append(" - unknown integer1:     ").append(std::to_string(unknownDWORD1)).push_back('\n');
	out.append(" - position X axis:      ").append(std::to_string(XaxisMap)).push_back('\n');
	out.append(" - position Z axis:      ").append(std::to_string(ZaxisMap)).push_back('\n');
	out.append(" - position Y axis:      ").append(std::to_string(YaxisMap)).push_back('\n');
	out.append(" - Location View table:  ").append(std::to_string(locationViews)).push_back('\n');
	out.append(" - Location right turns: ").append(std::to_string(locationViewLogic[RIGHTTURNS])).push_back('\n');
	out.append(" - Location left turns:  ").append(std::to_string(locationViewLogic[LEFTTURNS])).push_back('\n');
	out.append(" - Location script:      ").append(std::to_string(locationScript)).push_back('\n');
	out.append("\nVIEW TABLE HEADER:\n");
	out.append(" - location X:           ").append(std::to_string(sceneLocation[0])).push_back('\n');
	out.append(" - location Z:           ").append(std::to_string(sceneLocation[1])).push_back('\n');
	out.append(" - location Y:           ").append(std::to_string(sceneLocation[2])).push_back('\n');
	out.append(" - unknown integer2:     ").append(std::to_string(unknownVal[0])).push_back('\n');
	out.append(" - location to script    ").append(std::to_string(unknownVal[1])).push_back('\n');

	return out;
}

std::string DFset::SceneData::getViewData() {
	std::string out("\nVIEW TABLE ENTRIES:\n");
	out.append("Scene name:       ").append(sceneName).push_back('\n');
	out.append("Container ID:     ").append(std::to_string(locationViews)).push_back('\n');
	out.append("Scene View count: ").append(std::to_string(SceneViewCount)).push_back('\n');
	for (uint32_t i = 0; i < SceneViewCount; i++) {
		out.append(" - view Name:        ").append(sceneViews[i].viewName).push_back('\n');
		out.append(" - global view ID?:  ").append(std::to_string(sceneViews[i].viewID)).push_back('\n');
		out.append(" - Object reference: ").append(std::to_string(sceneViews[i].locationObjects)).push_back('\n');
		out.append(" - unknownDbl:       ").append(std::to_string(sceneViews[i].unknownDB2)).push_back('\n');
		out.append(" - unknownInt:       ").append(std::to_string(sceneViews[i].viewPairType)).append("\n\n");
	}
	return out;
}

std::string DFset::SceneData::getRightTurnLogic() {
	return getTurnData(RIGHTTURNS);
}

std::string DFset::SceneData::getLeftTurnLogic() {
	return getTurnData(LEFTTURNS);
}

std::string DFset::SceneData::getTurnData() {
	std::string out(getTurnData(RIGHTTURNS));
	out.append(getTurnData(LEFTTURNS));

	return out;
}

std::string DFset::SceneData::getTurnData(bool index) {

	static const char* turnstr[2]{ {" \"RIGHT\""},{" \"LEFT\""} };

	std::string out("TURN TABLE:\n"
		"Container:      ");
	out.append(std::to_string(locationViewLogic[index]));
	out.push_back('\n');

	out.append("\nUnknownInt:     ").append(std::to_string(rotationRegister[index].unknownInt)).push_back('\n');
	out.append("\nDestination:   ").append(std::to_string(rotationRegister[index].destination)).push_back('\n');
	out.append("\ntotal frames:   ").append(std::to_string(rotationRegister[index].frameCount)).append("\n\n");
	out.push_back('\n');

	for (uint32_t i = 0; i < rotationRegister[index].frameCount; i++) {
		out.append("FRAME ").append(std::to_string(i+1));;
		out.append(" OF ").append(std::to_string(rotationRegister[index].frameCount));
		out.append(" / ").append(sceneName);
		out.push_back('\n');

		if (rotationRegister[index].frameInfos[i].viewID == -1)
			out.append("No view name specified\n");
		else
			out.append("Name:   ").append(sceneViews[rotationRegister[index].frameInfos[i].viewID].viewName).push_back('\n');

		out.append(rotationRegister[index].frameInfos[i].getFrameInfo());
		out.push_back('\n');
	}

	return out;
}


/*
int32_t DFset::SceneData::printFrameInformation() {
	static const std::string rotDirs[2]{ "Right turn:\n","Left turn:\n" };

	std::cout << "Scene: " << sceneName << '\n';
	std::cout << "Num     |Locatio|Specs  |Transit|Resolution\n";
	std::cout << "--------+-------+-------+-------+-----------\n";

	int32_t totalFrames{ 0 };
	// we have to go twice though, because there are definitions for LEFT and RIGHT each
	for (uint8_t direction = 0; direction < 2; direction++) {
		std::cout << rotDirs[direction];
		// start with turn right frames
		for (uint32_t i = 0; i < turnCount[0]; i++) {

			std::cout << ++totalFrames << "\t|";
			std::cout << turns[direction][i].frameContainerLoc << "\t|";
			if (!turns[direction][i].motionInfo)
				std::cout << "Move";
			else if (turns[direction][i].motionInfo == 1)
				std::cout << "Low";
			else if (turns[direction][i].motionInfo == 2)
				std::cout << "High";
			else
				std::cout << turns[direction][i].motionInfo;

			std::cout << "\t|";

			if (turns[direction][i].transitionLog)
				std::cout << "Yes";
			else
				std::cout << "No";
			std::cout << "\t|" << *(int16_t*)setRef->fileBlock[turns[direction][i].frameContainerLoc].blockdata;
			std::cout << " x " << *(int16_t*)(setRef->fileBlock[turns[direction][i].frameContainerLoc].blockdata + 2) << '\n';
		}

		// now print all transition, if it has transition frames
		for (uint32_t i = 0; i < turnCount[direction]; i++) {
			if (turns[direction][i].transitionLog) {

				int32_t turnCountTr = *(int32_t*)(setRef->fileBlock[turns[direction][i].transitionLog].blockdata + 4);

				SceneTransition trans;
				uint8_t* curr = setRef->fileBlock[turns[direction][i].transitionLog].blockdata + 12;
				for (uint32_t i = 0; i < turnCountTr; i++) {
					//memcpy(&trans, curr + (sizeof(SceneTransition) * i), sizeof(SceneTransition));
					trans.readSceneTransitions(curr + 60 * i);

					std::cout << ++totalFrames << "\t|";
					std::cout << trans.frameContainerLoc << "\t|";
					if (!trans.motionInfo)
						std::cout << "Move";
					else if (trans.motionInfo == 1)
						std::cout << "Low";
					else if (trans.motionInfo == 2)
						std::cout << "High";
					else
						std::cout << trans.motionInfo;

					std::cout << "\t|";

					if (trans.transitionLog)
						std::cout << "Yes";
					else
						std::cout << "No";
					std::cout << "\t|" << *(int16_t*)setRef->fileBlock[trans.frameContainerLoc].blockdata;
					std::cout << " x " << *(int16_t*)(setRef->fileBlock[trans.frameContainerLoc].blockdata + 2) << '\n';
				}
			}
		}
	}


	return totalFrames;
}
*/

//void DFedit::SceneData::writeSceneScripts(const std::string& dir) {
//
//	std::string newDir(dir);
//	newDir.push_back('/');
//	newDir.append(sceneName);
//	CreateDirectoryA(newDir.c_str(), NULL);
//
//	instance->writeScript(newDir, locationScript, sceneName);
//}

/*
int32_t DFedit::SceneData::writeSceneFrames(const std::string& dir) {

			static const std::string rotDirs[2]{ "/D","/A" };	// just determines the folders writing to

			std::string newDir(dir);
			newDir.push_back('/');
			newDir.append(sceneName);
			CreateDirectoryA(newDir.c_str(), NULL);

			int32_t totalFrames{ 0 };
			for (uint8_t direction = 0; direction < 2; direction++) {
				// start with turn right frames
				for (uint32_t i = 0; i < turnCount[0]; i++) {

					std::string newDirAdd(newDir);
					newDirAdd.push_back('/');

					newDirAdd.append(rotDirs[direction]);
					CreateDirectoryA(newDirAdd.c_str(), NULL);

					instance->writeFrame(newDirAdd, turns[direction][i].frameContainerLoc, false);

					totalFrames++;
					//std::cout << "write frame container: " << turns[direction][i].frameContainerLoc << '\n';

				}

				// now write all transition, if it has transition frames
				for (uint32_t i = 0; i < turnCount[direction]; i++) {
					if (turns[direction][i].transitionLog) {

						int32_t turnCountTr = *(int32_t*)(instance->fileBlock[turns[direction][i].transitionLog].blockdata + 4);

						std::string newDirAdd(newDir);
						newDirAdd.append("/W");
						CreateDirectoryA(newDirAdd.c_str(), NULL);

						SceneTransition trans;
						char* curr = instance->fileBlock[turns[direction][i].transitionLog].blockdata + 12;
						for (uint32_t i = 0; i < turnCountTr; i++) {
							memcpy(&trans, curr + (sizeof(SceneTransition) * i), sizeof(SceneTransition));

							totalFrames++;
							instance->writeFrame(newDirAdd, trans.frameContainerLoc, false);
							//std::cout << "write frame container: " << trans.frameContainerLoc << '\n';
						}
					}
				}
			}


			return totalFrames;
}

*/

void DFset::SceneData::readViewTable() {

	swapEndians(sceneLocation[0], setRef->containers[locationViews].data);
	swapEndians(sceneLocation[1], setRef->containers[locationViews].data+8);
	swapEndians(sceneLocation[2], setRef->containers[locationViews].data+16);

	memcpy(unknownVal, setRef->containers[locationViews].data + (3 * sizeof(double)), 2 * sizeof(int32_t));
	// next line would be scene name once again, which we already have
	SceneViewCount = *(int32_t*)(setRef->containers[locationViews].data + 48);
	uint8_t* curr = setRef->containers[locationViews].data + 52;

	sceneViews.reserve(SceneViewCount);
	for (uint32_t i = 0; i < SceneViewCount; i++) {
		sceneViews.emplace_back();
		curr += swapEndians(sceneViews[i].rotation, curr);
		sceneViews[i].rotation8 = *(int16_t*)(curr);
		curr += 2;
		sceneViews[i].viewPairType = *(int32_t*)(curr);
		curr += 4;
		curr += swapEndians(sceneViews[i].unknownDB2, curr);
		sceneViews[i].viewID = *(int32_t*)(curr);
		curr += 4;
		sceneViews[i].locationObjects = *(int32_t*)(curr);
		curr += 4;
		// read objects if given
		if (sceneViews[i].locationObjects) {
			sceneViews[i].readObjects(setRef->containers[sceneViews[i].locationObjects].data, setRef);
		}
		sceneViews[i].viewName.assign((char*)curr, *curr++);
		curr += 15;
	}
}
