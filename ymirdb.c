/**
 * comp2017 - assignment 2
 * Tim Yang
 * yyan0195
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <ctype.h>
#include <limits.h>
#include "ymirdb.h"
#define MSG_NOKEY printf("no such key\n");
#define MSG_NOSNAP printf("no such snapshot\n");
#define MSG_NOPERM printf("not permitted\n");
#define MSG_OK printf("ok\n");

// Helper function to update is_simple status of entry
void update_is_simple(entry* e){
	if (e->forward_size > 0){
		e->is_simple = false;
	} else {
		e->is_simple = true;
	}
}

// Swaps two elements in idx1 and idx2 in any array
void swap(void* a1, int idx1, int idx2, size_t size_each_elem){
	void* temp = calloc(1, size_each_elem);
	memcpy(temp, a1+idx1*size_each_elem, size_each_elem); 
	memcpy(a1+idx1*size_each_elem, a1+idx2*size_each_elem, size_each_elem);
	memcpy(a1+idx2*size_each_elem, temp, size_each_elem);
	free(temp);
}

// Bye command prints bye before exiting
void command_bye() {
	printf("bye\n");
}

// Prints the help string for the help command
void command_help() {
	printf("%s", HELP);
}

// Returns if a string follows a numeric format (positive/negative int)
bool string_isnumeric(char* string){
	char* cursor = string;
	if (*cursor == '-'){
		cursor++;
	}

	while (*cursor != '\0'){
		if (!isdigit(*cursor)){
			return false;
		}
		cursor++;
	}
	return true;
}

// Prints out content of entry in a string format
void entry_tostring(entry* e){
	char* string = calloc(e->length*2, sizeof(char));
	printf("[");
	for (int i = 0; i < e->length; i++){
		element* current_element = (e->values+i);
		if (current_element->type == ENTRY){
			printf("%s", current_element->entry->key);
		} else {
			printf("%d", current_element->value);
		}
		
		if (i != e->length-1){
			printf(" ");
		}
	}
	printf("]\n");
	free(string);
}	


// Returns an entry given the key and the current state's last entry
entry* entry_get(char* key, entry** current_state_ptr){

	// Perform linear search over current database
	entry* cursor = *current_state_ptr;
	while (cursor != NULL){
		if (strcmp(key, cursor->key) == 0){
			return cursor;
		}
		cursor = cursor->next;
	}
	return NULL;
}

// Connect e to forward entry (forward -> e and e -> forward)
void entry_connect(entry* e, entry* forward){

	// Resize the memory 
	e->forward_size++;
	e->forward = realloc(e->forward, (e->forward_size)*sizeof(entry*));
	forward->backward_size++;
	forward->backward = realloc(forward->backward, 
								(forward->backward_size)*sizeof(entry*));

	// Add forward reference and backward references to the end of the list
	e->forward[e->forward_size-1] = forward;
	forward->backward[forward->backward_size-1] = e;

}

// Creates values array for entry from cmdline args and the current state
element* elements_create(char** args, size_t args_size, entry** current_state_ptr){
	element* elements = calloc(args_size, sizeof(element)); 
	for (int i = 0; i < args_size; i++){
		element* current_elem = elements + i;
		char* current_arg = args[i];
		if (string_isnumeric(current_arg)){
			current_elem->type = INTEGER;		
			current_elem->value = atoi(current_arg);
		} else {
			current_elem->type = ENTRY;
			entry* forward_link = entry_get(current_arg, current_state_ptr);
			current_elem->entry = forward_link;
		}
	}
	return elements;
}

// Returns if a key is a valid key that is alphabetical and of valid length
bool key_isvalid(char* key){
	if (!isalpha(*key) || strlen(key) > 15) return false;
	return true;
}

// Creates an entry from args, connects entry to all relevant forward links 
entry* entry_create(char** args, size_t args_size, entry** current_state_ptr){

	// Perform error checking
	if (args_size <= 1){
		printf("Cannot create entry with no key or with a key but has no values\n");
		return NULL;
	}

	if (!key_isvalid(args[0])){
		MSG_NOPERM
		return NULL;
	}

	// Initialise empty entry
	char* key = args[0];
	entry* e = (entry *)calloc(1, sizeof(entry)); 
	e->is_simple = true;
	e->has_visited = false;
	e->copy_reference = NULL;

	// Check if entry contains links to other keys & keys exist & no loop to self
	for (int i = 1; i < args_size; i++){
		char* arg = args[i];
		if (string_isnumeric(arg) == false){
			e->is_simple = false;
			entry* forward_link = entry_get(arg, current_state_ptr);
			
			if (strcmp(key, arg) == 0){
				MSG_NOPERM;
				entry_free(e);
				return NULL;
			} else if (forward_link == NULL){
				MSG_NOKEY
				entry_free(e);
				return NULL;
			}  


			// Connect elements if forward link is valid (handles adding to )
			entry_connect(e, forward_link);
		}
	}

	// Create copy of string in args for entry
	memcpy(e->key, key, strlen(key)+1);

	// Set elements for entry
	e->values = elements_create(args+1, args_size-1, current_state_ptr);
	e->length = args_size-1;

	// Set max, min, sum
	entry_recalcsmm(e);

	return e;
}

// Adds the entry to the database state (current_state)
void state_push(entry* e, entry** current_state_ptr){
	if (*current_state_ptr == NULL){
		*current_state_ptr = e;
	} else {
		// Add the entry to stack (current_state)
		entry* old_state = *current_state_ptr;
		*current_state_ptr = e;
		(*current_state_ptr)->next = old_state;
		old_state->prev = *current_state_ptr;
	}
}

// O(n) returns true if appended/pushed are valid -> prints errors.
bool _entry_values_change_is_valid(entry* e, size_t len, element* elements){
    // Check none of the elements pushed result in cycle or non-existant key
    for (int i = 0; i < len; i++){
        element* current_element = elements+i;
        if (current_element->type == ENTRY){
            entry* forward_link = current_element->entry;
            if (forward_link == NULL){
                MSG_NOKEY
                return false;
            } else if (strcmp(forward_link->key, e->key) == 0){
                MSG_NOPERM
                return false;
            }
        }
    }
    return true;
}

// Appends values to an entry's values array, returns false if failed
bool entry_append(entry* e, char** args, size_t args_size, 
entry** current_state_ptr){
	// Create array of elements to attach to the entry
	element* elements = elements_create(args, args_size, current_state_ptr);
    
    if (!_entry_values_change_is_valid(e, args_size, elements)){
        free(elements);
        return false;
    }
    
	int old_length = e->length;
	e->length = e->length + args_size;
	e->values = realloc(e->values,	(e->length)*sizeof(element)); 
	memcpy(e->values+old_length, elements, sizeof(element)*args_size);

	for (int i = 0; i < args_size; i++){
		element* current_element = elements+i;
		if (current_element->type == ENTRY){
			entry* forward_link = current_element->entry;
			entry_connect(e, forward_link);
		} 
	}	

	free(elements);
	entry_recalcsmm(e);
    update_is_simple(e);
	return true;
}

// Reverses an array
void _reverse_array(void* array, int length, size_t size){
	for (int i = 0; i < length/2; i++){
		swap(array, i, length-i-1, size);
	}
}

// Pushes values to an entry's values array, returns false if failed
bool entry_push(entry* e, char** args, size_t args_size, entry** current_state_ptr){
	// Create array of elements to attach to the entry
	element* elements = elements_create(args, args_size, current_state_ptr);

    if (!_entry_values_change_is_valid(e, args_size, elements)){
        free(elements);
        return false;
    }

	// Reverse elements insert a copy of that array to the front of e's values
    _reverse_array((void*)elements, (int)args_size, sizeof(element));
	e->length = e->length + args_size;
	e->values = realloc(e->values,	(e->length)*sizeof(element)); 
	memmove(e->values+args_size, e->values, sizeof(element)*(e->length-args_size));
	memcpy(e->values, elements, sizeof(element)*args_size);

	// Connect all of our inserted elements to e (if they are general entries)
	for (int i = 0; i < args_size; i++){
		element* current_element = elements+i;
		if (current_element->type == ENTRY){
			entry* forward_link = current_element->entry;
			entry_connect(e, forward_link);
			e->is_simple = false;
		} 
	}	

	// Remove the local elements array, recalculate e's sum, max, len, type
	free(elements); 
	entry_recalcsmm(e);
	update_is_simple(e);
    return true;
}

// Prints out the minimum value for an entry
void entry_min(entry* e){

	// Get all forward links the current entry
	int min = e->min;
	int forward_size = 0;
	entry** forwards  = get_forward_links(e, &forward_size);

	// Compare e's own min with its forward link's mins
	for (int i = 0; i < forward_size; i++){
		if (forwards[i]->min < min){
			min = forwards[i]->min;
		}
	}

	printf("%d\n", min);

	if (forward_size > 0){
		free(forwards);
	}

}

// Prints out the maximum value for an entry
void entry_max(entry* e){

	// Get all forward links the current entry
	int max = e->max;
	int forward_size = 0;
	entry** forwards  = get_forward_links(e, &forward_size);
	
	// Compare e's own max with its forward link's maxes
	for (int i = 0; i < forward_size; i++){
		if (forwards[i]->max > max){
			max = forwards[i]->max;
		}
	}
	
	printf("%d\n", max);

	if (forward_size > 0){
		free(forwards);
	}
}

// Helper function entry_sum -> calculates forward sum of e + caches sum
int _calculate_sum(entry* e){
	if (e->has_visited){
		return e->sum_forward;
	}
	
	// Compute sum from e's sum and sum of its forwards
	int sum = e->sum;
	for (int i = 0; i < e->forward_size; i++){
		sum += _calculate_sum(e->forward[i]);
	}

	// Cache result so visits to this entry does not result in recomputation
	e->sum_forward = sum;
	e->has_visited = true;
	return sum;
}


// Prints out the sum of an entry
void entry_sum(entry* e){

	// Get e's forward links so we can clear the helper function's effects
	int forward_size = 0;
	entry** forwards  = get_forward_links(e, &forward_size);
	int sum = _calculate_sum(e);
	
	// Get all of e's unique forward links and clear 	
	e->has_visited = false;
	for (int i = 0; i < forward_size; i++){
		forwards[i]->has_visited = false;
		forwards[i]->sum_forward = 0;
	}
	
	// Free memory for forwards array
	if (forward_size > 0){
		free(forwards);
	}
	
	printf("%d\n", sum);
}

// Helper function to calculate length for entries
int _calculate_len(entry* e){
	if (e->has_visited){
		return e->len_forward;
	}
	
	int len = e->length - e->forward_size;
	for (int i = 0; i < e->forward_size; i++){
		len += _calculate_len(e->forward[i]);
	}

	e->len_forward = len;
	e->has_visited = true;
	return len;
}

// Private method used by entry_len to get length of a general entry
int entry_len(entry* e){
	
	// Get forward uniques first so we can clear has_visited later
	int forward_size = 0;
	entry** forwards  = get_forward_links(e, &forward_size);
	int len = _calculate_len(e);
	
	// Get all of e's unique forward links and clear forward_visited
	e->has_visited = false;
	for (int i = 0; i < forward_size; i++){
		forwards[i]->has_visited = false;
		forwards[i]->len_forward = 0;
	}
	
	// Free memory for forwards array
	if (forward_size > 0){
		free(forwards);
	}
	
	printf("%d\n", len);
	
	return len;
}

// Frees an entry and all its associated arrays
void entry_free(entry* e){	
	// Free all values pointed to by e
	if (e->values != NULL) free(e->values);
	if (e->forward != NULL) free(e->forward);
	if (e->backward != NULL) free(e->backward);
	if (e->copy_reference != NULL) free(e->copy_reference);
	free(e);
}

// Removes entry with address rm from an array
entry** _entries_remove(entry** entries, size_t* entries_len, entry* rm){

	int idx = 0;

	// Grab index to remove
	for (; idx < *entries_len; idx++){
		if (strcmp(entries[idx]->key, rm->key) == 0){
			break;
		}
	}

	// Copy everything beyond index to index position
	if (*entries_len == 1){ 
		*entries_len = *entries_len - 1;
		free(entries); 
		entries = NULL;
	} else {
		memmove(entries+idx, entries+idx+1, 
				(*entries_len-(idx+1))*sizeof(entry*));
		*entries_len = *entries_len - 1;
		entries = realloc(entries, (*entries_len)*sizeof(entry*));
	}

	return entries;
}

// Helper function that replaces a target in entries array with replacement
entry** _entries_replace(entry** entries, size_t* entries_len, entry* target, 
entry* replacement){
	
	// Grab index to remove
	int idx = 0;
	for (; idx < *entries_len; idx++){
		if (strcmp(entries[idx]->key, target->key) == 0){
			break;
		}
	}

	entries[idx] = replacement;
	return entries;
}

// Helper function that replaces target in elements array with replacement
element* _elements_replace(element* elements, size_t* elements_len, 
entry* target, entry* replacement){
	// Grab index to remove
	int idx = 0;
	for (; idx < *elements_len; idx++){
		if (elements[idx].type == ENTRY && strcmp(elements[idx].entry->key, 
			target->key) == 0){
			break;
		}
	}

	elements[idx].entry = replacement;
	return elements;
}


// makes a new entry take the place of an existing entry
void entry_set(entry* e, entry** current_state_ptr){

	// Search through current state and see if the entry with key is tehre
	entry* existing = entry_get(e->key, current_state_ptr);
	if (existing == NULL){
		state_push(e, current_state_ptr); 
	} else {

		_rm_forward_links_to(existing);

		// Make all existing back entries point forward to new replacement entry
		for (int i = 0; i < existing->backward_size; i++){
			entry* backward = existing->backward[i];
			backward->forward = _entries_replace(backward->forward, 
									&backward->forward_size, existing, e);
			backward->values = _elements_replace(backward->values, 
									&backward->length, existing, e);
		}

		// Make e take position of existing state
		entry* before = existing->prev;
		entry* after = existing->next;

		e->next = after;
		e->prev = before;

		if (before != NULL){
			before->next = e;
		} else {
			*current_state_ptr = e;
		}

		if (after != NULL){
			after->prev = e;
		}
		
		// Make new state have back references of previous state
		entry** backward_copy = calloc(existing->backward_size, sizeof(entry*));
		memcpy(backward_copy, existing->backward, 
				existing->backward_size*(sizeof(entry*)));
		e->backward = backward_copy;
		e->backward_size = existing->backward_size;

		entry_free(existing);
	}
}

// Reverses and entry
void entry_reverse(entry* e){
	if (e->is_simple == false){
		printf("Cannot reverse an entry that is not simple!\n");
		return;
	}

	for (int i = 0; i < e->length/2; i++){
		swap(e->values, i, e->length-1-i, sizeof(element));
	}
}

// Appends entry to the entry array 
entry** _entries_append(entry** list, entry* e, int* list_size){
	*list_size = *list_size + 1;
	if (*list_size == 1){
		list = calloc(1, sizeof(entry*));
	} else {
		list = realloc(list, (*list_size)*sizeof(entry*));
	}
	list[*list_size-1] = e;
	return list;
}

// Helper function: Visits all forward links only once to get all forwards in list
entry** _get_forward_links(entry* e, int* size){
	entry** forwards = NULL;
	entry** next_forwards = NULL;
	int next_size = 0;

	// If the current entry has been visited/added to list, don't add it again
	e->has_visited = true;

	// Add all forwards list 
	for (int i = 0; i < e->forward_size; i++){
		entry* forward_link = e->forward[i];
		
		// If the current entry has been visited/added to list, don't add again
		if (forward_link->has_visited == true) continue;	

		// Resize forwards and add link to end of list
		forwards = _entries_append(forwards, forward_link, size);	
	
		// DFS forward - Get array containing recursive 
		next_size = 0; 
		next_forwards = _get_forward_links(forward_link, &next_size);
	
		// Attach fowards from recursive call to end of current list		
		if (next_size > 0){
			*size += next_size;
			forwards = realloc(forwards, (*size)*sizeof(entry*));		
			memcpy(forwards+*size-next_size, next_forwards, 
					next_size*sizeof(entry*));
			free(next_forwards);
		}
	}
	
	return forwards;
}

// Returns array containing all forward_entries of an element e.
entry** get_forward_links(entry* e, int* size){
	entry** forwards = _get_forward_links(e, size);
	e->has_visited = false;
	for (int i = 0; i < *size; i++){
		forwards[i]->has_visited = false;
	}	
	return forwards;
}

// Helper function: Visits all back links only once to get all back links in list
entry** _get_backward_links(entry* e, int* size){
	entry** backwards = NULL;
	entry** next_backwards = NULL;
	int next_size = 0;

	// If the current entry has been visited/added to list, don't add it again
	e->has_visited = true;

	// Add all forwards list 
	for (int i = 0; i < e->backward_size; i++){
		entry* backward_link = e->backward[i];
		
		// If the current entry has been visited/added to list, don't add it again
		if (backward_link->has_visited == true) continue;	

		// Resize forwards and add link to end of list
		backwards = _entries_append(backwards, backward_link, size);	
	
		// DFS forward - Get array containing recursive 
		next_size = 0; 
		next_backwards = _get_backward_links(backward_link, &next_size);
	
		// Attach fowards from recursive call to end of current list		
		if (next_size > 0){
			*size += next_size;
			backwards = realloc(backwards, (*size)*sizeof(entry*));		
			memcpy(backwards+*size-next_size, next_backwards, 
					next_size*sizeof(entry*));
			free(next_backwards);
		}
	}
	
	return backwards;
}

// Returns all backward links (recursive) for e, clears all cache variables.
entry** get_backward_links(entry* e, int* size){
	entry** backwards = _get_backward_links(e, size);
	e->has_visited = false;
	for (int i = 0; i < *size; i++){
		backwards[i]->has_visited = false;
	}
	return backwards;
}

// Comparator used by sorting algorithm to sort entry order for forwards/bacl
int entry_keycomp(const void* e1, const void* e2){
	entry** entry_1 = (entry**) e1;
	entry** entry_2 = (entry**) e2; 
	int result = strcmp(entry_1[0]->key, entry_2[0]->key);
	return result;
}

// Prints out all forward entries that e has (recursive search)
void entry_forward(entry* e){
	int size = 0;
	entry** forward_entries = get_forward_links(e, &size);	
	qsort(forward_entries, size, sizeof(entry*), &entry_keycomp);

	if (size == 0){
		printf("nil\n");
		return;
	} else {
		for (int i = 0; i < size-1; i++){
			printf("%s, ", forward_entries[i]->key);
		}
		printf("%s\n", forward_entries[size-1]->key);
		free(forward_entries);
	} 	
}

// Prints out all backward entries that e has (recursive search)
void entry_backward(entry* e){

	// Retrieve backward all of e's backward entries using helper function
	int size = 0;
	entry** backward_entries = get_backward_links(e, &size);	
	
	// Sort to lexicographical order;
	qsort(backward_entries, size, sizeof(entry*), entry_keycomp);

	// Print output
	if (size == 0){
		printf("nil\n");
		return;
	} else {
		for (int i = 0; i < size-1; i++){
			printf("%s, ", backward_entries[i]->key);
		}
		printf("%s\n", backward_entries[size-1]->key);
		free(backward_entries);
	}
}

// Prints out whether an entry is simple or general
void entry_type(entry* e){
	if (e->is_simple){
		printf("simple\n");
	} else {
		printf("general\n");
	}
}

// Remove forward entries' back links to the current entry in worst (O(n^2))
void _rm_forward_links_to(entry* e){
	// Remove each back link to e every for every forward link e has
	for (int i = 0; i < e->forward_size; i++){
		entry* forward_link = e->forward[i];
		forward_link->backward = _entries_remove(forward_link->backward, 
										&forward_link->backward_size, e);
	}
}

// Returns true if an entry is allowed to be deleted
bool entry_candel(entry* e){
	return e->backward_size == 0;
}

// Deletes an entry from the current state (not snapshots)
void entry_delete(entry* e, entry** current_state_ptr){

	if (!entry_candel(e)){
		return;
	} else {
		_rm_forward_links_to(e);

		entry* before = e->prev;
		entry* after = e->next;

		// Reset before and after links
		if (before != NULL){
			before->next = after;
		}
		if (after != NULL){
			after->prev = before;
		}

		// Reset current state
		if ((*current_state_ptr)->key == e->key){
			*current_state_ptr = e->next;
		}	

		entry_free(e);
	}
}

// Elements are the same if their values are the same (only for simple entries)
int element_compare(const void* e1, const void* e2){
	element* element_1 = (element*) e1;
	element* element_2 = (element*) e2;
	int result = element_1->value - element_2->value; 
	return result;
}

// Use C library's sorting algos
void entry_sort(entry* e){
	qsort(e->values, e->length, sizeof(element), &element_compare); 
}

// Create a new array and iterate through array adding adjacently unique entries
void entry_unique(entry* e){
	if (e->is_simple == false){
		printf("Cannot make unique an entry that is not simple!\n");
	}

	element* new_values = calloc(e->length, sizeof(element));
	element* last_word = NULL; 
	element* cursor = e->values;

	int new_size = 0;
	for (int i = 0; i < e->length; i++){
		if (last_word == NULL || element_compare(last_word, cursor) != 0){
			memcpy(new_values+new_size, cursor, sizeof(element));
			new_size++;
		}

		last_word = cursor;
		cursor++;
	}

	// Shrink values array to required size for new_values
	new_values = realloc(new_values, new_size*sizeof(element));
	free(e->values); 
	e->length = new_size;
	e->values = new_values;
}

// Print out a value at a specified index within an entry
void entry_pick(entry* e, int index){	
	
	if (index < 0 || index >= e->length){
		printf("index out of range\n");
		return;
	}

	element* elem = e->values+index;

	if (elem->type == INTEGER){
		printf("%d\n", elem->value);
	} else {
		printf("%s\n", elem->entry->key);
	}
}

// Extra O(n) operation to calculate local min, max, and sum.
void entry_recalcsmm(entry* e){
	int min = INT_MAX;
	int max = INT_MIN;
	int sum = 0;

	// Search through entries to find new min new max
	for (int i = 0; i < e->length; i++){
		element* current_element = e->values+i;
		if (current_element->type != ENTRY){

			// Update sum, min, max
			if (current_element->value < min){
				min = current_element->value;
			}
			if (current_element->value > max){
				max = current_element->value;
			}
			sum += current_element->value;	
		}
	}	
	e->min = min;
	e->max = max;
	e->sum = sum;

}

// Removes value at index and updates back/forward links if ENTRY is removed
void entry_pluck(entry* e, int index){

	if (index < 0 || index >= e->length){
		printf("index out of range\n");
		return;
	}	

	entry_pick(e, index);
	element* elem_to_remove = e->values+index;
	item_type type = elem_to_remove->type;

	// Remove backlinks to e for entries that link back to e 
	if (type == ENTRY){
		entry* forward = elem_to_remove->entry;
		// remove back link
		forward->backward =_entries_remove(forward->backward,
											&forward->backward_size, e);
		// remove forwad link	 
		e->forward = _entries_remove(e->forward, &e->forward_size, 
									elem_to_remove->entry); 
	}
	
	// Shrink the values array and minimise memory use
	e->length--;
	memmove(elem_to_remove, elem_to_remove+1, (e->length-index)*sizeof(element)); 
	e->values = realloc(e->values, e->length*sizeof(element));
	if (type == INTEGER){
		entry_recalcsmm(e);
	}

	update_is_simple(e);
}


// Pluck the first index of an entry for pop command if e is non-empty
void entry_pop(entry* e){

	// Check we have non-empty entry before popping
	if (e->length == 0){
		printf("nil\n");
		return;
	}
	entry_pluck(e, 0);
}

// List all keys for the current state by iterating through all keys
void list_keys(entry** current_state_ptr){
	entry* cursor = *current_state_ptr;

	if (cursor == NULL){
		printf("no keys\n");
		return;
	}

	while (cursor != NULL){
		printf("%s\n", cursor->key);
		cursor = cursor->next;
	}
}

// List all keys for current state by iterating all entries and their keys
void list_entries(entry** current_state_ptr){
	entry* cursor = *current_state_ptr;

	if (cursor == NULL){
		printf("no entries\n");
		return;
	}

	while (cursor != NULL){
		printf("%s ", cursor->key);
		entry_tostring(cursor); 
		cursor = cursor->next;
	}

}

// List all snapshots in the current data base if there are snapshots
void list_snapshots(snapshot** latest_snap_ptr){
	snapshot* cursor = *latest_snap_ptr;

	if (cursor == NULL){
		printf("no snapshots\n");
		return;
	}

	while (cursor != NULL){
		printf("%d\n", cursor->id);
		cursor = cursor->prev;
	}
}

// Gets a snapshot by its id in the current database
snapshot* snapshot_get(int id, snapshot** latest_snap_ptr){
	snapshot* cursor = *latest_snap_ptr;
	while (cursor != NULL){
		if (id == cursor->id){
			return cursor;
		}
		cursor = cursor->prev;
	}
	return NULL;
}

// Sets the correct next and prev pointers for the snapshot to be appended.
void snapshot_append(snapshot* snap, snapshot** latest_snap_ptr){
	if (*latest_snap_ptr == NULL){
		*latest_snap_ptr = snap;
	} else {
		(*latest_snap_ptr)->next = snap;
		snap->prev = (*latest_snap_ptr);
		(*latest_snap_ptr) = snap;
	}
}

// Creates copy for new snapshot
entry* entry_copy_local_values(entry* e){
	entry* copy = calloc(1, sizeof(entry));	
	memcpy(copy, e, sizeof(entry));
	
	// Copy old backward array
	entry** old_backward = copy->backward;
	copy->backward = calloc(copy->backward_size, sizeof(entry*));
	memcpy(copy->backward, old_backward, copy->backward_size*sizeof(entry*));

	// Copy old forward array
	entry** old_forward = copy->forward;
	copy->forward = calloc(copy->forward_size, sizeof(entry*));
	memcpy(copy->forward, old_forward, copy->forward_size*sizeof(entry*));

	// Copy old values array
	element* old_values = copy->values;
	copy->values = calloc(copy->length, sizeof(element));
	memcpy(copy->values, old_values, copy->length*sizeof(element));

	return copy;
}

// Create copy of entries array with forward and backward links
snapshot* snapshot_create(entry* entries, int id){
	entry* cursor = entries;
	entry* entries_copy = NULL;
	entry* previous = NULL;

	// First pass to create copy of values
	while (cursor != NULL){
		// Make copy of local values
		entry* copy = entry_copy_local_values(cursor);
		cursor->copy_reference = copy;

		// Link entry to previous entry in chain
		if (previous != NULL){
			previous->next = copy;
			copy->prev = previous;
		} 
	
		// Ensure entries copy points to first copied entry;
		if (entries_copy == NULL){
			entries_copy = copy;
		}

		previous = copy;
		cursor = cursor->next;
	}

	// Second pass to create forward and backward links + values array
	cursor = entries_copy;
	while (cursor != NULL){
		
		// Copy values array
		for (int i = 0; i < cursor->length; i++){
			element value = cursor->values[i];
			if (value.type == ENTRY){
				cursor->values[i].entry = value.entry->copy_reference;
			}
		}

		// Copy forwards array
		for (int i = 0; i < cursor->forward_size; i++){
			entry* fwd = cursor->forward[i];
			cursor->forward[i] = fwd->copy_reference;
		}

		// Copy backwards array
		for (int i = 0; i < cursor->backward_size; i++){
			entry* bwd = cursor->backward[i];
			cursor->backward[i] = bwd->copy_reference;
		}

		cursor = cursor->next;
	}

	// Set all elements' copy_reference to null after creating snapshot
	cursor = entries;
	while (cursor != NULL){
		cursor->copy_reference = NULL;
		cursor->has_visited = false;
		cursor = cursor->next;
	}

	snapshot* new_snapshot = calloc(1, sizeof(snapshot));

	new_snapshot->entries = entries_copy;
	new_snapshot->id = id;
	return new_snapshot;
}

// Free up all the entries in the snapshot
void snapshot_free(entry* entries){
	
	entry* cursor = entries;
	entry* old = NULL;
	while (cursor != NULL){
		old = cursor;
		cursor = cursor->next; 
		entry_free(old);
	}
}

// Drops a snapshot and updates the ptr to the latest_snapshot
void snapshot_drop(snapshot* snap, snapshot** latest_snap_ptr){
	snapshot* before = snap->prev;
	snapshot* after = snap->next;

	// Free up all the entries in the snapshot
	snapshot_free(snap->entries);

	// Get previous snapshot to point to next snapshot and vice versa
	if (after != NULL){
		after->prev = before;
	}

	if (before != NULL){
		before->next = after;
	}
	
	if (snap->id == (*latest_snap_ptr)->id){
		*latest_snap_ptr = before;
	} 

	free(snap);
}

// Frees memory from all snapshots and the current state
void program_clear(entry** current_state_ptr, snapshot** latest_snap_ptr){
	snapshot* cursor = *latest_snap_ptr; 
	snapshot* old;

	snapshot_free(*current_state_ptr); 

	while (cursor != NULL){
		old = cursor;
		cursor = cursor->prev;
		snapshot_drop(old, latest_snap_ptr);
	}
}


// Frees current state and makes a copy of the specified snap into current_state
void snapshot_rollback(snapshot* snap, entry** current_state_ptr, 
snapshot** latest_snap_ptr){
	snapshot* cursor = *latest_snap_ptr; 
	snapshot* old;

	snapshot_free(*current_state_ptr);

	// Got the snapshot we want (deleting snapshots along the way)
	while (cursor != NULL){
		if (cursor == snap){
			break;
		}
		old = cursor;
		cursor = cursor->prev;
		snapshot_drop(old, latest_snap_ptr);
	}

	// Create copy of snapshot we want to rollback to
	snapshot* snap_copy = snapshot_create(cursor->entries, 0); 
	*current_state_ptr = snap_copy->entries;
	free(snap_copy);
}


// Create a copy of snap and set the current state that snap
void snapshot_checkout(snapshot* snap, entry** current_state_ptr){
	// Free current state before pointing it to snap copy
	snapshot_free(*current_state_ptr);
	snapshot* snap_copy = snapshot_create(snap->entries, 0);
	*current_state_ptr = snap_copy->entries;
	free(snap_copy); //? don't need id for copy of snapshot
}

// Copy snapshot based on current lifetime id and current state.
snapshot* snapshot_save(int id, entry** current_state_ptr, 
snapshot** latest_snap_ptr){
	snapshot* new_snapshot = snapshot_create(*current_state_ptr, id); 
	snapshot_append(new_snapshot, latest_snap_ptr);
	return new_snapshot;
}

void function(char* long_word_here, char* another_long_word, 
entry* an_important_argument){

}


// Returns false if a key with back entries is found in snapshots or current state
bool can_purge(char* key, entry** current_state_ptr, 
snapshot** latest_snap_ptr){
	entry* original_state = *current_state_ptr;
	entry* to_delete;
	snapshot* snap = *latest_snap_ptr;
	
	// Check key can be deleted in all snapshot entries
	while (snap != NULL){
		*current_state_ptr = snap->entries;
		to_delete = entry_get(key, current_state_ptr);
		if (to_delete != NULL && !entry_candel(to_delete)){
			*current_state_ptr = original_state;
			return false;
		}
		snap->entries = *current_state_ptr; 
		snap = snap->prev;
	}

	// Check key can be deleted in teh current state
	*current_state_ptr = original_state;
	to_delete = entry_get(key, current_state_ptr);
	if (to_delete != NULL && !entry_candel(to_delete)){
		return false;
	}

	return true;
}

// Remove entry with matching key from current_state and all snapshots
void purge(char* key, entry** current_state_ptr, 
snapshot** latest_snap_ptr){

	if (!can_purge(key, current_state_ptr, latest_snap_ptr)){
		printf("not permitted\n");
		return;
	}
	
	entry* original_state = *current_state_ptr;
	entry* to_delete = NULL;
	
	// Find key in snapshots and delete
	snapshot* snap = *latest_snap_ptr;
	while (snap != NULL){
		*current_state_ptr = snap->entries;
		to_delete = entry_get(key, current_state_ptr);

		if (to_delete != NULL){
			entry_delete(to_delete, current_state_ptr);
		}

		snap->entries = *current_state_ptr; 
		snap = snap->prev;
	}

	// Restore original state after purging
	// Find key in current database and delete
	*current_state_ptr = original_state;
	to_delete = entry_get(key, current_state_ptr);
	if (to_delete != NULL){
		entry_delete(to_delete, current_state_ptr);
	} 

}

// Main function handles command parsing.
int main(void) {

	char line[MAX_LINE];	
	int next_snap_id = 1;

	entry* current_state = NULL; 
	snapshot* latest_snapshot = NULL;

	while (true) {
		printf("> ");
	
		if (NULL == fgets(line, MAX_LINE, stdin)) {
			printf("\n");
			program_clear(&current_state, &latest_snapshot);
			command_bye();
			return 0;
		}

		char* word = strtok(line, " \n\r"); 
		char** args = calloc(MAX_LINE, sizeof(char*));
		size_t args_size = 0;
		while (word != NULL) {
			args[args_size] = word;
			args_size++;
			word = strtok(NULL, " \n\r"); 
		}
	
		char* command_type = args[0];
		if (command_type == NULL){
			free(args);
			continue;
		}
		
		if (strcasecmp(command_type, "SET") == 0){
			entry* e = entry_create(args+1, args_size-1, &current_state); 
			if (e != NULL){
				entry_set(e, &current_state);
				MSG_OK
			}
		} else if (strcasecmp(command_type, "PUSH") == 0){
			entry* e = entry_get(args[1], &current_state);
			if (e == NULL) {
				MSG_NOKEY
			} else {
				if (entry_push(e, args+2, args_size-2, &current_state)){
    				MSG_OK
                } 
			}
		} else if (strcasecmp(command_type, "APPEND") == 0){
			entry* e = entry_get(args[1], &current_state); 
			if (e == NULL){
				MSG_NOKEY
			} else {
                if (entry_append(e, args+2, args_size-2, &current_state)){
    				MSG_OK
                }
			}
		} else if (strcasecmp(command_type, "GET") == 0){
			entry* e = entry_get(args[1], &current_state);
			if (e == NULL) {
				MSG_NOKEY
			} else {
				entry_tostring(e);
			}
		} else if (strcasecmp(command_type, "DEL") == 0){
			entry* e = entry_get(args[1], &current_state);
			if (e == NULL){
				MSG_NOKEY
			} else if (!entry_candel(e)){
				MSG_NOPERM
			} else {
				entry_delete(e, &current_state);
				MSG_OK
			}
			// fwrapper_entry(e, &entry_delete);
		} else if (strcasecmp(command_type, "MIN") == 0){
			entry* e = entry_get(args[1], &current_state);
			if (e == NULL) {
				MSG_NOKEY
			} else {
				entry_min(e);
			}			
		} else if (strcasecmp(command_type, "MAX") == 0){
			entry* e = entry_get(args[1], &current_state);
			if (e == NULL) {
				MSG_NOKEY
			} else {
				entry_max(e);
			}	
		} else if (strcasecmp(command_type, "SUM") == 0){
			entry* e = entry_get(args[1], &current_state); 
			if (e == NULL){
				MSG_NOKEY
			} else {
				entry_sum(e);
			}
		} else if (strcasecmp(command_type, "LEN") == 0){
			entry* e = entry_get(args[1], &current_state);
			if (e == NULL) {
				MSG_NOKEY
			} else {
				entry_len(e);
			}
		} else if (strcasecmp(command_type, "SORT") == 0){
			entry* e = entry_get(args[1], &current_state);
            if (e == NULL){
                MSG_NOKEY
            } else{
                if (e->is_simple == false){
			    	printf("simple entry only\n");
			    } else {
                    entry_sort(e);
                    MSG_OK
                }  
            }
		} else if (strcasecmp(command_type, "REV") == 0){
			entry* e = entry_get(args[1], &current_state);
			if (e == NULL){
                MSG_NOKEY
            } else{
                if (e->is_simple == false){
			    	printf("simple entry only\n");
			    } else {
                    entry_reverse(e);
                    MSG_OK
                }  
            }
		} else if (strcasecmp(command_type, "UNIQ") == 0){
			entry* e = entry_get(args[1], &current_state); 
			if (e == NULL){
                MSG_NOKEY
            } else{
                if (e->is_simple == false){
			    	printf("simple entry only\n");
			    } else {
                    entry_unique(e);
                    MSG_OK
                }  
            }
		} else if (strcasecmp(command_type, "PLUCK") == 0){
			entry* e = entry_get(args[1], &current_state);
			if (e == NULL) {
				MSG_NOKEY
			} else {
				if (!string_isnumeric(args[2])){
					printf("Cannot pluck an index that is not numeric!\n");
				} else {
					int index = atoi(args[2])-1;
					entry_pluck(e, index);	
				}	
			}
		} else if (strcasecmp(command_type, "PICK") == 0){
			entry* e = entry_get(args[1], &current_state);
			if (e == NULL) {
				MSG_NOKEY
			} else {
				if (!string_isnumeric(args[2])){
					printf("Cannot pick an index that is not numeric!\n");
				} else {
					int index = atoi(args[2])-1;
					entry_pick(e, index);	
				}
			}
		} else if (strcasecmp(command_type, "POP") == 0){
			entry* e = entry_get(args[1], &current_state);
			if (e == NULL){
				MSG_NOKEY
			} else {
				entry_pop(e);
			}
		} else if (strcasecmp(command_type, "FORWARD") == 0){
			entry* e = entry_get(args[1], &current_state);
			if (e == NULL){	
				MSG_NOKEY  
			} else {
				entry_forward(e);
			}
		} else if (strcasecmp(command_type, "BACKWARD") == 0){
			entry* e = entry_get(args[1], &current_state);
			if (e == NULL){	
				MSG_NOKEY  
			} else {
				entry_backward(e);
			}
		} else if (strcasecmp(command_type, "PURGE") == 0){
			char* key = args[1];
			if (!can_purge(key, &current_state, &latest_snapshot)){
				MSG_NOPERM
			} else {
				purge(key, &current_state, &latest_snapshot);
				MSG_OK
			}
		}  else if (strcasecmp(command_type, "TYPE") == 0){
			entry* e = entry_get(args[1], &current_state);
			if (e == NULL) {
				MSG_NOKEY 
			} else {
				entry_type(e);
			}
		} else if (strcasecmp(command_type, "LIST") == 0){
			char* option = args[1];
			if (strcasecmp(option, "KEYS") == 0){
				list_keys(&current_state);
			} else if (strcasecmp(option, "ENTRIES") == 0){
				list_entries(&current_state);
			} else if (strcasecmp(option, "SNAPSHOTS") == 0){
				list_snapshots(&latest_snapshot);
			} 
		} else if (strcasecmp(command_type, "SNAPSHOT") == 0){
		 	snapshot* snap = snapshot_save(next_snap_id++, &current_state, 
			 								&latest_snapshot);
			printf("saved as snapshot %d\n", snap->id);
		} else if (strcasecmp(command_type, "DROP") == 0){ 
			if (!string_isnumeric(args[1])){
				printf("You must provide a valid ID for a snapshot!\n");
			} else {
				int id = atoi(args[1]);
				snapshot* snap = snapshot_get(id, &latest_snapshot);
				if (snap == NULL){
					MSG_NOSNAP
				} else {
					snapshot_drop(snap, &latest_snapshot);
					MSG_OK
				}
			}
		} else if (strcasecmp(command_type, "ROLLBACK") == 0){ 
			if (!string_isnumeric(args[1])){
				printf("You must provide a valid ID for a snapshot!\n");
			} else {
				int id = atoi(args[1]);
				snapshot* snap = snapshot_get(id, &latest_snapshot);
				if (snap == NULL){
					MSG_NOSNAP
				} else {
					snapshot_rollback(snap, &current_state, &latest_snapshot);
					MSG_OK
				}
			}
		} else if (strcasecmp(command_type, "CHECKOUT") == 0){
			if (!string_isnumeric(args[1])){
				printf("You must provide a valid ID for a snapshot!\n");
			} else {
				int id = atoi(args[1]);
				snapshot* snap = snapshot_get(id, &latest_snapshot);
				if (snap == NULL){
					MSG_NOSNAP
				} else {
					snapshot_checkout(snap, &current_state);
					MSG_OK
				}
			}
		} else if (strcasecmp(command_type, "HELP") == 0){
		 	command_help();
		} else if (strcasecmp(command_type, "BYE") == 0){
			command_bye();
			program_clear(&current_state, &latest_snapshot); 
			free(args);
			return 0;
		}

		printf("\n");
		free(args);
  	}

	return 0;
}
