#include <iostream>

#include <boost/program_options.hpp>

#include "addressextract.hpp"

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
	Options opts;

	po::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "produce help message")
		("errors,e", po::bool_switch(&opts.errors), "Do error analysis")
		("missing,m", po::bool_switch(&opts.missing), "Only add missing postcode and city")
		("nocache", po::bool_switch(&opts.nocache), "Do not use boundary caching")
		("verbose,v", po::bool_switch(&opts.verbose), "Print index statistics to stdout")
		("postcoderegex", po::value<std::string>(&opts.postcoderegex)->default_value(opts.postcoderegex), "Postcode validation regex")
		("infile,i", po::value<std::string>(&opts.infile)->required(), "Input file")
		("outfile,o", po::value<std::string>(&opts.outfile)->required(), "Output file")
	;
	po::variables_map vm;
	try {
		po::store(po::parse_command_line(argc, argv, desc), vm);
		/* --help must work without the required options */
		if (!vm.count("help"))
			po::notify(vm);
	} catch(const po::error& e) {
		std::cerr << "Error: " << e.what() << "\n";
		std::cout << desc << std::endl;
		return -1;
	}

	if (vm.count("help")) {
		std::cout << desc << "\n";
		return 1;
	}

	process_file(opts);
	return 0;
}
