#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

#include "addressextract.hpp"

static const std::string datadir=TEST_DATA_DIR;

/* unique per process: mutation testing runs tests in parallel */
static std::string tmpfile(const std::string& name) {
	return (std::filesystem::temp_directory_path() /
		("unittest_" + std::to_string(getpid()) + "_" + name + ".json")).string();
}

static std::string slurp(const std::string& path) {
	std::ifstream in(path);
	std::stringstream ss;
	ss << in.rdbuf();
	return ss.str();
}

static Options options(const std::string& out, bool errors=false, bool missing=false,
		bool nocache=false, bool verbose=false) {
	Options o;
	o.infile=datadir + "/synthetic.osm";
	o.outfile=out;
	o.errors=errors;
	o.missing=missing;
	o.nocache=nocache;
	o.verbose=verbose;
	return o;
}

/* Run on synthetic.osm, compare with golden file */
static void golden(const std::string& expected, bool errors, bool missing, bool nocache) {
	auto out=tmpfile(expected);
	process_file(options(out, errors, missing, nocache));
	REQUIRE(slurp(out) == slurp(datadir + "/expected_" + expected + ".json"));
	std::filesystem::remove(out);
}

TEST_CASE("run: default") { golden("plain", false, false, false); }
TEST_CASE("run: errors") { golden("errors", true, false, false); }
TEST_CASE("run: missing") { golden("missing", false, true, false); }
TEST_CASE("run: nocache plain") { golden("plain", false, false, true); }
TEST_CASE("run: errors nocache") { golden("errors", true, false, true); }
TEST_CASE("run: missing nocache") { golden("missing", false, true, true); }

TEST_CASE("run: postcode regex option is applied") {
	auto out=tmpfile("regex");
	auto o=options(out, true);
	o.postcoderegex="^9{5}$";
	process_file(o);
	/* no synthetic postcode matches, so the format error shows up */
	REQUIRE(slurp(out).find("Postcode format issues") != std::string::npos);
	std::filesystem::remove(out);
}

/* stdout of a run */
static std::string stdout_of(const Options& o) {
	std::ostringstream captured;
	auto *old=std::cout.rdbuf(captured.rdbuf());
	process_file(o);
	std::cout.rdbuf(old);
	std::filesystem::remove(o.outfile);
	return captured.str();
}

static void golden_stats(const std::string& name, bool errors, bool missing, bool nocache) {
	auto o=options(tmpfile("stats"), errors, missing, nocache, true);
	REQUIRE(stdout_of(o) == slurp(datadir + "/expected_stats_" + name + ".txt"));
}

TEST_CASE("stats: no output without -v") {
	REQUIRE(stdout_of(options(tmpfile("quiet"), true)).empty());
}

TEST_CASE("stats: errors") { golden_stats("errors", true, false, false); }
TEST_CASE("stats: missing") { golden_stats("missing", false, true, false); }
TEST_CASE("stats: errors nocache") { golden_stats("errors_nocache", true, false, true); }
TEST_CASE("stats: plain") { golden_stats("plain", false, false, false); }
