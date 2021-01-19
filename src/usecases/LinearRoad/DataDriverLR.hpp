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

#ifndef DataDriverLR_hpp_
#define DataDriverLR_hpp_

#include "core/Punctuation.hpp"
#include "core/Tuple.hpp"
#include "qop/DataSource.hpp"
#include "pubsub/channels/ConnectChannels.hpp"
#include "qop/BaseOp.hpp"
#include "qop/OperatorMacros.hpp"

#include <chrono>
#include <iostream>

#include "LRDataProvider.h"

//namespace pfabric { //NOT POSSIBLE because of namespace clash ("Tuple")

/**
 * @brief A DataDriver operator creates a stream of tuples according to
 * the linear road benchmark.
 */
template<typename StreamElement>
class DataDriverLR : public pfabric::DataSource<StreamElement> {
public:

  DataDriverLR(const std::string& fname) : mfname(fname) {}

  unsigned long start() {
    //lock for serialized shared resources
    pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;

    //reads and provides tuples
    CLRDataProvider* provider = new CLRDataProvider();

    //std::string to char* (filename)
    std::vector<char> cvec(mfname.c_str(), mfname.c_str() + mfname.size() + 1);
    char* fileName = cvec.data();

    //check & open file
    int ret = provider->Initialize(fileName, //1000000, 
                                   &mutex_lock);

    if (ret != SUCCESS) {
      ErrorHandler(ret);
	    return 0;
    } else {
      int nTuplesRead = 0;
      int nMaxTuples  = 100000;
      LPTuple lpTuples = new Tuple[nMaxTuples];

      if(provider->PrepareData(provider) == SUCCESS) {
        for(;;) {
          int ret;
          int cnt = 0; //info print

          for(;;) {
            //get available data based on timestamps
            ret = provider->GetData(lpTuples, nMaxTuples, nTuplesRead);

            if (ret < 0) {
              //errors, end of file?
              ErrorHandler(ret);
              break;
            }

            //no available tuples
            if (nTuplesRead == 0) {
              std::this_thread::sleep_for(std::chrono::microseconds(1));
              break;
            }

            //for all available tuples, transform them into pfabric tuples and publish
            for(int i = 0; i < nTuplesRead; i++) {
              cnt++; //info print
              auto tup = pfabric::makeTuplePtr(
                lpTuples[i].m_iType, lpTuples[i].m_iTime, lpTuples[i].m_iVid,
                lpTuples[i].m_iSpeed, lpTuples[i].m_iXway, lpTuples[i].m_iLane,
                lpTuples[i].m_iDir, lpTuples[i].m_iSeg, lpTuples[i].m_iPos,
                lpTuples[i].m_iQid, lpTuples[i].m_iSinit, lpTuples[i].m_iSend,
                lpTuples[i].m_iDow, lpTuples[i].m_iTod, lpTuples[i].m_iDay
              );
              this->getOutputDataChannel().publish(tup, false);
            }

            //last tuple was read
            if (nTuplesRead<nMaxTuples) {
              std::cout<<"DataDriverLR: "<<lpTuples[0].m_iTime<<"s, TP/s: "<<cnt<<std::endl; //info print
              break;
            }
          }
          if (ret < SUCCESS) {
            break;
          }
        }
      }
      provider->Uninitialize();
    }
    delete provider;
    return 1;
  }

private:

  //error handling
  inline void ErrorHandler(const int nErrorCode) {
    switch(nErrorCode) {
      case END_OF_FILE:
        std::cout<<"End of data file"<<std::endl;
        break;
      case ERROR_FILE_NOT_FOUND:
        std::cout<<"Data file not found. Check data file path name."<<std::endl;
        break;
      case ERROR_INVALID_FILE:
        std::cout<<"Invalid file handler. Restart the system."<<std::endl;
        break;
      case ERROR_BUFFER_OVERFLOW:
        std::cout<<"Buffer over flow. Increase the buffer size."<<std::endl;
        break;
      default:
        std::cout<<"Programming error."<<std::endl;
        break;
	  }
  }

  std::string mfname; //filename
};

#endif

