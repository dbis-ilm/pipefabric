#ifndef DEBS2016Defs_hpp_
#define DEBS2016Defs_hpp_

#include <string>
#include <list>
#include <unordered_map>

#include "pfabric.hpp"

using namespace pfabric;

// --------------------------------- post types --------------------------------
// ts, post_id, user_id, post, user
typedef TuplePtr<Tuple<std::string, long, long, std::string, std::string>> RawPostType;
// ts, post_id, user_id
typedef TuplePtr<Tuple<Timestamp, long, long>> PostType;

// ---------------------------------- max_ts -----------------------------------
typedef TuplePtr<Tuple<Timestamp>> TimestampTupleType;

// ------------------------------ comment types --------------------------------
// ts, comment_id, user_id, comment, user, comment_replied, post_commented
typedef TuplePtr<Tuple<std::string, long, long, std::string, std::string, long, long>> RawCommentType;
// ts, comment_id, comment_replied, post_commented
typedef TuplePtr<Tuple<Timestamp, long, long, long>> CommentType;

// ------------------ types for posts with scores and comments -----------------
struct Commentor {
	Commentor(Timestamp t, long c) : ts(t), commentId(c) {}
	Timestamp ts;
	long commentId;
};
typedef std::list<Commentor> CommentorList;
typedef std::shared_ptr<CommentorList> CommentorListPtr;

// ts, post_id, post_user, score, list_of_commentors
typedef TuplePtr<Tuple<Timestamp, long, long, int, CommentorListPtr>> CommentedPostType;

inline CommentorListPtr makeCommentorList() { return std::make_shared<CommentorList>(); }

inline CommentorListPtr addCommentor(CommentorListPtr lst, const CommentType& cmt) {
	lst->push_back(Commentor(cmt->getAttribute<0>(), cmt->getAttribute<1>()));
	return lst;
}

inline CommentorListPtr removeCommentor(CommentorListPtr lst, const CommentType& cmt) {
	lst->remove_if([&cmt](const Commentor& c) -> bool {
		return c.commentId == cmt->getAttribute<1>();
	});
	return lst;
}

inline int calcScore(Timestamp ts, Timestamp currentTime) {
	auto s = TimestampHelper::toDays(currentTime - ts);
	return std::max(std::min(s, 10u), 0u);
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

#endif
