#pragma once

#include <fstream>
#include <string>
#include <filesystem>
#include "DFscript.h"
#include "lodepng.h"

class DFfile
{
public:
	// every file is splitten into multiple containers.
	// this struct includes all necessary block inormation to read from
	struct Container {
		enum {
			DEFAULT,
			TABLE,
			SCRIPT,
			IMAGE,
			AUDIO,
			MAP
		};

		int32_t id;
		uint32_t size = 0;
		uint8_t* data = nullptr;

		uint8_t type = DEFAULT;
		std::string info = " - NOT DETECTED -";

		// in some cases we need to keep the pointers... only delete manually
		void clearContent() {
			delete[] data;
			data = nullptr;
		}
	};

	struct ColorPalette {
		int16_t index;
		int16_t RGB[3];
	};

	uint32_t fileCounter = 0;
	ColorPalette colors[256];
	uint16_t colorCount = 256;

	std::vector<Container> containers;

	DFfile(const std::string& fileName, int8_t fileType);
	~DFfile();

	std::string versionSig;

	// receive
	uint16_t getColorCount() {
		return colorCount;
	}
	std::string getContainerInfo();						// prints all
	std::string getContainerInfo(uint16_t id);			// prints specific container information
	virtual std::string getHeaderInfo() = 0;

	void writeContainersToFiles(const std::string& path);

	// will be overridden in their individual classes
	virtual void writeAllScripts() {
		status = ERRNOSCRIPT;
	}
	virtual void writeAllAudio() {
		status = ERRNOAUDIO;
	}
	virtual void writeAllFrames() {
		status = ERRNOFRAMES;
	}

	// image processing
	// get it in memory
	void getBMPimage(DFfile::Container& container, uint8_t*& image1, uint8_t*& image2, uint32_t& fileSize1, uint32_t& fileSize2);
	void getBMPimage(DFfile::Container& container, uint8_t*& image1, uint32_t& fileSize1);
	// write from memory
	void writeBMPimage(DFfile::Container& container, const std::string& path, const std::string& customFileName = "");
	void writePNGimage(const std::string& path, int32_t containerID);
	void writeTransPNGimage(const std::string& writeTo, int32_t containerID);

	// script writing
	bool writeScript(DFfile::Container& container, std::string fileName) {
		fileName.append(".txt");
		std::string out = scripter.binaryScriptToText(container.data);
		if (out.length() > 1) {
			std::ofstream scr(fileName, std::ios::trunc);
			scr << "// Extracted with DF Extraction Tool - version ";
			scr << versionSig;
			scr << "\n\n";
			scr << out;
			return true;
		}
		return false;
	}

	bool writeAllAudioC(const std::string& path);

	// misc
	//void changeOutputPath(std::string& newPath);
	// used to update container information if overview gets printed
	virtual void updateContainerInfo() {
		return;
	}

	const char* getErrorMsg();
	static const char* getErrorMsg(int32_t& errCode);

	DFscript scripter;

	// keeps file type around, set pup etc
	const int8_t fileType;
	const std::string currentFileName;
	// default location would be where the file is saved
	std::string outPutPath;

protected:

	enum errorCodes {
		NOERRORS,
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
		ERRSUFFIX,
		ERRCONVIMAGE,
		ERRPROT
	};

	enum fileTypeID {
		DFBOOTFILE,	// BOOT FILE
		DFMOV,		// MOVIE FILES
		DFPUP,		// PUPPET FILES
		DFSET,		// SET FILES
		DFSHP,		// SHOP FILES
		DFTRK,		// TRACK FILES AUDIO
		DFSFX,		// SFX AUDIO
		DF11K,		// LOW AUDIO
		DFSTG,		// STAGE FILES
		DFCST,		// CAST FILES
		DFSND		// SOUND FILE
	};

	// File header information
	struct FileHeader {
		int32_t FourCC;
		int32_t fileSize;
		int32_t unknown[3];
		int32_t containerCount;
		int32_t type; // mostly 0, sometimes 1, sometimes 2
		int32_t gapWhere;
		const char format[9]{ "LPPALPPA" };
		char unknown2[983];
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

	struct AudioBlockInfos {

		struct AudioChunks {
			int32_t chunkBlockID;
			int32_t unknownInt;
			bool unknownBool; // usually 0, sometimes its 1
			uint8_t strSize;
			char identifier[31];
		};


		AudioBlockInfos(DFfile* inst) {

			// chunk information is splitten in 2 blocks, while the second block is mostly empty
			// the first block has the audio chunks with loop information
			// the second block has audio that never loops, it only plays once

			uint8_t multiplyer = 1;		// used for string size. move file strings are bigger

			// read location of 2nd chunk in chunk0

			if (inst->fileType == DFMOV) {
				chunkInfo2Loc = *(int32_t*)(inst->containers[0].data + 96);
				multiplyer = 2;
				memcpy(trackName, inst->currentFileName.c_str(), inst->currentFileName.length() + 1); // No name necessary
			}
			else {
				chunkInfo2Loc = *(int32_t*)(inst->containers[0].data + 32);
				uint8_t strSize = *(uint8_t*)(inst->containers[0].data + 36);
				memcpy(trackName, (inst->containers[0].data + 37), 16);
				trackName[strSize - 4] = '\0';	// minus 4 because I dont want the suffix
			}


			int8_t* container = (int8_t*)inst->containers[1].data;

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

			container = (int8_t*)inst->containers[chunkInfo2Loc].data;
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
					container += (16 * multiplyer) - 1;
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
	struct waveHeader {

		waveHeader(int32_t chunkSize, int32_t hertz, std::string& version, int16_t bitsPerSample = 8)
			: chunkSize(chunkSize + 36), sampleRate(hertz), bitsPerSample(bitsPerSample),
			blockAlign(1 * bitsPerSample / 8),
			byteRate(hertz* (1 * bitsPerSample / 8))
		{

			// not very optimal. might optimize the constructor in the future.
			if (version.empty())
				version.assign("X.XX");

			const int32_t totalListSize = 136;
			this->chunkSize += totalListSize + 8;
			headerSize = totalListSize + 44 + 8;

			int32_t strSizeBuffer;
			char* curr = listdata;
			// construct list data
			memcpy(curr, "LIST", 4);
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
			memcpy(curr, version.data(), 5);
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

		const char StartChunkID[4]{ 'R','I','F','F' };
		int32_t chunkSize;
		const char format[8]{ 'W','A','V','E','f','m', 't', ' ' };
		const int32_t subChunksize{ 16 }; // size of rest of the sub chunk
		const int16_t audioFormat{ 1 }; // 1 for PCM
		const int16_t monoOrStereo{ 1 }; // 1 = Mono
		const int32_t sampleRate; // Hertz
		const int32_t byteRate; // sample rate * numChannels * bitsPerSample /8
		const int16_t blockAlign; // number of channels * bitsPerSample / 8
		const int16_t bitsPerSample; // ...
		//const char subChunk2ID[4]{ 'd','a','t','a' };
		//const int32_t subChunk2Size;
		// lit dat:
		char listdata[255];

		// sound data follows

		// the header size to copy the mem from is used from here. change it if header changes
		int16_t headerSize;
	};

	// BMP header constructor for 8bit images
	class BMPheader {
	public:
		BMPheader(int32_t height, int32_t width, bool zImage, int16_t maxColors, ColorPalette* colorsRef)
			: height(height), width(width), maxColors(maxColors)
		{
			headerSize = 54 + (maxColors *4);
			headerData = new uint8_t[headerSize];
			uint8_t* bmpCurr = headerData;
			memcpy(bmpCurr, "BM", 2);
			bmpCurr += 2;
			*(int32_t*)bmpCurr = headerSize + (width * height);	// total file size
			int32_t* bmpFileSize = (int32_t*)bmpCurr;
			bmpCurr += 4;
			*(int32_t*)bmpCurr = 0;	// additional data
			bmpCurr += 4;
			*(int32_t*)bmpCurr = headerSize;	// begin of raw pixel data
			int32_t* rawPixelBegin = (int32_t*)bmpCurr;
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
			*(int32_t*)bmpCurr = maxColors;	// number of colors
			int32_t* numberOfColors = (int32_t*)bmpCurr;
			bmpCurr += 4;
			*(int32_t*)bmpCurr = 0;	// number of important colors. I use 0 for ALL
			bmpCurr += 4;

			// first color always black
			*bmpCurr++ = 0x00;
			*bmpCurr++ = 0x00;
			*bmpCurr++ = 0x00;
			*bmpCurr++ = 0;
			for (int16_t color = 1; color < maxColors; color++) {
				*bmpCurr++ = *(((uint8_t*)&colorsRef[color].RGB[2]) + 1);
				*bmpCurr++ = *(((uint8_t*)&colorsRef[color].RGB[1]) + 1);
				*bmpCurr++ = *(((uint8_t*)&colorsRef[color].RGB[0]) + 1);
				*bmpCurr++ = 0;
			}
			// last color always white
			if (maxColors == 256) {
				bmpCurr -= 4;
				*bmpCurr++ = 0xFF;
				*bmpCurr++ = 0xFF;
				*bmpCurr++ = 0xFF;
				bmpCurr++;
			}
		}

		BMPheader(int32_t height, int32_t width, bool zImage, int16_t maxColors)
			// if no color palette is given, z images are expected
			: height(height), width(width), maxColors(maxColors)
		{
			headerSize = 54 + (maxColors * 4);
			headerData = new uint8_t[headerSize];
			uint8_t* bmpCurr = headerData;
			memcpy(bmpCurr, "BM", 2);
			bmpCurr += 2;
			*(int32_t*)bmpCurr = headerSize + (width * height);	// total file size
			int32_t* bmpFileSize = (int32_t*)bmpCurr;
			bmpCurr += 4;
			*(int32_t*)bmpCurr = 0;	// additional data
			bmpCurr += 4;
			*(int32_t*)bmpCurr = headerSize;	// begin of raw pixel data
			int32_t* rawPixelBegin = (int32_t*)bmpCurr;
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
			*(int32_t*)bmpCurr = maxColors;	// number of colors
			int32_t* numberOfColors = (int32_t*)bmpCurr;
			bmpCurr += 4;
			*(int32_t*)bmpCurr = 0;	// number of important colors. I use 0 for ALL
			bmpCurr += 4;

			// now read in the colors to the palette
			for (int16_t color = 0; color < maxColors; color++) {
				// depth has 24 levels max. 25th is 0 and always super close
				// multiply by ten to give a better contrast
				*bmpCurr++ = 255 - (color * 10);
				*bmpCurr++ = 255 - (color * 10);
				*bmpCurr++ = 255 - (color * 10);
				// alpha
				*bmpCurr++ = 0;
			}
		}

		~BMPheader() {
			delete[] headerData;
		}

		uint8_t* getData() {
			return headerData;
		}

		int32_t getHeaderSize() {
			return headerSize;
		}

	private:
		uint8_t* headerData;

		int32_t height, width, headerSize;
		int16_t maxColors;
	};

	//ColorPalette colors[256];


	// used for sanity check, 0 = good. All operations will stop if this is not 0
	int32_t status = 0;
	
	DataBuffer containerDataBuffer;

	AudioBlockInfos* audioPtr = nullptr;

	int32_t readFileIntoMemory(const std::string& fileLocation);

	bool writeWavFromContainer(const std::string& path, const std::string& name, int32_t containerId);

	bool audioDecoder_v40(int32_t& uncompressedSize, int8_t* soundFile, int8_t* decodedOutput);
	bool audioDecoder_v41(int32_t& uncompressedSize, int8_t* soundFile, int8_t* decodedOutput);
	bool getRawImageData(DFfile::Container& container, bool& zImage, int16_t& height, int16_t& width);
	virtual int32_t initContainerData() = 0;


	friend int32_t readDFfile(DFfile*& fileInst, const std::string& filePath);

public:


	FileHeader fileHeader;

	static uint8_t swapEndians(double& dest, uint8_t* source);
	static uint8_t swapEndians(float& dest, uint8_t* source);
};

