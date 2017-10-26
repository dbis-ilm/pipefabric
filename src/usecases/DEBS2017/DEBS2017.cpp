//This class contains the solution for the DEBS 2017 - Grand Challenge
//Provided by Anton Gohlke, modified by Constantin Pohl

#include <iostream> //std::cout, std::endl
#include <unordered_map> //container for metadata
#include <mutex> //multithreading purposes
#include <vector> //cluster and Markov chain
#include <chrono> //measuring necessary time for anomaly detection

#include "pfabric.hpp" 

//parameters
#define W 10 //time window steps
#define N 5   //number of transitions to be considered in anomaly detection
#define M 50  //number of maximum iterations for the clustering algorithm
#define T 0.005 //maximum probability

#define THREADS 1 //number of threads


using namespace pfabric;

//the structure of tuples we receive after extract
typedef TuplePtr<std::string, std::string, std::string> Triple;
//the structure of a tuple after RDF processing in data stream
typedef TuplePtr<std::string, std::string, std::string, std::string, std::string> TInPreprocessing;
//the structure of the output after preprocessing map
typedef TuplePtr<int, int, std::string, std::string, Timestamp, int, double, std::string, double> TInClustering;
//the strucure of the output after clustering
typedef TuplePtr<int, int, std::string, std::string, Timestamp, int, double, std::string, double, std::vector<int>> TInMarkov;
//the structure of the output of the metadata rdf tuple
typedef TuplePtr<std::string, std::string, std::string> MetadataRDFTuple;

//used for input data
struct MyState{
  MyState() : ValueDouble(0.0), ThresholdNumber(0.0), cnt(0), PassTuple(0), HasMetadata(0), NumberOfClusters(0), MetadataIDString(""), TS(""), Observation(""), Value(""), T1(0.0){}
  double ValueDouble, ThresholdNumber; //ValueDouble stores current value, ThresholdNumber stores current threshold
  int cnt, PassTuple, HasMetadata, NumberOfClusters; //cnt counts stateful properties per observation group(normally counts till 55, also used as counter for partitioning),
                                                     //PassTuple is a forward flag, HasMetadata indicates if currently observed property has metadata -> statefulProperty,
                                                     //NumberOfClusters stores the amount of clusters a property has
  std::string MetadataIDString, TS, Observation, Value; //MetadataIDSring stores the MetadataID/ID, TS stores the Timestamp as string, Value stores the current data point
  Timestamp T1; //stores the timestamp
};

//used for cluster
struct ClusterState{
  ClusterState() : DataVectorNew(0) {}
  std::vector<std::vector<double> > DataVectorNew;
};

//metadata variables
int MdMachine, MdCluster;
std::string MdID;
std::vector<TuplePtr<std::string, int>> VectorOfMetadata;

//counter for anomalies
long long SimpleCounter = 0;
std::vector<double> tempDoubleVector;

//counter for performance
long long tpProc = 0;

int main(int argc, char **argv){

  //define Parameters
  int WindowSize = 10;
  int ThreadAmount = 3;

  std::mutex mtx; //locks shared variables (used in clustering)
  std::string tempString; //string for temp storage
  tempDoubleVector.push_back(0.0); //double vector for temp storage
  unordered_map<string, int> MetadataMap; //stores the metadata, currently only cluster center is saved, since threshold is always 0.005 in the metadata

  std::cout<<"Processing Metadata"<<std::endl;

  //metadata stream handling
  Topology t2;
  auto s2 = t2.newStreamFromFile("./3rdparty/DEBS2017/data_10M/molding_machine_10M.metadata.nt") //input different source for metadata here
    .extract<Triple>(' ')
    .tuplify<MetadataRDFTuple>({ "<http://www.agtinternational.com/ontologies/WeidmullerMetadata#hasNumberOfClusters>",
                                "<http://www.agtinternational.com/ontologies/IoTCore#valueLiteral>"},
                               TuplifierParams::ORDERED)
    .notify([&](auto tp, bool outdated) {
      tempString=get<1>(tp);
      if(tempString.size() < 1){
        tempString = get<0>(tp);
        tempString.erase(tempString.begin(), tempString.begin()+84);
        tempString.pop_back(); //now the value looks like AA_BBB with AA=machine, BBB=Value
        MdID = tempString;
        MetadataMap.insert({MdID, MdCluster});
      }else{
        tempString = get<1>(tp);
        tempString.erase(0,1);
        tempString.erase(tempString.end()-41, tempString.end());
        MdCluster = atoi(tempString.c_str());
      }
    });

  t2.start(false);

  std::cout<<"Stateful Properties: "<<MetadataMap.size()<<std::endl;
  std::cout<<"Metadata stored"<< std::endl;

  //actual stream with real-time data
  Topology t;
  auto s = t.newStreamFromFile("./3rdparty/DEBS2017/data_10M/molding_machine_10M.nt") //input different source for metadata here
    .extract<Triple>(' ')
    .tuplify<TInPreprocessing>({"<http://purl.oclc.org/NET/ssnx/ssn#observedProperty>",
                                "<http://www.agtinternational.com/ontologies/I4.0#observedCycle>",
                                "<http://www.agtinternational.com/ontologies/IoTCore#valueLiteral>",
                                "<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>",
                                "<http://www.agtinternational.com/ontologies/IoTCore#valueLiteral>"},
                               TuplifierParams::ORDERED)
    .statefulMap<TInClustering, MyState>([&](auto tp, bool, std::shared_ptr<MyState> state){
      tpProc++;
      std::string testString = get<0>(tp);
      state->PassTuple = 0; //initialize the flag

      if(testString.find("Observation_") != std::string::npos){
        //get the Value ID, it starts at the 51th position of the string and the ">" at the end of the string has to be deleted
        state->Observation = testString.substr(57);
        state->Observation.pop_back();
        state->MetadataIDString = get<1>(tp);
        state->MetadataIDString.erase(state->MetadataIDString.begin(), state->MetadataIDString.begin()+64);
        state->MetadataIDString.pop_back();

        //check if the data has metadata (otherwise it is useless)
        std::unordered_map<std::string,int>::const_iterator got = MetadataMap.find (state->MetadataIDString);
        if(got != MetadataMap.end()){
          state->HasMetadata++;
          state->NumberOfClusters = MetadataMap[state->MetadataIDString];
        }else{
          state->HasMetadata = 0;
        }
      }else{
        if(testString.find("Value") != std::string::npos){
          if(state->HasMetadata > 0){
            state->cnt++;
            state->Value = get<3>(tp);
            state->Value.erase(0,1);

            if(state->Value.size()>0){ //needed as catch if it doesn't contain content
              state->Value.erase(state->Value.end()-44, state->Value.end());
              //convert the string into double
              state->ValueDouble = std::stod(state->Value);
              state->PassTuple = 1;
            }
          }
        }else{
          if(testString.find("Timestamp") != std::string::npos){ //get timestamp
            state->TS = get<3>(tp); //get the whole date+time
            state->TS.erase(0,1);
            state->TS.erase(state->TS.end()-52, state->TS.end());
            state->T1 = TimestampHelper::stringToTimestamp(state->TS); //is the new TS and the following TS calculations are obsolete

            //now the TS variable is used to store the time stamp name
            state->TS = testString;
            state->TS = state->TS.substr(45);
            state->TS.pop_back();
            //counter for partitionBy, counts the amount of properties in one timestamp
            state->cnt = 0; 
          }
        }
      }

      return makeTuplePtr(state->cnt-1, state->PassTuple, state->MetadataIDString, state->TS, state->T1, state->NumberOfClusters, state->ThresholdNumber, state->Observation,
                          state->ValueDouble);
    })
    .where([](auto tp, bool){return get<1>(tp) != 0; }) //filters unuseful and redundant tuples
    .assignTimestamps<4>()
    .slidingWindow(WindowParams::RangeWindow, WindowSize-1) //W-1 so it has W tuples in the window
    .partitionBy([&ThreadAmount](auto tp) { return get<0>(tp) % ThreadAmount; }, ThreadAmount)
    .statefulMap<TInMarkov, ClusterState>([&mtx](auto tp, bool outdated, std::shared_ptr<ClusterState> state){ //this operator clusters
      int FoundRowNew = 0, RowNumberNew = 0, HasEnoughValues = 0;
      double testDouble = 0.0;
      std::vector<int> ClusterSequence(0, 0); //handover variable 
      std::string TempMetadataString = get<2>(tp); //check for outdated tuple or not (within the window or not)
      double TempMachineNumber, TempValueNumber;
      TempMachineNumber = std::stod(TempMetadataString.substr(0, TempMetadataString.find("_")));
      TempValueNumber = std::stod(TempMetadataString.substr(TempMetadataString.find("_")+1));
      vector< vector<double> >::iterator RowNew;
      vector<double>::iterator ColNew;

      if(outdated == true){ //check if outdated
        for (RowNew = state->DataVectorNew.begin(); RowNew != state->DataVectorNew.end(); RowNew++){
          ColNew = RowNew->begin();
          if((*ColNew) == TempMachineNumber){
            ColNew++;
            if((*ColNew) == TempValueNumber){
              FoundRowNew++;
              break;
            }
          }
          RowNumberNew++;
        }
        mtx.lock();
        state->DataVectorNew[RowNumberNew].erase(state->DataVectorNew[RowNumberNew].begin() + 2);
        mtx.unlock();
      }else{//if the data is not outdated it should be processed
        vector< vector<double> >::iterator RowNew;
        vector<double>::iterator ColNew;

        //check where to add the new datapoint
        for (RowNew = state->DataVectorNew.begin(); RowNew != state->DataVectorNew.end(); RowNew++){
          ColNew = RowNew->begin();
          if(( *ColNew) == TempMachineNumber){
            ColNew++;
            if((*ColNew) == TempValueNumber){
              FoundRowNew++;
              break;
            }
          }
          RowNumberNew++;
        }

        if(FoundRowNew == 0){ //is there already data to this id
          mtx.lock();
          tempDoubleVector.clear();
          tempDoubleVector.push_back(TempMachineNumber);
          tempDoubleVector.push_back(TempValueNumber);
          tempDoubleVector.push_back(get<8>(tp));
          state->DataVectorNew.push_back(tempDoubleVector); //create new variable
          mtx.unlock();
        }else{
          mtx.lock();
          state->DataVectorNew[RowNumberNew].push_back(get<8>(tp)); //add data at the end of row x
          mtx.unlock();
        }

        int NumberOfClusters = get<5>(tp);
        int NumberOfValues = (int)state->DataVectorNew[RowNumberNew].size()-2;

        if(NumberOfValues > N){ //check if there are enough values for clustering
          std::vector<double> ClusterValueVector (1, 0.0);
          std::vector<int> ClusterAssignVector(NumberOfValues, 0);
          std::vector<double>::iterator it1;
          it1 = state->DataVectorNew[RowNumberNew].begin()+2;
          std::vector<double> ValuesVector; 
          ValuesVector.assign(it1, state->DataVectorNew[RowNumberNew].end());

          ClusterValueVector[0] = ValuesVector[0]; //initialize ClusterVector, use the distinct first values of ValuesVector
          for(int i = 1; (int)ClusterValueVector.size() < NumberOfClusters && i <= (int)ValuesVector.size(); i++){
            if(std::find(ClusterValueVector.begin(), ClusterValueVector.end(), ValuesVector[i]) != ClusterValueVector.end()){
            }else{
              ClusterValueVector.push_back(ValuesVector[i]);
            }
          }

          //iterate to convergence
          int NumberOfChanges = 1;
          int ClusterIt = 0;
          while(ClusterIt<M && NumberOfChanges != 0){
            NumberOfChanges = 0;
            //calculate distance for each value, iterate through all values
            for(int i = 0; (int)ValuesVector.size() > i; i++){
              //distance to the current assigned clustercenter
              double OldDistance = (ValuesVector[i]-ClusterValueVector[ClusterAssignVector[i]])*(ValuesVector[i]-ClusterValueVector[ClusterAssignVector[i]]);
              for(int j = 0; (int)ClusterValueVector.size() > j; j++){ //first values get doublechecked
                //distance to the other clustercenters
                double NewDistance = (ValuesVector[i]-ClusterValueVector[j])*(ValuesVector[i]-ClusterValueVector[j]);
                //check if the distance is the same
                if (NewDistance == OldDistance){
                  if (ClusterValueVector[j] > ClusterValueVector[ClusterAssignVector[i]]){
                    ClusterAssignVector[i] = j;
                  }
                }
                //check if the distance is smaller
                if (NewDistance < OldDistance){
                  OldDistance = NewDistance;
                  ClusterAssignVector[i] = j;
                  NumberOfChanges++;
                }
              }
            }

            //calculate new clustercenters
            for(int i = 0; (int)ClusterValueVector.size() > i; i++){
              int CenterCounter = 0;
              double ClusterSum = 0;
              for(int j = 0; (int)ClusterAssignVector.size() > j; j++){
                if(ClusterAssignVector[j] == i){
                  CenterCounter++;
                  ClusterSum += ValuesVector[j];
                }
              }

              if (ClusterSum != 0){ //check for 0
                double NewClusterValueVector = ClusterSum/(double)CenterCounter;
                if(NewClusterValueVector != ClusterValueVector[i]){ //clusterValueVector is different so another iteration is needed
                  NumberOfChanges++;
                }
                ClusterValueVector[i] = NewClusterValueVector;
              }
            }
            ClusterIt++;
          }
          ClusterSequence = ClusterAssignVector;
          HasEnoughValues = 1;
        }else{
          HasEnoughValues = 0;
        }
      }

      return makeTuplePtr(get<0>(tp), HasEnoughValues, get<2>(tp), get<3>(tp), get<4>(tp), get<5>(tp), get<6>(tp), get<7>(tp), get<8>(tp), ClusterSequence);
    })
    .map<TInMarkov>([](auto tp, bool outdated){ //here does the Markov-Magic happen
      std::vector<int> SequenceVector = get<9>(tp);
      if(!outdated && (get<1>(tp)==1)){ //current data and enough data points?
        int NrOfCluster = get<5>(tp);
        //sequenz is equal/bigger the the amount of clusters and the seqence is bigger as the required amount 
        if((int)SequenceVector.size() > N){
          double TransitionCounter[NrOfCluster][NrOfCluster]; //matrix to count the transitions
          memset(TransitionCounter, 0, sizeof(TransitionCounter));  //only used because for whatever reason ={{0}} doesn't work
          int LastClusterCenter = SequenceVector[0];
          int NextClusterCenter = 0;

          for(int i = 1; (int)SequenceVector.size() > i; i++){ //count the transitions
            NextClusterCenter = SequenceVector[i];
            TransitionCounter[LastClusterCenter][NextClusterCenter]++;
            LastClusterCenter = NextClusterCenter;
          }

          for(int i = 0; NrOfCluster > i; i++){ //calculate the transition probability
            double sum = 0.0;
            for(int j = 0; NrOfCluster > j; j++){
              sum += TransitionCounter[i][j];
            }
            if(sum != 0){
              for(int j = 0; NrOfCluster > j; j++){
                TransitionCounter[i][j] = TransitionCounter[i][j]/sum;
              }
            }
          }

          //calculate the probability of the sequence
          double TransitionProbability = 1;
          LastClusterCenter = SequenceVector[(int)SequenceVector.size()-N-1];
          for(int i = 1; i <= N; i++ ){
            NextClusterCenter = SequenceVector[(int)SequenceVector.size()-N-1+i];
            TransitionProbability *= TransitionCounter[LastClusterCenter][NextClusterCenter];
            LastClusterCenter = NextClusterCenter;
          }

          if(TransitionProbability < T){
            SimpleCounter++;
            std::cout<<SimpleCounter<<"th Anomaly! TransitionProb: "<<TransitionProbability<<" "<<get<3>(tp)<<" with time: "<<TimestampHelper::timestampToString(get<4>(tp))
              <<" Observation: "<<get<7>(tp)<<" MetadataID: "<<get<2>(tp)<<std::endl;
          }
        }
      }
      return tp;
    })
    ;

  int cmpCnt = 0;
  auto start = std::chrono::high_resolution_clock::now();
  t.start(false);
  while(SimpleCounter!=cmpCnt) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    cmpCnt = SimpleCounter;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  auto end = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
  std::cout<<"Finished. Time taken: "<<duration<<"ms for "<<SimpleCounter<<" anomalies in "<<tpProc<<" processed tuples."<<std::endl;
}
