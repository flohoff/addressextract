
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

	po::options_description         desc("Allowed options");
	desc.add_options()
		("help,h", "produce help message")
		("infile,i", po::value<std::string>(), "Input file")
	;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	osmium::io::File input_file{vm["infile"].as<std::string>()};



	osmium::area::Assembler::config_type assembler_config;

	AreaIndex<Boundary>	boundaryindex;
	osmium::TagsFilter	boundaryfilter{false};
	boundaryfilter.add_rule(true, osmium::TagMatcher{"boundary", "administrative"});
	osmium::area::MultipolygonManager<osmium::area::Assembler> boundarymp_manager{assembler_config, boundaryfilter};

	AreaIndex<PostCode>	postcodeindex;
	osmium::TagsFilter	postcodefilter{false};
	postcodefilter.add_rule(true, osmium::TagMatcher{"boundary", "postal_code"});
	osmium::area::MultipolygonManager<osmium::area::Assembler> postcodemp_manager{assembler_config, postcodefilter};

	// We read the input file twice. In the first pass, only relations are
	// read and fed into the multipolygon manager.
	std::cerr << "Reading relations for boundarys and postcodes" << std::endl;
	osmium::relations::read_relations(input_file, boundarymp_manager, postcodemp_manager);

	index_type		index;
	location_handler_type	location_handler{index};
	location_handler.ignore_errors();

	std::cerr << "Building geometry indexes for boundarys and postcodes" << std::endl;
	osmium::io::Reader reader{input_file};
	osmium::apply(reader,
		location_handler,
		boundarymp_manager.handler([&boundaryindex](osmium::memory::Buffer&& buffer) {
			osmium::apply(buffer, boundaryindex);
		}),
		postcodemp_manager.handler([&postcodeindex](osmium::memory::Buffer&& buffer) {
			osmium::apply(buffer, postcodeindex);
		})
	);
	reader.close();

	std::cerr << "Looking for addresses" << std::endl;
	AddressHandler	ahandler{boundaryindex, postcodeindex};

	osmium::io::Reader readerpass3{input_file};
	osmium::apply(readerpass3,
			location_handler,
			ahandler);
	readerpass3.close();

	ahandler.dump();

	OGRRegisterAll();
}
