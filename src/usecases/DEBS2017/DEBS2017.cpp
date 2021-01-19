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

//This class contains the solution for the DEBS 2017 - Grand Challenge
//Provided by Anton Gohlke, modified by Constantin Pohl

#include <iostream> //std::cout, std::endl
#include <unordered_map> //container for metadata
#include <mutex> //multithreading purposes
#include <vector> //cluster and Markov chain
#include <chrono> //measuring necessary time for anomaly detection

#include "pfabric.hpp"

using namespace pfabric;


/*
 * -----------------------------------------------------------------------------------------------------------------------
 * Parameters
 * -----------------------------------------------------------------------------------------------------------------------
 */

int trans_num = 5; //number of transitions to be considered in anomaly detection
double trans_prob = 0.005; //probability for anomaly detection
int max_cluster_iterations = 50; //number of maximum iterations for the clustering algorithm

//counter for performance and results
long long tuples_processed = 0;
long long anomalies_found = 0;


/*
 * -----------------------------------------------------------------------------------------------------------------------
 * Metadata
 * -----------------------------------------------------------------------------------------------------------------------
 */

//metadata location and dataset file
std::string metadata_loc = pfabric::gDataPath + "DEBS2017/data_10M/molding_machine_10M.metadata.nt";

//the structure of tuples we receive after extract as well as tuplifying
typedef TuplePtr<std::string, std::string, std::string> Triple;

//saves metadata in proper format for later usage
std::unordered_map<string, int> MetadataMap;

//machine number, used later for clustering
int MdCluster;

//helper method for storing metadata into container for later usage
inline void processMetadata(Triple meta) {
  std::string tempString = get<1>(meta);
  if(tempString.size() < 1) {
    tempString = get<0>(meta);
    tempString.erase(tempString.begin(), tempString.begin()+84);
    tempString.pop_back(); //now the value looks like AA_BBB with AA=machine, BBB=Value
    MetadataMap.insert({tempString, MdCluster});
  } else {
    tempString = get<1>(meta);
    tempString.erase(0,1);
    tempString.erase(tempString.end()-41, tempString.end());
    MdCluster = atoi(tempString.c_str());
  }
}

//main method for processing the metadata
inline void streamMetadata() {
  std::cout<<"Processing metadata."<<std::endl;

  Topology t;
  auto s = t.newStreamFromFile(metadata_loc)
    //first, extract the metadata and convert it to string triple
    .extract<Triple>(' ')
    //now transform the triples by grouping, according to a RDF schema
    .tuplify<Triple>({ "<http://www.agtinternational.com/ontologies/WeidmullerMetadata#hasNumberOfClusters>",
                      "<http://www.agtinternational.com/ontologies/IoTCore#valueLiteral>"},
                     TuplifierParams::ORDERED)
    //finally the metadata is processed and stored for later usage
    .notify([&](auto tp, bool outdated) {
      processMetadata(tp);
    })
    ;

  t.start(false);

  std::cout<<"Amount of stateful properties: "<<MetadataMap.size()<<"."<<std::endl;
  std::cout<<"Metadata stored."<< std::endl;
}


/*
 * -----------------------------------------------------------------------------------------------------------------------
 * Preprocessing input data into state
 * -----------------------------------------------------------------------------------------------------------------------
 */

//input data location and dataset file
std::string inputdata_loc = pfabric::gDataPath + "DEBS2017/data_10M/molding_machine_10M.nt";

//the structure of a tuple after RDF processing input data stream
typedef TuplePtr<std::string, std::string, std::string, std::string, std::string> Input_tp;
//the structure of the output after preprocessing map
typedef TuplePtr<int, int, std::string, std::string, Timestamp, int, double, std::string, double> Preproc_Output_tp;

//used for input data
struct InputState {
  //constructor
  InputState() : valueDouble(0.0), thresholdNumber(0.0), property_cnt(0), passTuple(0), hasMetadata(0),
    numberOfClusters(0), metadataIDString(""), timestamp_str(""), observation(""), value(""), timestamp(0) {
  }

  double valueDouble; //stores current input value
  double thresholdNumber; //stores current threshold

  int property_cnt; //counts stateful properties per observation group
  int passTuple; //forward flag
  int hasMetadata; //check for available metadata
  int numberOfClusters; //amount of clusters a property has

  std::string metadataIDString; //stores the MetadataID/ID
  std::string timestamp_str; //stores the timestamp as string
  std::string observation; //stores the tuple ID for anomaly output
  std::string value; //stores the current data point

  Timestamp timestamp; //stores the raw timestamp
};

//helper method for writing the states
inline Preproc_Output_tp calculateStates(Input_tp tp, std::shared_ptr<InputState> state) {
  std::string testString = get<0>(tp);
  state->passTuple = 0;

  if(testString.find("Observation_") != std::string::npos){
    //get the Value ID, it starts at the 51th position of the string
    //and the ">" at the end of the string has to be deleted
    state->observation = testString.substr(57);
    state->observation.pop_back();
    state->metadataIDString = get<1>(tp);
    state->metadataIDString.erase(state->metadataIDString.begin(), state->metadataIDString.begin()+64);
    state->metadataIDString.pop_back();

    //check if the data has metadata (otherwise it is useless)
    std::unordered_map<std::string,int>::const_iterator got = MetadataMap.find (state->metadataIDString);
    if(got != MetadataMap.end()){
      state->hasMetadata++;
      state->numberOfClusters = MetadataMap[state->metadataIDString];
    }else{
      state->hasMetadata = 0;
    }
  } else {
    if(testString.find("Value") != std::string::npos){
      if(state->hasMetadata > 0){
        state->property_cnt++;
        state->value = get<3>(tp);
        state->value.erase(0,1);

        if(state->value.size() > 0){ //needed as catch if it doesn't contain content
          state->value.erase(state->value.end()-44, state->value.end());
          state->valueDouble = std::stod(state->value); //convert the string into double
          state->passTuple = 1;
        }
      }
    } else {
      if(testString.find("Timestamp") != std::string::npos){ //get timestamp
        state->timestamp_str = get<3>(tp); //get the whole date+time
        state->timestamp_str.erase(0,1);
        state->timestamp_str.erase(state->timestamp_str.end()-52, state->timestamp_str.end());
        state->timestamp = TimestampHelper::stringToTimestamp(state->timestamp_str);

        //now the TS variable is used to store the time stamp name
        state->timestamp_str = testString;
        state->timestamp_str = state->timestamp_str.substr(45);
        state->timestamp_str.pop_back();
        state->property_cnt = 0;
      }
    }
  }
  return makeTuplePtr(state->property_cnt-1, state->passTuple, state->metadataIDString, state->timestamp_str,
                      state->timestamp, state->numberOfClusters, state->thresholdNumber, state->observation,
                      state->valueDouble);
}


/*
 * -----------------------------------------------------------------------------------------------------------------------
 * Clustering
 * -----------------------------------------------------------------------------------------------------------------------
 */

//locks shared variables for multithreading
std::mutex mtx;

//the strucure of the output after clustering
typedef TuplePtr<int, int, std::string, std::string, Timestamp, int, double,
                 std::string, double, std::vector<int>> Cluster_Output_tp;

//used for cluster
struct ClusterState{
  ClusterState() : DataVectorNew(0) {}
  std::vector<std::vector<double> > DataVectorNew;
};

//main method for clustering
inline Cluster_Output_tp calculateClusters(Preproc_Output_tp tp, bool outdated, std::shared_ptr<ClusterState> state) {

  //clustering variables
  bool foundRowNew = false;
  int rowNumberNew = 0;
  int hasEnoughValues = 0;
  std::vector<int> clusterSequence(0, 0);
  std::string tempMetadataString = get<2>(tp); //check for outdated tuple or not (within the window or not)
  double tempMachineNumber = std::stod(tempMetadataString.substr(0, tempMetadataString.find("_")));
  double tempValueNumber = std::stod(tempMetadataString.substr(tempMetadataString.find("_")+1));

  //check where the data point belongs to
  for (auto rowNew = state->DataVectorNew.begin(); rowNew != state->DataVectorNew.end(); rowNew++) {
    auto colNew = rowNew->begin();
    if(( *colNew) == tempMachineNumber) {
      colNew++;
      if((*colNew) == tempValueNumber) {
        foundRowNew = true;
        break;
      }
    }
    rowNumberNew++;
  }

  //check if outside of the window
  if(outdated == true) {
    mtx.lock();
    state->DataVectorNew[rowNumberNew].erase(state->DataVectorNew[rowNumberNew].begin() + 2);
    mtx.unlock();

  //if the data is not outdated it should be processed
  } else {
    //if there is already data to this id
    if(!foundRowNew){
      mtx.lock();
      std::vector<double> tempDoubleVector;
      tempDoubleVector.push_back(tempMachineNumber);
      tempDoubleVector.push_back(tempValueNumber);
      tempDoubleVector.push_back(get<8>(tp));
      //create new variable
      state->DataVectorNew.push_back(tempDoubleVector);
      mtx.unlock();
    } else {
      mtx.lock();
      //add data at the end of row x
      state->DataVectorNew[rowNumberNew].push_back(get<8>(tp));
      mtx.unlock();
    }

    int NumberOfValues = (int)state->DataVectorNew[rowNumberNew].size()-2;

    //check if there are enough values for clustering
    if(NumberOfValues > trans_num) {
      std::vector<double> clusterValueVector (1, 0.0);
      std::vector<int> clusterAssignVector(NumberOfValues, 0);
      std::vector<double> valuesVector;
      valuesVector.assign(state->DataVectorNew[rowNumberNew].begin()+2, state->DataVectorNew[rowNumberNew].end());

      //initialize clusterValueVector, use the distinct first values of valuesVector
      clusterValueVector[0] = valuesVector[0];

      for(int i = 1; (int)clusterValueVector.size() < get<5>(tp) && i <= (int)valuesVector.size(); i++) {
        if(!(std::find(clusterValueVector.begin(), clusterValueVector.end(), valuesVector[i])
             != clusterValueVector.end())) {
          clusterValueVector.push_back(valuesVector[i]);
        }
      }

      //iterate to convergence
      int numberOfChanges = 1;
      int clusterIt = 0;

      //start clustering actualization
      while(clusterIt<max_cluster_iterations && numberOfChanges != 0) {
        numberOfChanges = 0;
        //calculate distance for each value, iterate through all values
        for(int i = 0; (int)valuesVector.size() > i; i++){
          //distance to the current assigned clustercenter
          double oldDistance = (valuesVector[i]-clusterValueVector[clusterAssignVector[i]])
            *(valuesVector[i]-clusterValueVector[clusterAssignVector[i]]);

          //first values get doublechecked
          for(int j = 0; (int)clusterValueVector.size() > j; j++) {
            //distance to the other clustercenters
            double newDistance = (valuesVector[i]-clusterValueVector[j])*(valuesVector[i]-clusterValueVector[j]);
            //check if the distance is the same
            if (newDistance == oldDistance){
              if (clusterValueVector[j] > clusterValueVector[clusterAssignVector[i]]) {
                clusterAssignVector[i] = j;
              }
            //check if the distance is smaller
            } else if (newDistance < oldDistance) {
              oldDistance = newDistance;
              clusterAssignVector[i] = j;
              numberOfChanges++;
            }
          }
        }

        //calculate new clustercenters
        for(int i = 0; (int)clusterValueVector.size() > i; i++) {
          int centerCounter = 0;
          double clusterSum = 0;
          for(int j = 0; (int)clusterAssignVector.size() > j; j++) {
            if(clusterAssignVector[j] == i){
              centerCounter++;
              clusterSum += valuesVector[j];
            }
          }

          if (clusterSum != 0) {
            double newClusterValueVector = clusterSum/(double)centerCounter;

            //clusterValueVector is different so another iteration is needed
            if(newClusterValueVector != clusterValueVector[i]) {
              numberOfChanges++;
            }
            clusterValueVector[i] = newClusterValueVector;
          }
        }
        clusterIt++;
      }
      clusterSequence = clusterAssignVector;
      hasEnoughValues = 1;
    } else {
      hasEnoughValues = 0;
    }
  }
  return makeTuplePtr(get<0>(tp), hasEnoughValues, get<2>(tp), get<3>(tp), get<4>(tp), get<5>(tp), get<6>(tp),
                      get<7>(tp), get<8>(tp), clusterSequence);
}


/*
 * -----------------------------------------------------------------------------------------------------------------------
 * Markov chain
 * -----------------------------------------------------------------------------------------------------------------------
 */

//main method for Markov chain
inline void calculateMarkov(Cluster_Output_tp tp, bool outdated) {
  std::vector<int> sequenceVector = get<9>(tp);

  //if data not outdated and enough data points
  if(!outdated && (get<1>(tp)==1)){
    int nrOfCluster = get<5>(tp);

    //sequenz is equal/bigger the the amount of clusters and the seqence is bigger as the required amount
    if((int)sequenceVector.size() > trans_num) {
      //matrix to count the transitions
      double transitionCounter[nrOfCluster][nrOfCluster];
      memset(transitionCounter, 0, sizeof(transitionCounter));
      int lastClusterCenter = sequenceVector[0];
      int nextClusterCenter = 0;

      //count the transitions
      for(int i = 1; (int)sequenceVector.size() > i; i++) {
        nextClusterCenter = sequenceVector[i];
        transitionCounter[lastClusterCenter][nextClusterCenter]++;
        lastClusterCenter = nextClusterCenter;
      }

      //calculate the transition probability
      for(int i = 0; nrOfCluster > i; i++) {
        double sum = 0.0;
        for(int j = 0; nrOfCluster > j; j++) {
          sum += transitionCounter[i][j];
        }
        if(sum != 0) {
          for(int j = 0; nrOfCluster > j; j++) {
            transitionCounter[i][j] = transitionCounter[i][j]/sum;
          }
        }
      }

      //calculate the probability of the sequence
      double transitionProbability = 1;
      lastClusterCenter = sequenceVector[(int)sequenceVector.size()-trans_num-1];
      for(int i = 1; i <= trans_num; i++ ){
        nextClusterCenter = sequenceVector[(int)sequenceVector.size()-trans_num-1+i];
        transitionProbability *= transitionCounter[lastClusterCenter][nextClusterCenter];
        lastClusterCenter = nextClusterCenter;
      }

      //anomaly found
      if(transitionProbability < trans_prob){
        anomalies_found++;
        std::cout<<anomalies_found<<"th Anomaly! TransitionProb: "<<transitionProbability<<" "
          <<get<3>(tp)<<" with time: "<<TimestampHelper::timestampToString(get<4>(tp))
          <<" Observation: "<<get<7>(tp)<<" MetadataID: "<<get<2>(tp)<<std::endl;
      }
    }
  }
}


/*
 * -----------------------------------------------------------------------------------------------------------------------
 * Main
 * -----------------------------------------------------------------------------------------------------------------------
 */

int main(int argc, char **argv) {

  //individual parameters for the query
  int windowSize = 10;
  int threadAmount = 3;


  //-----Processing Metadata-----
  streamMetadata();

  //-----Start DEBS2017 challenge query-----
  Topology top;
  auto s = top.newStreamFromFile(inputdata_loc)
    //first, extract the metadata and convert it to string triple
    .extract<Triple>(' ')
    //now transform the triples by grouping, according to a RDF schema
    .tuplify<Input_tp>({"<http://purl.oclc.org/NET/ssnx/ssn#observedProperty>",
                             "<http://www.agtinternational.com/ontologies/I4.0#observedCycle>",
                             "<http://www.agtinternational.com/ontologies/IoTCore#valueLiteral>",
                             "<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>",
                             "<http://www.agtinternational.com/ontologies/IoTCore#valueLiteral>"},
                            TuplifierParams::ORDERED)

  //-----Preprocessing Input data-----
    //write the tuples to state, return the preprocessed state as new tuple
    .statefulMap<Preproc_Output_tp, InputState>([&](auto tp, bool,
      StatefulMap<Input_tp, Preproc_Output_tp, InputState>& self) {
      tuples_processed++; //for statistics
      return calculateStates(tp, self.state());
    })
    //filters unuseful and redundant tuples
    .where([](auto tp, bool){return get<1>(tp) != 0; })
    //timestamps
    .assignTimestamps<4>()
    //use a window for regarding only the newest tuples
    .slidingWindow(WindowParams::RangeWindow, windowSize-1)
    //partitioning for multithreaded execution, improving performance
    .partitionBy([&threadAmount](auto tp) { return get<0>(tp) % threadAmount; }, threadAmount)

  //-----Clustering step-----
    .statefulMap<Cluster_Output_tp, ClusterState>([](auto tp, bool outdated,
      StatefulMap<Preproc_Output_tp, Cluster_Output_tp, ClusterState>& self){
      return calculateClusters(tp, outdated, self.state());
    })

  //-----Markov chain-----
    .map<Cluster_Output_tp>([](auto tp, bool outdated) {
      calculateMarkov(tp, outdated);
      return tp;
    })
    ;

  /*
  * ---------------------------------------------------------------------------------------------------------------------
  * Performance measurements
  * ---------------------------------------------------------------------------------------------------------------------
  */

  int cmpCnt = 0;
  auto start = std::chrono::high_resolution_clock::now();
  top.start(false);

  //periodically check if no more anomalies are found
  while(anomalies_found!=cmpCnt) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    cmpCnt = anomalies_found;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  auto end = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
  std::cout<<"Finished. Time taken: "<<duration<<"ms for "<<anomalies_found<<" anomalies in "
    <<tuples_processed<<" processed tuples."<<std::endl;
}
