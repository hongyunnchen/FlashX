/**
 * Copyright 2014 Open Connectome Project (http://openconnecto.me)
 * Written by Da Zheng (zhengda1936@gmail.com)
 *
 * This file is part of FlashGraph.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <Rcpp.h>

#include "FGlib.h"

#if 0
size_t estimate_diameter(FG_graph::ptr fg, int num_bfs, bool directed,
		int num_sweeps);
FG_vector<float>::ptr compute_sstsg(FG_graph::ptr fg, time_t start_time,
		time_t interval, int num_intervals);
void fetch_subgraphs(FG_graph::ptr graph, FG_vector<vertex_id_t>::ptr cluster_ids,
		const std::set<vertex_id_t> &wanted_clusters, std::map<vertex_id_t,
		graph::ptr> &clusters);
void compute_subgraph_sizes(FG_graph::ptr graph, FG_vector<vertex_id_t>::ptr cluster_ids,
		const std::set<vertex_id_t> &wanted_clusters,
		std::map<vertex_id_t, std::pair<size_t, size_t> > &sizes);
FG_vector<float>::ptr compute_betweenness_centrality(FG_graph::ptr fg, vertex_id_t id);
FG_vector<vsize_t>::ptr get_ts_degree(FG_graph::ptr fg, edge_type type,
		time_t start_time, time_t time_interval);
#endif

FG_graph::ptr R_FG_get_graph(SEXP graph)
{
	std::string graph_file = CHAR(STRING_ELT(VECTOR_ELT(graph, 0), 0));
	std::string index_file = CHAR(STRING_ELT(VECTOR_ELT(graph, 1), 0));
	std::string conf_file = CHAR(STRING_ELT(VECTOR_ELT(graph, 2), 0));
	config_map::ptr configs = config_map::create(conf_file);
	if (!configs) {
		fprintf(stderr, "can't read conf file\n");
		abort();
	}
	return FG_graph::create(graph_file, index_file, configs);
}

RcppExport SEXP R_FG_init()
{
	printf("init FlashGraph\n");
	boost::log::core::get()->set_filter(
			boost::log::trivial::severity > boost::log::trivial::info);
	return R_NilValue;
}

RcppExport SEXP R_FG_compute_wcc(SEXP graph)
{
	FG_graph::ptr fg = R_FG_get_graph(graph);
	FG_vector<vertex_id_t>::ptr fg_vec = compute_wcc(fg);
	Rcpp::IntegerVector res(fg_vec->get_size());
	fg_vec->copy_to(res.begin(), fg_vec->get_size());
	return res;
}

RcppExport SEXP R_FG_compute_scc(SEXP graph)
{
	FG_graph::ptr fg = R_FG_get_graph(graph);
	FG_vector<vertex_id_t>::ptr fg_vec = compute_scc(fg);
	Rcpp::IntegerVector res(fg_vec->get_size());
	fg_vec->copy_to(res.begin(), fg_vec->get_size());
	return res;
}

RcppExport SEXP R_FG_compute_transitivity(SEXP graph)
{
	FG_graph::ptr fg = R_FG_get_graph(graph);
	FG_vector<float>::ptr fg_vec = compute_transitivity(fg);
	Rcpp::NumericVector res(fg_vec->get_size());
	fg_vec->copy_to(res.begin(), fg_vec->get_size());
	return res;
}

RcppExport SEXP R_FG_get_degree(SEXP graph, SEXP ptype)
{
	FG_graph::ptr fg = R_FG_get_graph(graph);

	std::string type_str = CHAR(STRING_ELT(ptype, 0));
	edge_type type = edge_type::NONE;
	if (type_str == "in")
		type = edge_type::IN_EDGE;
	else if (type_str == "out")
		type = edge_type::OUT_EDGE;
	else if (type_str == "both")
		type = edge_type::BOTH_EDGES;

	FG_vector<vsize_t>::ptr fg_vec = get_degree(fg, type);
	Rcpp::IntegerVector res(fg_vec->get_size());
	fg_vec->copy_to(res.begin(), fg_vec->get_size());
	return res;
}

RcppExport SEXP R_FG_compute_pagerank(SEXP graph, SEXP piters, SEXP pdamping)
{
	FG_graph::ptr fg = R_FG_get_graph(graph);

	int num_iters = REAL(piters)[0];
	float damping_factor = REAL(pdamping)[0];

	FG_vector<float>::ptr fg_vec = compute_pagerank2(fg, num_iters, damping_factor);
	Rcpp::NumericVector res(fg_vec->get_size());
	fg_vec->copy_to(res.begin(), fg_vec->get_size());
	return res;
}

RcppExport SEXP R_FG_compute_undirected_triangles(SEXP graph)
{
	FG_graph::ptr fg = R_FG_get_graph(graph);
	FG_vector<size_t>::ptr fg_vec = compute_undirected_triangles(fg);
	Rcpp::IntegerVector res(fg_vec->get_size());
	fg_vec->copy_to(res.begin(), fg_vec->get_size());
	return res;
}

RcppExport SEXP R_FG_compute_directed_triangles(SEXP graph, SEXP ptype)
{
	FG_graph::ptr fg = R_FG_get_graph(graph);

	std::string type_str = CHAR(STRING_ELT(ptype, 0));
	directed_triangle_type type = directed_triangle_type::CYCLE;
	if (type_str == "cycle")
		type = directed_triangle_type::CYCLE;
	else
		type = directed_triangle_type::ALL;

	FG_vector<size_t>::ptr fg_vec = compute_directed_triangles_fast(fg, type);
	Rcpp::IntegerVector res(fg_vec->get_size());
	fg_vec->copy_to(res.begin(), fg_vec->get_size());
	return res;
}

RcppExport SEXP R_FG_compute_local_scan(SEXP graph, SEXP order)
{
	FG_graph::ptr fg = R_FG_get_graph(graph);
	FG_vector<size_t>::ptr fg_vec = compute_local_scan(fg);
	Rcpp::IntegerVector res(fg_vec->get_size());
	fg_vec->copy_to(res.begin(), fg_vec->get_size());
	return res;
}

RcppExport SEXP R_FG_compute_topK_scan(SEXP graph, SEXP order, SEXP K)
{
	size_t topK = REAL(K)[0];
	FG_graph::ptr fg = R_FG_get_graph(graph);
	FG_vector<std::pair<vertex_id_t, size_t> >::ptr fg_vec
		= compute_topK_scan(fg, topK);
	assert(fg_vec->get_size() == topK);
	Rcpp::IntegerVector vertices(fg_vec->get_size());
	Rcpp::IntegerVector scans(fg_vec->get_size());
	for (size_t i = 0; i < topK; i++) {
		std::pair<vertex_id_t, size_t> pair = fg_vec->get(i);
		vertices[i] = pair.first;
		scans[i] = pair.second;
	}
	return Rcpp::DataFrame::create(Named("vid", vertices), Named("scan", scans));
}

RcppExport SEXP R_FG_compute_kcore(SEXP graph, SEXP _k, SEXP _kmax)
{
	int k = REAL(_k)[0];
	int kmax = REAL(_kmax)[0];
	FG_graph::ptr fg = R_FG_get_graph(graph);
	FG_vector<size_t>::ptr fg_vec = compute_kcore(fg, k, kmax);
	Rcpp::IntegerVector res(fg_vec->get_size());
	fg_vec->copy_to(res.begin(), fg_vec->get_size());
	return res;
}

RcppExport SEXP R_FG_compute_overlap(SEXP graph, SEXP _vids)
{
	Rcpp::IntegerVector Rvids(_vids);
	std::vector<vertex_id_t> vids(Rvids.begin(), Rvids.end());
	std::vector<std::vector<double> > overlap_matrix;
	size_t num_vertices = vids.size();

	FG_graph::ptr fg = R_FG_get_graph(graph);
	compute_overlap(fg, vids, overlap_matrix);
	assert(overlap_matrix.size() == num_vertices);

	Rcpp::NumericMatrix res(num_vertices, num_vertices);
	for (size_t i = 0; i < num_vertices; i++) {
		assert(overlap_matrix[i].size() == num_vertices);
		for (size_t j = 0; j < num_vertices; j++) {
			res(i, j) = overlap_matrix[i][j];
		}
	}
	return res;
}
