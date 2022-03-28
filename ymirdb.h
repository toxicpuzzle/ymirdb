#ifndef YMIRDB_H
#define YMIRDB_H

#define MAX_KEY 16
#define MAX_LINE 1024

#include <stddef.h>
#include <sys/types.h>

typedef struct element element;
typedef struct entry entry;
typedef struct snapshot snapshot;
typedef enum item_type item_type;

// Data structures for database
enum item_type {
    INTEGER=0,
    ENTRY=1
};

struct element {
  enum item_type type;
  union {
    int value;
    struct entry *entry;
  };
};

struct entry {
  char key[MAX_KEY];
  char is_simple; 
  element * values; 
  size_t length; 

  // Cached for forward, backward, copy, sum, len, max commands
  entry* copy_reference; // Stores ptr to copy of current entry
  char has_visited; // If entry has been computed/visited by algorithm
  int sum_forward; // Stores the sum of all forward entries 
  int len_forward; // Stores the len of all forward entries
  int sum; // Local sum, updated with modifications to values array
  int max; // Local max, updated with modifications to values array
  int min; // Local min, updated with modifications to values array

  entry* next; // Points to next entry
  entry* prev; // Points to the previous entry
  
  size_t forward_size; // Number of forward links
//   size_t forward_max; //? What is this? Is this the max element out of all elements in the forward links?
  entry** forward;  // this entry depends on these
    
  size_t backward_size; // Number of backward links
//   size_t backward_max;  //? Min max element out of all elements in backward links? or just current max size (constant number of forward back links?) -> Resize when backward_size reaches this num? -> don't have to realloc too frequently
  entry** backward; // these entries depend on this //TODO: How to avoid O(n^2) time when deleting general entries?
};

struct snapshot {
  int id;
  entry* entries;
  snapshot* next;
  snapshot* prev;
};

// Functions

void update_is_simple(entry* e);
void swap(void* a1, int idx1, int idx2, size_t size_each_elem);
void command_bye();
void command_help();
bool string_isnumeric(char* string);
void entry_tostring(entry* e);
entry* entry_get(char* key, entry** current_state_ptr);
void entry_connect(entry* e, entry* forward);
element* elements_create(char** args, size_t args_size, entry** current_state_ptr);
bool key_isvalid(char* key);
entry* entry_create(char** args, size_t args_size, entry** current_state_ptr);
void state_push(entry* e, entry** current_state_ptr);
bool _entry_values_change_is_valid(entry* e, size_t len, element* elements);
bool entry_append(entry* e, char** args, size_t args_size, entry** current_state_ptr);
void _reverse_array(void* array, int length, size_t size);
bool entry_push(entry* e, char** args, size_t args_size, entry** current_state_ptr);
void entry_min(entry* e);
void entry_max(entry* e);
int _calculate_sum(entry* e);
void entry_sum(entry* e);
int _calculate_len(entry* e);
int entry_len(entry* e);
void entry_free(entry* e);
entry** _entries_remove(entry** entries, size_t* entries_len, entry* rm);
entry** _entries_replace(entry** entries, size_t* entries_len, entry* target, entry* replacement);
element* _elements_replace(element* elements, size_t* elements_len, entry* target, entry* replacement);
void entry_set(entry* e, entry** current_state_ptr);
void entry_reverse(entry* e);
entry** _entries_append(entry** list, entry* e, int* list_size);
entry** _get_forward_links(entry* e, int* size);
entry** get_forward_links(entry* e, int* size);
entry** _get_backward_links(entry* e, int* size);
entry** get_backward_links(entry* e, int* size);
int entry_keycomp(const void* e1, const void* e2);
void entry_forward(entry* e);
void entry_backward(entry* e);
void entry_type(entry* e);
void _rm_forward_links_to(entry* e);
bool entry_candel(entry* e);
void entry_delete(entry* e, entry** current_state_ptr);
int element_compare(const void* e1, const void* e2);
void entry_sort(entry* e);
void entry_unique(entry* e);
void entry_pick(entry* e, int index);
void entry_recalcsmm(entry* e);
void entry_pluck(entry* e, int index);
void entry_pop(entry* e);
void list_keys(entry** current_state_ptr);
void list_entries(entry** current_state_ptr);
void list_snapshots(snapshot** latest_snap_ptr);
snapshot* snapshot_get(int id, snapshot** latest_snap_ptr);
entry* _entry_copy(entry* e);
void snapshot_append(snapshot* snap, snapshot** latest_snap_ptr);
snapshot* snapshot_create(entry* entries, int id);
void snapshot_free(entry* entries);
void snapshot_drop(snapshot* snap, snapshot** latest_snap_ptr);
void program_clear(entry** current_state_ptr, snapshot** latest_snap_ptr);
void snapshot_rollback(snapshot* snap, entry** current_state_ptr, snapshot** latest_snap_ptr);
void snapshot_checkout(snapshot* snap, entry** current_state_ptr);
snapshot* snapshot_save(int id, entry** current_state_ptr, snapshot** latest_snap_ptr);
bool can_purge(char* key, entry** current_state_ptr, snapshot** latest_snap_ptr);
void purge(char* key, entry** current_state_ptr, snapshot** latest_snap_ptr);

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
