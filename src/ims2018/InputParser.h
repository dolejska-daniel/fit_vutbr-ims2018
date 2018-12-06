///
/// Prevzato z
/// @link https://stackoverflow.com/a/868894/3078351
///
#ifndef _INPUTPARSER_H
#define _INPUTPARSER_H

#include <string>
#include <vector>
#include <algorithm>

class InputParser
{
public:
	InputParser(int &argc, char **argv);

	/// @author iain
	const std::string& getCmdOption(const std::string &option) const;

	/// @author iain
	bool cmdOptionExists(const std::string &option) const;

private:
	std::vector <std::string> tokens;
};

#endif //_INPUTPARSER_H
