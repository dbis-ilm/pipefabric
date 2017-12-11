### Transactional Stream Processing ###

The example in this directory demonstrates the usage of transactional processing features in PipeFabric.
The demo program runs two topologies (queries):

  + a stream query which reads tuples from a file and stores them into a persistent table,
  + a batch query which simply reads this table periodically.

The data represent a simple banking scenario where the balance of accounts are updates. For this purpose, the
file (as well as the corresponding table `ACCOUNTS`) contain the following fields:

  + TransactionID - a value to specify which elements belong to the same transaction
  + AccountID - the account number
  + CustomerName - the name of the customer (account holder)
  + Balance - the new balance of the account

Both topologies run in a transactional context. For the stream query this means that `BOT` (begin on transaction)
`COMMIT` commands are inserted into the stream as punctuations. This is done by a `statefulMap` operator which
creates these commands based on the TransactionID field of the stream elements. In addition, an `assignTransactionID`
statement is needed to specify, which field contains the TransactionID value.

Currently, both topologies use special `toTxTable` and `selectFromTxTable` respectively statements which 
deal with transactional tables. This guarantees the following behavior: updates on the `ACCOUNTS` table are
visible for the batch query only after the `COMMIT` of the corresponding transaction.

