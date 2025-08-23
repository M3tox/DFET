#include "DFfile.h"

DFfile::DFfile(const std::string& currentFileName, int8_t fileType) :
	currentFileName(currentFileName), fileType(fileType) {};

DFfile::~DFfile() {
	for (auto& cont : containers) {
		cont.clearContent();
	}
}

int32_t DFfile::readFileIntoMemory(const std::string& fileLocation) {

	std::ifstream fileToRead(fileLocation, std::ios::binary);

	if (fileToRead.good()) {

		fileToRead.read((char*)&fileHeader, sizeof(fileHeader));

		uint32_t* containerPositions = new uint32_t[fileHeader.containerCount];

		uint32_t tableSize = fileHeader.containerCount * sizeof(int32_t);
		fileToRead.read((char*)containerPositions, tableSize);

		containers.reserve(fileHeader.containerCount);
		// default reading
		if (fileHeader.type == 1) {
			for (int32_t container = 0; container < fileHeader.containerCount; container++) {
				containers.emplace_back();
				if (container == fileHeader.gapWhere) {
					// some blocks are just empty dummies for whatever reason
					// they still affect the iterator, so I still do them. I write "NOP = NO OPERATION" just to know if I watch them
					constexpr int32_t dummySize{ 4 };
					char dummy[dummySize]{ 'T', 'P', '0' + fileHeader.type, '\0' };
					containers[container].size = dummySize;
					containers[container].data = new uint8_t[dummySize];
					memcpy(containers[container].data, dummy, dummySize);

				}
				else {
					// same as default reading, might want to improve in the future
					fileToRead.seekg(containerPositions[container]);

					fileToRead.read((char*)&containers[container].id, sizeof(containers[container].id));
					fileToRead.read((char*)&containers[container].size, sizeof(containers[container].size));

					containers[container].data = new uint8_t[containers[container].size];
					fileToRead.read((char*)containers[container].data, containers[container].size);
				}

				fileToRead.clear();
			}

		}
		else if (fileHeader.type == 2) {
			// TODO: Blocks with ZERO in the location are empty or silent blocks!
			for (int32_t container = 0; container < fileHeader.containerCount; container++) {
				containers.emplace_back();
				if (container == fileHeader.gapWhere - 1 || container == fileHeader.gapWhere) {
					// some blocks are just empty dummies for whatever reason
					// they still affect the iterator, so I still do them. I write "NOP = NO OPERATION" just to know if I watch them
					constexpr int32_t dummySize{ 4 };
					char dummy[dummySize]{ 'T', 'P', '0' + fileHeader.type, '\0' };
					containers[container].size = dummySize;
					containers[container].data = new uint8_t[dummySize];
					memcpy(containers[container].data, dummy, dummySize);

				}
				else {
					// same as default reading, might want to improve in the future
					fileToRead.seekg(containerPositions[container]);

					fileToRead.read((char*)&containers[container].id, sizeof(containers[container].id));
					fileToRead.read((char*)&containers[container].size, sizeof(containers[container].size));

					containers[container].data = new uint8_t[containers[container].size];
					fileToRead.read((char*)containers[container].data, containers[container].size);
				}

				fileToRead.clear();
			}
		}
		else {

			for (int32_t container = 0; container < fileHeader.containerCount; container++) {

				containers.emplace_back();
				if (containerPositions[container] > 1024) {
					fileToRead.seekg(containerPositions[container]);

					fileToRead.read((char*)&containers[container].id, sizeof(containers[container].id));
					fileToRead.read((char*)&containers[container].size, sizeof(containers[container].size));

					containers[container].data = new uint8_t[containers[container].size];
					fileToRead.read((char*)containers[container].data, containers[container].size);
				}
				else {
					containers[container].id = container;
					containers[container].size = 8;
					containers[container].data = new uint8_t[8]{ 0 };
				}


				fileToRead.clear();
			}
		}

		delete[] containerPositions;
	}
	else
		return ERRREADCONTAINERS;


	fileToRead.close();

	return this->initContainerData();
}

const char* DFfile::getErrorMsg() {
	if (!this)
		return "DF file Object not created, use the error code to call the static function instead!\n";
	return getErrorMsg(status);
}

void DFfile::writeContainersToFiles(const std::string& path) {
	for (int32_t container = 0; container < containers.size(); container++) {

		std::string FileName(path);
		FileName.push_back('/');
		if (fileType != DFBOOTFILE) {
			FileName.append(currentFileName.substr(0, currentFileName.length()-4));
		} else
			FileName.append(currentFileName);
		FileName.push_back('_');
		FileName.append("container ");
		FileName.append(std::to_string(container));
		FileName.append(".dta");

		std::ofstream fileEXT(FileName, std::ios::binary);
		fileEXT.write((char*)containers[container].data, containers[container].size);
		fileEXT.close();
	}
}

// RECEIVE INFOS
std::string DFfile::getContainerInfo() {
	this->updateContainerInfo();
	std::string out("ID      Container Information\n");
	for (uint32_t cont = 0; cont < fileHeader.containerCount; cont++) {
		out.append(std::to_string(cont));
		out.append(")\t");
		out.append(containers[cont].info);
		out.push_back('\n');
	}
	return out;
}

std::string DFfile::getContainerInfo(uint16_t id) {
	this->updateContainerInfo();
	if (id >= fileHeader.containerCount) {
		std::string out("ERROR: Container ");
		out.append(std::to_string(id));
		out.append(" out of bounds!\n");
		return out;
	}

	return containers[id].info;
}


// get both possible images
void DFfile::getBMPimage(DFfile::Container& container, uint8_t* &image1, uint8_t* &image2, uint32_t& fileSize1, uint32_t& fileSize2) {
	// writes image + possible zimage into buffer
	bool zImage;
	int16_t width, height;
	if (!getRawImageData(container, zImage, height, width))
		return;

	BMPheader bmpHeader(height, width, 0, colorCount, colors);

	fileSize1 = bmpHeader.getHeaderSize() + (height * width);
	image1 = new uint8_t[fileSize1];

	uint8_t* imageCurr = image1;
	memcpy(imageCurr, bmpHeader.getData(), bmpHeader.getHeaderSize());
	imageCurr += bmpHeader.getHeaderSize();
	for (int16_t i = 0; i < height; i++) {
		memcpy(imageCurr, (char*)containerDataBuffer.GetContent() + ((height - (i + 1)) * width), width);
		imageCurr += width;
	}

	if (zImage) {
		BMPheader bmpHeaderZ(height, width, 0, 26);
		fileSize2 = bmpHeaderZ.getHeaderSize() + (height * width);
		image2 = new uint8_t[fileSize2];
		imageCurr = image2;
		memcpy(imageCurr, bmpHeaderZ.getData(), bmpHeaderZ.getHeaderSize());
		imageCurr += bmpHeaderZ.getHeaderSize();
		for (int16_t i = 0; i < height; i++) {
			memcpy(imageCurr, ((char*)containerDataBuffer.GetContent() + ((height - (i + 1)) * width)) + (width * height), width);
			imageCurr += width;
		}
	}
	else {
		image2 = nullptr;
		fileSize2 = 0;
	}
}

// use this if only care about first image (probably most of the times)
void DFfile::getBMPimage(DFfile::Container& container, uint8_t*& image1, uint32_t& fileSize1) {
	// writes image + possible zimage into buffer
	bool zImage;
	int16_t width, height;
	if (!getRawImageData(container, zImage, height, width))
		return;

	BMPheader bmpHeader(height, width, 0, colorCount, colors);

	fileSize1 = bmpHeader.getHeaderSize() + (height * width);
	image1 = new uint8_t[fileSize1];

	uint8_t* imageCurr = image1;
	memcpy(imageCurr, bmpHeader.getData(), bmpHeader.getHeaderSize());
	imageCurr += bmpHeader.getHeaderSize();
	for (int16_t i = 0; i < height; i++) {
		memcpy(imageCurr, (char*)containerDataBuffer.GetContent() + ((height - (i + 1)) * width), width);
		imageCurr += width;
	}
}

void DFfile::writeBMPimage(DFfile::Container& container, const std::string& path, const std::string& customFileName) {

	// writes image + possible zimage into buffer
	bool zImage;
	int16_t width, height;
	if (!getRawImageData(container, zImage, height, width))
		return;

	BMPheader bmpHeader(height, width, 0, colorCount, colors);


	// actual image
	std::string fileName(path);
	fileName.push_back('/');
	if (customFileName.empty()) {
		//fileName.append(currentFileName);
		fileName.append("frame ");
		fileName.append(std::to_string(container.id));
	}
	else {
		fileName.append(customFileName);
	}

	fileName.append(".bmp");

	// write bmp image file
	std::ofstream imageFile(fileName, std::ios::binary, std::ios::trunc);
	imageFile.write((char*)bmpHeader.getData(), bmpHeader.getHeaderSize());
	for (int16_t i = 0; i < height; i++) {
		imageFile.write((char*)containerDataBuffer.GetContent() + ((height - (i + 1)) * width), width);
	}
	imageFile.close();


	if (zImage) {
		// write z image if there are
		// recycle filename
		fileName.insert(path.length()+1, "Z/");
		fileName.insert(fileName.length() - 4, " Z");
		BMPheader bmpHeaderZ(height, width, 0, 26);

		std::ofstream zimageFile(fileName, std::ios::binary, std::ios::trunc);
		zimageFile.write((char*)bmpHeaderZ.getData(), bmpHeaderZ.getHeaderSize());
		for (int16_t i = 0; i < height; i++) {
			zimageFile.write(((char*)containerDataBuffer.GetContent() + ((height - (i + 1)) * width))+(width*height), width);
		}
		zimageFile.close();
	}

	return;
}

void DFfile::writeTransPNGimage(const std::string& writeTo, int32_t containerID) {
	// images with possible transparency use a different decompression algorithm, a simpler one.
	uint8_t* currIn = (uint8_t*)containers[containerID].data;

	const int16_t imageHeight = *(int16_t*)currIn;
	currIn += 2;
	const int16_t imageWidth = *(int16_t*)currIn;
	currIn += 2;
	// vertical drawing start position
	// TODO: screen space data shouldnt be hard coded, take it somewhere from header file container 0
	const int16_t imagePosY = 384 / 2 - (*(int16_t*)currIn);
	currIn += 2;
	// horizontal drawing start position
	const int16_t imagePosX = 512 / 2 - (*(int16_t*)currIn);
	currIn += 2;

	//uint8_t* out = new uint8_t[imageHeight * imageWidth];


	std::vector<uint8_t> image;
	image.resize(imageWidth * imageHeight * 4);
	uint8_t* currOut = image.data();

	short heightRest = imageHeight;
	// compressed image is splitten into multiple segments
	// segment size is indicated in shorts in every beginning of a segment
	// decompress the file by going though segment for segment	
	while (heightRest--) {
		int16_t segmentSize = *(int16_t*)currIn;
		currIn += 2;

		const uint8_t* segmendEnd = currIn + segmentSize;
		while (currIn < segmendEnd) {
			uint8_t bitFlag = *currIn++;

			// check first two bits
			if (bitFlag & 1) {
				uint8_t copyCount = bitFlag >> 2;
				if (bitFlag & 2) {
					// Mode 4 selected
					// copy X amount directly from file
					//memcpy(currOut, currIn, copyCount);
					for (uint32_t i = 0; i < copyCount; i++) {
						static int16_t check[3]{ 0xFFFF,0xFFFF,0xFFFF };
						if (!memcmp(colors[*currIn].RGB, check, 6)) {
							*currOut++ = 0;
							*currOut++ = 0;
							*currOut++ = 0;
						}
						else {
							*currOut++ = *(((uint8_t*)&colors[*currIn].RGB[0]) + 1);
							*currOut++ = *(((uint8_t*)&colors[*currIn].RGB[1]) + 1);
							*currOut++ = *(((uint8_t*)&colors[*currIn].RGB[2]) + 1);
						}

						// transparency value
						*currOut++ = 0xFF;
						currIn++;
					}
				}
				else {
					// Mode 2 selected
					// write transparent pixels
					memset(currOut, 0, copyCount * 4);
					currOut += copyCount * 4;
				}
			}
			else {
				// check if mode 1, 2 is selected
				uint8_t copyCount = bitFlag >> 2;
				if (bitFlag & 2) {

					// mode 2 selected, shift flag bits away and copy memory
					for (uint32_t i = 0; i < copyCount; i++) {
						static int16_t check[3]{ 0xFFFF,0xFFFF,0xFFFF };
						if (!memcmp(colors[*currIn].RGB, check, 6)) {
							*currOut++ = 0;
							*currOut++ = 0;
							*currOut++ = 0;
						}
						else {
							*currOut++ = *(((uint8_t*)&colors[*currIn].RGB[0]) + 1);
							*currOut++ = *(((uint8_t*)&colors[*currIn].RGB[1]) + 1);
							*currOut++ = *(((uint8_t*)&colors[*currIn].RGB[2]) + 1);
						}
						*currOut++ = 0xFF;
					}
					currIn++;
				}
				else {
					// mode 1 selected
					// copy content from previous row itself
					memcpy(currOut, currOut - imageWidth * 4, copyCount * 4);
					currOut += copyCount * 4;
				}
			}
		}
	}

	// TODO! SHOULD BE IN ONE PLACE ONLY
	// when here, the image is completly written to memory.


	// actual image
	std::string fileName(writeTo);
	//fileName.push_back('/');

	//fileName.append(currentFileName);
	fileName.append("/frame_");
	fileName.append(std::to_string(containerID));
	fileName.append(".png");

	lodepng::encode(fileName, image, imageWidth, imageHeight);
}

void DFfile::writePNGimage(const std::string& path, int32_t containerID) {

	// todo in case I want bmp as png instead?
	writeTransPNGimage(path, containerID);
	return;
}


// UTILLITIES
//void DFfile::changeOutputPath(std::string& newPath) {
//	outPutPath.assign(newPath);
//}
const char* DFfile::getErrorMsg(int32_t& errorCode) {

	constexpr int32_t errorCount{ 16 };
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
		/*13*/  {"File suffix not specified!"},
		/*14*/  {"Could not convert image!"},
		/*15*/	{"Can't extract this file. The creator has protected its content, sorry!"}
	};

	return errorMsgs[errorCode];
}

uint8_t DFfile::swapEndians(double& dest, uint8_t* source) {

	for (uint8_t i = 0; i < sizeof(double); i++) {
		*(((uint8_t*)&dest) + sizeof(double) - i - 1) = *(source + i);
	}
	// return size to increment source pointer
	return sizeof(double);
}

uint8_t DFfile::swapEndians(float& dest, uint8_t* source) {

	for (uint8_t i = 0; i < sizeof(float); i++) {
		*(((uint8_t*)&dest) + sizeof(float) - i - 1) = *(source + i);
	}
	// return size to increment source pointer
	return  sizeof(float);
}

bool DFfile::writeAllAudioC(const std::string& path) {

	// first check, if this file has audio data anyway (sometimes they dont)
	if (audioPtr->containsAudioData) {

		if (audioPtr->audioLoopChunkCount) {

			// first we need the total file size of all Audioloops together
			int32_t totalFileSize{ 0 };
			int32_t* fileSizes = new int32_t[audioPtr->totalLoopsChunks];
			for (int32_t loop = 0; loop < audioPtr->totalLoopsChunks; loop++) {

				fileSizes[loop] = *(int32_t*)(containers[audioPtr->audioLoopChunks[audioPtr->chunkOrder[loop] - 1].chunkBlockID].data + 36);
				totalFileSize += fileSizes[loop];
			}

			// get hertz rate from first block. 
			int32_t hertz = *(int32_t*)(containers[audioPtr->audioLoopChunks[0].chunkBlockID].data + 28);
			// Hertz fix: priotize bigger
			if (audioPtr->audioLoopChunkCount > 1) {
				int32_t hertzTmp = *(int32_t*)(containers[audioPtr->audioLoopChunks[1].chunkBlockID].data + 28);
				if (hertz < hertzTmp)
					hertz = hertzTmp;
			}

			// get codec flag from first chunk to determine format for the whole track
			int16_t codec_flag = *(int16_t*)(containers[audioPtr->audioLoopChunks[0].chunkBlockID].data + 0x1A);

			// construct audio name based on blockZero
			std::string filname(path);
			filname.push_back('/');
			filname.append(audioPtr->trackName);
			filname.append(".wav");

			waveHeader header(totalFileSize, hertz, versionSig, (codec_flag == 1) ? 8 : 16);

			containerDataBuffer.resize(totalFileSize + header.headerSize + 3);
			//int8_t* toOutput = new int8_t[totalFileSize + header.headerSize + 3]; // 3 extra overflow decoder bytes
			memcpy(containerDataBuffer.GetContent(), &header, header.headerSize);

			std::ofstream audioFile(filname, std::ios::binary | std::ios::trunc);

			int32_t current{ 0 };
			for (int32_t loop = 0; loop < audioPtr->totalLoopsChunks; loop++) {
				bool success;
				if (codec_flag == 1) {
					success = audioDecoder_v40(fileSizes[loop], (int8_t*)containers[audioPtr->audioLoopChunks[audioPtr->chunkOrder[loop] - 1].chunkBlockID].data, (int8_t*)containerDataBuffer.GetContent() + header.headerSize + current);
				}
				else if (codec_flag == 2) {
					success = audioDecoder_v41(fileSizes[loop], (int8_t*)containers[audioPtr->audioLoopChunks[audioPtr->chunkOrder[loop] - 1].chunkBlockID].data, (int8_t*)containerDataBuffer.GetContent() + header.headerSize + current);
				}

				if (!success) {
					status = ERRDECODEAUDIO;
					return false;
				}
				current += fileSizes[loop];
			}
			audioFile.write((char*)containerDataBuffer.GetContent(), totalFileSize + header.headerSize);
			audioFile.close();
			fileCounter++;
			delete[] fileSizes;
		}

		

			for (int32_t shot = 0; shot < audioPtr->audioSingleChunkCount; shot++) {

				int32_t blockID = audioPtr->audioSingleChunks[shot].chunkBlockID;
				uint8_t* container_data = containers[blockID].data;

				int16_t codec_flag = *(int16_t*)(container_data + 0x1A);
				int32_t hertz = *(int32_t*)(container_data + 28);
				int32_t fileSize = *(int32_t*)(container_data + 36);

				waveHeader header(fileSize, hertz, versionSig, (codec_flag == 1) ? 8 : 16);

				containerDataBuffer.resize(fileSize + header.headerSize + 3);
				//int8_t* toOutput = new int8_t[fileSize + header.headerSize + 3]; // extra bytes decoder overflow
				memcpy(containerDataBuffer.GetContent(), &header, header.headerSize);

				bool success;
				if (codec_flag == 1) {
					success = audioDecoder_v40(fileSize, (int8_t*)container_data, (int8_t*)containerDataBuffer.GetContent() + header.headerSize);
				}
				else {
					success = audioDecoder_v41(fileSize, (int8_t*)container_data, (int8_t*)containerDataBuffer.GetContent() + header.headerSize);
				}

				if (!success) {
					status = ERRDECODEAUDIO;
					return false;
				}

				std::string filname(path);
				filname.push_back('/');
				// sometimes they have subfolders. detect those and ignore them
				std::string temp(audioPtr->audioSingleChunks[shot].identifier);
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
			}
		
	}

	// operation succesful
	return true;
}

// audio decoder decompress the file to readable DCP
// Function was refactored after version 0.89, less allocations, less castings, easier to understand and maintain.
bool DFfile::audioDecoder_v40(int32_t& uncomprBlockSize, int8_t* soundContainer, int8_t* decodedOutput) {

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

	
	int8_t* currOutput;
	int8_t prevByte;
	int8_t nextByte;

	// check if block format is correct 00 00 01 00 (sound format version 1.0)
	if (0x00010000 != *(int32_t*)soundContainer) return false;

	// set compressed file pointer to begin of data
	soundContainer += *(int32_t*)(soundContainer + 44);

	nextByte = *soundContainer++;
	*decodedOutput = nextByte + 0x40;

	currOutput = decodedOutput + 1;
	prevByte = nextByte;

	// iterate until the known end 
	while (currOutput < (decodedOutput + uncomprBlockSize)) {
		nextByte = *soundContainer++;

		// check if last bit is set 0
		if ((nextByte & 0x80) == false) {
			// MODE I: copy 1 byte directly from source and add 0x40 to it
			*(int8_t*)currOutput++ = nextByte + 0x40;
			prevByte = nextByte;
		}
		// check if second last bit is set 0
		else if ((nextByte & 0x40) == false) {
			// MODE II: find and write difference with step tables
			uint8_t byteCount = nextByte & 0x3f;

			do {
				nextByte = *soundContainer++;
				int8_t step = prevByte + StepSizeTable[(uint8_t)nextByte];
				int8_t index = step + IndexTable[(uint8_t)nextByte];
				
				*currOutput++ = step + 0x40;
				*currOutput++ = index + 0x40;
				
				prevByte = index;
			} while (byteCount--);
		}
		else {
			// MODE III: copy x amount from previous byte
			uint8_t byteCount = (nextByte & 0x3f) + 1;
			memset(currOutput, prevByte + 0x40, byteCount);
			currOutput += byteCount;
		}
		
	}
	// this container should be decoded now, check if pointer adress matches with expected
	if (currOutput != (decodedOutput + uncomprBlockSize)) {
		return false;
	}
	return true;
}

bool DFfile::audioDecoder_v41(int32_t& uncomprBlockSize, int8_t* soundContainer, int8_t* decodedOutput) {
	int16_t* out_ptr = (int16_t*)decodedOutput;
	int num_samples = uncomprBlockSize / 2;

	uint8_t* in_ptr = (uint8_t*)soundContainer + *(int32_t*)(soundContainer + 44);

	int16_t current_sample = 0;

	for (int i = 0; i < num_samples; i++) {
		uint8_t input_byte = in_ptr[i];

		if ((input_byte & 0x80) == 0) {
			// High bit is 0: it's a delta value.
			int16_t delta = (input_byte << 9);
			delta >>= 4; // Sign-extend and scale
			current_sample += delta;
		}
		else {
			// High bit is 1: it's a new absolute value.
			current_sample = (input_byte << 9); // Sign-extend to 16 bits
		}
		out_ptr[i] = current_sample;
	}
	return true; // Success
}

// Function completly rewritten since DFET version 0.89! Shorter, faster and more reliant
bool DFfile::getRawImageData(DFfile::Container& container, bool& zImage, int16_t& height, int16_t& width) {
	
	height = *(int16_t*)container.data;
	width = *(int16_t*)(container.data + 2);
	uint8_t* currIN = (uint8_t*)container.data + 4;

	const int32_t totalSize = height * width;
	//int16_t heightRest{ height };
	
	int16_t lookUpOffset{ 0 };
	
	containerDataBuffer.resize(totalSize * 2);	// make it double as big so we can store secondary images
	//uint8_t* frameOutput = new uint8_t[totalSize];
	uint8_t* rawPixelOutput = containerDataBuffer.GetContent();

	uint8_t* currOUT = rawPixelOutput;
	for (int16_t heightc = 0; heightc < height; heightc++) {

		int16_t currWith = 0 ;
		// READ first parameter before processing width data
		// TODO: what are the first two bits for?
		int8_t param = *currIN++ >> 2;

		if (param == 1) {
			// read whole width from compressed file
			memcpy(currOUT, currIN, width);
			currWith = width;
			currOUT += width;
			currIN += width;
		}
		if (param <= 5) {
			lookUpOffset = width * (6 - param);
		}
		else if (param <= 9) {
			lookUpOffset = width * (5 - param);
		}
		else if (param == 10) {
			// skip row, information from previous image
			currWith = width;
			currOUT += width;
		}
		else if (param <= 14) {
			lookUpOffset = width * (15 - param);
			memcpy(currOUT, currOUT - lookUpOffset, width);
			currWith = width;
			currOUT += width;
		}
		else if (param <= 18) {
			lookUpOffset = width * (14 - param);
			memcpy(currOUT, currOUT - lookUpOffset, width);
			currWith = width;
			currOUT += width;
		}
		else {
			// error if nothing applies
			status = ERRCONVIMAGE;
			return false;
		}

		while (currWith < width) {

			// get control flag byte
			uint8_t modeSel = *currIN & 7;
			uint16_t count = *currIN >> 3;
			currIN++;
			// if count does not fit in 6 bits
			// use second byte and add it up
			if (!count) {
				count += 32 + *currIN++;
			}

			switch (modeSel) {
			case 2:
				// Mode 010
				// skip x count pixel information taken from previous image
				break;
			case 3:
				// Mode 011
				// copy x count from decompressed image
				memcpy(currOUT, currOUT - lookUpOffset, count);
				break;
			case 4:
				// Mode 100
				// copy last pixel x times
				memset(currOUT, *(currOUT - 1), count);
				break;
			case 5:
				// Mode 101
				// copy x pixel directly from compressed image file
				memcpy(currOUT, currIN, count);
				currIN += count;
				break;
			case 6:
				// Mode 110
				// copy given pixel data from compressed file x times to output
				memset(currOUT, *currIN++, count);
				break;
			case 7:
				// Mode 111
				// copy x count bytes from offset specified by given int16_t
				memcpy(currOUT, currOUT - *(uint16_t*)currIN, count);
				currIN += 2;
				break;
			default:
				// must be mode 0 or 1
			{
				uint16_t outCounter = 0;
				int16_t lookUpOffsetSingle = 1;

				if (modeSel == 0)
					// Mode 000
					*(currOUT + outCounter++) = *currIN++;
				else
					// Mode 001 does the same as 000 but with an offset change before
					lookUpOffsetSingle = lookUpOffset;

				uint32_t flags;
				*(((uint8_t*)&flags) + 3) = *currIN;
				*(((uint8_t*)&flags) + 2) = *(currIN+1);
				*(((uint8_t*)&flags) + 1) = *(currIN+2);
				*((uint8_t*)&flags) = *(currIN+3);

				currIN += 2;
				//  && flagBitPos != 0
				int8_t flagBitPos = 16;
				for (; outCounter < count; outCounter++) {
					// find position of first bit in flag
					uint8_t firstBitPos = 0;
					uint32_t bitCheck = 0x80000000;
					for (int8_t i = 15; i >= 0; i--) {
						if (flags & bitCheck) {
							firstBitPos = i;
							break;
						}
						bitCheck >>= 1;
					}

					if (firstBitPos == 0xF) {
						// if first flag is set just, copy last cell
						*(currOUT + outCounter) = *(currOUT + outCounter - lookUpOffsetSingle);
						flagBitPos--;
						flags <<= 1;
					}
					else if (firstBitPos < 0x8) {
						*(((uint8_t*)&flags) + 2) += *(currOUT + outCounter - lookUpOffsetSingle);
						*(currOUT + outCounter) = *(((uint8_t*)&flags) + 2);
						flagBitPos -= 16;
						flags <<= 16;
					}
					else {
						// difference depending in the amount of zero bits
						const uint8_t difference = 15 - firstBitPos;
						// check if bit behind is set or not
						if (flags & (1 << firstBitPos + 15))
							*(currOUT + outCounter) = (*(currOUT + outCounter - lookUpOffsetSingle)) + difference;
						else
							*(currOUT + outCounter) = (*(currOUT + outCounter - lookUpOffsetSingle)) - difference;

						// update flagBitPos
						// the difference is also the amount of bits that needs to be pushed away
						flagBitPos -= difference + 2;
						flags <<= difference + 2;
					}

					if (flagBitPos < 0) {
						// push in two more byte flags
						// adjust bit flag in order to make the bit chain continiuos
						flags >>= -flagBitPos;
						currIN += 2;
						*(((uint8_t*)&flags) + 1) = *currIN;
						*((uint8_t*)&flags) = *(currIN + 1);
						flags <<= -flagBitPos;
						flagBitPos += 16;
					}
				}

				// if second byte was not used, retract it for later operations
				// if bitPos is still 8-15 the second byte was still not touched
				if (flagBitPos >= 8) currIN--;
			}
				break;
			}
			currWith += count;
			currOUT += count;
		}
	}

	// if container end is not reached yet, Z images are expected
	if ((currIN - container.data) < container.size) {
		// Z IMAGE PROCESSING
		
		zImage = true;
		const uint8_t* tableStart = currIN;
		const uint16_t* depthInfoPos = (uint16_t*)tableStart;


		uint8_t* depthColorCurr = containerDataBuffer.GetContent() + (height * width);	// write image in second region

		for (int32_t i = 0; i < height; i++) {
			for (uint8_t depth = 0; depth < *(tableStart + depthInfoPos[i]); depth++) {
				const uint8_t valCount = *(tableStart + depthInfoPos[i] + 1 + (depth * 2));
				memset(depthColorCurr, *(tableStart + depthInfoPos[i] + 2 + (depth * 2)), valCount);
				depthColorCurr += valCount;
			}
		}
	}
}
