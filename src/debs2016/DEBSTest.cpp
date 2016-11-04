#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"
#include "pfabric.hpp"
#include "DEBS2016Defs.hpp"

using namespace pfabric;

TEST_CASE("Create and manipulate post with comments", "[Comment][Post]") {
  // typedef TuplePtr<Tuple<Timestamp, long, long, int, CommentorListPtr>> CommentedPostType;

  auto post = makeTuplePtr((Timestamp)1, 11l, 12l, 0, makeCommentorList());

  auto c1 = makeTuplePtr((Timestamp)1, 101l, 0l, 11l);
  auto c2 = makeTuplePtr((Timestamp)2, 102l, 0l, 11l);
  auto c3 = makeTuplePtr((Timestamp)3, 103l, 0l, 11l);
  auto c4 = makeTuplePtr((Timestamp)3, 103l, 0l, 11l);

  {
    auto commentorList = post->getAttribute<4>();

    // typedef TuplePtr<Tuple<Timestamp, long, long, long>> CommentType;
    addCommentor(commentorList, c1);

    addCommentor(commentorList, c2);

    addCommentor(commentorList, c3);

    REQUIRE(commentorList->size() == 3);
  }
  {
    auto commentorList = post->getAttribute<4>();
    removeCommentor(commentorList, c2);
    removeCommentor(commentorList, c4);

    REQUIRE(commentorList->size() == 1);
  }
}

TEST_CASE("Calculate the score of posts", "[Comment][Post]") {
  // TODO
}
