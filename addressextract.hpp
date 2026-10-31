#ifndef ADDRESSEXTRACT_HPP
#define ADDRESSEXTRACT_HPP

#include <string>

struct Options {
	std::string	infile;
	std::string	outfile;
	std::string	postcoderegex="^[0-9]{5}$";
	bool		errors=false;
	bool		missing=false;
	bool		nocache=false;
	bool		verbose=false;
};

void process_file(const Options& opts);

#endif
