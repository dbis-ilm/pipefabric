/*
 * Copyright (C) 2014-2021 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with PipeFabric. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#define CATCH_CONFIG_MAIN

#include <catch.hpp>
#include <vector>

#include "pfabric.hpp"
#include "operations/Graph.hpp"
#include "operations/GraphAlgorithms.hpp"
#include "matrix/Matrix.hpp"
#include "matrix/ReaderValue.hpp"
#include "qop/ToMatrix.hpp"
#include "StreamMockup.hpp"


using namespace pfabric;

typedef int                                                         CellType;
typedef TuplePtr<int, int, CellType>                                InputType;
typedef pfabric::Matrix<CellType, pfabric::ReaderValue<InputType> > MatrixType; 
typedef pfabric::Graph< MatrixType >                                GraphType;

// adjacent matrix contains edges and values
std::vector< InputType > inputs = {
  makeTuplePtr(0, 1, 1), makeTuplePtr(0, 2, 3),
  makeTuplePtr(1, 0, 1), makeTuplePtr(1, 4, 1), 
	makeTuplePtr(2, 0, 3), makeTuplePtr(2, 3, 5), makeTuplePtr(2, 4, 4), 
  makeTuplePtr(3, 2, 5), makeTuplePtr(3, 4, 2),
  makeTuplePtr(4, 1, 1), makeTuplePtr(4, 2, 4), makeTuplePtr(4, 3, 2) 
};

std::shared_ptr<MatrixType> init_matrix(const std::vector<InputType> &inputs)
{
  auto matrix = make_shared<MatrixType>();
  auto mockup = std::make_shared<StreamMockup<InputType, InputType> >(inputs, inputs);  
  auto op     = std::make_shared<ToMatrix<MatrixType>> (matrix);

  CREATE_DATA_LINK(mockup, op);

  mockup->start();
  for(const auto& tuple : inputs ) {
    auto x = tuple->getAttribute<0>();
    auto y = tuple->getAttribute<1>();
    auto z = tuple->getAttribute<2>();
    matrix->set(x, y, z);
  }
  return matrix;
}

TEST_CASE("Observe graph values", "[GraphTest]")
{
	auto matrix = init_matrix(inputs);

  GraphType graph(matrix.get());

  REQUIRE(matrix->getRows() == 5);
 	REQUIRE(matrix->getCols() == 5);
	REQUIRE(boost::num_vertices(graph) == 5);

	auto weight_map = boost::get(boost::edge_weight, graph);
	auto range_vertices_it = boost::vertices(graph);
	for(auto vert_it = range_vertices_it.first; vert_it != range_vertices_it.second; ++vert_it) {
		
		auto out_edges_pair = boost::out_edges(*vert_it, graph);
		for(auto edge_it = out_edges_pair.first; edge_it != out_edges_pair.second; ++edge_it)
		{
			auto edge = *edge_it;
			auto found_edge_it = std::find_if(inputs.begin(), inputs.end(), [&edge](const InputType& tuple) {
				return edge.first == get<0>(tuple) && edge.second == get<1>(tuple);
			});
			if(found_edge_it != inputs.end()) {
				REQUIRE(weight_map[edge] == get<2>(*found_edge_it));
			} else {
				REQUIRE(false);
			}
		}
	}
}
TEST_CASE("Shortest path", "[GraphTest]")
{
  auto matrix = init_matrix(inputs);

  GraphType graph(matrix.get());
  auto node1 = 0;
  {
    auto node2 = 4;
    REQUIRE(pfabric::shortest_path(graph, node1, node2) == 2);
  }
  {
    auto node2 = 3; 
    REQUIRE(pfabric::shortest_path(graph, node1, node2) == 4);
  }
}

TEST_CASE("Shortest path with context", "[GraphTest]")
{
  StreamGenerator<InputType>::Generator stream([]( size_t n) -> InputType {
      return inputs[n];
    });

    const std::string matrixName("exMatrix1");
    PFabricContext ctx;
    {
      // A matrix is filling in one place
      auto t = ctx.createTopology();
      auto matrix = ctx.createMatrix<MatrixType>(matrixName);
      auto s = t->streamFromGenerator<InputType>(stream, inputs.size())
        .toMatrix<MatrixType>(matrix)
      ;
      t->start(false);
  }

  // Shortest path is computed on another place by getting matrix from context
  auto matrix = ctx.getMatrix<MatrixType>(matrixName);
    GraphType graph(matrix.get());
    auto node1 = 0;
  {
    auto node2 = 4;
    REQUIRE(pfabric::shortest_path(graph, node1, node2) == 2);
  }
  {
    auto node2 = 3; 
    REQUIRE(pfabric::shortest_path(graph, node1, node2) == 4);
  }
}

TEST_CASE("Graph streaming", "[GraphTest]")
{
  StreamGenerator<InputType>::Generator stream([]( size_t n) -> InputType {
      return inputs[n];
    });

    const std::string matrixName("exMatrix");
    PFabricContext ctx;
    auto t = ctx.createTopology();
    auto matrix = ctx.createMatrix<MatrixType>(matrixName);
    auto s = t->streamFromGenerator<InputType>(stream, inputs.size())
        .toMatrix<MatrixType>(matrix)
    ;
    CellType shortest_path = std::numeric_limits<CellType>::max();
    t->newStreamFromMatrix<MatrixType>(matrix)
      .notify([&matrix, &shortest_path](const auto &tuple, bool outdated){
        auto node1 = 0; auto node2 = 4;
        GraphType graph(matrix.get());
        auto res = pfabric::shortest_path(graph, node1, node2);
        if(res < shortest_path) {
          shortest_path = res;
          std::cout << "\nshortest path: " << shortest_path;
        }
      })
    ;

    t->start();
    t->wait();
}


TEST_CASE("Sparse edge iterator", "[GraphTest]")
{
  auto matrix = init_matrix(inputs);
  GraphType g(matrix.get());

  auto verify = [&g](std::size_t vertex, std::size_t id) {
    auto range_it = boost::out_edges(vertex, g);

    for(auto beg_it = range_it.first, end_it = range_it.second; beg_it != end_it; ++beg_it ) {
      auto edge = *beg_it;
      auto tuple = inputs[id++];

      REQUIRE(edge.first == get<0>(tuple));
      REQUIRE(edge.second == get<1>(tuple));

    }
  };
  verify(0, 0);
  verify(1, 2);
  verify(2, 4);
  verify(3, 7);
  verify(4, 9);
}

TEST_CASE("Edges iterator", "[GraphTest]")
{
  auto matrix = init_matrix(inputs);

  GraphType g(matrix.get());

  auto range_it = boost::edges(g);
  auto id = 0u;

  for(auto beg_it = range_it.first, end_it = range_it.second; beg_it != end_it; ++beg_it ) {

    auto edge = *beg_it;
    auto tuple = inputs[id++];

    REQUIRE(edge.first == get<0>(tuple));
    REQUIRE(edge.second == get<1>(tuple));
  }
}

TEST_CASE("Vertices iterator", "[GraphTest]")
{
  std::vector<typename GraphType::vertex_descriptor> expected = {0, 1, 2, 3, 4};
  auto matrix = init_matrix(inputs);

  GraphType g(matrix.get());

  auto id = 0u;
  auto range_it = boost::vertices(g);    

  for(auto beg_it = range_it.first, end_it = range_it.second; beg_it != end_it; ++beg_it ) {
    REQUIRE(*beg_it == expected[id++]);
  }
}

TEST_CASE("Count edges", "[GraphTest]")
{
  auto matrix = init_matrix(inputs);
  GraphType g(matrix.get());

  REQUIRE(boost::num_edges(g) == inputs.size());
}

TEST_CASE("In edge test", "[GraphTest], [UndirectedGraph]")
{
  auto matrix = init_matrix(inputs);
  GraphType g(matrix.get());

  auto verify = [&g](std::size_t vertex, std::size_t id)
  {
    auto range_it = boost::in_edges(vertex, g);
    for(auto beg_it = range_it.first, end_it = range_it.second; beg_it != end_it; ++beg_it ) {
      auto edge = *beg_it;
      auto tuple = inputs[id];
      
      REQUIRE(edge.first == get<1>(tuple));
      REQUIRE(edge.second == get<0>(tuple));
    }
  };

  verify(0, 0);
  verify(1, 2);
  verify(2, 4);
  verify(3, 7);
  verify(4, 9);
}

TEST_CASE("In degree", "[GraphTest], [UndirectedGraph]")
{
  auto matrix = init_matrix(inputs);
  GraphType g(matrix.get());

  REQUIRE(boost::in_degree(0, g) == 2);
  REQUIRE(boost::in_degree(1, g) == 2);
  REQUIRE(boost::in_degree(2, g) == 3);
  REQUIRE(boost::in_degree(3, g) == 2);
  REQUIRE(boost::in_degree(4, g) == 3);
}

TEST_CASE("Vertex test", "[GraphTest]")
{
  auto matrix = init_matrix(inputs);
  GraphType g(matrix.get());

  REQUIRE(boost::vertex(0, g) == 0);
  REQUIRE(boost::vertex(1, g) == 1);
}

TEST_CASE("Edge test", "[GraphTest]")
{
  auto matrix = init_matrix(inputs);
  GraphType g(matrix.get());

  auto range_it = boost::edges(g);
  for(auto beg_it = range_it.first, end_it = range_it.second; beg_it != end_it; ++beg_it ) {

    auto edge = *beg_it;
    REQUIRE(boost::edge(edge.first, edge.second, g).second);
  }
}

TEST_CASE("Adjacent test", "[GraphTest], [UndirectedGraph]")
{
  auto matrix = init_matrix(inputs);
  GraphType g(matrix.get());

  auto verify = [&g](std::size_t vertex, std::size_t id) {
    auto range_it = boost::adjacent_vertices(vertex, g);
    for(auto beg_it = range_it.first, end_it = range_it.second; beg_it != end_it; ++beg_it ) {
      auto tuple = inputs[id];
      
      REQUIRE(*beg_it == get<0>(tuple));
    }
  };
  verify(0, 0);
  verify(1, 2);
  verify(2, 4);
  verify(3, 7);
  verify(4, 9);
}
TEST_CASE("Edge test with nonexistent edges", "[GraphTest], [UndirectedGraph]")
{
  std::vector<InputType> inputs = {makeTuplePtr(0, 1, 5), makeTuplePtr(0, 3, 2)
    , makeTuplePtr(3, 2, 7)
  };
  std::vector<InputType> nonexistent = {makeTuplePtr(0, 2, 4), makeTuplePtr(1, 2, 7)};

  auto matrix = init_matrix(inputs);
  GraphType g(matrix.get());

  { // Precondition
    auto range_it = boost::edges(g);
    for(auto beg_it = range_it.first, end_it = range_it.second; beg_it != end_it; ++beg_it ) {
  
      auto edge = *beg_it;
      REQUIRE(boost::edge(edge.first, edge.second, g).second);
    }
  }

  for(const auto &tuple : nonexistent) {
    auto x = get<0>(tuple);
    auto y = get<1>(tuple);

    REQUIRE_FALSE(boost::edge(x, y, g).second);
  }
}


TEST_CASE("Remove edge", "[GraphTest]")
{
  auto matrix = init_matrix(inputs);
  GraphType g(matrix.get());

  REQUIRE(g.num_edges() == inputs.size());
  auto numEdges = boost::in_degree(2, g);
  REQUIRE(numEdges == 3);

  auto range_it = boost::out_edges(2, g);

  for(auto beg = range_it.first; beg != range_it.second; ++beg) {
    auto edge = *beg;
    g.remove_edge(edge.first, edge.second);
  }
  auto undirectedGr = 2;
  REQUIRE(boost::num_edges(g) == inputs.size()-numEdges*undirectedGr);
}
TEST_CASE("Clear vertex", "[GraphTest] [UndirectedGraph]")
{
  auto matrix = init_matrix(inputs);
  GraphType g(matrix.get());
  auto numEdges = boost::in_degree(2, g); 
  REQUIRE(numEdges == 3);

  boost::clear_vertex(2, g);

  auto undirectedGr = 2;
  REQUIRE(boost::num_edges(g) == inputs.size()-numEdges*undirectedGr);
  REQUIRE(boost::in_degree(2, g) == 0);
  REQUIRE(boost::out_degree(2, g) == 0);
}
