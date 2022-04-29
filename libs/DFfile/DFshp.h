#pragma once

#include "DFfile.h"

class DFshp : public DFfile
{
	using DFfile::DFfile;
public:
	std::string getHeaderInfo() { return std::string(); }

	static DFshp* getDFshp(DFfile* ptr) {
		if (ptr->fileType != DFSHP)
			return nullptr;
		return (DFshp*)ptr;
	}

	void writeAllScripts() {

		std::string out(outPutPath);
		std::filesystem::create_directory(out);

		if (writeScript(containers[mainScriptLocation], out + "/mainScript"))
			fileCounter++;

		for (int32_t objGrp = 0; objGrp < objGroups.size(); objGrp++) {
			std::string outGrp(out);
			outGrp.push_back('/');
			outGrp.append(objGroups[objGrp].name);
			std::filesystem::create_directory(outGrp);

			if (writeScript(containers[objGroups[objGrp].scriptContainerLocation], outGrp + "/Script"))
				fileCounter++;

		}
	}

	void writeAllFrames() {
		std::string out(outPutPath);
		std::filesystem::create_directory(out);

		for (int32_t objGrp = 0; objGrp < objGroups.size(); objGrp++) {
			std::string outGrp(out);
			outGrp.push_back('/');
			outGrp.append(objGroups[objGrp].name);
			std::filesystem::create_directory(outGrp);

			for (int32_t entry = 0; entry < objGroups[objGrp].entries.size(); entry++) {
				std::string subOut(outGrp);
				subOut.push_back('/');
				subOut.append(objGroups[objGrp].entries[entry].identifier);
				std::filesystem::create_directory(subOut);

				for (int32_t subEntry = 0; subEntry < objGroups[objGrp].entries[entry].subEntries.size(); subEntry++) {

					writeTransPNGimage(subOut, objGroups[objGrp].entries[entry].subEntries[subEntry].location);
					fileCounter++;
				}
			}
		}
	}

protected:

	struct ObjectGroup {
		struct Entry {
			struct subEntry {
				int32_t location;
			};
			int32_t location;
			int32_t unknownInt1;
			int32_t unknownInt2;
			int32_t unknownInt3; 
			std::string identifier;

			std::vector<subEntry>subEntries;
		};

		ObjectGroup(int32_t location, DFfile* file) : location(location) {
		
			uint8_t* curr = file->containers[location].data + 24;
		
			unknownInt1 = *(int32_t*)curr;
			curr += 4;
			memcpy(&unkknownShort1, curr, 5 * sizeof(int16_t));
			curr += 5 * sizeof(int16_t);

			scriptContainerLocation = unknownInt1 = *(int32_t*)curr;
			curr += 4;
			name.assign((char*)curr, *curr++);
			curr += 47; 

			// read entries
			int32_t entryCount = *(int32_t*)curr;
			curr += 4;
			entries.reserve(*(int32_t*)curr);
			
			// read 32 byte entries
			for (int32_t entry = 0; entry < entryCount; entry++) {
				entries.emplace_back();
				memcpy(&entries.back().location, curr, 4 * sizeof(int32_t));
				curr += 4 * sizeof(int32_t);
				entries.back().identifier.assign((char*)curr, *curr++);
				curr += 15;

				int32_t subEntriesCount = *(int32_t*)(file->containers[entries.back().location].data + 114);
				entries.back().subEntries.reserve(subEntriesCount);
				for (int32_t subEntry = 0; subEntry < subEntriesCount; subEntry++) {
					entries.back().subEntries.emplace_back();
					entries.back().subEntries.back().location = *(int32_t*)(file->containers[entries.back().location].data + 118 + (44* subEntry));
				}
			}
		}

		int32_t location;

		// this is in the actual header
		int32_t unknownInt1;
		// I think these 3  belong togehter
		int16_t unkknownShort1;
		int16_t unkknownShort2;
		int16_t unkknownShort3;
		// and these two?
		int16_t unkknownShort4;
		int16_t unkknownShort5;

		int32_t scriptContainerLocation;
		std::string name;

		std::vector<Entry>entries;
	};

	int32_t initContainerData() {

		// dont even continue if version does not match with 4.0
		if (*(int32_t*)(containers[0].data + 0x2) != 4) {
			return ERRDFVERSION;
		}

		// copy color palette
		colorCount = 256;
		memcpy(colors, containers[0].data + 36, colorCount * sizeof(ColorPalette));

		mainScriptLocation = *(int32_t*)(containers[0].data + 20);
		refName.assign((char*)(containers[0].data + 2345), *(containers[0].data + 2344));

		// read object group locations
		const int32_t objGrpCount = *(int32_t*)(containers[0].data + 2360);
		objGroups.reserve(objGrpCount);

		uint8_t* curr = containers[0].data + 2364;
		for (int32_t objGrp = 0; objGrp < objGrpCount; objGrp++) {
			objGroups.emplace_back(*(int32_t*)curr, this);
			curr += 16;
		}
		return 0;
	}

	int32_t mainScriptLocation;
	std::string refName;

	public:

	std::vector<ObjectGroup>objGroups;
};