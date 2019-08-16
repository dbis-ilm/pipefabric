/*
 * Copyright (C) 2014-2019 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PipeFabric. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GRAPHALGS_HH
#define GRAPHALGS_HH

#include "Graph.hpp"
#include <boost/graph/dijkstra_shortest_paths.hpp>

namespace pfabric
{
	template <class NewGraph, class Tag>
	struct graph_copier 
	  : public boost::base_visitor<graph_copier<NewGraph, Tag> >
	{
	  	typedef Tag event_filter;
		
		graph_copier(NewGraph& graph) : new_g(graph) { }
		
	  	template <class Edge, class Graph>
	  	void operator()(Edge e, Graph& g) {
	  	    std::cout << "\ngraph_copier edge first: " << e.first << " second " << e.second << "\n";
	  	}
	private:
	  	NewGraph& new_g;
	};
	
	template <class NewGraph, class Tag>
	inline graph_copier<NewGraph, Tag>
	copy_graph(NewGraph& g, Tag) {
		return graph_copier<NewGraph, Tag>(g);
	}


	template<typename G>
	typename G::element_type
	shortest_path(G& g, std::size_t node1, std::size_t node2)
	{
		typedef typename G::vertex_descriptor vertex_descriptor;
		typedef typename G::element_type D;

    	std::vector<int> p(boost::num_vertices(g));
    	std::vector< D > d(boost::num_vertices(g));
    	std::vector<int> color(boost::num_vertices(g), boost::white_color);
    	auto distance_map = boost::get(boost::vertex_distance, d);
		
		auto cmp = std::less<D>();
		auto combine = boost::closed_plus<D>();
		auto predecessor_map = boost::make_iterator_property_map(p.begin(), get(boost::vertex_index, g));
		auto weight_map = boost::get(boost::edge_weight, g);
		auto vertex_map = boost::get(boost::vertex_index, g);
		auto inf = std::numeric_limits<D>::max();
		D zero = D();
		auto visitor = boost::make_dijkstra_visitor(copy_graph(g, boost::on_examine_edge()));
    	auto color_map = boost::get(boost::vertex_color, color);

	 	boost::dijkstra_shortest_paths(g, node1
			, predecessor_map
			, distance_map
			, weight_map
			, vertex_map
			, cmp
			, combine
			, inf, zero 
			, visitor
			, color_map
		);

	 	return d[node2];
	}

	template<typename G, typename Tout>
	void kruskal_spanning_tree(G& g, Tout &spanning_tree)
	{
  		typedef typename boost::graph_traits<G>::vertices_size_type size_type;
  		typename boost::graph_traits<G>::vertices_size_type n;
  		typedef typename G::edge_descriptor edge_descriptor;
 
  
  		auto weight_map = boost::get(boost::edge_weight, g);
  		auto vertex_map = boost::get(boost::vertex_index, g);
  
  		n = boost::num_vertices(g);
  		std::vector<size_type> rank_map(n);
  
  		auto rank = make_iterator_property_map(
  		    rank_map.begin()
  		    , vertex_map
  		    , rank_map[0]
  		    );
  

  		kruskal_minimum_spanning_tree(g
  		  , std::back_inserter(spanning_tree)
  		  , boost::weight_map(weight_map)
  		  . vertex_index_map(vertex_map)
  		  . rank_map(rank)
    	);
	}
}

#endif //GRAPHALGS_HH
