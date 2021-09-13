#ifndef SRC_ALGO_FIND_FEASIBLE_HPP_
#define SRC_ALGO_FIND_FEASIBLE_HPP_

#include "../geometry.hpp"
#include "../translation_vector.hpp"
#include "trim_vector.hpp"
#include "touching_point.hpp"

namespace libnfporb {
std::vector<TranslationVector> findFeasibleTranslationVectors(polygon_t::ring_type& ringA, polygon_t::ring_type& ringB, const std::vector<TouchingPoint>& touchers) {
	//use a set to automatically filter duplicate vectors
	std::vector<TranslationVector> potentialVectors;
	std::vector<std::pair<segment_t, segment_t>> touchEdges;

	for (psize_t i = 0; i < touchers.size(); i++) {
		point_t& vertexA = ringA[touchers[i].A_];
		vertexA.marked_ = true;

		// adjacent A vertices
		signed long prevAindex = touchers[i].A_ - 1;
		signed long nextAindex = touchers[i].A_ + 1;

		prevAindex = (prevAindex < 0) ? ringA.size() - 2 : prevAindex; // loop
		nextAindex = (static_cast<psize_t>(nextAindex) >= ringA.size()) ? 1 : nextAindex; // loop

		point_t& prevA = ringA[prevAindex];
		point_t& nextA = ringA[nextAindex];

		// adjacent B vertices
		point_t& vertexB = ringB[touchers[i].B_];

		signed long prevBindex = touchers[i].B_ - 1;
		signed long nextBindex = touchers[i].B_ + 1;

		prevBindex = (prevBindex < 0) ? ringB.size() - 2 : prevBindex; // loop
		nextBindex = (static_cast<psize_t>(nextBindex) >= ringB.size()) ? 1 : nextBindex; // loop

		point_t& prevB = ringB[prevBindex];
		point_t& nextB = ringB[nextBindex];

		if (touchers[i].type_ == TouchingPoint::VERTEX) {
			segment_t a1 = { vertexA, nextA };
			segment_t a2 = { vertexA, prevA };
			segment_t b1 = { vertexB, nextB };
			segment_t b2 = { vertexB, prevB };

			//swap the segment elements so that always the first point is the touching point
			//also make the second segment always a segment of ringB
			touchEdges.push_back( { a1, b1 });
			touchEdges.push_back( { a1, b2 });
			touchEdges.push_back( { a2, b1 });
			touchEdges.push_back( { a2, b2 });
#ifdef NFP_DEBUG
			write_svg("touchersV" + std::to_string(i) + ".svg", {a1,a2,b1,b2});
#endif

			//TODO test parallel edges for floating point stability
			Alignment al;
			//a1 and b1 meet at start vertex
			al = get_alignment(a1, b1.second);
			if (al == LEFT) {
				potentialVectors.push_back( { b1.first - b1.second, b1, false, "vertex1" });
			} else if (al == RIGHT) {
				potentialVectors.push_back( { a1.second - a1.first, a1, true, "vertex2" });
			} else {
				potentialVectors.push_back( { a1.second - a1.first, a1, true, "vertex3" });
			}

			//a1 and b2 meet at start and end
			al = get_alignment(a1, b2.second);
			if (al == LEFT) {
				//no feasible translation
				DEBUG_MSG("not feasible", a1.second - a1.first);
			} else if (al == RIGHT) {
				potentialVectors.push_back( { a1.second - a1.first, a1, true, "vertex5" });
			} else {
				potentialVectors.push_back( { a1.second - a1.first, a1, true, "vertex6" });
			}

			//a2 and b1 meet at end and start
			al = get_alignment(a2, b1.second);
			if (al == LEFT) {
				potentialVectors.push_back( { b1.first - b1.second, b1, false, "vertex7" });
			} else if (al == RIGHT) {
				potentialVectors.push_back( { b1.first - b1.second, b1, false, "vertex8" });
			} else {
				potentialVectors.push_back( { b1.first - b1.second, b1, false, "vertex9" });
			}
		} else if (touchers[i].type_ == TouchingPoint::B_ON_A) {
			segment_t a1 = { vertexB, vertexA };
			segment_t a2 = { vertexB, prevA };
			segment_t b1 = { vertexB, prevB };
			segment_t b2 = { vertexB, nextB };

			touchEdges.push_back( { a1, b1 });
			touchEdges.push_back( { a1, b2 });
			touchEdges.push_back( { a2, b1 });
			touchEdges.push_back( { a2, b2 });
#ifdef NFP_DEBUG
			write_svg("touchersB" + std::to_string(i) + ".svg", {a1,a2,b1,b2});
#endif
			potentialVectors.push_back( { vertexA - vertexB, { vertexB, vertexA }, true, "bona" });
		} else if (touchers[i].type_ == TouchingPoint::A_ON_B) {
			//TODO testme
			segment_t a1 = { vertexA, prevA };
			segment_t a2 = { vertexA, nextA };
			segment_t b1 = { vertexA, vertexB };
			segment_t b2 = { vertexA, prevB };
#ifdef NFP_DEBUG
			write_svg("touchersA" + std::to_string(i) + ".svg", {a1,a2,b1,b2});
#endif
			touchEdges.push_back( { a1, b1 });
			touchEdges.push_back( { a2, b1 });
			touchEdges.push_back( { a1, b2 });
			touchEdges.push_back( { a2, b2 });
			potentialVectors.push_back( { vertexA - vertexB, { vertexA, vertexB }, false, "aonb" });
		}
	}

#ifdef NFP_DEBUG
	DEBUG_VAL("touching edges:");
	std::stringstream ss;

	for(const auto& te: touchEdges) {
		ss << te.first << " (" << (te.first.second - te.first.first) << ") <-> " << te.second << " (" << (te.second.second - te.second.first) << ')';
		DEBUG_VAL(ss.str());
		ss.str("");
	}
	DEBUG_VAL("");

	DEBUG_VAL("potential vectors:");
	for(const auto& pv: potentialVectors) {
		DEBUG_VAL(pv);
	}
	DEBUG_VAL("");
#endif
	//discard immediately intersecting translations
	std::vector<TranslationVector> vectors;
	for (const auto& v : potentialVectors) {
		bool discarded = false;
		for (const auto& sp : touchEdges) {
			point_t normEdge = normalize(v.edge_.second - v.edge_.first);
			point_t normFirst = normalize(sp.first.second - sp.first.first);
			point_t normSecond = normalize(sp.second.second - sp.second.first);

			Alignment a1 = get_alignment( { { 0, 0 }, normEdge }, normFirst);
			Alignment a2 = get_alignment( { { 0, 0 }, normEdge }, normSecond);

			if (a1 == a2 && a1 != ON) {
				LongDouble df = get_inner_angle( { 0, 0 }, normEdge, normFirst);
				LongDouble ds = get_inner_angle( { 0, 0 }, normEdge, normSecond);

				point_t normIn = normalize(v.edge_.second - v.edge_.first);
				if (equals(df, ds)) {
					TranslationVector trimmed = trimVector(ringA, ringB, v);
					polygon_t::ring_type translated;
					trans::translate_transformer<coord_t, 2, 2> translate(trimmed.vector_.x_, trimmed.vector_.y_);
					boost::geometry::transform(ringB, translated, translate);
					DEBUG_MSG("intersects", bg::intersects(translated, ringA));
					DEBUG_MSG("overlaps", bg::overlaps(translated, ringA));
					DEBUG_MSG("covered_byL", bg::covered_by(translated, ringA));
					DEBUG_MSG("covered_byR", bg::covered_by(ringA, translated));

					if (!(bg::intersects(translated, ringA) && !bg::overlaps(translated, ringA) && !bg::covered_by(translated, ringA) && !bg::covered_by(ringA, translated))) {
						DEBUG_MSG("discarded0", v);
						discarded = true;
						break;
					}
				} else {
					if (equals(normIn, normalize(v.vector_))) {
						if (!equals(df, 0) && larger(ds, df)) {
							DEBUG_MSG("df", df);
							DEBUG_MSG("ds", ds);
							DEBUG_MSG("discarded1", v);
							discarded = true;
							break;
						}
					} else {
						if (!equals(ds, 0) && smaller(ds, df)) {
							DEBUG_MSG("df", df);
							DEBUG_MSG("ds", ds);
							DEBUG_MSG("discarded2", v);
							discarded = true;
							break;
						}
					}
				}
			}
		}
		if (!discarded) {
			vectors.push_back(v);
		}
	}
	return vectors;
}

}


#endif /* SRC_ALGO_FIND_FEASIBLE_HPP_ */
