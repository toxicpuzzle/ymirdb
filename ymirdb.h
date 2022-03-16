#ifndef YMIRDB_H
#define YMIRDB_H

#define MAX_KEY 16
#define MAX_LINE 1024

#include <stddef.h>
#include <sys/types.h>

enum item_type {
    INTEGER=0,
    ENTRY=1
};

typedef struct element element;
typedef struct entry entry;
typedef struct snapshot snapshot;

struct element {
  enum item_type type;
  union {
    int value;
    struct entry *entry;
  };
};

//! Additional attributes to help with copying/handling forward/backward references.
// entry* copy_reference - if is_visiting, recursive function will point to copy_reference //! how can you guaratee that values array for entries are in order though?
// char has_visited - Used to check if node should be copied, reference to current entry stored in array of visited nodes, and all nodes visited will have is_visiting attribute and copy_reference cleared.
// For copying - Run through every element in the list, if !has_visited, create pointer to copy of entry, set copy_reference to equal that pointer, set has_visited true, and copy values over to copy of entry, recursively call copy on every forward node. Once full loop finishes, reset has_visited to false for all nodes, and clear copy_reference.
// To get the entries in order - run through every element in the original list, store the copy_reference, go the next element in original list, attach the current Cr to the previous Cr, and repeat until done. return first copy_reference as start to copy of current tree.



struct entry {
  char key[MAX_KEY];
  char is_simple; //? What is thiis? is this a boolean to demonstrate if the entry is simple?
  element * values; //? Stores the values that are associated with this key
  size_t length; //? Stores the number of values associated with the key
  entry* copy_reference;
  char has_visited;
  int sum;
  int max;
  int min; 

  entry* next; //? Points to the next entry or to do with the history of the entry?
  entry* prev; // Points to the previous entry
  
  size_t forward_size; // Number of forward links
//   size_t forward_max; //? What is this? Is this the max element out of all elements in the forward links?
  entry** forward;  // this entry depends on these
    
  size_t backward_size; // Number of backward links
//   size_t backward_max;  //? Min max element out of all elements in backward links? or just current max size (constant number of forward back links?) -> Resize when backward_size reaches this num? -> don't have to realloc too frequently
  entry** backward; // these entries depend on this //TODO: How to avoid O(n^2) time when deleting general entries?
};

//? Maybe add a length variable to keep track of how many entries snapshot contains -> Easier deleting
struct snapshot {
  int id;
  entry* entries;
  snapshot* next;
  snapshot* prev;
};


const char* HELP =
	"BYE   clear database and exit\n"
	"HELP  display this help message\n"
	"\n"
	"LIST KEYS       displays all keys in current state\n"
	"LIST ENTRIES    displays all entries in current state\n"
	"LIST SNAPSHOTS  displays all snapshots in the database\n"
	"\n"
	"GET <key>    displays entry values\n"
	"DEL <key>    deletes entry from current state\n"
	"PURGE <key>  deletes entry from current state and snapshots\n"
	"\n"
	"SET <key> <value ...>     sets entry values\n"
	"PUSH <key> <value ...>    pushes values to the front\n"
	"APPEND <key> <value ...>  appends values to the back\n"
	"\n"
	"PICK <key> <index>   displays value at index\n"
	"PLUCK <key> <index>  displays and removes value at index\n"
	"POP <key>            displays and removes the front value\n"
	"\n"
	"DROP <id>      deletes snapshot\n"
	"ROLLBACK <id>  restores to snapshot and deletes newer snapshots\n"
	"CHECKOUT <id>  replaces current state with a copy of snapshot\n"
	"SNAPSHOT       saves the current state as a snapshot\n"
	"\n"
	"MIN <key>  displays minimum value\n"
	"MAX <key>  displays maximum value\n"
	"SUM <key>  displays sum of values\n"
	"LEN <key>  displays number of values\n"
	"\n"
	"REV <key>   reverses order of values (simple entry only)\n"
	"UNIQ <key>  removes repeated adjacent values (simple entry only)\n"
	"SORT <key>  sorts values in ascending order (simple entry only)\n"
	"\n"
	"FORWARD <key> lists all the forward references of this key\n"
	"BACKWARD <key> lists all the backward references of this key\n"
	"TYPE <key> displays if the entry of this key is simple or general\n";

#endif
