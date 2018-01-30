/*
 * Copyright (C) 2014-2018 DBIS Group - TU Ilmenau, All Rights Reserved.
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

 #include "Dataflow.hpp"

 using namespace pfabric;

 Dataflow::BaseOpIterator Dataflow::addPublisher(Dataflow::BaseOpPtr op) {
   publishers.push_back(op);
   auto iter = publishers.end();
   return --iter;
 }

 Dataflow::BaseOpIterator Dataflow::addPublisherList(const Dataflow::BaseOpList& lst) {
   publishers.insert(publishers.end(), lst.begin(), lst.end());
   auto iter = publishers.end();
   std::advance(iter, -lst.size());
   return iter;
 }

 void Dataflow::addSink(Dataflow::BaseOpPtr op) { sinks.push_back(op); }

 /**
  * @brief Returns the operator at the end of the publisher list.
  *
  * Returns the operator which acts as the publisher for the next
  * added operator.
  *
  * @return
  *    the last operator in the publisher list
  */
Dataflow::BaseOpPtr Dataflow::getPublisher() { return publishers.back(); }

 Dataflow::BaseOpIterator Dataflow::getPublishers(unsigned int num) {
   auto iter = publishers.end();
   std::advance(iter, -num);
   return iter;
 }

 std::size_t Dataflow::size() const { return publishers.size(); }
