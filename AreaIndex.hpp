#ifndef AREAINDEX_HPP
#define AREAINDEX_HPP

#include <SpatialIndex.h>
#include <osmium/handler.hpp>
#include <osmium/geom/ogr.hpp>
#include <iostream>

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
	std::unique_ptr<si::IStorageManager>	sm;
	std::unique_ptr<si::ISpatialIndex>	rtree;

	si::id_type index_id;

	typedef std::array<double, 2> coord_array_t;
	int64_t		id=0;

	osmium::geom::OGRFactory<>	m_factory;
	OGRSpatialReference		oSRS;

	std::vector<AreaType*>		areavector;
public:
	int	returned=0;
	int	compared=0;
	int	queries=0;
	int	inserts=0;

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
	AreaIndex(int icapacity, int lcapacity, int dim, double fill) :
		sm(si::StorageManager::createNewMemoryStorageManager()),
		rtree(si::RTree::createNewRTree(*sm, fill, icapacity, lcapacity, dim, si::RTree::RV_LINEAR, index_id)) {

		oSRS.importFromEPSG(4326);
	}

	~AreaIndex() {
		while(!areavector.empty()) {
			AreaType *at=areavector.back();
			areavector.pop_back();
			delete(at);
		}
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
		inserts++;
	}

	// This callback is called by osmium::apply for each area in the data.
	void area(const osmium::Area& area) {
		try {
			std::unique_ptr<OGRGeometry>	geom=m_factory.create_multipolygon(area);
			geom->assignSpatialReference(&oSRS);

			AreaType	*a=new AreaType(std::move(geom), area);
			areavector.push_back(a);

			insert(a);

		} catch (const osmium::geometry_error& e) {
			std::cerr << "GEOMETRY ERROR: " << e.what() << "\n";
		} catch (const osmium::invalid_location& e) {
			std::cerr << "Invalid location way id " << area.orig_id() << std::endl;
		}
	}

	void dump_stats(std::ostream& os) {
		os << " Inserts: "
			<< inserts << std::endl
			<< " Queries: "
			<< queries << std::endl
			<< " Returned: "
			<< returned << std::endl
			<< " Compared: "
			<< compared << std::endl
			<< std::endl;
	}
};

#endif
