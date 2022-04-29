#include "DFboot.h"

std::string DFboot::getHeaderInfo() {
	return std::string();
};

void DFboot::writeAllScripts() {
	std::string out(outPutPath);
	if (!outPutPath.empty())
		out.append("/_BOOTFILE");
	else
		out.append("_BOOTFILE");
	std::filesystem::create_directory(out);

	for (int32_t container = 1; container < containers.size(); container++) {
		std::string sub(out);
		sub.append("/Script ");
		sub.append(std::to_string(container));
		if (writeScript(containers[container], sub))
			fileCounter++;
	}
}

int32_t DFboot::initContainerData() {
	// nothing to initiliaze...
	return 0;
};