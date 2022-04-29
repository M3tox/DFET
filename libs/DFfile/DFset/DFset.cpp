#include "DFset.h"

DFset::~DFset() {
	// clean up everything if containers were deleted already
	if (!containers.size()) {
		for (int32_t scene = 0; scene < scenes.size(); scene++) {
			for (int32_t view = 0; view < scenes[scene].SceneViewCount; view++) {
				if (scenes[scene].sceneViews[view].objTable) {
					for (int32_t obj = 0; obj < scenes[scene].sceneViews[view].objTable->objectCount; obj++) {
						scenes[scene].sceneViews[view].objTable->objectEntries[obj].containerObjScript.clearContent();
					}
					delete scenes[scene].sceneViews[view].objTable;
				}
				scenes[scene].containerSceneScript.clearContent();
			}
			for (uint8_t dir = 0; dir < 2; dir++)
				for (int32_t frame = 0; frame < scenes[scene].rotationRegister[dir].frameCount; frame++)
					scenes[scene].rotationRegister[dir].frameInfos[frame].containerFrame.clearContent();
		}


		for (int32_t road = 0; road < transitionTable.transitionCount; road++)
			for (uint8_t dir = 0; dir < 2; dir++)
				for (int32_t frame = 0; frame < transitionTable.transitions[road].frameRegister[dir].frameCount; frame++)
					transitionTable.transitions[road].frameRegister[dir].frameInfos[frame].containerFrame.clearContent();

		setHeader.containerMainScript.clearContent();
		setHeader.containermapLight.clearContent();
		setHeader.containermapDark.clearContent();
	}
}

int32_t DFset::initContainerData() {

	// start with data from first container, head information
	setHeader.version = *(int32_t*)(containers[0].data + 0x2);
	// dont even continue if version does not match with 4.0
	if (setHeader.version != 4) {
		return ERRDFVERSION;
	}

	uint8_t* curr = containers[0].data + 0x18;
	setHeader.mapLight = *(int32_t*)curr;
	curr += 4;
	setHeader.mapDark = *(int32_t*)curr;
	curr += 8;
	setHeader.mapHeight = *(int16_t*)curr;
	curr += 2;
	setHeader.mapWidth = *(int16_t*)curr;
	curr += 6;
	setHeader.setDimensionsY = *(int16_t*)curr;
	curr += 2;
	setHeader.setDimensionsX = *(int16_t*)curr;
	curr += 18;
	curr += swapEndians(setHeader.setDimensionsYf, curr);
	curr += swapEndians(setHeader.setDimensionsXf, curr);

	// copy various register locations
	memcpy(&setHeader.sceneRegister, curr, 32);
	curr += 32;
	uint8_t strSize = *curr++;
	setHeader.setName.assign((char*)curr, strSize);

	curr = containers[0].data + 0x84;
	setHeader.viewPortWidth = *(int16_t*)curr;
	curr += 2;
	setHeader.viewPortHeight = *(int16_t*)curr;
	curr += 2;
	// coopy coordinate/rotational data
	curr += swapEndians(setHeader.coords[0], curr);
	curr += swapEndians(setHeader.coords[1], curr);
	curr += swapEndians(setHeader.coords[2], curr);
	curr += swapEndians(setHeader.rotations[0], curr);
	curr += swapEndians(setHeader.rotations[1], curr);

	curr += swapEndians(setHeader.unknownF0xB0, curr);
	curr += swapEndians(setHeader.unknownF0xB4, curr);
	curr += swapEndians(setHeader.unknownF0xB8, curr);

	// get yet unknown dataset that has 3 entries
	for (uint8_t i = 0; i < 3; i++) {
		setHeader.uEntries[i].count = *(int16_t*)curr;
		curr += 2;
		setHeader.uEntries[i].limit = *(int32_t*)curr;
		curr += 4;
		setHeader.uEntries[i].unknownInt[0] = *(int32_t*)curr;
		curr += 4;
		setHeader.uEntries[i].unknownInt[1] = *(int32_t*)curr;
		curr += 4;
		setHeader.uEntries[i].rotation8[0] = *(int16_t*)curr;
		curr += 2;
		setHeader.uEntries[i].rotation8[1] = *(int16_t*)curr;
		curr += 2;
	}

	// the locations and most important data is fed. Now get the color palette
	//memcpy(colors, curr, sizeof(ColorPalette) * 256);
	memcpy(colors, curr, 256* sizeof(ColorPalette));
	colorCount = 128;	// careful, set files only utilize the first 128 colors!
	curr += sizeof(ColorPalette) * 256;
	strSize = *curr++;
	setHeader.secondaryRefName.assign((char*)curr, strSize);
	curr += 255;

	curr += swapEndians(setHeader.unknownD0x9F2, curr);
	memcpy(&setHeader.unknownS0x9FA, curr, 6);
	curr += 6;
	memcpy(&setHeader.unknownI0xA00, curr, 8);
	curr += 8;

	setHeader.setDimensionsY_2 = *(int16_t*)curr;
	curr += 2;
	setHeader.heightDifference = *(int32_t*)curr;
	curr += 4;
	strSize = *curr++;
	setHeader.defaultSceneName.assign((char*)curr, strSize);
	curr += 15;
	strSize = *curr++;
	setHeader.defaultViewName.assign((char*)curr, strSize);

	// copy ressource references
	setHeader.containerMainScript = containers[setHeader.mainScript];
	setHeader.containermapLight = containers[setHeader.mapLight];
	setHeader.containermapDark = containers[setHeader.mapDark];
	// HEADER INFORMATION READ!


	actors.readActorsContainer(containers[setHeader.actorRegister].data);

	// read transition data
	transitionTable.readTransitions(this);
	// read scene rotation table
	//sceneRotationTable.readRotations(containers[setHeader.sceneRegister].data);
	// now read the big tables
	scenes.reserve(setHeader.sceneCount);
	for (int32_t scene = 0; scene < setHeader.sceneCount; scene++) {
		scenes.emplace_back(scene, this);
	}

	return 0;
}

std::string DFset::getHeaderInfo() {
	std::string out("SET HEADER INFORMATION");

	out.append("\nVersion:                       ").append(std::to_string(setHeader.version));
	out.append("\nlocation map light:            ").append(std::to_string(setHeader.mapLight));
	out.append("\nlocation map dark:             ").append(std::to_string(setHeader.mapDark));
	out.append("\nMap pixel width:               ").append(std::to_string(setHeader.mapWidth));
	out.append("\nMap pixel height:              ").append(std::to_string(setHeader.mapHeight));
	out.append("\nset dimensions X:              ").append(std::to_string(setHeader.setDimensionsY));
	out.append("\nset dimensions Z:              ").append(std::to_string(setHeader.setDimensionsX));
	out.append("\nset dimensions decimel X:      ").append(std::to_string(setHeader.setDimensionsYf));
	out.append("\nset dimensions decimel Z:      ").append(std::to_string(setHeader.setDimensionsXf));
	// locations
	out.append("\nlocation scene register:       ").append(std::to_string(setHeader.sceneRegister));
	out.append("\nlocation transition register:  ").append(std::to_string(setHeader.transitionRegister));
	out.append("\nlocation transition register:  ").append(std::to_string(setHeader.actorRegister));
	out.append("\nlocation main script:          ").append(std::to_string(setHeader.mainScript));
	out.append("\nunknown at 0x68:               ").append(std::to_string(setHeader.unknown0x68));
	out.append("\nunknown at 0x6C:               ").append(std::to_string(setHeader.unknown0x6C));
	// more infos
	out.append("\ninner Set name:                ").append(setHeader.setName);
	out.append("\nview port width:               ").append(std::to_string(setHeader.viewPortWidth));
	out.append("\nview port height:              ").append(std::to_string(setHeader.viewPortHeight));
	out.append("\ncoordinates X:                 ").append(std::to_string(setHeader.coords[0]));
	out.append("\ncoordinates Z:                 ").append(std::to_string(setHeader.coords[1]));
	out.append("\ncoordinates Y:                 ").append(std::to_string(setHeader.coords[2]));
	out.append("\nrotation X:                    ").append(std::to_string(setHeader.rotations[0]));
	out.append("\nrotation Y:                    ").append(std::to_string(setHeader.rotations[1]));
	out.append("\nunknown 0xB0:                  ").append(std::to_string(setHeader.unknownF0xB0));
	out.append("\nunknown 0xB4:                  ").append(std::to_string(setHeader.unknownF0xB4));
	out.append("\nunknown 0B8:                   ").append(std::to_string(setHeader.unknownF0xB8));
	out.append("\n\n3 unknown entries:");
	for (uint8_t i = 0; i < 3; i++) {
		// collect entry data...
		out.append("\n\n\tEntry ");
		out.append(std::to_string(i + 1));
		out.append("\n\tcount?:           ").append(std::to_string(setHeader.uEntries[i].count));
		out.append("\n\tcolor limit?:     ").append(std::to_string(setHeader.uEntries[i].limit));
		out.append("\n\tunknownInt1:      ").append(std::to_string(setHeader.uEntries[i].unknownInt[0]));
		out.append("\n\tunknownInt2:      ").append(std::to_string(setHeader.uEntries[i].unknownInt[1]));
		out.append("\n\tunknownShort1:    ").append(std::to_string(setHeader.uEntries[i].rotation8[0]));
		out.append("\n\tunknownShort2:    ").append(std::to_string(setHeader.uEntries[i].rotation8[1]));
	}

	out.append("\n\nColor palette:");
	out.append("\n     -----------+-------+-------+-------");
	out.append("\n        index   | red   | green | yellow");
	out.append("\n     -----------+-------+-------+-------");
	for (uint16_t color = 0; color < 256; color++) {
		// collect entry data...
		out.append("\n\t");
		out.append(std::to_string(color));
		out.append("\t| ").append(std::to_string(*(((uint8_t*)&colors[color].RGB[0])+1)));
		out.append("\t| ").append(std::to_string(*(((uint8_t*)&colors[color].RGB[1]) + 1)));
		out.append("\t| ").append(std::to_string(*(((uint8_t*)&colors[color].RGB[2]) + 1)));
	}

	out.append("\n\nsecondary ref name:            ").append(setHeader.secondaryRefName);
	out.append("\nunknownD0x9F2:                 ").append(std::to_string(setHeader.unknownD0x9F2));
	out.append("\nunknown shorts at 0x9FA        ").append(std::to_string(setHeader.unknownS0x9FA[0]));
	out.append(", ").append(std::to_string(setHeader.unknownS0x9FA[1]));
	out.append(", ").append(std::to_string(setHeader.unknownS0x9FA[2]));
	out.append("\nunknown Integers at 0xA00:     ").append(std::to_string(setHeader.unknownI0xA00[0]));
	out.append(", ").append(std::to_string(setHeader.unknownI0xA00[1]));

	out.append("\nMap width (again?):            ").append(std::to_string(setHeader.setDimensionsY_2));
	out.append("\ndefault height?:               ").append(std::to_string(setHeader.heightDifference));

	out.append("\ndefault scene name:            ").append(setHeader.defaultSceneName);
	out.append("\ndefault view name:             ").append(setHeader.defaultViewName);

	out.append("\n\n");

	return out;
}

void DFset::updateContainerInfo() {
	// NOW update NOTES TO CONTAINER DATA 
	containers[0].info.assign("general set data and references");

	containers[setHeader.mapLight].info.assign("Map light");
	containers[setHeader.mapLight].type = Container::MAP;

	containers[setHeader.mapDark].info.assign("Map dark");
	containers[setHeader.mapDark].type = Container::MAP;

	containers[setHeader.sceneRegister].info.assign("scene register");
	containers[setHeader.transitionRegister].info.assign("transition register");
	containers[setHeader.actorRegister].info.assign("actors register");
	containers[setHeader.mainScript].info.assign("Main Script");
	containers[setHeader.mainScript].type = Container::SCRIPT;
	containers[setHeader.mainSceneRegister].info.assign("FULL scene register with names");


	for (int32_t scene = 0; scene < setHeader.sceneCount; scene++) {

		// update container info
		containers[scenes[scene].locationScript].info = scenes[scene].sceneName;
		containers[scenes[scene].locationScript].info.append(" script");
		containers[scenes[scene].locationScript].type = Container::SCRIPT;

		containers[scenes[scene].locationViews].info = scenes[scene].sceneName;
		containers[scenes[scene].locationViews].info.append(" view table");

		containers[scenes[scene].locationViewLogic[RIGHTTURNS]].info = scenes[scene].sceneName;
		containers[scenes[scene].locationViewLogic[RIGHTTURNS]].info.append(" turn table RIGHT");

		containers[scenes[scene].locationViewLogic[LEFTTURNS]].info = scenes[scene].sceneName;
		containers[scenes[scene].locationViewLogic[LEFTTURNS]].info.append(" turn table LEFT");

		// now get the frame location from its turn tables
		// RIGHT
		for (int32_t frame = 0; frame < scenes[scene].rotationRegister[RIGHTTURNS].frameCount; frame++) {
			if (scenes[scene].rotationRegister[RIGHTTURNS].frameInfos[frame].frameContainerLoc) {
				containers[scenes[scene].rotationRegister[RIGHTTURNS].frameInfos[frame].frameContainerLoc].info = scenes[scene].sceneName;
				containers[scenes[scene].rotationRegister[RIGHTTURNS].frameInfos[frame].frameContainerLoc].info.append(" rotation frame RIGHT");
				containers[scenes[scene].rotationRegister[RIGHTTURNS].frameInfos[frame].frameContainerLoc].type = Container::IMAGE;
			}
		}

		// LEFT
		for (int32_t frame = 0; frame < scenes[scene].rotationRegister[LEFTTURNS].frameCount; frame++) {
			if (scenes[scene].rotationRegister[LEFTTURNS].frameInfos[frame].frameContainerLoc) {
				containers[scenes[scene].rotationRegister[LEFTTURNS].frameInfos[frame].frameContainerLoc].info = scenes[scene].sceneName;
				containers[scenes[scene].rotationRegister[LEFTTURNS].frameInfos[frame].frameContainerLoc].info.append(" rotation frame LEFT");
				containers[scenes[scene].rotationRegister[LEFTTURNS].frameInfos[frame].frameContainerLoc].type = Container::IMAGE;
			}
		}

		// FIND OBJECT REFERENCES!
		for (int32_t view = 0; view < scenes[scene].SceneViewCount; view++) {
			if (scenes[scene].sceneViews[view].locationObjects) {

				int32_t objectRef = scenes[scene].sceneViews[view].locationObjects;
				containers[objectRef].info = "Objects in ";
				containers[objectRef].info.append(scenes[scene].sceneViews[view].viewName);
				containers[objectRef].info.append(", ");
				containers[objectRef].info.append(scenes[scene].sceneName);

				// go though object reference and get their script location
				for (int32_t obj = 0; obj < scenes[scene].sceneViews[view].objTable->objectCount; obj++) {

					int32_t scriptRef = scenes[scene].sceneViews[view].objTable->objectEntries[obj].locationScript;
					containers[scriptRef].info = "Script for objects in ";
					containers[scriptRef].info.append(scenes[scene].sceneViews[view].viewName);
					containers[scriptRef].info.append(", ");
					containers[scriptRef].info.append(scenes[scene].sceneName);
					containers[scriptRef].type = Container::SCRIPT;
				}
			}
		}
	}


	// TRANSITION INFO TABLES
	for (int32_t trans = 0; trans < transitionTable.transitionCount; trans++) {
		containers[transitionTable.transitions[trans].locationTransitionInfo].info = "transition info, connecting transition tables ";
		containers[transitionTable.transitions[trans].locationTransitionInfo].info.append(std::to_string(transitionTable.transitions[trans].locationSceneB));
		containers[transitionTable.transitions[trans].locationTransitionInfo].info.append(" and ");
		containers[transitionTable.transitions[trans].locationTransitionInfo].info.append(std::to_string(transitionTable.transitions[trans].locationSceneA));

		containers[transitionTable.transitions[trans].locationSceneA].info = "transition info Scene A";
		containers[transitionTable.transitions[trans].locationSceneB].info = "transition info Scene B";

		// CHECK TRANSITION FRAMES
		for (uint8_t direction = 0; direction < 2; direction++) {
			for (uint32_t frame = 0; frame < transitionTable.transitions[trans].frameRegister[direction].frameCount; frame++) {
				if (transitionTable.transitions[trans].frameRegister[direction].frameInfos[frame].frameContainerLoc) {
					containers[transitionTable.transitions[trans].frameRegister[direction].frameInfos[frame].frameContainerLoc].info = "transition frame of ";
					containers[transitionTable.transitions[trans].frameRegister[direction].frameInfos[frame].frameContainerLoc].info.append(transitionTable.transitions[trans].transitionName);
					containers[transitionTable.transitions[trans].frameRegister[direction].frameInfos[frame].frameContainerLoc].type = Container::IMAGE;
				}
			}
		}
	}
}

void DFset::writeAllScripts() {

	namespace fs = std::filesystem;

	if (writeScript(setHeader.containerMainScript, outPutPath + "/main Script"))
		fileCounter++;

	for (int32_t scene = 0; scene < scenes.size(); scene++) {

		std::string scenePath(outPutPath);
		scenePath.push_back('/');
		scenePath.append(scenes[scene].sceneName);
		fs::create_directory(scenePath);

		if (writeScript(scenes[scene].containerSceneScript, scenePath + "/scene Script"))
			fileCounter++;

		// look for object scripts
		for (int32_t view = 0; view < scenes[scene].SceneViewCount; view++) {
			if (scenes[scene].sceneViews[view].objTable) {
				for (int32_t obj = 0; obj < scenes[scene].sceneViews[view].objTable->objectCount; obj++) {
					std::string vieScriptName(scenePath);
					vieScriptName.push_back('/');
					vieScriptName.append(scenes[scene].sceneViews[view].viewName);
					vieScriptName.append(" Script ");
					vieScriptName.append(std::to_string(obj + 1));
					if (writeScript(scenes[scene].sceneViews[view].objTable->objectEntries[obj].containerObjScript, vieScriptName))
						fileCounter++;
				}

			}
		}

	}

}
void DFset::writeAllFrames() {
	namespace fs = std::filesystem;

	for (int32_t scene = 0; scene < scenes.size(); scene++) {
		std::string scenePath(outPutPath);
		scenePath.push_back('/');
		scenePath.append(scenes[scene].sceneName);
		fs::create_directory(scenePath);

		static const char* dirStr[2]{ "/right","/left" };
		// process both directions
		for (uint8_t dir = 0; dir < 2; dir++) {
			std::string framePath(scenePath);
			framePath.append(dirStr[dir]);
			fs::create_directory(framePath);
			fs::create_directory(framePath + "/Z");

			for (int32_t frame = 0; frame < scenes[scene].rotationRegister[dir].frameCount; frame++) {
				writeBMPimage(scenes[scene].rotationRegister[dir].frameInfos[frame].containerFrame, framePath);
				fileCounter++;
			}
		}
	}

	for (int32_t road = 0; road < transitionTable.transitionCount; road++) {
		std::string roadPath(outPutPath);
		roadPath.push_back('/');
		roadPath.append(transitionTable.transitions[road].transitionName);
		fs::create_directory(roadPath);

		int32_t scenesIndexes[2];
		getScenesRoad(road, scenesIndexes[1], scenesIndexes[0]);

		for (uint8_t dir = 0; dir < 2; dir++) {
			std::string framePath(roadPath);
			framePath.push_back('/');
			framePath.append("to ");
			framePath.append(scenes[scenesIndexes[dir]].sceneName);
			fs::create_directory(framePath);
			fs::create_directory(framePath + "/Z");

			for (int32_t frame = 0; frame < transitionTable.transitions[road].frameRegister[dir].frameCount; frame++) {
				writeBMPimage(transitionTable.transitions[road].frameRegister[dir].frameInfos[frame].containerFrame, framePath);
				fileCounter++;
			}
		}
	}

	// double it because of the z frames...
	fileCounter <<= 1;

	// at last: add the maps, all colors utilized
	colorCount <<= 1;
	writeBMPimage(setHeader.containermapLight, outPutPath, "Map 1");
	fileCounter++;
	writeBMPimage(setHeader.containermapDark, outPutPath, "Map 2");
	fileCounter++;
	colorCount >>= 1;
}