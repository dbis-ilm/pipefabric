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

#ifndef Graph__HH
#define Graph__HH

#include "matrix/BaseMatrix.hpp"
#include <boost/graph/graph_traits.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/properties.hpp>


// Forward declaration
namespace pfabric {
template<typename M>
class Graph;
}

// traits for our Graph class
namespace boost {
template<typename M>
class graph_traits<pfabric::Graph<M> > {
 public:

  typedef typename pfabric::MatrixTraits<M>::IndexType vertex_descriptor;
  typedef typename pfabric::MatrixTraits<M>::edge edge_descriptor;

  typedef directed_tag directed_category;
  typedef disallow_parallel_edge_tag edge_parallel_category;
  typedef unsigned int vertices_size_type;
  typedef unsigned int edges_size_type;
  typedef unsigned int degree_size_type;

  struct traversal_category
    : public virtual boost::vertex_list_graph_tag,
      public virtual boost::adjacency_graph_tag,
      public virtual boost::incidence_graph_tag,
      public virtual boost::edge_list_graph_tag,
      public virtual boost::bidirectional_graph_tag //means we provide access to in_edges
    //and to out_edges of a given vertex
  {
  };

  static inline
  vertex_descriptor null_vertex();

  template<typename T>
  struct VertexIterator : public std::iterator<std::input_iterator_tag,
                                               T> {
   private:
    T id;
   public:
    typedef VertexIterator<T> self_type;

    VertexIterator()
      : id(0) {}

    VertexIterator(T id)
      : id(id) {}

    bool operator!=(const self_type &rhs) const { return this->id != rhs.id; }

    bool operator==(const self_type &rhs) const { return this->id == rhs.id; }

    self_type &operator++() {
      ++id;
      return *this;
    }

    self_type &operator--() {
      --id;
      return *this;
    }

    self_type &operator++(int) {
      operator++();
      return *this;
    }

    self_type &operator--(int) {
      operator--();
      return *this;
    }

    T operator*() const {
      return id;
    }
  };

  typedef VertexIterator<vertex_descriptor> vertex_iterator;
  typedef typename M::EdgeIterator out_edge_iterator;
  typedef typename M::EdgeIterator edge_iterator;
  typedef typename M::InEdgeIterator in_edge_iterator;
  typedef typename M::AdjacentVertexIterator adjacency_iterator;
};

} //namespace boost


//---------------------------------------------------------------------
// PFabric classes

namespace pfabric {

//---------------------------------------------------------------------
// Custom graph class for boost graph algorithms using Eigen matrices as container

template<typename M>
class Graph {
 private:
  // Sparse or Dense matrix
  M *matrix;

 public:

  typedef typename M::element_type element_type;
  typedef typename boost::graph_traits<Graph<M> >::vertex_descriptor vertex_descriptor;
  typedef typename boost::graph_traits<Graph<M> >::edge_descriptor edge_descriptor;

  Graph(M *m)
    : matrix(m) {
  }

  Graph(const Graph<M> &rhs)
    : matrix(rhs.matrix) {}

  element_type &get(size_t x, size_t y) { return matrix->get(x, y); }

  element_type get(size_t x, size_t y) const { return matrix->get(x, y); }

  inline
  vertex_descriptor
  add_vertex() {
    matrix->resize(matrix->getRows(), matrix->getCols());
    return matrix->getMatrix().outerSize();
  }

  inline void
  remove_vertex(vertex_descriptor u) {
    assert(false && "not realized yet");
  }

  inline bool
  add_edge(vertex_descriptor u, vertex_descriptor v) {
    matrix->set(u, v, 0);
    return true;
  }

  inline
  void remove_edge(vertex_descriptor u, vertex_descriptor v) {
    matrix->set(u, v, 0);
    matrix->set(v, u, 0);
    matrix->preemt(0);
  }

  inline
  void remove_edge(typename M::EdgeIterator beg, typename M::EdgeIterator end) {
    for (; beg != end; ++beg) {
      auto edge = *beg;
      matrix->set(edge.first, edge.second, 0);
      matrix->set(edge.second, edge.first, 0);
    }
    matrix->preemt(0);
  }

  inline typename boost::graph_traits<Graph<M> >::vertices_size_type
  num_vertices() const {
    auto rows = matrix->getRows();
    auto cols = matrix->getCols();
    return std::min(rows, cols);
  }

  inline typename boost::graph_traits<Graph<M> >::edges_size_type
  num_edges() const { return matrix->getNumElements(); }

  typename boost::graph_traits<Graph<M>>::vertex_iterator
  beginVertex() const {
    return typename boost::graph_traits<Graph<M>>::vertex_iterator(0);
  }

  typename boost::graph_traits<Graph<M>>::vertex_iterator
  endVertex() const {

    return typename boost::graph_traits<Graph<M>>::vertex_iterator(matrix->getRows());
  }

  typename boost::graph_traits<Graph<M>>::out_edge_iterator
  beginEdgeIterator() const {

    return beginEdgeIterator(0);
  }

  typename boost::graph_traits<Graph<M>>::out_edge_iterator
  endEdgeIterator() const {

    return endEdgeIterator(matrix->getMatrix().outerSize() - 1);
  }

  typename boost::graph_traits<Graph<M>>::out_edge_iterator
  beginEdgeIterator(vertex_descriptor beg) const {
    typedef typename boost::graph_traits<Graph<M>>::out_edge_iterator out_edge_iterator;

    return out_edge_iterator(matrix, beg);
  }

  typename boost::graph_traits<Graph<M>>::out_edge_iterator
  endEdgeIterator(vertex_descriptor end) const {
    typedef typename boost::graph_traits<Graph<M>>::out_edge_iterator out_edge_iterator;

    return out_edge_iterator(matrix, end);
  }

  typename boost::graph_traits<Graph<M>>::in_edge_iterator
  beginInEdgeIterator(vertex_descriptor beg) const {
    typedef typename boost::graph_traits<Graph<M>>::in_edge_iterator in_edge_iterator;

    return in_edge_iterator(matrix, beg);
  }

  typename boost::graph_traits<Graph<M>>::in_edge_iterator
  endInEdgeIterator(vertex_descriptor end) const {
    typedef typename boost::graph_traits<Graph<M>>::in_edge_iterator in_edge_iterator;

    return in_edge_iterator(matrix, end);
  }

  typename boost::graph_traits<Graph<M>>::adjacency_iterator
  beginAdjVertexIterator(vertex_descriptor beg) const {
    typedef typename boost::graph_traits<Graph<M>>::adjacency_iterator adjacency_iterator;
    return adjacency_iterator(matrix, beg);
  }

  typename boost::graph_traits<Graph<M>>::adjacency_iterator
  endAdjVertexIterator(vertex_descriptor end) const {
    typedef typename boost::graph_traits<Graph<M>>::adjacency_iterator adjacency_iterator;
    return adjacency_iterator(matrix, end);
  }

  typename boost::graph_traits<Graph<M>>::degree_size_type
  in_degree(typename boost::graph_traits<Graph<M> >::vertex_descriptor u) const {
    return matrix->getCountNonZerosByVer(u);
  }

  typename boost::graph_traits<Graph<M>>::degree_size_type
  out_degree(typename boost::graph_traits<Graph<M> >::vertex_descriptor u) const {
    //TODO: check directed or undirected graph
    return matrix->getCountNonZerosByVer(u);
  }

};

} //namespace pfabric

namespace boost {

template<typename G, typename Type, typename TypeRef>
class property_edge_map : public put_get_helper<TypeRef,
                                                property_edge_map<G, Type, TypeRef> > {
 public:
  typedef Type value_type;
  typedef TypeRef reference;
  typedef typename G::edge_descriptor key_type;

  typedef readable_property_map_tag category;

  G *graph;

  property_edge_map(G *g)
    : graph(g) {}

  reference operator[](key_type x) const {
    auto &w = graph->get(x.first, x.second);
    return w;
  }

  reference operator[](key_type x) {
    auto &w = graph->get(x.first, x.second);
    return w;
  }
};

template<typename G, typename T>
class property_data_map_index : public put_get_helper<const T &,
                                                      property_data_map_index<G, T> > {
 public:
  typedef T value_type;
  typedef const T &reference;
  typedef unsigned int key_type;

  typedef readable_property_map_tag category;

  const G *graph;

  property_data_map_index(const G *g)
    : graph(g) {}

  template<class V>
  value_type operator[](V x) const {
    return x;
  }

  template<class V>
  value_type operator[](V x) {
    return x;
  }
};

template<typename C, typename Type>
struct property_container : public put_get_helper<Type &, property_container<C, Type> > {
  typedef Type value_type;
  typedef Type &reference;
  typedef int key_type;
  typedef lvalue_property_map_tag category;

  C &container;

  property_container(C &c)
    : container(c) {}

  template<typename T>
  reference operator[](T index) const {
    return container[index];
  }

  template<typename T>
  reference operator[](T index) {
    return container[index];
  }
};

template<typename M>
struct property_map<pfabric::Graph<M>, vertex_index_t> {
  typedef property_data_map_index<pfabric::Graph<M>, typename pfabric::Graph<M>::vertex_descriptor> type;
  typedef property_data_map_index<pfabric::Graph<M>, typename pfabric::Graph<M>::vertex_descriptor> const_type;
};

template<typename M>
struct property_map<pfabric::Graph<M>, edge_weight_t> {
  typedef typename M::element_type element_type;
  typedef property_edge_map<pfabric::Graph<M>, element_type, element_type &> type;
};

template<typename C>
struct property_map<C, vertex_color_t> {
  typedef typename C::value_type element_type;
  typedef property_container<C, element_type> type;
};

template<typename C>
struct property_map<C, vertex_distance_t> {
  typedef typename C::value_type element_type;
  typedef property_container<C, element_type> type;
};

template<typename G, typename Type>
struct property_traits<property_data_map_index<G, Type> > {
  typedef typename property_data_map_index<G, Type>::value_type value_type;
  typedef typename property_data_map_index<G, Type>::key_type key_type;
  typedef typename property_data_map_index<G, Type>::reference reference;
  typedef typename property_data_map_index<G, Type>::category category;
};

template<typename M, typename Type, typename TypeRef>
struct property_traits<property_edge_map<pfabric::Graph<M>, Type, TypeRef> > {
  typedef typename property_edge_map<pfabric::Graph<M>, Type, TypeRef>::value_type value_type;
  typedef typename property_edge_map<pfabric::Graph<M>, Type, TypeRef>::category category;
  typedef typename property_edge_map<pfabric::Graph<M>, Type, TypeRef>::key_type key_type;
  typedef typename property_edge_map<pfabric::Graph<M>, Type, TypeRef>::reference reference;
};

template<typename C, typename Type>
struct property_traits<property_container<C, Type> > {
  typedef typename property_container<C, Type>::value_type value_type;
  typedef typename property_container<C, Type>::category category;
  typedef typename property_container<C, Type>::key_type key_type;
  typedef typename property_container<C, Type>::reference reference;
};

/*template<typename M>
inline
typename boost::graph_traits< pfabric::Graph<M> >::vertex_descriptor
source(std::pair<int, int> e,
		const pfabric::Graph< M > &g)
{
	return e.first;
}

template<typename M>
inline
typename boost::graph_traits< pfabric::Graph<M> >::vertex_descriptor
source(std::pair<int, int> e,
		pfabric::Graph< M > &g)
{
	return e.first;
}


template<typename M>
inline
typename boost::graph_traits< pfabric::Graph<M> >::vertex_descriptor
target(std::pair<int, int> e,
		const pfabric::Graph< M > &g)
{
	return e.second;
}

template<typename M>
inline
typename boost::graph_traits< pfabric::Graph<M> >::vertex_descriptor
target(std::pair<int, int> e,
		pfabric::Graph< M > &g)
{
	return e.second;
}
*/

template<typename M>
inline std::pair<
  typename boost::graph_traits<pfabric::Graph<M> >::vertex_iterator,
  typename boost::graph_traits<pfabric::Graph<M> >::vertex_iterator>
vertices(const pfabric::Graph<M> &g) {
  auto b = g.beginVertex();
  auto e = g.endVertex();
  return std::make_pair(b, e);
}

template<typename M>
inline
typename boost::graph_traits<pfabric::Graph<M> >::vertices_size_type
num_vertices(const pfabric::Graph<M> &g) {
  return g.num_vertices();
}

template<typename M>
inline
typename boost::graph_traits<pfabric::Graph<M> >::edges_size_type
num_edges(const pfabric::Graph<M> &g) {
  return g.num_edges();
}

template<typename M>
inline
typename boost::graph_traits<pfabric::Graph<M> >::degree_size_type
in_degree(
  typename boost::graph_traits<pfabric::Graph<M> >::vertex_descriptor u,
  const pfabric::Graph<M> &g) {
  return g.in_degree(u);
}

template<typename M>
inline
typename boost::graph_traits<pfabric::Graph<M> >::degree_size_type
out_degree(
  typename boost::graph_traits<pfabric::Graph<M> >::vertex_descriptor u,
  const pfabric::Graph<M> &g) {
  return g.out_degree(u);
}

template<typename M>
inline
typename boost::graph_traits<pfabric::Graph<M> >::degree_size_type
degree(
  typename boost::graph_traits<pfabric::Graph<M> >::vertex_descriptor u,
  const pfabric::Graph<M> &g) {
  return in_degree(u, g);
}

template<typename M>
std::pair<typename boost::graph_traits<pfabric::Graph<M> >::edge_iterator,
          typename boost::graph_traits<pfabric::Graph<M> >::edge_iterator
>
edges(const pfabric::Graph<M> &g) {
  auto b = g.beginEdgeIterator();
  auto e = g.endEdgeIterator();
  return {b, e};
}

//returns beg and end iterates of all out edges of given vertex
template<typename M>
std::pair<typename boost::graph_traits<pfabric::Graph<M> >::out_edge_iterator,
          typename boost::graph_traits<pfabric::Graph<M> >::out_edge_iterator
>
out_edges(
  typename boost::graph_traits<pfabric::Graph<M> >::vertex_descriptor u,
  const pfabric::Graph<M> &g) {
  auto beg = g.beginEdgeIterator(u);
  auto end = g.endEdgeIterator(u);
  return std::make_pair(beg, end);
}

template<typename M>
std::pair<typename boost::graph_traits<pfabric::Graph<M> >::in_edge_iterator,
          typename boost::graph_traits<pfabric::Graph<M> >::in_edge_iterator
>
in_edges(typename boost::graph_traits<pfabric::Graph<M> >::vertex_descriptor u,
         const pfabric::Graph<M> &g) {
  auto beg = g.beginInEdgeIterator(u);
  auto end = g.endInEdgeIterator(u);
  return std::make_pair(beg, end);
}

template<typename M>
std::pair<typename boost::graph_traits<pfabric::Graph<M> >::adjacency_iterator,
          typename boost::graph_traits<pfabric::Graph<M> >::adjacency_iterator
>
adjacent_vertices(
  typename boost::graph_traits<pfabric::Graph<M> >::vertex_descriptor u,
  const pfabric::Graph<M> &g) {
  auto beg = g.beginAdjVertexIterator(u);
  auto end = g.endAdjVertexIterator(u);
  return std::make_pair(beg, end);
}

template<typename M>
typename boost::graph_traits<pfabric::Graph<M> >::vertex_descriptor
add_vertex(pfabric::Graph<M> &g) {
  return g.add_vertex();
}

template<typename M>
void remove_vertex(typename boost::graph_traits<pfabric::Graph<M> >::vertex_descriptor u, pfabric::Graph<M> &g
) {
  g.remove_vertex(u);
}

template<typename M>
typename boost::graph_traits<pfabric::Graph<M> >::vertex_descriptor
vertex(typename boost::graph_traits<pfabric::Graph<M> >::vertices_size_type v,
       pfabric::Graph<M> &g) {
  return v;
}

template<typename M>
std::pair<typename boost::graph_traits<pfabric::Graph<M> >::edge_descriptor, bool>
edge(typename boost::graph_traits<pfabric::Graph<M> >::vertex_descriptor u_local,
     typename boost::graph_traits<pfabric::Graph<M> >::vertex_descriptor v_local, const pfabric::Graph<M> &g) {
  auto range_it = boost::out_edges(u_local, g);

  for (auto beg_it = range_it.first; beg_it != range_it.second; ++beg_it) {
    if (beg_it.getIndex() == v_local) {
      return {std::make_pair(u_local, v_local), true};
    }
  }
  return {std::make_pair(u_local, v_local), false};
}

template<typename M>
std::pair<typename boost::graph_traits<pfabric::Graph<M> >::edge_descriptor, bool>
add_edge(typename boost::graph_traits<pfabric::Graph<M> >::vertex_descriptor u_local,
         typename boost::graph_traits<pfabric::Graph<M> >::vertex_descriptor v_local, pfabric::Graph<M> &g) {
  auto res = g.add_edge(u_local, v_local);
  return {make_pair(u_local, v_local), res};
}

template<typename M>
void
remove_edge(
  typename boost::graph_traits<pfabric::Graph<M> >::vertex_descriptor u,
  typename boost::graph_traits<pfabric::Graph<M> >::vertex_descriptor v, pfabric::Graph<M> &g) {
  g.remove_edge(u, v);
}

template<typename M>
void
remove_edge(typename boost::graph_traits<pfabric::Graph<M> >::edge_descriptor e, pfabric::Graph<M> &g) {
  g.remove_edge(e.first, e.second);
}

template<typename M>
void
remove_edge(typename boost::graph_traits<pfabric::Graph<M> >::out_edge_iterator iter, pfabric::Graph<M> &g) {
  auto edge = *iter;
  boost::remove_edge(edge, g);
}

template<typename Pred, typename M>
void
remove_edge_if(Pred p, pfabric::Graph<M> &g) {
  auto pair_it = boost::edges(g);
  for (auto beg = pair_it.first; beg != pair_it.second; ++beg) {
    auto edge = *beg;
    if (p(edge)) {
      boost::remove_edge(edge.first, edge.second);
    }
  }
}

template<typename Pred, typename M>
void
remove_out_edge_if(Pred p, pfabric::Graph<M> &g) {
  auto pair_it = boost::out_edges(g);
  for (auto beg = pair_it.first; beg != pair_it.second; ++beg) {
    auto edge = *beg;
    if (p(edge)) {
      boost::remove_edge(edge.first, edge.second);
    }
  }
}

template<typename Pred, typename M>
void
remove_in_edge_if(Pred pred, pfabric::Graph<M> &g) {
  auto pair_it = boost::in_edges(g);
  for (auto beg = pair_it.first; beg != pair_it.second; ++beg) {
    auto edge = *beg;
    if (p(edge)) {
      boost::remove_edge(edge.first, edge.second);
    }
  }

}

template<typename M>
void
clear_vertex(
  typename boost::graph_traits<pfabric::Graph<M>>::vertex_descriptor u, pfabric::Graph<M> &g
) {
  //TODO: for directed graph
  auto pair_it = boost::out_edges(u, g);
  g.remove_edge(pair_it.first, pair_it.second);
}

template<typename M>
inline
typename boost::graph_traits<pfabric::Graph<M>>::vertex_descriptor
boost::graph_traits<pfabric::Graph<M>>::null_vertex() {
  return -1;
}
//---------------------------------------------------------------------

// Access function for vertex property
template<typename M>
inline
typename boost::property_map<pfabric::Graph<M>, boost::vertex_index_t>::type
get(boost::vertex_index_t, pfabric::Graph<M> &g) {
  typedef typename boost::property_map<pfabric::Graph<M>,
                                       boost::vertex_index_t>::type pmap_type;

  return pmap_type(&g);
}

template<typename M>
inline typename
boost::property_map<pfabric::Graph<M>, boost::vertex_index_t>::const_type
get(boost::vertex_index_t, const pfabric::Graph<M> &g) {
  typedef typename boost::property_map<pfabric::Graph<M>,
                                       boost::vertex_index_t>::const_type pmap;

  return pmap(&g);
}

template<typename M>
inline typename
boost::property_map<pfabric::Graph<M>, boost::edge_weight_t>::const_type
get(boost::edge_weight_t, const pfabric::Graph<M> &g) {
  typedef typename boost::property_map<pfabric::Graph<M>,
                                       boost::edge_weight_t>::const_type pmap;
  return pmap(&g);
}

template<typename M>
inline typename
boost::property_map<pfabric::Graph<M>, boost::edge_weight_t>::type
get(boost::edge_weight_t, pfabric::Graph<M> &g) {
  typedef typename boost::property_map<pfabric::Graph<M>,
                                       boost::edge_weight_t>::type pmap;
  return pmap(&g);
}

template<typename C>
inline typename
boost::property_map<C, boost::vertex_distance_t>::type
get(boost::vertex_distance_t, C &c) {
  typedef typename boost::property_map<C, boost::vertex_distance_t>::type
    pmap;
  return pmap(c);
}

template<typename C>
inline typename
boost::property_map<C, boost::vertex_color_t>::type
get(boost::vertex_color_t, C &c) {
  typedef typename boost::property_map<C, boost::vertex_color_t>::type
    pmap;
  return pmap(c);
}

} //namespace boost

#endif //Graph__HH
