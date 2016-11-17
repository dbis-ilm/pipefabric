#include <iostream>
#include <condition_variable>
#include <mutex>
#include <utility>
#include <unordered_map>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "pfabric.hpp"
#include "DEBS2016Defs.hpp"

using namespace std;
using namespace pfabric;

namespace po = boost::program_options;

std::string dataDir = ".";

class GlobalTime {
private:
	std::atomic<Timestamp> mCurrentTime;

public:
	std::condition_variable mCondVar;
	std::mutex mCondMtx;

	GlobalTime() {}

	void set(Timestamp ts) {
    if (ts == 0) return;
		mCurrentTime.store(ts);
		std::lock_guard<std::mutex> l(mCondMtx);
		mCondVar.notify_all();
	}
	Timestamp get() { return mCurrentTime.load(); }
};

GlobalTime globalTime;

int updateScore(CommentedPostType& cp, Timestamp currentTime) {
		int s = calcScore(cp->getAttribute<0>(), currentTime);
		auto lst = cp->getAttribute<4>();
		for (auto& cmt : *lst) {
			s += calcScore(cmt.ts, currentTime);
		}
		cp->setAttribute<0>(currentTime);
		cp->setAttribute<3>(s);
		return s;
}

// -----------------------------------------------------------------------------
std::shared_ptr<Topology>
buildQuery1(PFabricContext& ctx, boost::filesystem::path& dataPath) {
	auto endTime = TimestampHelper::stringToTimestamp("2016-12-31T23:59:59.000+0000");
	auto t = ctx.createTopology();
	auto ttl = ctx.createStream<TTLType>("ttl");
	auto postTable = ctx.getTable<CommentedPostType, long>("Posts");

	boost::filesystem::path postPath = dataPath;
	postPath += "/posts.dat";

	// ---------- posts ----------
  //
  // prepare the post tuples by reading the file and produce
  // a stream of tuples with timestamps.
	auto posts = t->newStreamFromFile(postPath.string())
 	 .extract<RawPostType>('|')
 	 .map<RawPostType, PostType>([](auto tp, bool) -> PostType {
 		return makeTuplePtr(TimestampHelper::stringToTimestamp(get<0>(tp)),
 												get<1>(tp),
 												get<2>(tp));
 	});

	// ---------- maxTime ----------
  //
  // the clock is driven by the posts - whenever we process a post, its
  // timestamp is used to update the global time.
  typedef Aggregator1<PostType, AggrGlobalMax<Timestamp>, 0> TimestampAggrState;

	auto maxTime = posts
    // compute the maximum of time
		.aggregate<PostType, TimestampTupleType, TimestampAggrState>()
    // update the global clock
		.notify<TimestampTupleType>([&](auto tp, bool outdated) {
			globalTime.set(get<0>(tp));
    }, [&](auto pp) {
      // in case of end of stream we set the clock to a time in the future
			globalTime.set(endTime);
		});
//    .print<TimestampTupleType>();

	// ---------- postsToTable ----------
  //
  // prepare the post tuples by adding a list of comments and a score value
  // and store them in the table "posts"
  auto postsToTable = posts
		.map<PostType, CommentedPostType>([](auto tp, bool) -> CommentedPostType {
			return makeTuplePtr(get<0>(tp),
													get<1>(tp),
													get<2>(tp),
													10, // initial score = 10
													makeCommentorList());
		})
    // column #1 is used as the key
		.keyBy<CommentedPostType, 1, long>()
    // store all tuples in the table
		.toTable<CommentedPostType, long>(postTable)
    // create a time-to-live tuple and send it to the ttl stream
		.map<CommentedPostType, TTLType>([](auto tp, bool) -> TTLType {
      // +1 day (when the score has to be updated) and 10 remaining days
			return makeTuplePtr(get<1>(tp), get<0>(tp) + 1000l * 60 * 60 * 24, 10);
		})
		.toStream<TTLType>(ttl);

	boost::filesystem::path commentPath = dataPath;
 	commentPath += "/comments.dat";

	// ---------- comments ----------
  //
  // prepare the comments tuples by finding the post to which they are assigned
 	auto comments = t->newStreamFromFile(commentPath.string())
		.extract<RawCommentType>('|')
		.map<RawCommentType,CommentType>([](auto tp, bool) -> CommentType {
			auto res = makeTuplePtr(TimestampHelper::stringToTimestamp(get<0>(tp)),
													get<1>(tp),
													get<5>(tp),
													get<6>(tp));
			// handle null values
			if (tp->isNull(5)) res->setNull(2);
			if (tp->isNull(6)) res->setNull(3);
			return res;
	    })
    // if a comment refers to another comment then we have to identify
    // the original post which is done via a Comments2PostMap
		.statefulMap<CommentType,CommentType,Comments2PostMap>(
			[](auto tp, bool, auto state) -> CommentType {
				auto postID = tp->isNull(3)
					? state->findPostIdForComment(get<2>(tp)) : get<3>(tp);
				// std::cout << "-> " << tp << " ==> post: " << postID << std::endl;
				state->registerPostForComment(get<1>(tp), postID);
				return makeTuplePtr(get<0>(tp),
														get<1>(tp),
														get<2>(tp),
														postID);
			})
    // make sure that comments are not newer than the posts we have already processed
		.barrier<CommentType>(globalTime.mCondVar, globalTime.mCondMtx, [&](auto tp) -> bool {
			return get<0>(tp) < globalTime.get();
		})
		.assignTimestamps<CommentType, 0>()
    // comments are outdated after 10 days
		.slidingWindow<CommentType>(WindowParams::RangeWindow, 60 * 60 * 24 * 10)
		.keyBy<CommentType, 3, long>()
    // update the table by adding or removing the comment to its post
		.updateTable<CommentType, CommentedPostType, long>(postTable,
			[](auto tp, bool outdated, auto oldRec) -> std::pair<CommentedPostType, bool> {
				auto tup = makeTuplePtr(get<0>(oldRec),
															get<1>(oldRec),
															get<2>(oldRec),
															get<3>(oldRec),
															outdated
																? removeCommentor(get<4>(oldRec), tp)
																: addCommentor(get<4>(oldRec), tp)
														);
				auto score = updateScore(tup, globalTime.get());
				return std::make_pair(tup, score > 0);
			})
      // and finally create a TTL tuple
			.map<CommentType, TTLType>([](auto tp, bool) -> TTLType {
				return makeTuplePtr(get<3>(tp), get<0>(tp) + 1000l * 60 * 60 * 24, 10);
			})
  .toStream<TTLType>(ttl);

  // ---------- scoreUpdates ----------
  //
  // process the ttl tuples to identify records in the table "posts" for which
  // the scores have to be updated.
	auto scoreUpdates = t->fromStream<TTLType>(ttl)
      // ttl tuples are blocked until they are on time
			.barrier<TTLType>(globalTime.mCondVar, globalTime.mCondMtx, [&](auto tp) -> bool {
				return get<1>(tp) < globalTime.get();
			})
      // if a tuple reaches end of life then we can remove it
      .where<TTLType>([&](auto tp, bool) -> bool {
        return get<2>(tp) > 0 && get<1>(tp) < endTime;
      })
      .keyBy<TTLType, 0, long>()
      // update the score of the post in the table - the key is taken fom the ttl tuple
      .updateTable<TTLType, CommentedPostType, long>(postTable,
                                                     [](auto tp, bool outdated, auto oldRec) -> std::pair<CommentedPostType, bool> {
          auto tup = oldRec;
          auto score = updateScore(tup, globalTime.get());
					// delete tuple if score == 0
          return std::make_pair(tup, score > 0);
      })
      // update the ttl field of the tuple
			.map<TTLType, TTLType>([](auto tp, bool) -> TTLType {
				return makeTuplePtr(get<0>(tp), get<1>(tp) + 1000l * 60 * 60 * 24, get<2>(tp) - 1);
      })
      // and add it again to the stream
  		.toStream<TTLType>(ttl);

	//
	auto topk = t->newStreamFromTable<CommentedPostType, long>(postTable)
	// TODO: maintain top-3
		.print<CommentedPostType>(std::cout, [&](std::ostream& os, auto tp) {
			os << TimestampHelper::timestampToString(get<0>(tp)) << ","
				 << get<1>(tp) << "," << get<3>(tp) << std::endl;
		});

  return t;
}

void processCmdLine(int argc, char **argv) {
	po::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "produce help message")
  	("dir,d", po::value<std::string>(), "directory of data files")
		;

	 po::variables_map vm;

	 try {
		 po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
		 po::notify(vm);
	 }
	 catch (po::error& exc) {
		 std::cerr << "ERROR: " << exc.what() << std::endl;
		 exit(-1);
	 }
	 if (vm.count("help")) {
		 std::cerr << desc << std::endl;
		 exit(1);
   }
	 if (vm.count("dir")) {
     dataDir = vm["dir"].as<std::string>();
   }
	 else {
		 std::cerr << "ERROR: missing data directory (--dir)" << std::endl;
		 exit(-1);
	 }
}


int main(int argc, char** argv) {
	PFabricContext ctx;

	processCmdLine(argc, argv);

	boost::filesystem::path dataPath(dataDir);
	if (! boost::filesystem::is_directory(dataPath)) {
		std::cout << "invalid data directory \"" << dataDir << "\"" << std::endl;
		exit(-1);
	}

	// we create a table for posts with scores and a list of comments to this post
	auto postTable = ctx.createTable<CommentedPostType, long>("Posts");

	auto t = buildQuery1(ctx, dataPath);

	t->start();
	t->wait();

	using namespace std::chrono_literals;
	// std::this_thread::sleep_for(120s);
	std::cout << "--------------------------- stopping ---------------------------" << std::endl;
}
