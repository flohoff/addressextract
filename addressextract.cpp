
#include <cstdlib>    // for std::exit
#include <cstring>    // for std::strcmp
#include <iostream>   // for std::cout, std::cerr
#include <sstream>    // for std::ostringstream
#include <functional> // for std::bind std::function

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
#include <nlohmann/json.hpp>

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

using json = nlohmann::json;
namespace po = boost::program_options;

static bool t_errors=false;
static bool t_missing=false;
static bool t_nocache=false;

void boundary_add_parent(AreaIndex<Boundary>& index, Boundary *b) {
	if (b->cache)
		return;
	if (b->up)
		return;

	std::vector<Boundary*> list;
	list.clear();
	index.findoverlapping(b, &list);
	std::sort(list.begin(), list.end(), compare_admin_level);

	for(auto overlapb : list) {
		if (b->id == overlapb->id)
			continue;
		if (b->cache)
			continue;
		if (b->admin_level <= overlapb->admin_level)
			continue;
		if (b->geometry->Within(overlapb->geometry.get())) {
			std::cerr << "Setting parent admin boundary on " << b->name
				<< " alvl " << b->admin_level
				<< " to " << overlapb->name
				<< " alvl " << overlapb->admin_level
				<< std::endl;
			b->up=overlapb;
			break;
		}
	}
}

void boundaryindex_add_hierarchy(AreaIndex<Boundary>& index) {
	for(auto& b : index.areavector) {
		boundary_add_parent(index, b.get());
	}
}

void process_file(po::variables_map& vm) {

	osmium::io::File input_file{vm["infile"].as<std::string>()};

	osmium::area::Assembler::config_type assembler_config;

	AreaIndex<Building>	buildingindex(false, 100,30,2,0.5);
	osmium::TagsFilter	buildingfilter{false};
	buildingfilter.add_rule(true, osmium::TagMatcher{"building"});
	osmium::area::MultipolygonManager<osmium::area::Assembler>
		buildingmp_manager{assembler_config, buildingfilter};

	AreaIndex<Boundary>	boundaryindex(!t_nocache, 100,30,2,0.5);
	osmium::TagsFilter	boundaryfilter{false};
	boundaryfilter.add_rule(true, "boundary", "administrative");
	osmium::area::MultipolygonManager<osmium::area::Assembler>
		boundarymp_manager{assembler_config, boundaryfilter};

	AreaIndex<PostCode>	postcodeindex(!t_nocache, 100,30,2,0.5);
	osmium::TagsFilter	postcodefilter{false};
	postcodefilter.add_rule(true, osmium::TagMatcher{"boundary", "postal_code"});
	osmium::area::MultipolygonManager<osmium::area::Assembler>
		postcodemp_manager{assembler_config, postcodefilter};

	std::cerr << "Reading relations for boundarys, postcodes and buildings" << std::endl;
	osmium::relations::read_relations(input_file,
			boundarymp_manager, postcodemp_manager, buildingmp_manager);

	index_type		index;
	location_handler_type	location_handler{index};
	location_handler.ignore_errors();

	std::cerr << "Building geometry indexes for boundarys, postcodes and buildings" << std::endl;
	osmium::io::Reader reader{input_file};
	osmium::io::Header header{reader.header()};

	json	jmeta;
	std::string timestamp=header.get("timestamp");
	if (!timestamp.empty()) {
		std::cerr << "Timestamp " << header.get("timestamp") << std::endl;
		jmeta["timestamp"]=timestamp;
	}

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

	if (!t_nocache)
		boundaryindex_add_hierarchy(boundaryindex);

	std::ofstream outfile(vm["outfile"].as<std::string>());

	outfile << "{" << std::endl
		<< "\"meta\": " << jmeta << "," << std::endl
		<< "\"addresses\": [" << std::endl;

	/* The finished record is held back until the next record arrives,
	 * so the "," lands at the end of every line but never after the
	 * last one. */
	std::string pendingline;
	auto AddressWriter=[&outfile,&pendingline](const Address::Object& ao) {
		std::ostringstream	line;

		line << ao;

		if (!pendingline.empty())
			outfile << pendingline << ",\n";
		pendingline=line.str();
	};

	std::cerr << "Looking for addresses" << std::endl;
	AddressHandler	ahandler{
			AddressWriter,
			boundaryindex,
			postcodeindex,
			buildingindex,
			t_errors,
			t_missing,
			t_nocache,
			vm["postcoderegex"].as<std::string>()};

	/* On the follow-up read pass the location index is already fully
	 * populated, so only the way() lookups are needed. Re-feeding every
	 * node into the FlexMem index (re-building and re-sorting it) is
	 * pure overhead, so the nodes are routed to a no-op handler here. */
	class WayOnlyLocationHandler : public osmium::handler::Handler {
		location_handler_type&	m_lh;
	public:
		explicit WayOnlyLocationHandler(location_handler_type& lh) : m_lh(lh) { }
		void way(osmium::Way& way) {
			m_lh.way(way);
		}
		void node(const osmium::Node& node) { (void)node; }
	} wayonlylocationhandler{location_handler};



	osmium::io::Reader readerpass3{input_file};
	osmium::apply(readerpass3,
			wayonlylocationhandler,
			ahandler);
	readerpass3.close();
	outfile << pendingline << std::endl;
	outfile << "]}" << std::endl;

	std::cerr << "Stats buildingindex: " << std::endl;
	buildingindex.dump_stats(std::cerr);
	std::cerr << "Stats postcodeindex: " << std::endl;
	postcodeindex.dump_stats(std::cerr);
	std::cerr << "Stats boundaryindex: " << std::endl;
	boundaryindex.dump_stats(std::cerr);

}

int main(int argc, char* argv[]) {

	po::options_description         desc("Allowed options");
	desc.add_options()
		("help,h", "produce help message")
		("errors,e", po::bool_switch(&t_errors), "Do error analysis")
		("missing,m", po::bool_switch(&t_missing), "Only add missing postcode and city")
		("nocache", po::bool_switch(&t_nocache), "Do not use boundary caching")
		("postcoderegex", po::value<std::string>()->default_value("^[0-9]{5}$"), "Postcode validation regex")
		("infile,i", po::value<std::string>()->required(), "Input file")
		("outfile,o", po::value<std::string>()->required(), "Output file")
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

	process_file(vm);
}
