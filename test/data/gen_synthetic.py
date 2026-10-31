#!/usr/bin/env python3
"""Generates synthetic.osm. Run: ./gen_synthetic.py > synthetic.osm"""
from xml.sax.saxutils import quoteattr

nodes, ways, rels = [], [], []
nid = [0]

def node(lon, lat, tags=None):
    nid[0] += 1
    nodes.append((nid[0], lon, lat, tags or {}))
    return nid[0]

wid = [0]
def way(refs, tags=None):
    wid[0] += 1
    ways.append((wid[0], refs, tags or {}))
    return wid[0]

rid = [0]
def rel(outer, tags):
    rid[0] += 1
    rels.append((rid[0], outer, tags))

def ring(pts, tags=None):
    """closed way from points"""
    ids = [node(x, y) for x, y in pts]
    return way(ids + [ids[0]], tags)

def box(x0, y0, x1, y1):
    return [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]

def area(pts, tags):
    w = ring(pts)
    t = {"type": "multipolygon"}
    t.update(tags)
    rel(w, t)

area(box(8.40, 51.90, 8.80, 52.30), {"boundary": "administrative", "admin_level": "6", "name": "Kreis Test"})
area(box(8.50, 52.00, 8.70, 52.20), {"boundary": "administrative", "admin_level": "8", "name": "Teststadt"})
area([(8.55, 52.00), (8.60, 52.05), (8.55, 52.10), (8.50, 52.05)],
     {"boundary": "administrative", "admin_level": "10", "name": "Altstadt"})
area(box(8.51, 52.11, 8.52, 52.12), {"boundary": "administrative", "admin_level": "10"})
area(box(8.45, 51.95, 8.65, 52.15), {"boundary": "postal_code", "postal_code": "33602"})
# a postcode without code is dropped
area(box(8.45, 52.16, 8.46, 52.17), {"boundary": "postal_code"})

# Building with its own address; the nodes inside are checked against it
ring(box(8.5200, 52.0200, 8.5210, 52.0210),
     {"building": "yes", "addr:street": "Hauptstraße", "addr:housenumber": "10",
      "addr:city": "Teststadt", "addr:postcode": "33602"})
# Plain building without address: not indexed as address
ring(box(8.5300, 52.0300, 8.5310, 52.0310), {"building": "yes"})

full = {"addr:street": "Hauptstraße", "addr:housenumber": "1", "addr:city": "Teststadt", "addr:postcode": "33602"}
def addr(lon, lat, **over):
    t = dict(full)
    for k, v in over.items():
        k = k.replace("_", ":", 1)
        if v is None:
            t.pop(k, None)
        else:
            t[k] = v
    node(lon, lat, t)

addr(8.5250, 52.0250)                                   # in suburb: clean
addr(8.6500, 52.1000, addr_city="Teststadt")            # in city, outside suburb
addr(8.6500, 52.1500, addr_housenumber=None)            # No housenumber
addr(8.6200, 52.1400, addr_postcode="33699")            # postcode mismatch
addr(8.6500, 52.1700, addr_city="Falschstadt")          # city mismatch
addr(8.6600, 52.1000, addr_country="Deutschland")       # country format
addr(8.6600, 52.1100, addr_housenumber="4a,b")          # housenumber format
addr(8.6600, 52.1200, addr_street="Haupt;str.")         # street format
addr(8.6600, 52.1300, addr_postcode="ABC")              # postcode format
addr(8.6600, 52.1400, addr_street=None)                 # No street
addr(8.6600, 52.1500, addr_city=None)                   # No city
addr(8.4200, 51.9200)                                   # county only
addr(9.5000, 53.0000)                                   # outside everything
# inside the addressed building, conflicting city
addr(8.52050, 52.02050, addr_city="Andersstadt")
addr(8.52055, 52.02055)                                 # inside, matching
# addr:place instead of street
addr(8.6700, 52.1000, addr_street=None, addr_place="Auf dem Berg")
# nodes without address tags
node(8.5, 52.0, {"amenity": "bench"})
node(8.6, 52.1)

# Address on a way (centroid is used)
w_ids = [node(8.6700, 52.1100), node(8.6710, 52.1100), node(8.6710, 52.1110)]
way(w_ids, {"addr:street": "Weg", "addr:housenumber": "7", "addr:city": "Teststadt", "addr:postcode": "33602"})
# plain way
way(w_ids[:2], {"highway": "residential"})

out = ['<?xml version="1.0" encoding="UTF-8"?>',
       '<osm version="0.6" generator="gen_synthetic.py">']
def attrs(tags):
    return "".join('    <tag k=%s v=%s/>\n' % (quoteattr(k), quoteattr(v)) for k, v in tags.items())
for i, lon, lat, tags in sorted(nodes):
    out.append('  <node id="%d" version="1" timestamp="2026-01-01T00:00:00Z" uid="1" user="t" changeset="1" lat="%.5f" lon="%.5f"%s' %
               (i, lat, lon, ">\n" + attrs(tags) + "  </node>" if tags else "/>"))
for i, refs, tags in ways:
    out.append('  <way id="%d" version="1" timestamp="2026-01-01T00:00:00Z" uid="1" user="t" changeset="1">' % i)
    out.extend('    <nd ref="%d"/>' % r for r in refs)
    out.append(attrs(tags).rstrip("\n") if tags else "")
    out.append('  </way>')
for i, outer, tags in rels:
    out.append('  <relation id="%d" version="1" timestamp="2026-01-01T00:00:00Z" uid="1" user="t" changeset="1">' % i)
    out.append('    <member type="way" ref="%d" role="outer"/>' % outer)
    out.append(attrs(tags).rstrip("\n"))
    out.append('  </relation>')
out.append('</osm>')
print("\n".join(l for l in out if l != ""))
