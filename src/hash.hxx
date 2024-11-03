/*
	CLASS HASH - Memory-Based hashing using open-addressed quadratic
        probing.

	Jim Fullton
	5/2/96

*/

#ifndef HASH_HXX
#define HASH_HXX

// typedef your hash table parameters
// these should be changed for your application


typedef INT Key_type;
typedef CHR *Data_type;

// this should be OK for most applications
typedef struct item_tag {
  Key_type Key;
  INT State;
  Data_type Block;
} Item_type;


//@ManMemo: Memory-Based hashing using open-addressed quadratic probing.
/*@Doc: The {\em HASH} class implements a simple memory-based hash table
using open-addressed quadratic probing.  As with any open-addressed hash
method, the hash table must be large enough to contain every entry, and
there must be no duplicate keys.

We handle the entry count problem by returning a status tha tindicates
whether or not an insertion occurred.  Insertion failure is not an error
condition in our Isearch application, so we simply flag it.

The default hash table size is 997.

The hash item contains a key, a state variable, and a pointer to a block
of data.  The data blocks must be dynamically allocated using {\bf
malloc()} as they are free'd in the hash table destructor.  The {\bf
State} variable contains the following information:

\begin{center}
\begin{tabular}{ll} {\bf Value} & {\bf Meaning} \\
\hline
0 & Empty Slot\\
1 & Filled Slot \\
2 & Deleted Slot - Continue Probe
\end{tabular}
\end{center}


*/

class HASH{
	
public:
//@ManMemo: Create a hash object of table size {\em Size} entries.

  HASH(INT Size);
/*@Doc This constructor simply creates a hash object with a hash table
of user-defined size.  It is important to pick a good number for hash
table sizes when dealing with open-addressing and quadratic probing -
prime numbers are frequently effective, but powers of two are very poor
choices.
*/

//@ManMemo: Create a hash object with default hash table size
  HASH();
/*@Doc: This constructor creates a hash object with a table size of 997
entries.
*/
		
//@ManMemo: Insert an item into the hash table
  INT Insert(Item_type item) const;
/*@Doc: The item is a Item_type object with the {\bf Key} and {\bf
Block} variables set.
*/

//@ManMemo: Find an item based on {\bf Key} in the hash table
  Item_type *Find(Key_type r) const;
/*@Doc: This method returns either the item matching {\bf Key} or
NULL on failure.*/

//@ManMemo: Check on an item based on {\bf Key} in the hash table
  INT Check(Key_type r) const;
/*@Doc: This method returns 1 if there is an item matching {\bf Key} or
0 if not.
*/

  void GetValue(const STRING& a, STRING *b) const;
  void AddEntry(const STRING& a) const;

//@ManMemo: Basic Hash object destructor.
  ~HASH();

/*@Doc: This basic destructor destroys the hash object, and also {\bf
deletes all data referenced by Table->Block!!}
*/
private:
  Item_type *H; // 997 is a nice prime number
  INT TableSize;
  INT HashFunction(Key_type item) const;
  void Setup(INT size);
	

};

typedef HASH *PHASH;
#endif
