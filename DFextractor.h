/*
COPYRIGHT MeToX
GPL-3.0 License 

This, together with its .cpp file is the heart, the core of the extraction tool.
Everything important is wrapped up in this single DFextractor class.
It contains all the code necessary to extract all supported file types.
It splits the containers, it decodes audio, it decompresses frames.

If you want to you use this class, use it with care.
Its public functions allow easy operations, while the most complex operations are abstracted away.
There might be functions that are not used anymore, which are probably leftovers from the research/testing time.
*/

#pragma once

#define MDEFTVERSION "0.71"	// make sure its always 4 characters!


#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <Windows.h>

#include "TIcmds.h"

class DFextractor
{
public:

	DFextractor(const std::string&);
	~DFextractor();

	void analyzeContainer() {
		lastError = containerType->analyze();
	}

	void writeAllScripts() {
		lastError = containerType->writeScripts();
	}

	void writeAllAudio() {
		lastError = containerType->writeAudio();
	}

	void writeAllFrames() {
		lastError = containerType->writeFrames();
	}

	void printCurrentOutputPath() {
		std::cout << containerType->pathToOutput << '\n';
	}

	void changeCurrentOutputPath(const std::string& newPath) {
		containerType->pathToOutput.assign(newPath);
	}


	void printFileHeaderInfo();
	void printAllDialougTexts();
	static void printLookUpTable();
	void printScript(uint16_t blockID);
	void printAudioInfo();
	void printColorPalette();

	void printMovFrameInfo();
	void printSetInfo();
	void printPupInfo();
	void printStageInfo();

	void initAudio();
	void initSET();
	void initMOV();
	void initSTG();

	void writeScript(const std::string& filePath, uint16_t blockID, const std::string& fileName = "");
	void writeScriptsSET(const std::string& dir);
	void writeScriptsPUP(const std::string& dir);
	void writeScriptsSTG(const std::string& dir);
	void writeScriptsBOOT(const std::string& dir);
	bool writeFrame(const std::string&, int32_t, bool brightness = false, const std::string& FileName = "");

	void writeBlockFiles(const std::string&);
	void writeAllDialougTexts(const std::string& path);
	bool writeAllAudio(const std::string&);
	bool writeAllAudioPUP(const std::string& path);

	bool writeAllFramesSET(const std::string&);
	bool writeAllFramesMOV(const std::string&);
	bool writeAllFramesSTG(const std::string& dir);
	
	bool readPuppetStrings();

	static const char* GetDFerrorString(int32_t& errorCode);

	// TEMP!!
	void writeSpecificAudio(int16_t blockID);
	void writeAudioRange(int16_t from, int16_t to);
  // TEMP END

	int32_t lastError{ 0 };
	uint8_t fileType{ 0 };
	uint32_t fileCounter{ 0 };

private:

	enum errorCodes {
		ERRNONE,
		ERRFILEFORMAT,
		ERRNOSCRIPT,
		ERRNOAUDIO,
		ERRNOFRAMES,
		ERRNOCONTAINERS,
		ERRREADAUDIO,
		ERRREADFRAME,
		ERRNOTSUPPORTED,
		ERRFILENOTFOUND,
		ERRREADCONTAINERS,
		ERRDECODEAUDIO,
		ERRDFVERSION,
		ERRSUFFIX
	};

	enum fileTypeID {
		DFERROR,
		DFBOOTFILE,
		DFMOV,
		DFPUP,
		DFSET,
		DFSHP,
		DFTRK,
		DFSFX,
		DF11K,
		DFSTG
	};

	// File header information
	struct Header {
		int32_t FourCC;
		int32_t fileSize;
		int32_t unknown[3];
		int32_t blockCount;
		int32_t type; // mostly 0, sometimes 1, sometimes 2
		int32_t gapWhere;
	};

	// every file is splitten in Blocks, with a maximum size of 2^16 bytes.
	// this struct includes all necessary block inormation to read from
	struct Blocks {
		int32_t blockID;
		int32_t blockLength;
		char* blockdata;

		~Blocks() {
			delete blockdata;
		}
	};

	// little struct to hold references to the string table, which is in the .exe
	struct StringTable {
		uint32_t filePos;
		uint16_t flag;
	};

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
		//char unknown[8];
		uint8_t textLength;
		char text[255];
		uint8_t identLength;
		char ident[31];
	};

	class DataBuffer {
	public:
		DataBuffer() {
			DataContent = new uint8_t[MaxBufferSize];
		}

		~DataBuffer() {
			delete[] DataContent;
		}


		void resize(uint32_t size) {
			ContentSize = size;

			if (size > MaxBufferSize) {
				delete[] DataContent;
				DataContent = new uint8_t[size];
				MaxBufferSize = size;
			}
		}

		uint8_t* GetContent() {
			return DataContent;
		}

		uint32_t GetSize() {
			return ContentSize;
		}

	private:

		uint32_t MaxBufferSize{ 1024 };		// buffer for decrypted and decompressed content, whole files
		uint32_t ContentSize{ 0 };			// actual content size
		uint8_t* DataContent;				// pointer to actual content
	};

	// This is used for literally container 1 (the second container) in the TRK files
	// The struct contains all data necessary to know the loops and which chunk to load
	struct AudioBlockInfos {

		struct AudioChunks {
			int32_t chunkBlockID;
			int32_t unknownInt;
			bool unknownBool; // usually 0, sometimes its 1
			uint8_t strSize;
			char identifier[31];
		};


		AudioBlockInfos(DFextractor* inst) {
			
			// chunk information is splitten in 2 blocks, while the second block is mostly empty
			// the first block has the audio chunks with loop information
			// the second block has audio that never loops, it only plays once

			uint8_t multiplyer = 1;		// used for string size. move file strings are bigger

			// read location of 2nd chunk in chunk0

			if (inst->fileType == DFMOV) {
				chunkInfo2Loc = *(int32_t*)(inst->fileBlock[0].blockdata + 96);
				multiplyer = 2;
				memcpy(trackName, inst->currentFileName.c_str(), inst->currentFileName.length()+1); // No name necessary
			}
			else {
				chunkInfo2Loc = *(int32_t*)(inst->fileBlock[0].blockdata + 32);
				uint8_t strSize = *(uint8_t*)(inst->fileBlock[0].blockdata + 36);
				memcpy(trackName, (inst->fileBlock[0].blockdata + 37), 16);
				trackName[strSize - 4] = '\0';	// minus 4 because I dont want the suffix
			}


			int8_t* container = (int8_t*)inst->fileBlock[1].blockdata;

			// start with first block
			// copy upper part with loop order
			unknown = *(int32_t*)container;
			container += 4;
			totalLoopsChunks = *(int16_t*)container;
			container += 2;

			// need to know the  info before
			chunkOrder = new int16_t[totalLoopsChunks];
			memcpy(chunkOrder, container, totalLoopsChunks * sizeof(int16_t));

			// copy lower part, which has the loop identifiers.
			container += 260; // <- not sure what is within this range

			audioLoopChunkCount = *(int16_t*)container;

			if (audioLoopChunkCount) {

				audioLoopChunks = new AudioChunks[audioLoopChunkCount];

				container += 4;
				for (int16_t i = 0; i < audioLoopChunkCount; i++) {
					// copy blockID, the unknwon val, the str size and the string.
					//memcpy(&audioChunks[i].chunkBlockID, block, 7+16);
					audioLoopChunks[i].unknownInt = *(int32_t*)container;
					container += 4;
					audioLoopChunks[i].chunkBlockID = *(int16_t*)container;
					container += 4;
					audioLoopChunks[i].unknownBool = *container;
					container += 2;
					audioLoopChunks[i].strSize = *(uint8_t*)container++;
					memcpy(&audioLoopChunks[i].identifier, container, 16);
					audioLoopChunks[i].identifier[audioLoopChunks[i].strSize] = '\0';
					//block += 7+16+3; // set pointer to next chunk info block
					container += 15;
				}

				containsAudioData = true;
			}

			// now what about the second block? Do we have any non looping data?
			
			container = (int8_t*)inst->fileBlock[chunkInfo2Loc].blockdata;
			container += 4;
			audioSingleChunkCount = *(int16_t*)container;
			if (audioSingleChunkCount) {
				// identical to previus block
				audioSingleChunks = new AudioChunks[audioSingleChunkCount];

				container += 4;
				for (int16_t i = 0; i < audioSingleChunkCount; i++) {
					audioSingleChunks[i].unknownInt = *(int32_t*)container;
					container += 4;
					audioSingleChunks[i].chunkBlockID = *(int32_t*)container;
					container += 4;
					audioSingleChunks[i].unknownBool = *container;
					container += 2;
					audioSingleChunks[i].strSize = *(uint8_t*)container++;
					memcpy(&audioSingleChunks[i].identifier, container, (16 * multiplyer) - 1);
					audioSingleChunks[i].identifier[audioSingleChunks[i].strSize] = '\0';
					container += (16* multiplyer)-1;
				}

				containsAudioData = true;
			}

		}
		bool containsAudioData{ false }; // I know it is weird, but sometimes they dont...

		char trackName[32];
		int32_t unknown;
		int16_t totalLoopsChunks;
		int16_t* chunkOrder;	// note: chunk order only applies for the first block!
		int32_t chunkInfo2Loc;

		int16_t audioLoopChunkCount;
		int16_t audioSingleChunkCount;

		AudioChunks* audioLoopChunks = nullptr;
		AudioChunks* audioSingleChunks = nullptr;

		~AudioBlockInfos() {
			delete[] chunkOrder;
			delete[] audioLoopChunks;
			delete[] audioSingleChunks;
		}
	};

	// set for 8 bit mono...
  // although the game settings allow 16bit stereo, there is no noticeable difference!
	struct waveHeader {

		waveHeader(int32_t chunkSize, int32_t hertz) 
			: chunkSize(chunkSize + 36), sampleRate(hertz), byteRate(hertz) {
		
			const int32_t totalListSize = 136;
			this->chunkSize += totalListSize +8;
			headerSize = totalListSize + 44 +8;

			int32_t strSizeBuffer;
			char* curr = listdata;
			// construct list data
			memcpy(curr, "LIST",4);
			curr += 4;
			memcpy(curr, &totalListSize, sizeof(int32_t));
			curr += 4;

			strSizeBuffer = 29;
			memcpy(curr, "INFOIART", 8);
			curr += 8;
			memcpy(curr, &strSizeBuffer, sizeof(int32_t));
			curr += sizeof(int32_t);
			memcpy(curr, "Scott Scheinbaum & Erik Holt", strSizeBuffer);
			curr += strSizeBuffer;

			memcpy(curr, "IGNR", 4);
			curr += 4;
			strSizeBuffer = 13;
			memcpy(curr, &strSizeBuffer, sizeof(int32_t));
			curr += sizeof(int32_t);
			memcpy(curr, "ingame audio", strSizeBuffer);
			curr += strSizeBuffer;

			memcpy(curr, "ISFT", 4);
			curr += 4;
			strSizeBuffer = 11;
			memcpy(curr, &strSizeBuffer, sizeof(int32_t));
			curr += sizeof(int32_t);
			memcpy(curr, "MDFETv", 6);	// Dream Factory Extraction Tool Version ...
			curr += 6;
			memcpy(curr, MDEFTVERSION, 5);
			curr += 5;
			
			memcpy(curr, "IENG", 4);
			curr += 4;
			strSizeBuffer = 6;
			memcpy(curr, &strSizeBuffer, sizeof(int32_t));
			curr += sizeof(int32_t);
			memcpy(curr, "MeToX", strSizeBuffer);	// because why not?
			curr += strSizeBuffer;

			memcpy(curr, "ICOP", 4);
			curr += 4;
			strSizeBuffer = 33;
			memcpy(curr, &strSizeBuffer, sizeof(int32_t));
			curr += sizeof(int32_t);
			memcpy(curr, "Copyright CyberFlix Incorporated", strSizeBuffer);	// because why not?
			curr += strSizeBuffer;

			memcpy(curr, "data", 4);
			curr += 4;
			strSizeBuffer = 33;
			memcpy(curr, &chunkSize, sizeof(int32_t));
		};

		const char StartChunkID[4]{'R','I','F','F' };
		int32_t chunkSize;
		const char format[8]{ 'W','A','V','E','f','m', 't', ' ' };
		const int32_t subChunksize{ 16 }; // size of rest of the sub chunk
		const int16_t audioFormat{ 1 }; // 1 for PCM
		const int16_t monoOrStereo{ 1 }; // 1 = Mono
		const int32_t sampleRate; // Hertz
		const int32_t byteRate; // sample rate * numChannels * bitsPerSample /8
		const int16_t blockAlign{ 1 }; // number of channels * bitsPerSample / 8
		const int16_t bitsPerSample{ 8 }; // ...
		// lit dat:
		char listdata[255];

		// sound data follows

		// the header size to copy the mem from is used from here. change it if header changes
		int16_t headerSize;	
	};

	// This class is able to read the entire scene data. Use one instance per scene
	// it will read all its corresponding views and logic, transition, rotation etc.
	class SceneData {

	public:
		SceneData(int32_t sceneID, DFextractor* inst) : instance(inst) {
			instance->fileBlock[0].blockdata;

			char* curr = instance->fileBlock[2].blockdata + (sceneID*42);
			unknownDWORD1 = *(int32_t*)(curr);
			curr += 4;
			unknownFT = *(float*)(curr);
			curr += 4;
			unknownWORD1 = *(int16_t*)(curr);
			curr += 2;
			locationViews = *(int32_t*)(curr);
			curr += 4;
			locationLogRight = *(int32_t*)(curr);
			curr += 4;
			locationLogLeft = *(int32_t*)(curr);
			curr += 4;
			locationScript = *(int32_t*)(curr);
			curr += 4;

			uint8_t stringSize = *curr++;
			memcpy(sceneName, curr, stringSize);
			sceneName[stringSize] = 0;

			readViews();
			readTransitionLogic(0, locationLogRight);	// its not left, so its false
			readTransitionLogic(1, locationLogLeft);	// its right, so its true
		};

		~SceneData() {
			delete[] sceneViews;
			delete[] turns[0];
			delete[] turns[1];
		}


		void printSceneData() {

			std::cout << "Name:                    " << sceneName << '\n';

			std::cout << "unidentified values:\n";
			std::cout << " - unknown dword1:       " << unknownDWORD1 << '\n';
			std::cout << " - unknown float:        " << unknownFT << '\n';
			std::cout << " - unknown word:         " << unknownWORD1 << '\n';
			std::cout << "identified values:\n";
			std::cout << " - Location Views:       " << locationViews << '\n'; 
			std::cout << " - Location right turns: " << locationLogRight << '\n';
			std::cout << " - Location left turns:  " << locationLogLeft << '\n';
			std::cout << " - Location script:      " << locationScript << '\n';
			std::cout << "data from view table:\n";
			std::cout << " - location X:           " << sceneLocation[0] << '\n';
			std::cout << " - location Y:           " << sceneLocation[1] << '\n';
			std::cout << " - location Z:           " << sceneLocation[2] << '\n';
			std::cout << " - unknown integer 1:    " << unknownVal[0] << '\n';
			std::cout << " - loc script (again):   " << unknownVal[1] << '\n';

			std::cout << std::endl;
		}

		void printViewData() {
			std::cout << "Scene View count: " << SceneViewCount << '\n';
			for (uint32_t i = 0; i < SceneViewCount; i++) {
				std::cout << " - view Name: " << sceneViews[i].viewName << '\n';
				std::cout << " - view ID:   " << sceneViews[i].viewID << '\n';
				std::cout << " - Obj ref:   " << sceneViews[i].locationObjects << '\n';
				std::cout << " - unknownDbl:" << sceneViews[i].unknownDB1 << "\n";
				std::cout << " - unknownInt:" << sceneViews[i].unknownINT1 << "\n\n";
			}
		}

		void printTurnData(bool index) {

			std::cout << "Turn count:  " << turnCount[index] << '\n';
			std::cout << "unknown val: " << turnUnknown[index] << '\n';
			std::cout << std::endl;
			for (uint32_t i = 0; i < turnCount[index]; i++) {
				// motionInfo variable can be 0, 1 or 2.
				// 0 = frame is in motion. Player cant move from here and its low res
				// 1 = frame is also low res, but player can turn or go forward
				// 2 = HiRes frame. Player can move and the image has a higher and clearer quality
				std::cout << " - Frame kind:       " << turns[index][i].motionInfo << '\n';
				// transition logic: If this is 0, the player can't move foward
				// if its not 0, it indicates the transition from this view to another
				// In this case its value is the location of the container that defines the transition logic
				std::cout << " - transition logic: " << turns[index][i].transitionLog << '\n';
				std::cout << " - unknown:          " << turns[index][i].unknownInt3 << '\n';
							    				   
				std::cout << " - frame container:  " << turns[index][i].frameContainerLoc << '\n';
				std::cout << " - frame ID?:        " << turns[index][i].frameID << '\n';
				std::cout << " - Quarternion?:\n";
				std::cout << " - QT? Double 1:     " << *(double*)turns[index][i].unknownDB1 << '\n';
				std::cout << " - QT? Double 2:     " << *(double*)turns[index][i].unknownDB2 << '\n';
				std::cout << " - QT? Double 3:     " << *(double*)turns[index][i].unknownDB3 << '\n';
				std::cout << " - QT? Double 4:     " << *(double*)turns[index][i].unknownDB4 << '\n';
				std::cout << " - unknown float 1:  " << turns[index][i].unknownFT1 << '\n';
				std::cout << " - unknown float 2:  " << turns[index][i].unknownFT2 << '\n';
				std::cout << '\n';
			}
		}

		int32_t getTurnCount(bool index) {
			return turnCount[index];
		}

		void printAllTransitions(bool index) {
			
			// check all blocks that have transition correlations
			for (uint32_t i = 0; i < turnCount[index]; i++)
				if (turns[index][i].transitionLog) {
					std::cout << "transition found at: " << turns[index][i].transitionLog << '\n';
					int32_t turnCountTr = *(int32_t*)(instance->fileBlock[turns[index][i].transitionLog].blockdata + 4);
					
					std::cout << "---Transitions:  " << turnCountTr << '\n';
					std::cout << "---unknown val: " << *(int32_t*)(instance->fileBlock[turns[index][i].transitionLog].blockdata + 8) << '\n';
					
					SceneTransition trans;
					char* curr = instance->fileBlock[turns[index][i].transitionLog].blockdata + 12;
					for (uint32_t i = 0; i < turnCountTr; i++) {
						memcpy(&trans, curr + (sizeof(SceneTransition)*i), sizeof(SceneTransition));

						std::cout << " - - - Not in motion:    " << trans.motionInfo << '\n';
						std::cout << " - - - transition logic: " << trans.transitionLog << '\n';
						std::cout << " - - - unknown2:         " << trans.unknownInt3 << '\n';

						std::cout << " - - - frame container:  " << trans.frameContainerLoc << '\n';
						std::cout << " - - - frame ID?:        " << trans.frameID << '\n';
						std::cout << " - - - Quarternion?:\n";
						std::cout << " - - - QT? Double 1:     " << *(double*)trans.unknownDB1 << '\n';
						std::cout << " - - - QT? Double 2:     " << *(double*)trans.unknownDB2 << '\n';
						std::cout << " - - - QT? Double 3:     " << *(double*)trans.unknownDB3 << '\n';
						std::cout << " - - - QT? Double 4:     " << *(double*)trans.unknownDB4 << '\n';
						std::cout << " - - - unknown float 1:  " << trans.unknownFT1 << '\n';
						std::cout << " - - - unknown float 2:  " << trans.unknownFT2 << '\n';
						std::cout << std::endl;
					}
				}
		}

		int32_t printFrameInformation() {
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
					std::cout << "\t|" << *(int16_t*)instance->fileBlock[turns[direction][i].frameContainerLoc].blockdata;
					std::cout << " x " << *(int16_t*)(instance->fileBlock[turns[direction][i].frameContainerLoc].blockdata + 2) << '\n';
				}

				// now print all transition, if it has transition frames
				for (uint32_t i = 0; i < turnCount[direction]; i++) {
					if (turns[direction][i].transitionLog) {
						
						int32_t turnCountTr = *(int32_t*)(instance->fileBlock[turns[direction][i].transitionLog].blockdata + 4);

						SceneTransition trans;
						char* curr = instance->fileBlock[turns[direction][i].transitionLog].blockdata + 12;
						for (uint32_t i = 0; i < turnCountTr; i++) {
							memcpy(&trans, curr + (sizeof(SceneTransition)*i), sizeof(SceneTransition));

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
							std::cout << "\t|" << *(int16_t*)instance->fileBlock[trans.frameContainerLoc].blockdata;
							std::cout << " x " << *(int16_t*)(instance->fileBlock[trans.frameContainerLoc].blockdata + 2) << '\n';
						}
					}
				}
			}


			return totalFrames;
		}
		void writeSceneScripts(const std::string& dir) {

			std::string newDir(dir);
			newDir.push_back('/');
			newDir.append(sceneName);
			CreateDirectoryA(newDir.c_str(), NULL);
			
			instance->writeScript(newDir, locationScript, sceneName);
		}

		int32_t writeSceneFrames(const std::string& dir) {

			static const std::string rotDirs[2]{"/D","/A"};	// just determines the folders writing to

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
							memcpy(&trans, curr + (sizeof(SceneTransition)*i), sizeof(SceneTransition));

							totalFrames++;
							instance->writeFrame(newDirAdd, trans.frameContainerLoc, false);
							//std::cout << "write frame container: " << trans.frameContainerLoc << '\n';
						}
					}
				}
			}


			return totalFrames;
		}

	private:

		DFextractor* instance;

		// PROCESS VIEWS!
		struct SceneViews {

			int32_t unknownINT1;
			double unknownDB1;
			int32_t viewID;
			int32_t locationObjects;
			char viewName[26];


		};
		int32_t SceneViewCount;
		SceneViews* sceneViews;

		void readViews() {

			memcpy(sceneLocation, instance->fileBlock[locationViews].blockdata, 3 * sizeof(double));
			memcpy(unknownVal, instance->fileBlock[locationViews].blockdata + (3 * sizeof(double)), 2*sizeof(int32_t));
			// next line would be scene name once again, which we already have
			SceneViewCount = *(int32_t*)(instance->fileBlock[locationViews].blockdata + 48);
			char* curr = instance->fileBlock[locationViews].blockdata + 62;
			sceneViews = new SceneViews[SceneViewCount];
			for (uint32_t i = 0; i < SceneViewCount; i++) {
				sceneViews[i].unknownINT1 = *(int32_t*)(curr);
				curr += 4;
				sceneViews[i].unknownDB1 = *(double*)(curr); // Whats here??
				curr += 8;	
				sceneViews[i].viewID = *(int32_t*)(curr);
				curr += 4;
				sceneViews[i].locationObjects = *(int32_t*)(curr);
				curr += 4;

				uint8_t stringSize = *curr++;
				memcpy(sceneViews[i].viewName, curr, stringSize);
				sceneViews[i].viewName[stringSize] = 0;
				curr += 25;
			}
		}


		struct SceneTransition {
			// its a clear pattern of 4x 8byte values but I dont know what they do
			// might be 4 doubles, indicating something. Location? rotation?

			uint8_t unknownDB1[8];	// these are doubles, but for some reason
			uint8_t unknownDB2[8];  // its 4 bytes too much if I put 4 doubles...
			uint8_t unknownDB3[8];	// so I read it like this and cast it later one
			uint8_t unknownDB4[8];

			float unknownFT1;
			float unknownFT2;

			int32_t motionInfo;			// 0 = in motion, 1= static low res, 2 = high res2
			int32_t frameContainerLoc;	// reference to the container that has frame
			int32_t frameID;	// another reference to the container that has frame
			int32_t transitionLog;		// mostly zero
			int32_t unknownInt3;		// ???
		};
		int32_t turnCount[2];
		int32_t turnUnknown[2];
		SceneTransition* turns[2];

		void readTransitionLogic(const bool turnIndex, const uint32_t& containerID) {
			// just using bool to make sure its never the index above 1
			turnCount[turnIndex] = *(int32_t*)(instance->fileBlock[containerID].blockdata + 4);
			turnUnknown[turnIndex] = *(int32_t*)(instance->fileBlock[containerID].blockdata + 8);
			char *curr = instance->fileBlock[containerID].blockdata + 12;

			turns[turnIndex] = new SceneTransition[turnCount[turnIndex]];

			memcpy(turns[turnIndex], curr, turnCount[turnIndex] * sizeof(SceneTransition));
		}

		
		int32_t unknownDWORD1;
		float unknownFT;
		int16_t unknownWORD1;

		int32_t locationViews;
		int32_t locationLogRight;
		int32_t locationLogLeft;
		int32_t locationScript;

		double sceneLocation[3];
		int32_t unknownVal[2];

		char sceneName[15];

		
	};


	class MovFrames {
	public:

		MovFrames(uint8_t* MovContainerZero) {
			frameCount = *(int32_t*)(MovContainerZero + 2168);
			uint8_t* curr = MovContainerZero + 2172;

			frames = new Frame[frameCount];

			for (int32_t i = 0; i < frameCount; i++) {
				frames[i].frameCondition = *(int32_t*)curr;	// 1 = yes, 0 = No
				curr += 4;
				frames[i].unknownWord1 = *(int16_t*)curr;
				curr += 2;
				frames[i].unknownWord2 = *(int16_t*)curr;
				curr += 2;
				frames[i].height = *(int16_t*)curr;
				curr += 2;
				frames[i].width = *(int16_t*)curr;
				curr += 2;
				frames[i].locationFrame = *(int32_t*)curr;
				curr += 4;
				frames[i].locationSec = *(int32_t*)curr;
				curr += 4;
				frames[i].unknownWord3 = *(int16_t*)curr;
				curr += 2;
				frames[i].unknownWord4 = *(int16_t*)curr;
				curr += 2;
				frames[i].unknownWord5 = *(int16_t*)curr;
				curr += 2;
				memcpy(frames[i].frameName, curr + 1, *curr);
				frames[i].frameName[*curr] = '\0';
				curr += 16;
			}
		}

		~MovFrames() {
			delete[] frames;
		}

		void printFrames() {
			std::cout << "FRAME information:\n";
			for (int32_t i = 0; i < frameCount; i++) {
				std::cout << "Start frame?:   " << frames[i].frameCondition << '\n';	// 1 = yes, 0 = No
				std::cout << "unknown word1:  " << frames[i].unknownWord1 << '\n';
				std::cout << "unknown word2:  " << frames[i].unknownWord2 << '\n';
				std::cout << "height?:        " << frames[i].height << '\n';
				std::cout << "width?:         " << frames[i].width << '\n';
				std::cout << "frame location: " << frames[i].locationFrame << '\n';
				std::cout << "second location:" << frames[i].locationSec << '\n';
				std::cout << "unknown word3:  " << frames[i].unknownWord3 << '\n';
				std::cout << "unknown word4:  " << frames[i].unknownWord4 << '\n';
				std::cout << "unknown word5:  " << frames[i].unknownWord5 << '\n';
				std::cout << "frame name:     " << frames[i].frameName << '\n';
				std::cout << std::endl;
			}
		}

		void writeFrames(DFextractor* inst, const std::string& path) {
			for (int32_t i = 0; i < frameCount; i++) {
				inst->writeFrame(path, frames[i].locationFrame, false, frames[i].frameName);
			}
		}

	private:

		int32_t frameCount;

		struct Frame {
			int32_t frameCondition;
			int16_t unknownWord1;
			int16_t unknownWord2;
			int16_t height;
			int16_t width;
			int32_t locationFrame;
			int32_t locationSec;
			int16_t unknownWord3;
			int16_t unknownWord4;
			int16_t unknownWord5;
			char frameName[15];
		};

		Frame* frames;

	};

	class STGdata {
	public:
		STGdata(DFextractor* inst) 
			: inst(inst) {
			frameCount = *(int32_t*)(inst->fileBlock[0].blockdata + 2120);

			frames = new Frame[frameCount];

			char* curr = inst->fileBlock[0].blockdata + 2124;

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
			

		}

		~STGdata() {
			delete[] frames;
		}

		void printFrameInfo() {
			for (uint32_t i = 0; i < frameCount; i++) {
				std::cout << "Identifier:           " << frames[i].frameName << '\n';
				std::cout << "Condition:            " << frames[i].frameCondition << '\n';
				std::cout << "unknown Word:         " << frames[i].unknownWord1 << '\n';
				std::cout << "Location Script:      " << frames[i].locationScript << '\n';
				std::cout << "Location Frame:       " << frames[i].locationFrame << '\n';
				std::cout << "Location click Logic: " << frames[i].locationClickLogic << '\n';
				std::cout << "Height:               " << frames[i].height << "px\n";
				std::cout << "Width:                " << frames[i].width << "px\n";
				std::cout << "Width:                " << frames[i].containerSize << " bytes\n";
				std::cout << std::endl;
			}
		}

		void writeScripts(const std::string& path) {
			for (uint32_t i = 0; i < frameCount; i++) {
				inst->writeScript(path, frames[i].locationScript, frames[i].frameName);
			}
		}

		int32_t writeSTGFrames(const std::string& path) {
			for (uint32_t i = 0; i < frameCount; i++) {
				inst->writeFrame(path, frames[i].locationFrame, false, frames[i].frameName);
			}

			return frameCount;
		}

	private:
		DFextractor* inst;

		// careful, this structure differs from the .mov files
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
	};

	class DFFile {
	public:
		explicit DFFile(DFextractor* DFinst, const std::string& path);

		virtual int32_t initContainer();
		virtual int32_t analyze();	// <- only used for def mode

		virtual int32_t writeScripts();
		virtual int32_t writeAudio();
		virtual int32_t writeFrames();

		std::string pathToOutput;

	protected:
		DFextractor* DFinst;


		int32_t lastError{ 0 };



	};

	// SFX, TRK and 11K
	class DFAudioFile : public DFFile {
		using DFFile::DFFile;

		int32_t initContainer() override;
		int32_t analyze() override;
		int32_t writeAudio() override;
	};

	// MOV files: cut scenes, inspect, slide shows etc.
	class DFMovFile : public DFFile {
		using DFFile::DFFile;

		int32_t initContainer() override;
		int32_t analyze() override;
		int32_t writeAudio() override;
		int32_t writeFrames() override;
	};

	// Puppets
	class DFPupFile : public DFFile {
		using DFFile::DFFile;

		int32_t initContainer() override;
		int32_t analyze() override;
		int32_t writeScripts() override;
		int32_t writeAudio() override;
		int32_t writeFrames() override;
	};

	// Set files are the rooms, cabins, corridors etc.
	class DFSetFile : public DFFile {
		using DFFile::DFFile;

		int32_t initContainer() override;
		int32_t analyze() override;
		int32_t writeScripts() override;
		int32_t writeFrames() override;
	};

	class DFStageFile : public DFFile {
		using DFFile::DFFile;

		int32_t initContainer() override;
		int32_t analyze() override;
		int32_t writeScripts() override;
		int32_t writeFrames() override;
	};

	class DFBootFile : public DFFile {
		using DFFile::DFFile;

		int32_t writeScripts() override;
	};

	DFFile* containerType = nullptr;

	AudioBlockInfos* AudioPtr;
	MovFrames* movFramePtr;
	STGdata* StgDataPtr;
	SceneData** scenesPtr;

	Header hd;
	int32_t* blockOffsets;
	Blocks* fileBlock;
	char* stringTableBlock;
	int16_t audioTextCount{ 0 };
	//uint16_t LookUpTableEntries{ 0 };
	std::string currentFileName;

	PuppetData* strBlocks0;
	StringTable* LookUpTable;

	//bool loadLookUpTable(const char* exe);
	bool readFileToBlocks(const char* file);
	
	int32_t sceneCount{ 0 };
	//DataBuffer soundBuffer;
	DataBuffer containerDataBuffer;

	const std::string readScriptBlock(Blocks&);
	void saveFileName(std::string fileName);

	bool audioDecoder(int32_t& uncompressedSize,int8_t* soundFile, int8_t* decodedOutput);

	std::string getDialougText(const std::string& ident);
};

