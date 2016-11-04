## PipeFabric Implementation for DEBS Grand Challenge 2016 ##

### Query 1 ###

www.ics.uci.edu/~debs2016/call-grand-challenge.html

"The goal of query 1 is to compute the top-3 scoring active posts, producing an
updated result every time they change.

The total score of an active post P is computed as the sum of its own score plus
the score of all its related comments. Active posts having the same total score
should be ranked based on their timestamps (in descending order), and if their
timestamps are also the same, they should be ranked based on the timestamps of
their last received related comments (in descending order). A comment C is
related to a post P if it is a direct reply to P or if the chain of C's
preceding messages links back to P.

Each new post has an initial own score of 10 which decreases by 1 each time
another 24 hours elapse since the post's creation. Each new comment's score is
also initially set to 10 and decreases by 1 in the same way (every 24 hours
since the comment's creation). Both post and comment scores are non-negative
numbers, that is, they cannot drop below zero. A post is considered no longer
active (that is, no longer part of the present and future analysis) as soon as
its total score reaches zero, even if it receives additional comments in the
future."

Query 1 is implemented by a set of dataflows:
 * The first dataflow (posts) reads the posts and converts them into tuples.
 * The second dataflow (maxTime) takes this stream of posts as input and retrieves the maximum
   timstamp in order to have an internal clock.
 * The third dataflow (postsToTable) processes also the stream of posts and stores them in a
   table POSTS.
 * With dataflow no. 4 (comments) comments are read and converted into tuples. These tuples are
   delayed until the internal clock driven by the posts is advanced accordingly.
   Then, the post to which the comment is related is determined and the POSTS table is updated.
 * Finally, dataflow no. 5 is triggered by the updates on the POSTS table and computes
   the top-3 posts.
