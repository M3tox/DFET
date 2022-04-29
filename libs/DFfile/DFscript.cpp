#include "DFscript.h"

const std::string DFscript::binaryScriptToText(uint8_t* curr) {

	std::string script = std::string();

	while (seg.cmd = *(uint16_t*)curr) {
		seg.info = *(uint32_t*)(curr + 2);
		seg.unknown = *(uint16_t*)(curr + 6);


		if (seg.unknown)
			// let me know if this is actually used for anything
			script.append("[UNKNOWN VALUE] ");

		switch (seg.cmd) {
		case STRING:
			script.push_back('"');
			script.append((char*)(curr + seg.info + 1), 0, *(curr + seg.info));
			script.append("\" ");
			break;
		case INTEGER:
			script.append(std::to_string(seg.info));
			script.push_back(' ');
			break;
		case VARIABLE:
			script.append((char*)(curr + seg.info + 1), 0, *(curr + seg.info));
			script.push_back(' ');
			break;
		case BREAK:
			if (!script.empty() && script.at(script.length() - 1) == ' ')
				script.at(script.length() - 1) = '\n';
			else
				script.push_back('\n');
			while (seg.info--)
				script.push_back('\t');
			break;
		default:
			if (seg.cmd == 4018 || seg.cmd == 8002) {
				// ( - 
				script.append(ScriptCommands.at(seg.cmd));
				break;
			}
			else if (seg.cmd == 8004) {
				// "/"
				if (script.length() > 1)
					if (script.at(script.length() - 1) == ' ')
						if (script.at(script.length() - 2) == '/') {
							script.at(script.length() - 1) = ('/');
							script.push_back(' ');
							break;
						}
			}
			else if (seg.cmd > 4018 && seg.cmd <= 4020) {
				// ) ,
				if (script.length() > 0)
					if (script.at(script.length() - 1) == ' ')
						script.pop_back();
			}
			script.append(ScriptCommands.at(seg.cmd));
			script.push_back(' ');
			break;
		}
		curr += 8;	// go to next segment
	}

	return script;
}

int32_t DFscript::TextScriptToBinary(uint8_t*& dest, uint32_t& dataLength, const std::string& script) {

	std::vector<Segment>segments;
	std::vector<std::string> scrStrings;
	std::string lastCmd;
	// iterate though every single character in the script
	for (std::string::size_type pos = 0; pos < script.size(); pos++) {
		const char ch = script.at(pos);

		switch (ch) {
		case ' ':
		case '\t':
			checkLastCommand(segments, scrStrings, lastCmd);
			break;
			// LINE BREAKS
		case '\r':
			pos++;
		case '\n':
		{
			checkLastCommand(segments, scrStrings, lastCmd);
			uint32_t tabCount{ 0 };
			while (pos < script.size() - 1) {
				if (script.at(pos + 1) != '\t')
					break;
				tabCount++;
				pos++;
			}
			segments.push_back(Segment());
			segments.back().cmd = BREAK;
			segments.back().info = tabCount;
			break;
		}
		// STRINGS
		case '"':
		{
			checkLastCommand(segments, scrStrings, lastCmd);
			// string begin or end
			std::string::size_type posBeg = ++pos;
			while (script.at(pos) != '"')
				// cancel process if end of file is reached
				if (pos++ > script.size())
					return ERRBADSTRFORMAT;

			lastCmd = script.substr(posBeg, pos - posBeg);
			segments.push_back(Segment());
			segments.back().cmd = STRING;
			for (int32_t str = 0; str < scrStrings.size(); str++) {
				if (!lastCmd.compare(scrStrings.at(str))) {
					segments.back().stringIndex = str;
					break;
				}
			}
			// was a string found? push in a new one if not.
			if (segments.back().stringIndex == -1) {
				scrStrings.push_back(lastCmd);
				segments.back().stringIndex = scrStrings.size() - 1;
			}


			lastCmd.clear();
			break;
		}

		// might be used for comments, extra check
		case '/':

			//if (script.at(pos+1) == '/') {
				// this is a comment in the script, for some reason all words here are considered as variables

			//}
			checkLastCommand(segments, scrStrings, lastCmd);
			segments.push_back(Segment());
			lastCmd.push_back(ch);
			segments.back().cmd = getCmdID(lastCmd);
			lastCmd.clear();
			break;
			// OPERATORS
		case '+':
		case '-':
		case '*':
		case '&':
		case '|':
		case '@':
		case '=':
		case '(':
		case ')':
		case ',':
			checkLastCommand(segments, scrStrings, lastCmd);
			segments.push_back(Segment());
			lastCmd.push_back(ch);
			segments.back().cmd = getCmdID(lastCmd);
			lastCmd.clear();
			break;
		case '<':
		case '>':
		case '!':
			checkLastCommand(segments, scrStrings, lastCmd);
			segments.push_back(Segment());
			lastCmd.push_back(ch);
			if (script.at(pos + 1) == '=') {
				lastCmd.push_back('=');
				pos++;
			}
			segments.back().cmd = getCmdID(lastCmd);
			lastCmd.clear();
			break;
			// INTEGERS
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		{
			// also variables can contain numbers
			if (!lastCmd.empty()) {
				lastCmd.push_back(ch);
				break;
			}
			checkLastCommand(segments, scrStrings, lastCmd);
			lastCmd.push_back(ch);
			while (script.at(pos + 1) < script.size()) {
				// break out when its not a number anymore
				if (script.at(pos + 1) < '0' || script.at(pos + 1) > '9')
					break;

				// keep pusing in as long as it is an umber
				lastCmd.push_back(script.at(++pos));
			}

			segments.push_back(Segment());
			segments.back().cmd = INTEGER;
			segments.back().info = std::atoi(lastCmd.c_str());
			lastCmd.clear();
			break;
		}

		default:
			lastCmd.push_back(ch);
		}
	}

	// add one empty container
	segments.push_back(Segment());


	dataLength = (segments.size() * 8);
	for (uint32_t strpos = 0; strpos < scrStrings.size(); strpos++) {
		dataLength += scrStrings.at(strpos).size() + 1;
	}

	dest = new uint8_t[dataLength];
	uint8_t* currStrPos = dest + (segments.size() * 8);

	// start with copying the strings and fill in the positin data for the segments to access
	// collect offset information for segments
	int32_t* strOffsets = new int32_t[scrStrings.size()];

	for (int32_t str = 0; str < scrStrings.size(); str++) {
		strOffsets[str] = currStrPos - dest;

		uint8_t strLength = scrStrings.at(str).length();

		*currStrPos++ = strLength;
		memcpy(currStrPos, scrStrings.at(str).data(), strLength);
		currStrPos += strLength;
	}

	// copy data segments
	for (uint32_t seg = 0; seg < segments.size(); seg++) {

		// update string offsets
		if (segments.at(seg).cmd == VARIABLE || segments.at(seg).cmd == STRING) {
			if (segments.at(seg).stringIndex == -1)
				return ERRBADSTRFORMAT;
			segments.at(seg).info = strOffsets[segments.at(seg).stringIndex] - (seg * 8);
		}

		memcpy(dest + (8 * seg), &segments.at(seg).cmd, 2);
		memcpy(dest + 2 + (8 * seg), &segments.at(seg).info, 4);
		memcpy(dest + 6 + (8 * seg), &segments.at(seg).unknown, 2);
	}

	delete[] strOffsets;
	return 0;
}

uint16_t DFscript::getCmdID(const std::string& command) {

	auto it = ScriptCommands.begin();
	while (it != ScriptCommands.end())
	{
		if (!command.compare(it->second))
			return  it->first;
		it++;
	}
	return 0;
}

void DFscript::checkLastCommand(std::vector<Segment>& seg, std::vector<std::string>& strs, std::string& lastCmd) {
	// if empty, all good.
	if (lastCmd.empty()) return;

	seg.push_back(Segment());
	if (uint16_t cmd = getCmdID(lastCmd)) {
		// command found!
		seg.back().cmd = cmd;
	}
	else {
		// assuming Variable. check if already existent in string vector
		seg.back().cmd = VARIABLE;
		for (int32_t str = 0; str < strs.size(); str++) {
			if (!lastCmd.compare(strs.at(str))) {
				seg.back().stringIndex = str;
				break;
			}
		}
		// was a string found? push in a new one if not.
		if (seg.back().stringIndex == -1) {
			strs.push_back(lastCmd);
			seg.back().stringIndex = strs.size() - 1;
		}
	}

	lastCmd.clear();
	return;
}