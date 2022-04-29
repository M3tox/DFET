#pragma once

#include "DFfile.h"

class DFboot : public DFfile
{
	using DFfile::DFfile;
public:
	std::string getHeaderInfo();

	void writeAllScripts();

protected:
	int32_t initContainerData();
};

