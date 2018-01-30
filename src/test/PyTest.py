
# Copyright (C) 2014-2018 DBIS Group - TU Ilmenau, All Rights Reserved.
#
# This file is part of the PipeFabric package.
#
# PipeFabric is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# PipeFabric is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with PipeFabric. If not, see <http://www.gnu.org/licenses/>.

import sys
sys.path.append('../../build/')
print(sys.path)

import pyfabric
import unittest
import os

class TestPipeFabricPython(unittest.TestCase):

  #test case for extract operator
  def test_extract(self):
    t = pyfabric.Topology()

    #write three tuples to file
    file = open("data.csv","w")
    file.write("1,teststring,1.5\n2,teststring,2.5\n3,teststring,3.5\n")
    file.close()

    strm = []
    expected = ['1','teststring','1.5','2','teststring','2.5','3','teststring','3.5']

    p = t.stream_from_file("data.csv") \
          .extract(',') \
          .notify(lambda tup, o: strm.extend((tup[0], tup[1], tup[2]))) \
          .pfprint() \

    t.start()

    self.assertEqual(strm, expected)

    os.remove("data.csv")


  #test case for map operator
  def test_map(self):
    t = pyfabric.Topology()

    #write three tuples to file
    file = open("data.csv","w")
    file.write("1,teststring,1.5\n2,teststring,2.5\n3,teststring,3.5\n")
    file.close()

    strm = []
    expected = [(1,'teststring','1.5'),(2,'teststring','2.5'),(3,'teststring','3.5')]

    p = t.stream_from_file("data.csv") \
          .extract(',') \
          .map(lambda t, o: (int(t[0]), t[1], t[2])) \
          .notify(lambda t, o: strm.append(t)) \

    t.start()

    self.assertEqual(strm, expected)

    os.remove("data.csv")


  #test case for where operator
  def test_where(self):
    t = pyfabric.Topology()

    #write three tuples to file
    file = open("data.csv","w")
    file.write("1,teststring,1.5\n2,teststring,2.5\n3,teststring,3.5\n")
    file.close()

    strm = []
    expected = [(2,'teststring','2.5'),(3,'teststring','3.5')]

    p = t.stream_from_file("data.csv") \
          .extract(',') \
          .map(lambda t, o: (int(t[0]), t[1], t[2])) \
          .where(lambda x, o: x[0] > 1) \
          .notify(lambda t, o: strm.append(t)) \

    t.start()

    self.assertEqual(strm, expected)

    os.remove("data.csv")


if __name__ == '__main__':
  unittest.main()
