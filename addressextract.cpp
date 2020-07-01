
#include <cstdlib>  // for std::exit
#include <cstring>  // for std::strcmp
#include <iostream> // for std::cout, std::cerr

// For assembling multipolygons
#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_manager.hpp>

// For the DynamicHandler class
#include <osmium/dynamic_handler.hpp>

// For the Dump handler
#include <osmium/handler/dump.hpp>

// For the NodeLocationForWays handler
#include <osmium/handler/node_locations_for_ways.hpp>

// Allow any format of input files (XML, PBF, ...)
#include <osmium/io/any_input.hpp>

// For the location index. There are different types of indexes available.
// This will work for all input files keeping the index in memory.
#include <osmium/index/map/flex_mem.hpp>

#include <boost/program_options.hpp>
#include <boost/format.hpp>

#include "Building.hpp"
#include "Boundary.hpp"
#include "PostCode.hpp"
#include "AreaIndex.hpp"
#include "AddressHandler.hpp"

// The type of index used. This must match the include file above
using index_type = osmium::index::map::FlexMem<osmium::unsigned_object_id_type, osmium::Location>;

// The location handler always depends on the index type
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

#define DEBUG 0

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
	bool t_errors=false;
	bool t_missing=false;
	bool t_nocache=false;

	po::options_description         desc("Allowed options");
	desc.add_options()
		("help,h", "produce help message")
		("errors,e", po::bool_switch(&t_errors), "Do error analysis")
		("missing,m", po::bool_switch(&t_missing), "Only add missing postcode and city")
		("nocache", po::bool_switch(&t_nocache), "Do not use boundary caching")
		("infile,i", po::value<std::string>()->required(), "Input file")
	;
	po::variables_map vm;
	try {
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);
	} catch(const boost::program_options::error& e) {
		std::cerr << "Error: " << e.what() << "\n";
		std::cout << desc << std::endl;
		exit(-1);
	}

	if (vm.count("help")) {
		std::cout << desc << "\n";
		return 1;
	}

	osmium::io::File input_file{vm["infile"].as<std::string>()};

	osmium::area::Assembler::config_type assembler_config;


	AreaIndex<Building>	buildingindex;
	osmium::TagsFilter	buildingfilter{false};
	buildingfilter.add_rule(true, osmium::TagMatcher{"building"});
	osmium::area::MultipolygonManager<osmium::area::Assembler> buildingmp_manager{assembler_config, buildingfilter};

	AreaIndex<Boundary>	boundaryindex;
	osmium::TagsFilter	boundaryfilter{false};
	boundaryfilter.add_rule(true, osmium::TagMatcher{"boundary", "administrative"});
	boundaryfilter.add_rule(true, osmium::TagMatcher{"admin_level"});
	osmium::area::MultipolygonManager<osmium::area::Assembler> boundarymp_manager{assembler_config, boundaryfilter};

	AreaIndex<PostCode>	postcodeindex;
	osmium::TagsFilter	postcodefilter{false};
	postcodefilter.add_rule(true, osmium::TagMatcher{"boundary", "postal_code"});
	osmium::area::MultipolygonManager<osmium::area::Assembler> postcodemp_manager{assembler_config, postcodefilter};

	// We read the input file twice. In the first pass, only relations are
	// read and fed into the multipolygon manager.
	std::cerr << "Reading relations for boundarys and postcodes" << std::endl;
	osmium::relations::read_relations(input_file, boundarymp_manager, postcodemp_manager, buildingmp_manager);

	index_type		index;
	location_handler_type	location_handler{index};
	location_handler.ignore_errors();

	std::cerr << "Building geometry indexes for boundarys and postcodes" << std::endl;
	osmium::io::Reader reader{input_file};
	osmium::io::Header header{reader.header()};

	std::cerr << "Timestamp " << header.get("timestamp") << std::endl;

	osmium::apply(reader,
		location_handler,
		boundarymp_manager.handler([&boundaryindex](osmium::memory::Buffer&& buffer) {
			osmium::apply(buffer, boundaryindex);
		}),
		postcodemp_manager.handler([&postcodeindex](osmium::memory::Buffer&& buffer) {
			osmium::apply(buffer, postcodeindex);
		}),
		buildingmp_manager.handler([&buildingindex](osmium::memory::Buffer&& buffer) {
			osmium::apply(buffer, buildingindex);
		})
	);
	reader.close();

	std::cerr << "Looking for addresses" << std::endl;
	AddressHandler	ahandler{boundaryindex, postcodeindex, buildingindex,
		t_errors, t_missing, t_nocache, header.get("timestamp")};

	osmium::io::Reader readerpass3{input_file};
	osmium::apply(readerpass3,
			location_handler,
			ahandler);
	readerpass3.close();

	ahandler.dump();

	OGRRegisterAll();
}
