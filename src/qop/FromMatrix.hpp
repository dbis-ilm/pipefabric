#ifndef FROMMATRIX__HH
#define FROMMATRIX__HH

#include <list>
#include <mutex>
#include <thread>
#include <memory>
#include <condition_variable>

#include "matrix/Matrix.hpp"
#include "qop/DataSource.hpp"

namespace pfabric {
	
	template<typename MatrixType>
	class FromMatrix : public DataSource< typename MatrixType::StreamElement > {
	public:
		PFABRIC_SOURCE_TYPEDEFS(typename MatrixType::StreamElement);
		
		typedef MatrixParams::ModificationMode 		ModeModification; 
		typedef std::shared_ptr< MatrixType > 		MatrixPtr;
		typedef typename MatrixType::StreamElement 	StreamElement;

		FromMatrix(MatrixPtr matrix)
		: interupted(false)
		{
			// set callback
			matrix->registerObserver([this](const StreamElement &tuple, ModeModification mode) {
				matrixCallback(tuple, mode);
			});
			producerTh = std::thread(&FromMatrix<MatrixType>::produce, this);
		}

		~FromMatrix()
		{
			interupted = true;
			{
				std::lock_guard<std::mutex> guard(mutex);
				condvar.notify_one();
			}
			if(producerTh.joinable()) {
				producerTh.join();
			}
		}


	private:

		void matrixCallback(const StreamElement &tuple, ModeModification mode)
		{
			// insert tuple to queue
			std::lock_guard<std::mutex> guard(this->mutex);
			queue.push_back({tuple, mode == ModeModification::Delete});
			condvar.notify_one();
		}

		void produce()
		{
			while(!interupted) {
				std::unique_lock<std::mutex> guard(mutex);
				condvar.wait( guard,
					[&](){ return interupted || !queue.empty(); }
					);
				// We acquare tuple from queue and forward tuples
				while(!queue.empty()) {
					auto changes = queue.front();
					queue.pop_front();
					this->getOutputDataChannel().publish(changes.first, changes.second);
				}
			}
		}
		bool 										interupted; //< Flag for stopping
		std::thread 								producerTh;	//< Thread for forwarding tuples
		std::mutex									mutex;		//< Barrier for threads
		std::condition_variable 					condvar;	//< Variable for notify
		std::list<std::pair<StreamElement, bool>> 	queue;		//< Thread-safe queue
	};
}


#endif //FROMMATRIX__HH