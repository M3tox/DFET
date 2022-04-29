#pragma once

#include "../DFfile.h"

#define PI 3.14159265358979323846

class DFset : public DFfile
{
	using DFfile::DFfile;
	
public:

	~DFset();

	enum {
		RIGHTTURNS,
		LEFTTURNS
	};

	std::string getHeaderInfo();
	
	// SPECIFIC SET FUNCTIONS
	static DFset* getDFset(DFfile* ptr) {
		if (ptr->fileType != DFSET)
			return nullptr;
		return (DFset*)ptr;
	}

	int32_t getSceneCount() {
		return scenes.size();
	}

	void updateContainerInfo();

	void writeAllScripts();
	void writeAllFrames();

	void getScenesRoad(int32_t road, int32_t& sceneFrom, int32_t& sceneto) {
		sceneFrom = -1;
		sceneto = -1;
		for (int32_t scene = 0; scene < scenes.size(); scene++) {
			if (sceneFrom != -1 && sceneto != -1) break;
			for (int32_t view = 0; view < scenes[scene].SceneViewCount; view++) {
				if (scenes[scene].sceneViews[view].viewID == transitionTable.transitions[road].viewIDstart) {
					sceneFrom = scene;
					break;
				}
				if (scenes[scene].sceneViews[view].viewID == transitionTable.transitions[road].viewIDend) {
					sceneto = scene;
					break;
				}
			}
		}
	}

	void clearSETcontainers() {
		for (auto& cont : containers) {
			if (cont.type == Container::DEFAULT)
				cont.clearContent();
		}
		containers.clear();
	}

protected:
	
	struct SetHeader {
		struct unknownDataEntries{
			// I dont know what it is, but its there 3 times in the same structure
			int16_t count;
			int32_t limit;
			int32_t unknownInt[2];
			int16_t rotation8[2];
		};
		/*
		~SetHeader() {
			containerMainScript.clearContent();
			containermapLight.clearContent();
			containermapDark.clearContent();
		}
		*/
		int32_t version;

		int32_t mapLight;
		int32_t mapDark;

		int16_t mapWidth;
		int16_t mapHeight;

		int16_t setDimensionsY;
		int16_t setDimensionsX;

		double setDimensionsYf;
		double setDimensionsXf;

		int32_t sceneRegister;
		int32_t transitionRegister;
		int32_t actorRegister;
		int32_t mainScript;
		int32_t mainSceneRegister;
		int32_t sceneCount;
		int32_t unknown0x68;
		int32_t unknown0x6C;
		std::string setName;

		int16_t viewPortWidth;
		int16_t viewPortHeight;
		double coords[3];
		double rotations[2];

		float unknownF0xB0;
		float unknownF0xB4;
		float unknownF0xB8;

		unknownDataEntries uEntries[3];


		std::string secondaryRefName;

		double unknownD0x9F2;
		int16_t unknownS0x9FA[3];
		int32_t unknownI0xA00[2];
		int16_t setDimensionsY_2;
		int32_t heightDifference;
		std::string defaultSceneName;
		std::string defaultViewName;

		DFfile::Container containerMainScript;
		DFfile::Container containermapLight;
		DFfile::Container containermapDark;
	};

	struct TransitionContainer {
		enum {
			SCENE_A,
			SCENE_B
		};

		struct FrameInformation {
			//~FrameInformation() {
			//	containerFrame.clearContent();
			//}
			// its a clear pattern of 4x 8byte values but I dont know what they do
			// might be 4 doubles, indicating something. Location? rotation?
			void readFrameInformation(uint8_t* dataEntry);
			void writeFrameInformation(uint8_t* dataOut);

			std::string getFrameInfo();

			double posX;
			double posZ;
			double posY;
			double axisX;
			int16_t posX16;
			int16_t posZ16;
			int16_t posY16;
			int16_t axisX8;

			int32_t motionInfo;
			// motionInfo variable can be 0, 1 or 2.
			// 0 = frame is in motion. Player cant move from here and its low res
			// 1 = frame is also low res, but player can turn or go forward
			// 2 = HiRes frame. Player can move and the image has a higher and clearer quality

			int32_t frameContainerLoc;	// reference to the container that has frame
			int32_t framePairID;			// unique number, sometimes equal to container location
			int32_t transitionLog;		// mostly zero
			int32_t viewID;				// Reference to view table, -1 has no view name

			// frame reference copy
			DFfile::Container containerFrame;
		};

		void readTransitionContainer(uint8_t* containerData, DFset* setRef) {
			unknownInt = *(int32_t*)containerData;
			containerData += 4;
			frameCount = *(int32_t*)containerData;
			containerData += 4;
			destination = *(int32_t*)containerData;
			containerData += 4;

			frameInfos.reserve(frameCount);
			for (int32_t frame = 0; frame < frameCount; frame++) {
				frameInfos.emplace_back();
				frameInfos[frame].readFrameInformation(containerData);
				// before you go: take a copy of the container
				frameInfos[frame].containerFrame = setRef->containers[frameInfos[frame].frameContainerLoc];
				containerData += 60;
			}
		}

		void writeTransitionContainer(uint8_t* containerData) {
			*(int32_t*)containerData = unknownInt;
			containerData += 4;
			*(int32_t*)containerData = frameCount;
			containerData += 4;
			*(int32_t*)containerData = destination;
			containerData += 4;

			for (int32_t frame = 0; frame < frameCount; frame++) {
				frameInfos[frame].writeFrameInformation(containerData);
				containerData += 60;
			}
		}

		void insertFrame(int32_t where, FrameInformation& frame) {
			if (where == frameInfos.size())
				frameInfos.push_back(frame);
			else
				frameInfos.insert(frameInfos.begin() + where, frame);
		}

		void insertFrame(int32_t where) {
			if (where == frameInfos.size())
				frameInfos.push_back(FrameInformation());
			else
				frameInfos.insert(frameInfos.begin() + where, FrameInformation());
		}

		int32_t unknownInt;	// usually zero
		int32_t frameCount; // amount of frame registers
		int32_t destination; // zero if there is no connection to another table

		std::vector<FrameInformation>frameInfos;
	};

	struct ActorsContainer {
		struct Actor {
			int32_t unknownInt;
			int16_t rotation8;
			int16_t positionX;
			int16_t positionZ;
			int16_t positionY;

			//int32_t unknownIntArr[3];
			//int16_t unknownShort;
			//uint8_t unknownByte;

			std::string identifier;
		};

		void readActorsContainer(uint8_t* container) {
			actorsCount = *(int32_t*)container;
			container += 4;
			unknownInt = *(int32_t*)container;
			container += 4;

			actors.reserve(actorsCount);
			for (int32_t act = 0; act < actorsCount; act++) {
				actors.emplace_back();
				actors.at(act).unknownInt = *(int32_t*)container;
				container += 4;
				actors.at(act).rotation8 = *(int16_t*)container;
				container += 2;
				actors.at(act).positionX = *(int16_t*)container;
				container += 2;
				actors.at(act).positionZ = *(int16_t*)container;
				container += 2;
				actors.at(act).positionY = *(int16_t*)container;
				container += 2;

				actors.at(act).identifier.assign((char*)container, *container++);
				container += 41;
				// I have no idea what these do... probably just old copied mem
				//memcpy(actors.at(act).unknownIntArr, container, sizeof(int32_t) * 3);
				//container += 3 * sizeof(int32_t);
				//actors.at(act).unknownShort = *(int16_t*)container;
				//container += 2;
				//actors.at(act).unknownByte = *container++;
			}
		}
		
		std::string getActorsInfo() {
			std::string out("ACTOR TABLE:\n");
			out.append("\nAmount of actors:    ").append(std::to_string(actorsCount));
			out.append("\nunknown Integer:     ").append(std::to_string(unknownInt));

			for (int32_t act = 0; act < actorsCount; act++) {
				out.append("\nACTOR ").append(std::to_string(act+1));
				out.append("\nunknown Integer:     ").append(std::to_string(actors.at(act).unknownInt));
				out.append("\nunknown Short:       ").append(std::to_string(actors.at(act).rotation8));
				out.append("\nPosition on axis X:  ").append(std::to_string(actors.at(act).positionX));
				out.append("\nPosition on axis Z:  ").append(std::to_string(actors.at(act).positionZ));
				out.append("\nPosition on axis Y:  ").append(std::to_string(actors.at(act).positionY));
				out.append("\nIdentifier:          ").append(actors.at(act).identifier);
				out.push_back('\n');
			}
			return out;
		}

		int32_t actorsCount;
		int32_t unknownInt;

		std::vector<Actor>actors;
	};

	
	class SceneData {

	public:

		SceneData(int32_t sceneID, DFset* inst);
		// empty constructor for manual adding operations
		SceneData() { };	

		~SceneData();

		std::string getTurnData();
		std::string getTurnData(bool turnDirection);

		std::string getSceneInfo();
		std::string getViewData();
		std::string getRightTurnLogic();
		std::string getLeftTurnLogic();

		

		//int32_t printFrameInformation();

		//void writeSceneScripts(const std::string& dir);
		//int32_t writeSceneFrames(const std::string& dir);


	private:

		void readViewTable();

		DFset* setRef;

		// PROCESS VIEWS!
		struct SceneViews {
			struct objectTable {
				struct ObjectEntries {
					//~ObjectEntries() {
					//	containerObjScript.clearContent();
					//}
					int32_t unknownInt;
					int16_t rotation8;
					int16_t unknownShort2;
					int16_t startRegionX;
					int16_t startRegionY;
					int16_t endRegionX;
					int16_t endRegionY;
					int32_t locationScript;

					std::string identifier;

					DFfile::Container containerObjScript;
				};

				int32_t objectCount;
				int32_t unknownInt;

				std::vector<ObjectEntries> objectEntries;
			};

			//~SceneViews() {
			//	delete objTable;
			//}

			void addNewObject(const std::string& name) {
				if (!objTable) {
					locationObjects = 1;	// not actual location, but 0 would mean missing
					objTable = new objectTable;
					objTable->objectCount = 0;
					objTable->unknownInt = 0;
				}

				objTable->objectCount++;
				objTable->objectEntries.reserve(objTable->objectCount);
				objTable->objectEntries.emplace_back();
				objTable->objectEntries.back().unknownInt = 0;
				objTable->objectEntries.back().rotation8 = 0;
				objTable->objectEntries.back().unknownShort2 = 0;
				objTable->objectEntries.back().startRegionX = 0;
				objTable->objectEntries.back().startRegionY = 0;
				objTable->objectEntries.back().endRegionX = 50;
				objTable->objectEntries.back().endRegionY = 50;
				objTable->objectEntries.back().locationScript = 0;	// actual location will be determined when saving
				objTable->objectEntries.back().identifier.assign(name);

				objTable->objectEntries.back().containerObjScript.data = new uint8_t[8]{ 0 };
				objTable->objectEntries.back().containerObjScript.size = 8;

			}

			void removeObject(int32_t obj) {
				// if only one object is given, this one is assumed and will be removed
				if (objTable->objectCount == 1) {
					objTable->objectEntries.back().containerObjScript.clearContent();
					objTable->objectEntries.clear();
					delete objTable;
					objTable = nullptr;

					locationObjects = 0;
					return;
				}

				objTable->objectEntries.at(obj).containerObjScript.clearContent();
				objTable->objectEntries.erase(objTable->objectEntries.begin() + obj);
				objTable->objectCount--;
			}

			void readObjects(uint8_t* container, DFset* setRef) {
				delete objTable; // delete old one in case new data will be feed
				objTable = new objectTable;

				objTable->objectCount = *(int32_t*)container;
				container += 4;
				objTable->unknownInt = *(int32_t*)container;
				container += 4;
				objTable->objectEntries.reserve(objTable->objectCount);
				for (int32_t obj = 0; obj < objTable->objectCount; obj++) {
					objTable->objectEntries.emplace_back();
					objTable->objectEntries.at(obj).unknownInt = *(int32_t*)container;
					container += 4;
					objTable->objectEntries.at(obj).rotation8 = *(int16_t*)container;
					container += 2;
					objTable->objectEntries.at(obj).unknownShort2 = *(int16_t*)container;
					container += 2;
					objTable->objectEntries.at(obj).startRegionX = *(int16_t*)container;
					container += 2;
					objTable->objectEntries.at(obj).startRegionY = *(int16_t*)container;
					container += 2;
					objTable->objectEntries.at(obj).endRegionX = *(int16_t*)container;
					container += 2;
					objTable->objectEntries.at(obj).endRegionY = *(int16_t*)container;
					container += 2;
					objTable->objectEntries.at(obj).locationScript = *(int32_t*)container;
					container += 4;
					objTable->objectEntries.at(obj).identifier.assign((char*)container, *container++);
					container += 15;

					objTable->objectEntries[obj].containerObjScript = setRef->containers[objTable->objectEntries.at(obj).locationScript];
				}
			}

			std::string getObjTableInfo() {
				if (!objTable)
					return "No object table given!";

				std::string out("OBJECT TABLE OF ");
				out.append(viewName);

				out.append("\nunknown Integer:   ").append(std::to_string(objTable->unknownInt));
				out.append("\nAmount of Objects: ").append(std::to_string(objTable->objectCount));
				out.push_back('\n');
				for (int32_t obj = 0; obj < objTable->objectCount; obj++) {
					out.append("\nOBJECT ").append(std::to_string(obj+1)).push_back(':');
					out.append("\nunknown Integer:    ").append(std::to_string(objTable->objectEntries.at(obj).unknownInt));
					out.append("\nunknown Short1:     ").append(std::to_string(objTable->objectEntries.at(obj).rotation8));
					out.append("\nunknown Short2:     ").append(std::to_string(objTable->objectEntries.at(obj).unknownShort2));
					out.append("\nstart region on X:  ").append(std::to_string(objTable->objectEntries.at(obj).startRegionX));
					out.append("\nstart region on Y:  ").append(std::to_string(objTable->objectEntries.at(obj).startRegionY));
					out.append("\nend region on X:    ").append(std::to_string(objTable->objectEntries.at(obj).endRegionX));
					out.append("\nend region on Y:    ").append(std::to_string(objTable->objectEntries.at(obj).endRegionY));
					out.append("\nlocation of script: ").append(std::to_string(objTable->objectEntries.at(obj).locationScript));
					out.append("\nidentifier:         ").append(objTable->objectEntries.at(obj).identifier);
					out.push_back('\n');
				}

				return out;
			}

			double rotation;
			int16_t rotation8;
			int32_t viewPairType;
			double unknownDB2;
			int32_t viewID;
			int32_t locationObjects;	// IF this is zero, there is no object table for this view !!
			std::string viewName;
			// 26 max!

			objectTable* objTable = nullptr;
		};

	public:
		int32_t SceneViewCount;
		std::vector<SceneViews>sceneViews;

		int32_t unknownDWORD1;
		// other representation of coordinates as integer?
		int16_t XaxisMap;
		int16_t YaxisMap;
		int16_t ZaxisMap;

		int32_t locationViews;
		int32_t locationViewLogic[2];
		int32_t locationScript;

		double sceneLocation[3];
		int32_t unknownVal[2];

		std::string sceneName;

		TransitionContainer rotationRegister[2];

		DFfile::Container containerSceneScript;
	};

	struct SceneTransitionTable {
		struct TransitionInfoEntries {
			double xAxis;
			double zAxis;
			double yAxis;
		};

		struct SceneTransitionEntries {

			int32_t locationTransitionInfo;
			int32_t locationSceneA;
			int32_t locationSceneB;
			int16_t unknownShort1;		 // seems always 0
			int16_t unknownShort2;		 // seems always 2

			// FOLLOWING DATA IS FROM ROAD INFO CONTAINER
			int32_t unknownInt;		// seems always 1
			int16_t rotation8;		// seems always -1
			int32_t viewIDstart;
			int32_t viewIDend;

			double xAxisStart;
			double zAxisStart;
			double yAxisStart;

			double xAxisEnd;
			double zAxisEnd;
			double yAxisEnd;

			std::string transitionName;

			int32_t entriesCount;
			std::vector<TransitionInfoEntries> entries;

			// INFORMATION FROM TRANSITION TABLES!
			TransitionContainer frameRegister[2];

			std::string getAllTransitions();
		};


		void readTransitions(DFset* ref);

		std::string getTransitionData();
		std::string getTransitionData(int32_t rot);

		//~SceneTransitionTable() {
		//	delete[] transitions;
		//}

		int32_t unknownInt;			//always zero
		int32_t transitionCount;

		std::vector<SceneTransitionEntries>transitions;
	};
	
	
	int32_t initContainerData();
	
public:

	SetHeader setHeader;
	ActorsContainer actors;
	std::vector<SceneData>scenes;
	//SceneRotationTable sceneRotationTable;
	SceneTransitionTable transitionTable;

	SceneData getSceneData(int32_t scene) {
		return scenes.at(scene);
	}
};

