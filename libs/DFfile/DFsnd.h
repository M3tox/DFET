#pragma once

#include "DFfile.h"

class DFsnd : public DFfile
{
	using DFfile::DFfile;
public:
	std::string getHeaderInfo() { return std::string(); }

	void writeAllAudio() {

		if (version4) {
			// TITANIC
			int32_t locationChunkOrder = *(int32_t*)(containers[0].data + 28);
			int32_t locationTrackNames = *(int32_t*)(containers[0].data + 32);

			int32_t trackCount = *(int32_t*)(containers[locationTrackNames].data+4);
			for (int32_t i = 0; i < trackCount; i++) {
				int32_t audioLoc = *(int32_t*)(containers[locationTrackNames].data + 12 + (i * 26));
				std::string name((char*)(containers[locationTrackNames].data + 19 + (i * 26)), *(containers[locationTrackNames].data + 18 + (i * 26)));

				processSingleAudio(name, audioLoc);
			}

		} else if (version1) {
			// DUST
			int32_t count = *(int32_t*)(containers[0].data + 174);

			int16_t chunksStart = *(int16_t*)(containers[0].data + 24);
			int16_t chunksCountUniqueAudios = *(int16_t*)(containers[0].data + 26);
			int16_t chunksCount = *(int16_t*)(containers[0].data + 28);
			uint8_t* curr = containers[0].data + 30;
			int16_t* audioChunks = (int16_t*)curr;

			curr = containers[0].data + 158;
			std::string audioName((char*)curr, *curr++);
			curr = containers[0].data + 186;

			// process single shots
			for (uint32_t file = 0; file < count; file++) {

				// break out if chunkstart is reached
				if (file == chunksStart && chunksCountUniqueAudios) break;

				std::string name((char*)curr, *curr++);
				curr += 23;

				processSingleAudio(name, file + 1);
			}

			// process combined audio chunks
			if (chunksCountUniqueAudios) {
				int32_t totalFileSize{ 0 };
				int32_t* fileSizes = new int32_t[chunksCount];
				for (int32_t loop = 0; loop < chunksCount; loop++) {

					fileSizes[loop] = *(int32_t*)(containers[chunksStart + audioChunks[loop]].data + 36);
					totalFileSize += fileSizes[loop];
				}

				// get hertz rate from first block. 
				int32_t hertz = *(int32_t*)(containers[chunksStart+1].data + 28);

				// construct audio name based on blockZero
				std::string filname(outPutPath);
				filname.push_back('/');
				filname.append(audioName);
				filname.append(".wav");

				waveHeader header(totalFileSize, hertz, versionSig);

				containerDataBuffer.resize(totalFileSize + header.headerSize + 3);
				//int8_t* toOutput = new int8_t[totalFileSize + header.headerSize + 3]; // 3 extra overflow decoder bytes
				memcpy(containerDataBuffer.GetContent(), header.StartChunkID, header.headerSize);

				std::ofstream audioFile(filname, std::ios::binary | std::ios::trunc);

				int32_t current{ 0 };
				for (int32_t loop = 0; loop < chunksCount; loop++) {
					if (!audioDecoder(fileSizes[loop], (int8_t*)containers[chunksStart + audioChunks[loop]].data, (int8_t*)containerDataBuffer.GetContent() + header.headerSize + current)) {
						status = ERRDECODEAUDIO;
						return;
					}
					current += fileSizes[loop];
				}
				audioFile.write((char*)containerDataBuffer.GetContent(), totalFileSize + header.headerSize);
				audioFile.close();
				fileCounter++;
				delete[] fileSizes;
			}
		}
	}

protected:

	bool version4 = false;
	bool version1 = false;
	int32_t initContainerData() {
		// check version
		if (*(int32_t*)(containers[0].data + 0x2) == 4) {
			version4 = true;
		}
		else if (*(int32_t*)(containers[0].data + 0x2) == 1) {
			version1 = true;
		}
		
		if (!version1 && !version4) return ERRDFVERSION;

		return 0;
	}

	void processSingleAudio(const std::string& name, int32_t container) {
		int32_t hertz = *(int32_t*)(containers[container].data + 28);
		int32_t fileSize = *(int32_t*)(containers[container].data + 36);

		waveHeader header(fileSize, hertz, versionSig);

		containerDataBuffer.resize(fileSize + header.headerSize + 3);
		//int8_t* toOutput = new int8_t[fileSize + header.headerSize + 3]; // extra bytes decoder overflow
		memcpy(containerDataBuffer.GetContent(), header.StartChunkID, header.headerSize);

		if (!audioDecoder(fileSize, (int8_t*)containers[container].data, (int8_t*)containerDataBuffer.GetContent() + header.headerSize)) {
			status = ERRDECODEAUDIO;
			return;
		}

		std::string filname(outPutPath);
		filname.push_back('/');
		// sometimes they have subfolders. detect those and ignore them
		std::string temp(name);
		std::string::size_type pos = temp.find('/');
		if (pos == std::string::npos) {
			filname.append(temp);
		}
		else
			filname.append(temp.substr(pos + 1));

		filname.append(".wav");

		std::ofstream fileOut(filname, std::ios::binary | std::ios::trunc);
		fileOut.write((char*)containerDataBuffer.GetContent(), fileSize + 44);
		fileOut.close();
		fileCounter++;
	}
};
