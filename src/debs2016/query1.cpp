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
		mCurrentTime.store(ts);
		std::lock_guard<std::mutex> l(mCondMtx);
		mCondVar.notify_all();
	}
	Timestamp get() { return mCurrentTime.load(); }
};

GlobalTime globalTime;

inline CommentorListPtr makeCommentorList() { return std::make_shared<CommentorList>(); }

CommentorListPtr addCommentor(CommentorListPtr lst, const CommentType& cmt) {
	lst->push_back(Commentor(cmt->getAttribute<0>(), cmt->getAttribute<1>()));
	return lst;
}

CommentorListPtr removeCommentor(CommentorListPtr lst, const CommentType& cmt) {
	lst->remove_if([&cmt](const Commentor& c) -> bool { return c.commentId == cmt->getAttribute<1>(); });
	return lst;
}

int calcScore(Timestamp ts, Timestamp currentTime) {
	auto s = TimestampHelper::toDays(currentTime - ts);
	return std::max(std::min(s, 10u), 0u);
}

void updateScore(CommentedPostType& cp, Timestamp currentTime) {
		int s = calcScore(cp->getAttribute<0>(), currentTime);
		auto lst = cp->getAttribute<4>();
		for (auto& cmt : *lst) {
			s += calcScore(cmt.ts, currentTime);
		}
		cp->setAttribute<3>(s);
}

struct Comments2PostMap {
		std::unordered_map<long, long> comment2post;

		inline long findPostIdForComment(long c_id) {
			return comment2post[c_id];
		}

		inline void registerPostForComment(long c_id, long p_id) {
			comment2post.insert(std::make_pair(c_id, p_id));
		}
};

// -----------------------------------------------------------------------------
void buildQuery1(std::shared_ptr<Topology> t,
								 boost::filesystem::path& dataPath,
							   std::shared_ptr<Table<CommentedPostType, long>> postTable) {
	boost::filesystem::path postPath = dataPath;
	postPath += "/posts.dat";

	// ---------- posts ----------
	typedef Aggregator1<PostType, AggrGlobalMax<Timestamp>, 0> TimestampAggrState;

	auto posts = t->newStreamFromFile(postPath.string())
 	 .extract<RawPostType>('|')
 	 .map<RawPostType, PostType>([](auto tp, bool) -> PostType {
 		return makeTuplePtr(TimestampHelper::stringToTimestamp(getAttribute<0>(tp)),
 												getAttribute<1>(tp),
 												getAttribute<2>(tp));
 	});

	// ---------- maxTime ----------
	auto maxTime = posts
		.aggregate<PostType, TimestampTupleType, TimestampAggrState>()
		.notify<TimestampTupleType>([&](auto tp, bool outdated) {
			globalTime.set(getAttribute<0>(tp));
    }, [&](auto pp) {
			globalTime.set(TimestampHelper::stringToTimestamp("2016-12-31T23:59:59.000+0000"));
		});
    // .print<TimestampTupleType>();

	// ---------- postsToTable ----------
  auto postsToTable = posts
		.map<PostType, CommentedPostType>([](auto tp, bool) -> CommentedPostType {
			return makeTuplePtr(getAttribute<0>(tp),
													getAttribute<1>(tp),
													getAttribute<2>(tp),
													10, // initial score = 10
													makeCommentorList());
		})
		.keyBy<CommentedPostType, 1, long>()
		.toTable<CommentedPostType, long>(postTable)
		.print<CommentedPostType>(std::cout, [](std::ostream& os, auto tp) {
			// os << "post -> " << TimestampHelper::timestampToString(getAttribute<0>(tp)) << std::endl;
		});


	boost::filesystem::path commentPath = dataPath;
 	commentPath += "/comments.dat";

	// ---------- comments ----------
	auto comments = t->newStreamFromFile(commentPath.string())
		.extract<RawCommentType>('|')
		.map<RawCommentType,CommentType>([](auto tp, bool) -> CommentType {
			auto res = makeTuplePtr(TimestampHelper::stringToTimestamp(getAttribute<0>(tp)),
													getAttribute<1>(tp),
													getAttribute<5>(tp),
													getAttribute<6>(tp));
			// handle null values
			if (tp->isNull(5)) res->setNull(2);
			if (tp->isNull(6)) res->setNull(3);
			return res;
	    })
		.statefulMap<CommentType,CommentType,Comments2PostMap>(
			[](auto tp, bool, auto state) -> CommentType {
				auto postID = tp->isNull(3)
					? state->findPostIdForComment(getAttribute<2>(tp))
					: getAttribute<3>(tp);
				// std::cout << "-> " << tp << " ==> post: " << postID << std::endl;
				state->registerPostForComment(getAttribute<1>(tp), postID);
				return makeTuplePtr(getAttribute<0>(tp),
														getAttribute<1>(tp),
														getAttribute<2>(tp),
														postID);
			})
		.barrier<CommentType>(globalTime.mCondVar, globalTime.mCondMtx, [&](auto tp) -> bool {
			return getAttribute<0>(tp) < globalTime.get();
		})
		.assignTimestamps<CommentType>([](auto tp) { return getAttribute<0>(tp); } )
		.slidingWindow<CommentType>(WindowParams::RangeWindow, 60 * 60 * 24 * 10)
		.keyBy<CommentType, 3, long>()
		.updateTable<CommentType, CommentedPostType, long>(postTable,
			[](auto tp, bool outdated, auto oldRec) -> CommentedPostType {
				auto tup = makeTuplePtr(getAttribute<0>(oldRec),
															getAttribute<1>(oldRec),
															getAttribute<2>(oldRec),
															getAttribute<3>(oldRec),
															outdated
																? removeCommentor(getAttribute<4>(oldRec), tp)
																: addCommentor(getAttribute<4>(oldRec), tp)
														);
				updateScore(tup, globalTime.get());
				std::cout << tup << std::endl;
				return tup;
			})
			.print<CommentType>(std::cout, [](std::ostream& os, auto tp) {
				// os << "comment -> " << TimestampHelper::timestampToString(getAttribute<0>(tp)) << std::endl;
			});

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

	auto t = ctx.createTopology();

	buildQuery1(t, dataPath, postTable);

	t->start();
	t->wait();
}
