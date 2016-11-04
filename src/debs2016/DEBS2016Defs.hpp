#ifndef DEBS2016Defs_hpp_
#define DEBS2016Defs_hpp_

#include <string>
#include <list>

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

#endif
