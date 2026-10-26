#ifndef AREAINDEX_HPP
#define AREAINDEX_HPP

#include <osmium/handler.hpp>
#include <osmium/geom/ogr.hpp>
#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/rtree.hpp>

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

typedef bg::model::point<double, 2, bg::cs::cartesian>	index_point_t;
typedef bg::model::box<index_point_t>			index_box_t;
typedef std::pair<index_box_t, void *>			index_value_t;

template <typename AreaType>
class AreaIndex : public osmium::handler::Handler {
	bool		cachegrid=false;
	bgi::rtree<index_value_t, bgi::quadratic<16, 4>>	rtree;

	int64_t		id=0;

	osmium::geom::OGRFactory<>	m_factory;
	OGRSpatialReference		oSRS;

public:
	std::vector<std::unique_ptr<AreaType>>		areavector;
	std::vector<std::unique_ptr<AreaType>>		cachevector;

	int	returned=0;
	int	compared=0;
	int	queries=0;
	int	inserts=0;
	int	cachehitpos=0;
	int	cachehitneg=0;
	int	cacheentriespos=0;

private:
	/* Envelope of an area as an rtree box */
	index_box_t box(AreaType *area) {
		OGREnvelope	env;
		area->envelope(env);

		return index_box_t(
			index_point_t(env.MinX, env.MinY),
			index_point_t(env.MaxX, env.MaxY));
	}

public:
	AreaIndex(bool cachegrid) :
		cachegrid(cachegrid) {

		oSRS.importFromEPSG(4326);
	}

	void findoverlapping(AreaType *area, std::vector<AreaType*> *list) {
		std::vector<index_value_t> results;
		rtree.query(bgi::intersects(box(area)), std::back_inserter(results));

		list->reserve(list->size() + results.size());
		for(auto &v : results)
			list->push_back((AreaType *) v.second);
	}

	void findoverlapping_geom(OGRGeometry *geom, std::vector<AreaType*> *list) {
		OGREnvelope	env;
		geom->getEnvelope(&env);

		index_box_t qbox(
			index_point_t(env.MinX, env.MinY),
			index_point_t(env.MaxX, env.MaxY));

		std::vector<index_value_t> results;
		rtree.query(bgi::intersects(qbox), std::back_inserter(results));

		list->reserve(list->size() + results.size());
		for(auto &v : results)
			list->push_back((AreaType *) v.second);
	}

/* Recursive (quadtree) segmentation of the area envelope. */
#define GRID_MAX_LEVEL	4
#define CACHE_NEGATIVE_CELLS 0

	std::unique_ptr<OGRGeometry> grid_cell(double minX, double minY, double maxX, double maxY) {
		auto ring=std::make_unique<OGRLinearRing>();

		ring->addPoint(minX, minY);
		ring->addPoint(minX, maxY);
		ring->addPoint(maxX, maxY);
		ring->addPoint(maxX, minY);
		ring->addPoint(minX, minY);
		ring->assignSpatialReference(&oSRS);

		auto geom=std::make_unique<OGRPolygon>();
		geom->addRingDirectly(ring.release());

		return geom;
	}

	void cache_insert_cell(AreaType *area, const osmium::Area& osmarea,
			std::unique_ptr<OGRGeometry> cell, bool within) {

		auto cachea=std::make_unique<AreaType>(std::move(cell), osmarea, false);

		cachea->cache=true;
		cachea->cachewithin=within;
		cachea->cacheintersects=within;
		cachea->parent=area;

		cacheentriespos++;
		rtree.insert(index_value_t(box(cachea.get()), cachea.get()));
		cachevector.push_back(std::move(cachea));
	}

	/* Recurse down the quadtree */
	void cache_subdivide(AreaType *area, const osmium::Area& osmarea,
			double minX, double minY, double maxX, double maxY, int level) {
		auto cell=grid_cell(minX, minY, maxX, maxY);

		/* Cells fully outside the geometry */
		if (level > 0 && !area->intersects(cell.get())) {
			return;
		}

		if (area->contains(cell.get())) {
			cache_insert_cell(area, osmarea, std::move(cell), true);
			return;
		}

		if (level >= GRID_MAX_LEVEL)
			return;

		double	midX=(minX+maxX)/2;
		double	midY=(minY+maxY)/2;

		cache_subdivide(area, osmarea, minX, minY, midX, midY, level+1);
		cache_subdivide(area, osmarea, midX, minY, maxX, midY, level+1);
		cache_subdivide(area, osmarea, minX, midY, midX, maxY, level+1);
		cache_subdivide(area, osmarea, midX, midY, maxX, maxY, level+1);
	}

	void insert_cache_grid(AreaType *area, const osmium::Area& osmarea) {
		OGREnvelope	env;
		area->envelope(env);

		cache_subdivide(area, osmarea, env.MinX, env.MinY, env.MaxX, env.MaxY, 0);
	}

	void insert(AreaType *area, const osmium::Area& osmarea) {
		rtree.insert(index_value_t(box(area), area));
		inserts++;

		if (cachegrid)
			insert_cache_grid(area, osmarea);
	}

	// This callback is called by osmium::apply for each area in the data.
	void area(const osmium::Area& osmarea) {
		try {
			std::unique_ptr<OGRGeometry>	geom=m_factory.create_multipolygon(osmarea);
			geom->assignSpatialReference(&oSRS);

			auto a=std::make_unique<AreaType>(std::move(geom), osmarea);

			if (!a->valid())
				return;

			areavector.push_back(std::move(a));
			insert(areavector.back().get(), osmarea);

		} catch (const osmium::geometry_error& e) {
			std::cerr << "GEOMETRY ERROR: " << e.what() << "\n";
		} catch (const osmium::invalid_location& e) {
			std::cerr << "Invalid location way id " << osmarea.orig_id() << std::endl;
		}
	}

	void dump_stats(std::ostream& os) {
		os << " Inserts: "
			<< inserts << std::endl
			<< " Cacheentriespos: "
			<< cacheentriespos << std::endl
			<< " Queries: "
			<< queries << std::endl
			<< " Returned: "
			<< returned << std::endl
			<< " Cachehitpos: "
			<< cachehitpos << std::endl
			<< " Cachehitneg: "
			<< cachehitneg << std::endl
			<< " Compared: "
			<< compared << std::endl
			<< std::endl;
	}
};

#endif
