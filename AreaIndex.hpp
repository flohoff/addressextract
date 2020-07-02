#ifndef AREAINDEX_HPP
#define AREAINDEX_HPP

#include <SpatialIndex.h>
#include <osmium/handler.hpp>
#include <osmium/geom/ogr.hpp>

#define DEBUG	0

namespace si = SpatialIndex;

template <typename AT>
class query_visitor : public si::IVisitor {
	std::vector<AT*>	*list;

	public:

	query_visitor(std::vector<AT*> *l) : list(l){}

	void visitNode(si::INode const& n) {
		(void)n;
	}

	void visitData(si::IData const& d) {
		uint64_t	id=d.getIdentifier();
		list->push_back((AT *)id);
	}

	void visitData(std::vector<si::IData const*>& v) {
		assert(!v.empty()); (void)v;
	}
};

template <typename AreaType>
class AreaIndex : public osmium::handler::Handler{
	si::ISpatialIndex	*rtree;
	si::IStorageManager	*sm;

	si::id_type index_id;
	uint32_t const index_capacity = 100;
	uint32_t const leaf_capacity = 100;
	uint32_t const dimension = 2;
	double const fill_factor = 0.5;

	typedef std::array<double, 2> coord_array_t;
	int64_t		id=0;

	osmium::geom::OGRFactory<>	m_factory;
	OGRSpatialReference		oSRS;
public:
	std::vector<AreaType*>		arealist;

private:
	si::Region region(AreaType *area) {
		OGREnvelope	env;
		area->envelope(env);

		coord_array_t const p1 = { env.MinX, env.MinY };
		coord_array_t const p2 = { env.MaxX, env.MaxY };

		si::Region region(
			si::Point(p1.data(), p1.size()),
			si::Point(p2.data(), p2.size()));

		return region;
	}

public:
	AreaIndex() {
		sm=si::StorageManager::createNewMemoryStorageManager();
		rtree=si::RTree::createNewRTree(*sm, fill_factor, index_capacity, leaf_capacity, dimension, si::RTree::RV_LINEAR, index_id);

		oSRS.importFromEPSG(4326);
	}

	void findoverlapping(AreaType *area, std::vector<AreaType*> *list) {
		query_visitor<AreaType> qvisitor{list};
		rtree->intersectsWithQuery(region(area), qvisitor);
	}

	void findoverlapping_geom(OGRGeometry *geom, std::vector<AreaType*> *list) {
		query_visitor<AreaType> qvisitor{list};

		OGREnvelope	env;
		geom->getEnvelope(&env);

		coord_array_t const p1 = { env.MinX, env.MinY };
		coord_array_t const p2 = { env.MaxX, env.MaxY };

		si::Region region(
			si::Point(p1.data(), p1.size()),
			si::Point(p2.data(), p2.size()));

		rtree->intersectsWithQuery(region, qvisitor);
	}

	void insert(AreaType *area) {
		if (DEBUG)
			std::cout << "Insert: " << area->id << std::endl;
		rtree->insertData(0, nullptr, region(area), (uint64_t) area);
	}

	// This callback is called by osmium::apply for each area in the data.
	void area(const osmium::Area& area) {
		try {
			std::unique_ptr<OGRGeometry>	geom=m_factory.create_multipolygon(area);
			geom->assignSpatialReference(&oSRS);
			AreaType	*a=new AreaType{std::move(geom), area};
			insert(a);
		} catch (const osmium::geometry_error& e) {
			std::cerr << "GEOMETRY ERROR: " << e.what() << "\n";
		} catch (const osmium::invalid_location& e) {
			std::cerr << "Invalid location way id " << area.orig_id() << std::endl;
		}
	}
};

#endif
