/*
COPYRIGHT MeToX
GPL-3.0 License 

This is the core of everything. Some of these functions took more months to construct.
Here you find the definitions about the audio decoder, frame decompressor, error handler and a lot of more stuff.
*/

#include "DFextractor.h"


DFextractor::DFextractor(const std::string& file) {

	std::ifstream test(file);
	if (!test.good()) {
		lastError = ERRFILENOTFOUND;
		return;
	}

	// sanity check:
	test.seekg(0, test.end);
	const std::streamoff fileSize = test.tellg();

	// valid DF file can't be smaller than this
	if (fileSize < 1024) {
		lastError = ERRFILEFORMAT;
		return;
	}

	// make sure the LPPALPPA format is written
	char FileData[8];
	test.seekg(32, test.beg);
	test.read(FileData, sizeof FileData);
	test.close();

	if (memcmp(FileData, "LPPALPPA", 8)) {
		lastError = ERRFILEFORMAT;
		return;
	}
	
	// check file format
	if (!readFileToBlocks(file.c_str())) {
		fileType = DFERROR;
		lastError = ERRREADCONTAINERS;
		return;
	}

	std::string fileName(file);
	std::string::size_type pos = fileName.rfind('\\');
	if (pos != std::string::npos) {
		fileName.assign(fileName.substr(pos+1, fileName.length()));
	}
	else {
		pos = fileName.rfind('/');
		if (pos != std::string::npos) {
			fileName.assign(fileName.substr(pos + 1, fileName.length()));
		}
	}

	pos = fileName.rfind('.');
	if (pos == std::string::npos) {
		if (fileName.compare("BOOTFILE"))
		{
			fileType = DFERROR;
			lastError = ERRSUFFIX;
			return;
		}

		// process BOOTFILE, should be the only one without suffix. Comparision makes sure
		saveFileName('_' + fileName);
		fileType = DFBOOTFILE;
		containerType = new DFBootFile(this, currentFileName);
		return;
	}

	std::string suffix = fileName.substr(pos + 1, fileName.length());
	// in case of the file name suffix was in small letters (for some reason) capitlize them
	for (uint8_t i = 0; i < suffix.length(); i++)
		suffix.at(i) = toupper(suffix.at(i));

	saveFileName(fileName.substr(0, pos));

	if (!suffix.compare("PUP")) {
		fileType = DFPUP;
		containerType = new DFPupFile(this, currentFileName);
	}
	else if (!suffix.compare("SET")) {

		if (*(int16_t*)(fileBlock[0].blockdata + 2) != 4) {
			fileType = DFERROR;
			lastError = ERRDFVERSION;
			return;
		}
			
		fileType = DFSET;
		containerType = new DFSetFile(this, currentFileName);
	}
	else if (!suffix.compare("TRK")) {
		fileType = DFTRK;
		containerType = new DFAudioFile(this, currentFileName);
	}
	else if (!suffix.compare("SFX")) {
		fileType = DFSFX;
		containerType = new DFAudioFile(this, currentFileName);
	}
	else if (!suffix.compare("11K")) {
		fileType = DF11K;
		containerType = new DFAudioFile(this, currentFileName);
	}
	else if (!suffix.compare("MOV")) {
    // sorry, but currently only version 4 is supported
		if (*(int16_t*)(fileBlock[0].blockdata + 2) != 4) {
			fileType = DFERROR;
			lastError = ERRDFVERSION;
			return;
		}
		fileType = DFMOV;
		containerType = new DFMovFile(this, currentFileName);
	}
	else if (!suffix.compare("STG")) {
		fileType = DFSTG;
		containerType = new DFStageFile(this, currentFileName);
	}
	else {
		fileType = DFERROR;
		lastError = ERRNOTSUPPORTED;
		return;
	}

	containerType->initContainer();
}

DFextractor::~DFextractor() {

	// clean up! We don't want memory leaks
	// especially if people extract multiple files!

	delete[] fileBlock;
	delete[] blockOffsets;
	delete[] stringTableBlock;
	delete[] strBlocks0;
	delete[] LookUpTable;
	
	delete movFramePtr;
	delete AudioPtr;
	delete StgDataPtr;

	for (int32_t sc = 0; sc < sceneCount; sc++) {
		delete scenesPtr[sc];
	}

	delete containerType;

}

bool DFextractor::readFileToBlocks(const char* file) {

	std::ifstream fileToRead(file, std::ios::binary);


	if (fileToRead.good()) {

		fileToRead.read((char*)&hd, sizeof(hd));

		fileToRead.seekg(1024);

		blockOffsets = new int32_t[hd.blockCount];
		
		fileToRead.read((char*)blockOffsets, hd.blockCount * sizeof(int32_t));

		
		fileBlock = new Blocks[hd.blockCount];
		// default reading
		if (hd.type == 1) {

			for (int32_t bli = 0; bli < hd.blockCount; bli++) {

				if (bli == hd.gapWhere) {
					// some blocks are just empty dummies for whatever reason
					// they still affect the iterator, so I still do them. I write "NOP = NO OPERATION" just to know if I watch them
					constexpr int32_t dummySize{ 4 };
					char dummy[dummySize]{ 'T', 'P', '0' + hd.type, '\0' };
					fileBlock[bli].blockLength = dummySize;
					fileBlock[bli].blockdata = new char[dummySize];
					memcpy(fileBlock[bli].blockdata, dummy, dummySize);

				}
				else {
					// same as default reading, might want to improve in the future
					fileToRead.seekg(blockOffsets[bli]);
					int test = fileToRead.tellg();

					fileToRead.read((char*)&fileBlock[bli].blockID, sizeof(fileBlock[bli].blockID));
					fileToRead.read((char*)&fileBlock[bli].blockLength, sizeof(fileBlock[bli].blockLength));
					fileBlock[bli].blockdata = new char[fileBlock[bli].blockLength];
					fileToRead.read(fileBlock[bli].blockdata, fileBlock[bli].blockLength);
				}

				fileToRead.clear();
			}

		}
		else if (hd.type == 2){

			// TODO: Blocks with ZERO in the location are empty or silent blocks!
			for (int32_t bli = 0; bli < hd.blockCount; bli++) {

				if (bli == hd.gapWhere-1 || bli == hd.gapWhere) {
					// some blocks are just empty dummies for whatever reason
					// they still affect the iterator, so I still do them. I write "NOP = NO OPERATION" just to know if I watch them
					constexpr int32_t dummySize{ 4 };
					char dummy[dummySize]{ 'T', 'P', '0'+hd.type, '\0' };
					fileBlock[bli].blockLength = dummySize;
					fileBlock[bli].blockdata = new char[dummySize];
					memcpy(fileBlock[bli].blockdata, dummy, dummySize);

				}
				else {
					// same as default reading, might want to improve in the future
					fileToRead.seekg(blockOffsets[bli]);

					fileToRead.read((char*)&fileBlock[bli].blockID, sizeof(fileBlock[bli].blockID));
					fileToRead.read((char*)&fileBlock[bli].blockLength, sizeof(fileBlock[bli].blockLength));
					fileBlock[bli].blockdata = new char[fileBlock[bli].blockLength];
					fileToRead.read(fileBlock[bli].blockdata, fileBlock[bli].blockLength);
				}

				fileToRead.clear();
			}
		}
		else {
			for (int32_t bli = 0; bli < hd.blockCount; bli++) {
				fileToRead.seekg(blockOffsets[bli]);

				fileToRead.read((char*)&fileBlock[bli].blockID, sizeof(fileBlock[bli].blockID));
				fileToRead.read((char*)&fileBlock[bli].blockLength, sizeof(fileBlock[bli].blockLength));				
				fileBlock[bli].blockdata = new char[fileBlock[bli].blockLength];
				fileToRead.read(fileBlock[bli].blockdata, fileBlock[bli].blockLength);

				fileToRead.clear();
			}
		}
	}
	else
		return false;


	fileToRead.close();
	return true;
}


const std::string DFextractor::readScriptBlock(Blocks& blockFile) {

	std::string script;

	if (!hd.blockCount) return std::string();

	for (int32_t i = 0; i < blockFile.blockLength; i += 6) {
		uint16_t flag;

		// read flag
		flag = *(uint16_t*)(blockFile.blockdata + i);
		i += 2;

		// if flag is zero, it has reached the end
		if (flag) {
			if (flag == 0x0003 || flag == 0x0005) {

				// read string via relative path			
				uint16_t relPos = *(uint16_t*)(blockFile.blockdata + i);

				// read pascal string at relative position
				uint8_t strSize = *(blockFile.blockdata + (i - 2) + relPos++);

				std::string textElement(blockFile.blockdata + (i - 2) + relPos, 0, strSize);
				script.append(textElement);

				// will be handy to the user to also write the dialouge texts into the scripts
				if (flag == 0x0003 && fileType == DFPUP) {
					script.append(getDialougText(textElement));
				}
				script.push_back(' ');


			}
			else if (flag == 0x0004) {
				// Integer flag
				// can probably also be negative
				int16_t intVal = *(uint16_t*)(blockFile.blockdata + i);
				script.append(std::to_string(intVal));

			}
			else if (flag == 0x0006) {
				// Indentation
				int16_t ident = *(uint16_t*)(blockFile.blockdata + i);

				script.push_back('\n');

				while (ident--) {
					script.push_back('\t');
				}

			}
			else {
				// lookup table
				script.append(ScriptCommands.at(flag));
				//script.append(getString(flag));
				script.push_back(' ');
			}
		}
		else
			break;

	}

	return script;
}

bool DFextractor::readPuppetStrings() {

	// calculate number of text blocks, block size always 312 bytes
	// thanks god they always start at 2184, in DF and TI
	
	audioTextCount = *(int16_t*)(fileBlock[0].blockdata + 2158);

	const uint16_t offsetStartText{ 2160 };
	strBlocks0 = new PuppetData[audioTextCount];

	memcpy(strBlocks0, fileBlock[0].blockdata + offsetStartText, sizeof(PuppetData)*audioTextCount);

	
	// append zeros to strings
	for (short i = 0; i < audioTextCount; i++) {
		strBlocks0[i].ident[strBlocks0[i].identLength] = '\0';
		strBlocks0[i].text[strBlocks0[i].textLength] = '\0';
	}

	return true;
}

// FRIENDS INITS

void DFextractor::initAudio() {
	AudioPtr = new AudioBlockInfos(this);
}

void DFextractor::initMOV() {
	movFramePtr = new MovFrames((uint8_t*)fileBlock[0].blockdata);
	AudioPtr = new AudioBlockInfos(this);
}

void DFextractor::initSET() {
	sceneCount = *(int32_t*)(fileBlock[0].blockdata + 100);
	scenesPtr = new SceneData*[sceneCount];
	for (int32_t sc = 0; sc < sceneCount; sc++) {
		scenesPtr[sc] = new SceneData(sc, this);
	}
}

void DFextractor::initSTG() {
	StgDataPtr = new STGdata(this);
}

////////////////////////////////
//// PUBLIC PRINT FUNCTIONS
////////////////////////////////

void DFextractor::printFileHeaderInfo() {

	for (int32_t bli = 0; bli < hd.blockCount; bli++) {
		
		std::cout << "Container:        " << bli << '\n';
		std::cout << "Container ID:     " << fileBlock[bli].blockID << '\n';
		std::cout << "Container Offset: " << blockOffsets[bli] << '\n';
		std::cout << "Container Length: " << fileBlock[bli].blockLength << "\n\n";
	}
}

void DFextractor::printScript(uint16_t blockID) {
	std::cout << readScriptBlock(fileBlock[blockID]) << std::endl;
}

void DFextractor::printAllDialougTexts() {

	if (fileType != DFPUP) {
		std::cout << "Error: puppet dialouges only exist in PUP files!\n";
		return;
	}
	for (short i = 0; i < audioTextCount; i++) {

		std::cout << "string " << i << ", ID: " << strBlocks0[i].ident << ", Text: " << strBlocks0[i].text << '\n';
	}
}

void DFextractor::printLookUpTable() {

	std::cout << "commands based on DreamFactory version 4.0\n";
	for (auto& scr : ScriptCommands)
		std::cout << scr.first << '\t' << scr.second << '\n';
}


void DFextractor::printColorPalette() {

	struct ColorPalette {
		int16_t index;
		int16_t RGB[3];
	};

	ColorPalette CP[256];

	if (fileType == DFSET)
		memcpy(CP, fileBlock[0].blockdata + 0xF2, sizeof(ColorPalette) * 256);
	else if (fileType == DFPUP)
		memcpy(CP, fileBlock[0].blockdata + 0x3A, sizeof(ColorPalette) * 256);
	else if (fileType == DFMOV)
		memcpy(CP, fileBlock[0].blockdata + 0x6C, sizeof(ColorPalette) * 256);
	else if (fileType == DFSTG)
		memcpy(CP, fileBlock[0].blockdata + 0x38, sizeof(ColorPalette) * 256);
	else {
		std::cout << "Error: No color palette detected!\n";
		return;
	}


	for (int16_t color = 0; color < 256; color++) {
		std::cout << CP[color].index <<
			":\t R: " << (unsigned)*(((uint8_t*)&CP[color].RGB[0]) + 1) <<
			"\t G: " << (unsigned)*(((uint8_t*)&CP[color].RGB[1]) + 1) <<
			"\t B: " << (unsigned)*(((uint8_t*)&CP[color].RGB[2]) + 1) << "\n";
	}
}

void DFextractor::saveFileName(std::string fileName) {

	std::string::size_type pos = fileName.rfind('/');
	if (pos != std::string::npos) {
		fileName = fileName.substr(pos + 1, fileName.length());
	}

	pos = fileName.rfind('\\');
	if (pos != std::string::npos) {
		fileName = fileName.substr(pos + 1, fileName.length());
	}

	currentFileName.assign(fileName);

	return;
}

////////////////////////////////
//// SOME WRITE TO FILE FUNCTIONS
////////////////////////////////

void DFextractor::writeScript(const std::string& path, uint16_t blockID, const std::string& name) {
	
	std::string fileName(path);
	fileName.push_back('/');
	fileName.append(name);
	// if no name was given, attach atleast a generic name with the container ID
	if (name.empty()) {
		fileName.append("Script");
		fileName.append(std::to_string(blockID));
	}
	fileName.append(".txt");

	const std::string foutput = readScriptBlock(fileBlock[blockID]);
	// sometimes the script containers are just empty. no point to write a file then
	if (!foutput.empty()) {
		std::ofstream output(fileName, std::ios::binary, std::ios::trunc);
		output << foutput;
		output.close();
		fileCounter++;
	}

}

void DFextractor::writeBlockFiles(const std::string& path) {
	for (int32_t bli = 0; bli < hd.blockCount; bli++) {

		std::string FileName(path);
		FileName.push_back('/');
		FileName.append(currentFileName);
		FileName.push_back('_');
		FileName.append("container ");
		FileName.append(std::to_string(bli));
		FileName.append(".dta");

		std::ofstream fileEXT(FileName, std::ios::binary);
		fileEXT.write(fileBlock[bli].blockdata, fileBlock[bli].blockLength);
		fileEXT.close();
	}
}

void DFextractor::writeAllDialougTexts(const std::string& path) {

	std::string fileName(path);
	fileName.append("\\AUDIO\\texts.csv");
	std::ofstream output(fileName, std::ios::binary, std::ios::trunc);
	// header:
	output << "\"ID\", " << "\"container\", " << "\"File name\", " << "\"Text\"\n";
	for (short i = 0; i < audioTextCount; i++) {
		output << i + 1 << ", " << strBlocks0[i].audioLocation << ", \"" << strBlocks0[i].ident << ".wav\", \"" << strBlocks0[i].text << "\"\n";
		//output <<  i+1 << "\tAudioLoc: "<< strBlocks0[i].audioLocation << "\tFile name: " << strBlocks0[i].ident << ".wav\tText: " << strBlocks0[i].text << '\n';
	}
	output.close();
	fileCounter++;
}


bool DFextractor::writeFrame(const std::string& path, int32_t blockID, bool brightness, const std::string& customFileName) {
  /*
  This function is one of the most powerful and complex ones.
  It works 99% relaible, but right now it is a huge mess. The amount of loops and if-statements is insane.
  If you know C++ and want to help me improving it, feel free to contact me, MeToX.
  */

	auto getMsb = [](int32_t flags)
	{
		uint32_t bitindex = 0x80000000;

		for (int8_t i = 15; i >= 0; i--) {
			if (flags & bitindex) {
				return i;
			}
			bitindex /= 2;
		}

		return (int8_t)0;
	};


	// lookup table for difference offsets
	static constexpr uint8_t diffs[16]{ 8, 8 , 8, 8, 8, 8, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0};

	int16_t height = *(int16_t*)fileBlock[blockID].blockdata;
	int16_t width = *(int16_t*)(fileBlock[blockID].blockdata + 2);

	const int32_t totalSize = height * width;

	uint8_t* currIN = (uint8_t*)fileBlock[blockID].blockdata + 4;

	containerDataBuffer.resize(totalSize);
	//uint8_t* frameOutput = new uint8_t[totalSize];
	uint8_t* frameOutput = containerDataBuffer.GetContent();

	uint8_t* currOUT = frameOutput;

	
	bool breakOut = false;
	bool strangeUniqueFunc = false;
	int8_t flagBitPos{ 0 };
	bool weird = false;
	int16_t heightRest{ height };
	int16_t lookUpOffset{ 0 };
	while (heightRest-- > 0) {


		currIN++;
		if (weird) 
			weird = false;
		else
			currIN += 2; // korrektur

		strangeUniqueFunc = true;

		int32_t chunkSize{ 0 };
		uint32_t flags{ 0 };


		uint8_t bitindex;
		int8_t offset;
		
		int16_t widthRest{ width };
		int16_t offsetSingle{ 1 };

		//while (currOUT < frameOutput+totalSize) {
		while ((height - heightRest)*width != currOUT - frameOutput) {	

			if (--chunkSize <= 0) {		// EIP 0044201A ( WENN NULL! )
				// go to EIP 00442092 if condition is true
				offset = flagBitPos;
				flagBitPos = 0; // reset flagbit position to zero
				offsetSingle = 1;

				// adjust offsets
				currIN -= 2;
				//if (offset == 0)
					//std::cout << "TEST\n";
				if (offset >= 8) {
					currIN--;
					if (offset >= 16) currIN--;	// another decrement? Possibly?
				}

				// very unelegant... but it works
				if (strangeUniqueFunc) {
					//std::cout << height-heightRest <<  ": Selection: " << (int)*(currIN - 1) << '\n';
					
					while (true) {
						lookUpOffset = (int32_t)*(currIN - 1);
						switch (lookUpOffset) {
						case 4:
							//std::cout << "Error: undefined function: LookUpTable = 4\n";

							memcpy(currOUT, currIN, width);
							currOUT += width;
							currIN += width+1;
							heightRest--;


							if (heightRest)	continue;
							breakOut = true;
							break;
						case 8:		// to 41DEB
							lookUpOffset = width * 4;	// 4x = ebp-18
							break;
						case 12:	// to 41DF5
							lookUpOffset = width * 3;	// 3 = ebp-10
							break;	// to 41DFF
						case 16:
							lookUpOffset = width * 2;	// 2x = ebp-14
							break;
						case 20:	// to 41E09
							lookUpOffset = width;	// (1x = ebp-04)
							break;
						case 24:	// to 41E10
							lookUpOffset = -width;
							break;
						case 28:	// to 41E17
							lookUpOffset = width * -2;
							break;
						case 32:	// to 41E1E
							lookUpOffset = width * -3;
							break;
						case 36:	// to 41E25
							lookUpOffset = width * -4;
							break;
						default:
							switch (lookUpOffset) {
							case 40:	// to 41E2C
								// SKIP ROWS
								break;
							case 44:	// to 41E34
								lookUpOffset = width * 4;
								memcpy(currOUT, currOUT - lookUpOffset, width);
								// jump to TI.EXE+41E72
								break;
							case 48:	// to 41E3C
								lookUpOffset = width * 3;
								memcpy(currOUT, currOUT - lookUpOffset, width);
								// jump to TI.EXE+41E72
								break;
							case 52:	// to 41E44
								lookUpOffset = width * 2;
								memcpy(currOUT, currOUT - lookUpOffset, width);
								// jump to TI.EXE+41E72
								break;
							case 56:	// to 41E4C
								lookUpOffset = width;
								memcpy(currOUT, currOUT - lookUpOffset, width);
								// jump to TI.EXE+41E72
								break;
							case 60:	// to 41E54
								lookUpOffset = -width;
								memcpy(currOUT, currOUT - lookUpOffset, width);
								// jump to TI.EXE+41E72
								break;
							case 64:	// to 41E5C
								lookUpOffset = width * -2;
								memcpy(currOUT, currOUT - lookUpOffset, width);
								// jump to TI.EXE+41E72
								break;
							case 68:	// to 41E64
								lookUpOffset = width * -3;
								memcpy(currOUT, currOUT - lookUpOffset, width);
								// jump to TI.EXE+41E72
								break;
							case 72:	// to 41E6C
								lookUpOffset = width * -4;
								memcpy(currOUT, currOUT - lookUpOffset, width);
								// jump to TI.EXE+41E72
								break;
							default:
								std::cout << "ERROR: LookupTable out of range!\n";
							}

							currOUT += width;
							currIN++;

							if (heightRest) {
								heightRest--;
								continue;
							}
              
							breakOut = true;
							break;

						}
            
						if (!breakOut) strangeUniqueFunc = false;
						break;
					}

					if (strangeUniqueFunc)
						break;
				}
					

				while (widthRest > 0) {	// EIP==0x00441FC1

					chunkSize = *currIN++;

					if (chunkSize & 1) {	// pos 41F33
						chunkSize >>= 1;

						if (chunkSize & 1) {	// pos 41F72
							chunkSize >>= 1;

							
							if (chunkSize & 1) {	// pos 41F9A
								// copy content from defined by the compressed file
								// here it can be from a specific position
								chunkSize >>= 1;
								if (!chunkSize) {
									chunkSize += *currIN++; // might be incorrect
									chunkSize += 32;
								}

								widthRest -= chunkSize;
								int16_t insertPosOffset = *(int16_t*)currIN;
								currIN += 2;

								memcpy(currOUT, currOUT - insertPosOffset, chunkSize);
								currOUT += chunkSize;

							}
							else {
								// copy content from exactly the width length before
								// here fixed width, always exactly one line above
								chunkSize >>= 1;
								if (!chunkSize) {
									chunkSize += *currIN++;
									chunkSize += 32;
								}

								widthRest -= chunkSize;

								memcpy(currOUT, currOUT - lookUpOffset, chunkSize);
								currOUT += chunkSize;
							}

							continue;
						}
						else
							chunkSize >>= 1;
						


						if (chunkSize & 1) {	// pos 41F53
							chunkSize >>= 1;

							if (!chunkSize) {
								chunkSize += *currIN++;
								chunkSize += 32;
							}

							widthRest -= chunkSize;

							memcpy(currOUT, currIN, chunkSize);
							currOUT += chunkSize;
							currIN += chunkSize;
							
						}
						else {
							chunkSize >>= 1;
							// LETZTE EDI 0x46703AC
							if (!chunkSize) {
								chunkSize += *currIN++;
								chunkSize += 32;
							}
							widthRest -= chunkSize;
							offsetSingle = lookUpOffset;
							break;

						}


						continue;
					}
					chunkSize >>= 1;

					if (chunkSize & 1) {	// pos 41EEF
						chunkSize >>= 1;

						if (chunkSize & 1) {	// pos 41F05
							chunkSize >>= 1;

							if (!chunkSize) {
								chunkSize += *currIN++;
								chunkSize += 32;
							}

							widthRest -= chunkSize;

							memset(currOUT, *currIN++, chunkSize);
							currOUT += chunkSize;
              
						}
						else {
							chunkSize >>= 1;
							//std::cout << "Error: Pos 41F05 FALE\n" << currOUT - frameOutput;

							if (!chunkSize) {
								chunkSize += *currIN++;
								chunkSize += 32;
							}
							currOUT += chunkSize;
							widthRest -= chunkSize;
							//offsetSingle = lookUpOffset;
						}

						continue;
					}
					chunkSize >>= 1;

					// if carry bit is set, do something
					if (chunkSize & 1) {	// Pos 41EBF
						chunkSize >>= 1;

						if (!chunkSize) {
							chunkSize += *currIN++;
							chunkSize += 32;
						}

						//currIN += chunkSize; // ???
						widthRest -= chunkSize;

						memset(currOUT, *(currOUT - 1), chunkSize);
						currOUT += chunkSize;

						// EIP 441ED8

						continue;
					}
					chunkSize >>= 1;
					// if none of the last 3 bits are set, break out from this loop:
					if (!widthRest) break;

					if (!chunkSize) {	// EIP 00441EA7
						chunkSize += *currIN++;
						chunkSize += 32;
					}

					widthRest -= chunkSize--;		// EIP 00441EAE
					*currOUT++ = *currIN++;		

					break;

				}
				if (!widthRest) {	
					if ((height - heightRest)*width == currOUT - frameOutput)
						break;
					else
						weird = true;
				}

				*(((uint8_t*)&flags) + 3) = *currIN++;	// EIP 00441FD7
				*(((uint8_t*)&flags) + 2) = *currIN++;	// EIP 00441FDD


			}


			// fill up the flags bits if they are zero
			if (!flagBitPos) {
				*(((uint8_t*)&flags) + 1) = *currIN++;
				*((uint8_t*)&flags) = *currIN++;

				flagBitPos = 16;
			}

			// iterate though bit flags




			bitindex = getMsb(flags);	// EIP 0x00441FEC

			if (bitindex == 0xF) {	// EIP 0x00441FF2

				*currOUT++ = *(currOUT - offsetSingle);
				flags <<= 1;

				flagBitPos--;
				// Jump to EIP 00442092 if flagBitPos is zero!
			}
			else {

				if (bitindex < 0x8) { // 5
					
					*(((uint8_t*)&flags) + 2) += *(currOUT - offsetSingle);
					*currOUT++ = *(((uint8_t*)&flags) + 2);

					offset = 16;
					// EIP==0x0044204E

				}
				else {

					bitindex--;

					offset = diffs[bitindex];

					// manchmal PLUS offset? Prüfe 4670022
					if (flags & (1 << bitindex + 16))
						*currOUT++ = (*(currOUT - offsetSingle)) + offset;
					else
						*currOUT++ = (*(currOUT - offsetSingle)) - offset;


					offset += 2;

					// here it checks if flagBitPos is equal with offset
					// if not, it does:

					if (offset <= flagBitPos) {
						flagBitPos -= offset;
						flags <<= offset;
						continue;
					}
				}
				// swap
				int8_t tmp = offset;
				offset = flagBitPos;
				flagBitPos = tmp;

				flags <<= offset;

				flagBitPos -= offset;

				*(((uint8_t*)&flags) + 1) = *currIN++;
				*((uint8_t*)&flags) = *currIN++;
				offset = 16;

				tmp = offset;
				offset = flagBitPos;
				flagBitPos = tmp;


				flagBitPos -= offset;
				flags <<= offset;
				// normalize flagPos
				
			}

		}
	}


	// when here, the image is completly written to memory.
	struct ColorPalette {
		int16_t index;
		int16_t RGB[3];
	};

	ColorPalette CP[256];

	if (fileType == DFSET)
		memcpy(CP, fileBlock[0].blockdata + 0xF2, sizeof(ColorPalette) * 256);
	else if (fileType == DFPUP)
		memcpy(CP, fileBlock[0].blockdata + 0x3A, sizeof(ColorPalette) * 256);
	else if (fileType == DFMOV)
		memcpy(CP, fileBlock[0].blockdata + 0x6C, sizeof(ColorPalette) * 256);
	else if (fileType == DFSTG)
		memcpy(CP, fileBlock[0].blockdata + 0x38, sizeof(ColorPalette) * 256);
	// colors in CP array set

	constexpr int32_t bmpHeaderSize{ 1078 };
	uint8_t* bmpHeader = new uint8_t[bmpHeaderSize-1024];
	uint8_t* bmpCurr = bmpHeader;
	memcpy(bmpCurr, "BM", 2);
	bmpCurr += 2;
	*(int32_t*)bmpCurr = bmpHeaderSize + (width*height);	// total file size
	bmpCurr += 4;
	*(int32_t*)bmpCurr = 0;	// additional data
	bmpCurr += 4;
	*(int32_t*)bmpCurr = bmpHeaderSize;	// begin of raw pixel data
	bmpCurr += 4;
	*(int32_t*)bmpCurr = 40;	// sub header size
	bmpCurr += 4;
	*(int32_t*)bmpCurr = width;
	bmpCurr += 4;
	*(int32_t*)bmpCurr = height;
	bmpCurr += 4;
	*(int16_t*)bmpCurr = 1;
	bmpCurr += 2;
	*(int16_t*)bmpCurr = 8;	// bit per pixel
	bmpCurr += 2;
	*(int32_t*)bmpCurr = 0;	// compression methode
	bmpCurr += 4;
	*(int32_t*)bmpCurr = width * height;	// raw pixel size
	bmpCurr += 4;
	*(int32_t*)bmpCurr = 2834;	// pixel per metre horizontal
	bmpCurr += 4;
	*(int32_t*)bmpCurr = 2834;	// pixel per metre vertical
	bmpCurr += 4;
	*(int32_t*)bmpCurr = 256;	// number of colors
	bmpCurr += 4;
	*(int32_t*)bmpCurr = 0;	// number of important colors. I use 0 for ALL
	bmpCurr += 4;


	std::string fileName(path);
	fileName.push_back('/');
	if (customFileName.empty()) {
		fileName.append(currentFileName);
		fileName.append("frame_");
		fileName.append(std::to_string(blockID));
	}
	else {
		fileName.append(customFileName);
	}

	fileName.append(".bmp");

	std::ofstream testFile(fileName, std::ios::binary, std::ios::trunc);
	testFile.write((char*)bmpHeader, bmpHeaderSize - 1024);

	for (int16_t color = 0; color < 256; color++) {
		char output[4];
		
		// check if its a white tone, which is indicated with FFFF FFFF FFFF
		static int16_t check[3]{ 0xFFFF,0xFFFF,0xFFFF };
		if (!memcmp(CP[color].RGB, check, 6)) {
			*output = 0;
			*(output + 1) = 0;
			*(output + 2) = 0;
		}
		else {
			*output = *(((uint8_t*)&CP[color].RGB[2]) + 1);
			*(output + 1) = *(((uint8_t*)&CP[color].RGB[1]) + 1);
			*(output + 2) = *(((uint8_t*)&CP[color].RGB[0]) + 1);
		}

		*(output + 3) = 0;

		testFile.write(output, 4);
	}

	for (int16_t i = 0; i < height; i++) {
		testFile.write((char*)frameOutput + ((height - (i + 1))*width), width);
	}

	testFile.close();
	fileCounter++;

	delete[] bmpHeader;

	return true;
}

void DFextractor::printAudioInfo() {

	std::cout << "Audio information:\n";
	std::cout << "Total data blocks: " << hd.blockCount << '\n';
	if (AudioPtr->containsAudioData) {
		if (AudioPtr->audioLoopChunkCount) {
			std::cout << "Block 1 information (combined loops):\n";
			for (int32_t bl = 0; bl < AudioPtr->audioLoopChunkCount; bl++) {

				std::cout << "Block ID: " << AudioPtr->audioLoopChunks[bl].chunkBlockID
					<< ", Loop ID: " << bl + 1
					<< ", unknown: " << AudioPtr->audioLoopChunks[bl].unknownInt
					<< ", param: " << AudioPtr->audioLoopChunks[bl].unknownBool
					<< ", name: " << AudioPtr->audioLoopChunks[bl].identifier
					<< ", Hertz: " << *(int32_t*)(fileBlock[AudioPtr->audioLoopChunks[bl].chunkBlockID].blockdata+28)
					<< ", Ratio: " << *(int16_t*)(fileBlock[AudioPtr->audioLoopChunks[bl].chunkBlockID].blockdata + 33)
					<< '\n';

			}

			std::cout << "Loop Order: ";
			for (uint16_t loops = 0; loops < AudioPtr->totalLoopsChunks; loops++)
				std::cout << AudioPtr->chunkOrder[loops] << ", ";
			std::cout << '\n';
		}
		else
			std::cout << "Block 1 does not contain additional information." << std::endl;


		if (AudioPtr->audioSingleChunkCount) {
			std::cout << "Block " << AudioPtr->chunkInfo2Loc << " information (single audio files):\n";
			for (int32_t bl = 0; bl < AudioPtr->audioSingleChunkCount; bl++) {

				std::cout << "Block ID: " << AudioPtr->audioSingleChunks[bl].chunkBlockID
					<< ", Single ID: " << bl + 1
					<< ", unknown: " << AudioPtr->audioSingleChunks[bl].unknownInt
					<< ", param: " << AudioPtr->audioSingleChunks[bl].unknownBool
					<< ", name: " << AudioPtr->audioSingleChunks[bl].identifier
					<< ", Hertz: " << *(int32_t*)(fileBlock[AudioPtr->audioSingleChunks[bl].chunkBlockID].blockdata + 28)
					<< ", Ratio: " << *(int16_t*)(fileBlock[AudioPtr->audioSingleChunks[bl].chunkBlockID].blockdata + 33)
					<< '\n';
			}
		}
		else
			std::cout << "Block " << AudioPtr->chunkInfo2Loc << " does not contain additional information." << std::endl;


	} else
		std::cout << "This file has no audio data." << std::endl;


}

void DFextractor::printMovFrameInfo() {
	movFramePtr->printFrames();
}

void DFextractor::printPupInfo() {

	for (int32_t i = 0; i < audioTextCount; i++) {

		std::cout << "index:           " << i << '\n';
		std::cout << "unknownInt1:     " << strBlocks0[i].unknownInt1 << '\n';
		std::cout << "unknownShort1:   " << strBlocks0[i].unknownShort1 << '\n';
		std::cout << "unknownShort1:   " << strBlocks0[i].unknownShort2 << '\n';
		std::cout << "unknownInt2:     " << strBlocks0[i].unknownInt2 << '\n';
		std::cout << "unknownInt3:     " << strBlocks0[i].unknownInt3 << '\n';

		std::cout << "Audio Container: " << strBlocks0[i].audioLocation << '\n';
		std::cout << "animation count: " << strBlocks0[i].animLogic << '\n';	// number of elements
		std::cout << "identifation:    " << strBlocks0[i].ident << '\n';
		std::cout << std::endl;
	}
}

void DFextractor::writeSpecificAudio(int16_t blockID) {

	
	int32_t hertz = *(int32_t*)(fileBlock[blockID].blockdata + 28);
	int32_t fileSize = *(int32_t*)(fileBlock[blockID].blockdata + 36);

	waveHeader header(fileSize, hertz);

	int8_t* toOutput = new int8_t[fileSize + header.headerSize +3];	//plus 3 byte because of overflow of decoder
	memcpy(toOutput, header.StartChunkID, header.headerSize);

	audioDecoder(fileSize, (int8_t*)fileBlock[blockID].blockdata, toOutput + header.headerSize);

	std::string filname(currentFileName);
	filname.push_back('/');
	filname.append("Block");
	filname.append(std::to_string(blockID));
	filname.append(".wav");
	std::ofstream file(filname, std::ios::binary | std::ios::trunc);
	file.write((char*)toOutput, fileSize + header.headerSize);
	file.close();

	delete[] toOutput;
}

void DFextractor::writeAudioRange(int16_t from, int16_t to) {

	std::string filname(currentFileName);
	filname.push_back('/');
	filname.append("Block");
	filname.append(std::to_string(from));
	filname.append("to");
	filname.append(std::to_string(to));
	filname.append(".wav");
	

	// get total file size
	int32_t totalFileSize{ 0 };
	const int32_t fileSizeCount{ to - from + 1 };
	int32_t* fileSizes = new int32_t[fileSizeCount];

	for (int16_t blockID = 0; blockID < fileSizeCount; blockID++) {
		fileSizes[blockID] = *(int32_t*)(fileBlock[blockID+from].blockdata + 36);
		totalFileSize += fileSizes[blockID];
	}

	// just use first block for hertz (not optimal)
	waveHeader header(totalFileSize, *(int32_t*)(fileBlock[from].blockdata + 28));

	int8_t* toOutput = new int8_t[totalFileSize + header.headerSize+3];	// duno why but decoder outputs 3 extra bytes with no content


	memcpy(toOutput, header.StartChunkID, header.headerSize);

	int32_t currPos{ header.headerSize };
	for (int16_t blockID = from; blockID <= to; blockID++) {
		if (!audioDecoder(fileSizes[blockID-from], (int8_t*)fileBlock[blockID].blockdata, toOutput + currPos))
			std::cout << "ERROR: Bad format!\n";
		currPos += fileSizes[blockID-from];
	}



	std::ofstream file(filname, std::ios::binary | std::ios::trunc);
	file.write((char*)toOutput, totalFileSize + header.headerSize);
	file.close();

	delete[] fileSizes;
	delete[] toOutput;
}

void DFextractor::printSetInfo() {
	std::cout << "Block 4: \n";
	int32_t allSceneframes{ 0 };
	std::cout << sceneCount << " Scenes: \n";
	for (int32_t sc = 0; sc < sceneCount; sc++) {
		//SceneData scd(sc, this);
		std::cout << "\nSCENE HEADER:\n";
		scenesPtr[sc]->printSceneData();
		std::cout << "\nVIEW TABLE:\n";
		scenesPtr[sc]->printViewData();
		std::cout << "\nTURN RIGHT:\n";
		scenesPtr[sc]->printTurnData(0);
		std::cout << "\nTURN LEFT:\n";
		scenesPtr[sc]->printTurnData(1);

		std::cout << "\nRIGHT TURN TRANSITIONS:\n";
		scenesPtr[sc]->printAllTransitions(0);
		std::cout << "\nLEFT TURN TRANSITIONS:\n";
		scenesPtr[sc]->printAllTransitions(1);
		std::cout << "\nFRAME SUMMARY:\n";
		allSceneframes += scenesPtr[sc]->printFrameInformation();
		std::cout << std::endl;
	}
	std::cout << "total frames: " << allSceneframes << '\n';
}

void DFextractor::printStageInfo() {
	StgDataPtr->printFrameInfo();
}


bool DFextractor::writeAllAudioPUP(const std::string& path) {

	for (short i = 0; i < audioTextCount; i++) {

		int32_t hertz = *(int32_t*)(fileBlock[strBlocks0[i].audioLocation].blockdata + 28);
		int32_t fileSize = *(int32_t*)(fileBlock[strBlocks0[i].audioLocation].blockdata + 36);

		waveHeader header(fileSize, hertz);

		containerDataBuffer.resize(fileSize + header.headerSize+3);
		//int8_t* toOutput = new int8_t[fileSize + header.headerSize];
		memcpy(containerDataBuffer.GetContent(), header.StartChunkID, header.headerSize);

		uint8_t* test = containerDataBuffer.GetContent();
		if (!audioDecoder(fileSize, (int8_t*)fileBlock[strBlocks0[i].audioLocation].blockdata, (int8_t*)containerDataBuffer.GetContent() + header.headerSize)) {
			fileType = DFERROR;
			lastError = ERRDECODEAUDIO;
			return false;
		}


		std::string filname(path);
		filname.append("/AUDIO");
		CreateDirectoryA(filname.c_str(), NULL);
		filname.push_back('/');
		filname.append(strBlocks0[i].ident);
		filname.append(".wav");
		std::ofstream file(filname, std::ios::binary | std::ios::trunc);
		file.write((char*)containerDataBuffer.GetContent(), fileSize + header.headerSize);
		file.close();
		fileCounter++;
	}

	// little extra: output dialoug as textfile as well
	writeAllDialougTexts(path);

	return true;
}

bool DFextractor::writeAllAudio(const std::string& path) {


	if (AudioPtr->containsAudioData) {

		if (AudioPtr->audioLoopChunkCount) {

			// first we need the total file size of all Audioloops together
			int32_t totalFileSize{ 0 };
			int32_t* fileSizes = new int32_t[AudioPtr->totalLoopsChunks];
			for (int32_t loop = 0; loop < AudioPtr->totalLoopsChunks; loop++) {

				fileSizes[loop] = *(int32_t*)(fileBlock[AudioPtr->audioLoopChunks[AudioPtr->chunkOrder[loop] - 1].chunkBlockID].blockdata + 36);
				totalFileSize += fileSizes[loop];
			}

			// get hertz rate from first block. 
			int32_t hertz = *(int32_t*)(fileBlock[AudioPtr->audioLoopChunks[0].chunkBlockID].blockdata + 28);
			// Hertz fix: priotize bigger
			if (AudioPtr->audioLoopChunkCount > 1) {
				int32_t hertzTmp = *(int32_t*)(fileBlock[AudioPtr->audioLoopChunks[1].chunkBlockID].blockdata + 28);
				if (hertz < hertzTmp)
					hertz = hertzTmp;
			}
			 
			
			// construct audio name based on blockZero
			std::string filname(path);
			filname.push_back('/');
			filname.append(AudioPtr->trackName);
			filname.append(".wav");

			waveHeader header(totalFileSize, hertz);

			containerDataBuffer.resize(totalFileSize + header.headerSize + 3);
			//int8_t* toOutput = new int8_t[totalFileSize + header.headerSize + 3]; // 3 extra overflow decoder bytes
			memcpy(containerDataBuffer.GetContent(), header.StartChunkID, header.headerSize);

			std::ofstream audioFile(filname, std::ios::binary | std::ios::trunc);

			int32_t current{ 0 };
			for (int32_t loop = 0; loop < AudioPtr->totalLoopsChunks; loop++) {
				if (!audioDecoder(fileSizes[loop], (int8_t*)fileBlock[AudioPtr->audioLoopChunks[AudioPtr->chunkOrder[loop] - 1].chunkBlockID].blockdata, (int8_t*)containerDataBuffer.GetContent() + header.headerSize + current)) {
					fileType = DFERROR;
					lastError = ERRDECODEAUDIO;
					return false;
				}
				current += fileSizes[loop];
			}
			audioFile.write((char*)containerDataBuffer.GetContent(), totalFileSize + header.headerSize);
			audioFile.close();
			fileCounter++;
			delete[] fileSizes;
			//delete[] toOutput;

		}
		//else
		//	std::cout << "No loop chunks included." << std::endl;


		// check if the file has single shots
		if (AudioPtr->audioSingleChunkCount) {

			for (int32_t shot = 0; shot < AudioPtr->audioSingleChunkCount; shot++) {

				int32_t blockID = AudioPtr->audioSingleChunks[shot].chunkBlockID;

				int32_t hertz = *(int32_t*)(fileBlock[blockID].blockdata + 28);
				int32_t fileSize = *(int32_t*)(fileBlock[blockID].blockdata + 36);

				waveHeader header(fileSize, hertz);

				containerDataBuffer.resize(fileSize + header.headerSize + 3);
				//int8_t* toOutput = new int8_t[fileSize + header.headerSize + 3]; // extra bytes decoder overflow
				memcpy(containerDataBuffer.GetContent(), header.StartChunkID, header.headerSize);

				if (!audioDecoder(fileSize, (int8_t*)fileBlock[blockID].blockdata, (int8_t*)containerDataBuffer.GetContent() + header.headerSize)) {
					fileType = DFERROR;
					lastError = ERRDECODEAUDIO;
					return false;
				}

				std::string filname(path);
				filname.push_back('/');
				// sometimes they have subfolders. detect those and ignore them
				std::string temp(AudioPtr->audioSingleChunks[shot].identifier);
				std::string::size_type pos = temp.find('/');
				if (pos == std::string::npos) {
					filname.append(temp);
				}
				else
					filname.append(temp.substr(pos + 1));

				filname.append(".wav");

				std::ofstream file(filname, std::ios::binary | std::ios::trunc);
				file.write((char*)containerDataBuffer.GetContent(), fileSize + 44);
				file.close();
				fileCounter++;
				//delete[] toOutput;
			}
		}
	}

	// operation succesful
	return true;
}

bool DFextractor::writeAllFramesSET(const std::string& dir) {

	int32_t allSceneframes{ 0 };
	for (int32_t sc = 0; sc < sceneCount; sc++)
		allSceneframes += scenesPtr[sc]->writeSceneFrames(dir);
	
	if (allSceneframes == 0) return false;

	return true;
}

bool DFextractor::writeAllFramesSTG(const std::string& dir) {
	std::string newFolder(dir);
	newFolder.append("/FRAMES");
	CreateDirectoryA(newFolder.c_str(), NULL);
	
	int32_t allSceneframes = StgDataPtr->writeSTGFrames(newFolder);

	if (!allSceneframes) return false;
	return true;
}

void DFextractor::writeScriptsSET(const std::string& dir) {

	writeScript(dir, 1, "Main");
	// additional script register
	// the ones I have seen there were only one
	// but there might be also sets with more
	// sometimes there is no secondary script container. check that first
	if (*(int32_t*)(fileBlock[6].blockdata) == 1) {
		int32_t location = *(int32_t*)(fileBlock[6].blockdata + 24);
		char secScript[15];
		memcpy(secScript, fileBlock[6].blockdata + 29, *(fileBlock[6].blockdata + 28));
		secScript[*(fileBlock[6].blockdata + 28)] = '\0';
		writeScript(dir, location, secScript);
	}


	for (int32_t sc = 0; sc < sceneCount; sc++) {
		scenesPtr[sc]->writeSceneScripts(dir);
	}

}

void DFextractor::writeScriptsPUP(const std::string& dir) {

	struct PupScripts {
		int32_t location;
		char scriptName[31];
	};

	int16_t pupScriptscount = *(int16_t*)(fileBlock[2].blockdata + 22);

	char* curr = fileBlock[2].blockdata + 24;

	for (int32_t i = 0; i < pupScriptscount; i++) {
		PupScripts ps;	// no reason to keep this in memory
		ps.location = *(int32_t*)(curr);
		curr += 4;
		curr += 4; // another unknown value here, mostly zero
		uint8_t strSize = *curr++;
		memcpy(ps.scriptName, curr, strSize);
		ps.scriptName[strSize] = '\0';
		curr += 31;

		writeScript(dir, ps.location, ps.scriptName);
	}

}

void DFextractor::writeScriptsSTG(const std::string& dir) {
	std::string newFolder(dir);
	newFolder.append("/SCRIPTS");
	CreateDirectoryA(newFolder.c_str(), NULL);
	writeScript(newFolder, 1, "Main");

	StgDataPtr->writeScripts(newFolder);
}

void DFextractor::writeScriptsBOOT(const std::string& dir) {
	std::string newFolder(dir);
	newFolder.append("/SCRIPTS");
	CreateDirectoryA(newFolder.c_str(), NULL);

	for (uint8_t i = 1; i < hd.blockCount; i++) {
		writeScript(newFolder, i, "cont" + std::to_string(i));
	}
}

bool DFextractor::writeAllFramesMOV(const std::string& dir) {

	std::string newFolder(dir);
	newFolder.append("/FRAMES");
	CreateDirectoryA(newFolder.c_str(), NULL);
	movFramePtr->writeFrames(this, newFolder);
	return true;
}


std::string DFextractor::getDialougText(const std::string& ident) {
	for (short i = 0; i < audioTextCount; i++) {
		if (!memcmp(ident.c_str(), strBlocks0[i].ident, strBlocks0[i].identLength + 1)) {
			std::string output(": \"");
			output.append(strBlocks0[i].text);
			output.push_back('"');
			return output;
		}
	}
	return std::string();
}


bool DFextractor::audioDecoder(int32_t& uncomprBlockSize, int8_t* soundFile, int8_t* decodedOutput) {

  /*
  If you want to decode the audio, this is the right place.
  It took me a long time to figure it out but here it is
  The function itself is already very optimized and works 100% reliable to all containers I have tested.
  */
  
	static constexpr int8_t StepSizeTable[256]{
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
		0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
		0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05,
		0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06,
		0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
		0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
		0x07, 0x07, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
		0xF8, 0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9,
		0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA,
		0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB,
		0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFC, 0xFC, 0xFC, 0xFC,
		0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFD, 0xFD,
		0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD,
		0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE,
		0xFE, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF
	};



	static constexpr int8_t IndexTable[256]{
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD,
		0xFE, 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0xF8, 0xF9, 0xFA, 0xFB,
		0xFC, 0xFD, 0xFE, 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0xF8, 0xF9,
		0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
		0x06, 0x07, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0x00, 0x01, 0x02, 0x03,
		0x04, 0x05, 0x06, 0x07, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0x00, 0x01,
		0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD,
		0xFE, 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0xF8, 0xF9, 0xFA, 0xFB,
		0xFC, 0xFD, 0xFE, 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0xF8, 0xF9,
		0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
		0x06, 0x07, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0x00, 0x01, 0x02, 0x03,
		0x04, 0x05, 0x06, 0x07, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0x00, 0x01,
		0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD,
		0xFE, 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0xF8, 0xF9, 0xFA, 0xFB,
		0xFC, 0xFD, 0xFE, 0xFF
	};



	uint16_t indexFlags;
	uint32_t flagParams;

	int8_t *blockFirstByte;
	int8_t *PointerToFilePos;

	int16_t step;
	uint16_t index;
	uint32_t *resultOutput;
	uint32_t *tmpResult;
	uint32_t indexParam;
	//int32_t uncomprBlockSize;
	int8_t byteRead;
	int32_t offsetToBlock;

	// check if block format is correct 00 00 01 00
	if (0x00010000 != *(int32_t*)soundFile) return false;

	//blockFirstByte = soundFile;
	blockFirstByte = soundFile + *(int*)(soundFile + 44);

	PointerToFilePos = blockFirstByte + 1;
	resultOutput = (uint32_t *)(decodedOutput + 1);
	byteRead = *blockFirstByte;

	*decodedOutput = byteRead + 0x40;
	indexParam = (uint32_t)byteRead;


	// iterate until the known end 
	while (resultOutput < (uint32_t*)(decodedOutput + uncomprBlockSize)) {
		byteRead = *PointerToFilePos++;

		// check if last bit is set
		if ((byteRead & 0x80) == false) {
			*(int8_t *)resultOutput = byteRead + 0x40;
			resultOutput = (uint32_t *)((int32_t)resultOutput + 1);
			indexParam = (uint32_t)byteRead;
		}
		else {
			// check if second last bit is set
			if ((byteRead & 0x40) == false) {
				flagParams = byteRead & 0xffff003f;
				tmpResult = resultOutput;

				do {
					byteRead = *PointerToFilePos++;
					step = (int16_t)indexParam + (int16_t)StepSizeTable[(uint8_t)byteRead];
					resultOutput = (uint32_t *)((int32_t)tmpResult + 2);
					*(int8_t *)tmpResult = (int8_t)step + 0x40;

					index = step + IndexTable[(uint8_t)byteRead];
					indexParam = (uint32_t)index;
					indexFlags = (int16_t)flagParams - 1;
					flagParams = (uint32_t)indexFlags;

					*(int8_t *)((int32_t)tmpResult + 1) = (int8_t)index + 0x40;
					tmpResult = resultOutput;
				} while ((int16_t)indexFlags >= 0);
			}
			else {
				tmpResult = resultOutput;

				indexFlags = (byteRead & 0x3f) + 1;
				flagParams = indexParam + 0x40 & 0xff;

				uint32_t subCheck1 = (uint32_t)(indexFlags >> 2);
				uint32_t subCheck2 = flagParams | (indexParam + 0x40 & 0xff) << 8;

				while (subCheck1--) {
					*tmpResult++ = subCheck2 << 0x10 | subCheck2;
				}
				subCheck2 = indexFlags & 3;
				while (subCheck2--) {

					*tmpResult = (int8_t)flagParams;
					// only step one, instead of 4
					tmpResult = (uint32_t *)((int32_t)tmpResult + 1);
				}
				resultOutput = (uint32_t *)((int32_t)resultOutput + (uint32_t)indexFlags);
			}
		}
	}
	if (resultOutput != (uint32_t *)(decodedOutput + uncomprBlockSize)) {
		std::cout << "ERROR: Container Size different!\n";
		return false;
	}
	return true;
}

const char* DFextractor::GetDFerrorString(int32_t& errorCode) {
  // Error handler: Annoying, but necessary.

	constexpr int32_t errorCount{ 14 };
	if (errorCode > errorCount) return "invalid Error code!";
	static const char* errorMsgs[errorCount]
	{
		/* 0*/	{"No errors."},
		/* 1*/	{"File format not supported!"},
		/* 2*/	{"This file has no scripts to extract!"},
		/* 3*/	{"There is no known audio source!"},
		/* 4*/	{"There are no frames to extract!"},
		/* 5*/	{"Can't read audio container!"},
		/* 6*/	{"Can't read frame container!"},
		/* 7*/	{"No containers to initiate!"},
		/* 8*/	{"Not supported yet!"},
		/* 9*/	{"File not found!"},
		/*10*/	{"Can't read file containers!"},
		/*11*/	{"Can't decode audio container!"},
		/*12*/  {"Currently only SET & MOV files from DF version 4.0 are supported!"},
		/*13*/  {"File suffix not specified!"}
	};

	return errorMsgs[errorCode];
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DFextractor::DFFile::DFFile(DFextractor* DFinst, const std::string& path)
	: DFinst(DFinst), pathToOutput(path) {}

int32_t DFextractor::DFFile::analyze() {
	return 1;
}

int32_t DFextractor::DFFile::writeScripts() {
	return 2;
}

int32_t DFextractor::DFFile::writeAudio() {
	return 3;
}

int32_t DFextractor::DFFile::writeFrames() {
	return 4;
}

int32_t DFextractor::DFFile::initContainer() {
	return 7;
}

////////////////////////////////////
//// AUDIO FILES: TRK, SFX, 11K ////
////////////////////////////////////
int32_t DFextractor::DFAudioFile::initContainer() {
	DFinst->initAudio();
	return 0;
}

int32_t DFextractor::DFAudioFile::analyze() {
	DFinst->printAudioInfo();
	return 0;
}

int32_t DFextractor::DFAudioFile::writeAudio() {
	if (!DFinst->writeAllAudio(pathToOutput))
		return 5;
	return 0;
}

////////////////////
////  MOV FILE  ////
////////////////////
int32_t DFextractor::DFMovFile::initContainer() {
	DFinst->initMOV();
	return 0;
}

int32_t DFextractor::DFMovFile::analyze() {
	DFinst->printAudioInfo();
	DFinst->printMovFrameInfo();
	return 0;
}

int32_t DFextractor::DFMovFile::writeAudio() {
	if (!DFinst->writeAllAudio(pathToOutput))
		return 5;
	return 0;
}

int32_t DFextractor::DFMovFile::writeFrames() {
	if (!DFinst->writeAllFramesMOV(pathToOutput))
		return 6;
	return 0;
}

///////////////////
////  PUPPETS  ////
///////////////////

int32_t DFextractor::DFPupFile::initContainer() {
	DFinst->readPuppetStrings();
	return 0;
}

int32_t DFextractor::DFPupFile::analyze() {
	DFinst->printPupInfo();
	return 0;
}

int32_t DFextractor::DFPupFile::writeScripts() {
	DFinst->writeScriptsPUP(pathToOutput);
	return 0;
}

int32_t DFextractor::DFPupFile::writeAudio() {
	if (!DFinst->writeAllAudioPUP(pathToOutput))
		return 5;
	return 0;
}

int32_t DFextractor::DFPupFile::writeFrames() {
	// TODO!
	return 8;
}

///////////////////
////  SETS     ////
///////////////////

int32_t DFextractor::DFSetFile::initContainer() {
	DFinst->initSET();
	return 0;
}

int32_t DFextractor::DFSetFile::analyze() {
	DFinst->printSetInfo();
	return 0;
}

int32_t DFextractor::DFSetFile::writeScripts() {
	DFinst->writeScriptsSET(pathToOutput);
	return 0;
}

int32_t DFextractor::DFSetFile::writeFrames() {
	if (!DFinst->writeAllFramesSET(pathToOutput))
		return 6;
	return 0;
}

////////////////
////  STG   ////
////////////////

int32_t DFextractor::DFStageFile::initContainer() {
	DFinst->initSTG();
	return 0;
}

int32_t DFextractor::DFStageFile::analyze() {
	DFinst->printStageInfo();
	return 0;
}

int32_t DFextractor::DFStageFile::writeScripts() {
	DFinst->writeScriptsSTG(pathToOutput);
	return 0;
}

int32_t DFextractor::DFStageFile::writeFrames() {
	if (!DFinst->writeAllFramesSTG(pathToOutput))
		return 6;
	return 0;
}

////////////
/// BOOT ///
////////////

/// No init, scripts only
int32_t DFextractor::DFBootFile::writeScripts() {
	DFinst->writeScriptsBOOT(pathToOutput);
	return 0;
}
