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
# define TEST 0
#define PRINT_COMMAND 0

entry* current_state;
snapshot* first_snapshot;
snapshot* last_snapshot;

//
// We recommend that you design your program to be
// modular where each function performs a small task
//
// e.g.
//
// command_bye
// command_help
// ...
// entry_add
// entry_delete
// ...
//

entry** get_forward_links(entry* e, int* size);
void entry_recalcsmm(entry* e);

void _inspect_state();

void fwrapper_entry(entry* e, void f(entry*)){
	if (e == NULL) {
		printf("no such key\n");
	} else {
		f(e); //TODO: make it so teh push is not atoi
		printf("ok\n");
	}
}

void swap(void* a1, int idx1, int idx2, size_t size_each_elem){
	void* temp = calloc(1, size_each_elem);
	memcpy(temp, a1+idx1*size_each_elem, size_each_elem); 
	memcpy(a1+idx1*size_each_elem, a1+idx2*size_each_elem, size_each_elem);
	memcpy(a1+idx2*size_each_elem, temp, size_each_elem);
	free(temp);
}

void command_bye() {
	printf("bye\n");
}

void command_help() {
	printf("%s", HELP);
}

bool string_isnumeric(char* string){
	char* cursor = string;
	while (*cursor != '\0'){
		if (!isdigit(*cursor)){
			return false;
		}
		cursor++;
	}
	return true;
}

// TEMP: Just making the function print out the entry for now, will make it return a string instead later
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
		
		//TODO: Write recursive function that converts links to other keys to strings within current string.
	}
	printf("]\n");
	free(string);
	// return string;
}	


//! Not to be confused with the command -> just returns the entry
entry* entry_get(char* key){
	// Perform linear search over current database
	entry* cursor = current_state;
	while (cursor != NULL){
		if (strcmp(key, cursor->key) == 0){
			return cursor;
		}
		cursor = cursor->next;
	}
	return NULL;
}

// TODO: Test this
//! getting backward references works properly, but forward references results in extra (null) printed
// Connects e to forward by first resizing reference arrays and own size variables and then by adding references
void entry_connect(entry* e, entry* forward){
	// Resize the memory 
	e->forward_size++;
	e->forward = realloc(e->forward, (e->forward_size)*sizeof(entry*));
	forward->backward_size++;
	forward->backward = realloc(forward->backward, (forward->backward_size)*sizeof(entry*));

	// Add forward reference and backward references to the end of the list
	e->forward[e->forward_size-1] = forward;
	forward->backward[forward->backward_size-1] = e;

}

// TODO: Make this return information about how some of the elements that were created were not simple! pass is simple boolean by arg?
element* elements_create(char** args, size_t args_size){
	element* elements = calloc(args_size, sizeof(element)); 
	for (int i = 0; i < args_size; i++){
		element* current_elem = elements + i;
		char* current_arg = args[i];
		if (string_isnumeric(current_arg)){
			current_elem->type = INTEGER;		
			current_elem->value = atoi(current_arg);
		} else {
			current_elem->type = ENTRY;
			entry* forward_link = entry_get(current_arg);
			current_elem->entry = forward_link;
			//! Think about how to added forward link to the back of the entry referenced.
			// forward_link->backward
			
		}
	}
	return elements;
}

// TODO: Deal with potential undefined behaviour from strings e.g. strlen not copying the null byte
// TODO: Include creation of complex entries i.e. ones with links to other keys (should include char* values) instead of int* values?;
// ? Note that the keys and values must also be inputted in the order that they are stored in so we probably cannot just have int array.
// Used to create simple entries (We create this in the main function then pass the entry around to other functions)
entry* entry_create(char** args, size_t args_size){
	
	if (args_size <= 1){
		perror("Cannot create an an entry with no key or with a key but has no values\n");
		return NULL;
	}

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
			entry* forward_link = entry_get(arg);

			if (strcasecmp(arg, key) == 0){
				printf("not permitted\n");
				return NULL;
			}
			
			if (forward_link == NULL){
				printf("no such key\n");
				return NULL;;
			} 

			// Connect elements if forward link is valid (handles adding to )
			entry_connect(e, forward_link);
		}
	}

	// Set key for entry
	// e->values = calloc(args_size-1, sizeof(element));
	memcpy(e->key, key, strlen(key)+1);

	// Set elements for entry
	e->values = elements_create(args+1, args_size-1);
	e->length = args_size-1;

	// Set max, min, sum
	entry_recalcsmm(e);

	return e;
}

// Adds the entry to the database state (current_state)
void state_push(entry* e){
	if (current_state == NULL){
		current_state = e;
	} else {
		// Add the entry to stack (current_state)
		entry* old_state = current_state;
		current_state = e;
		current_state->next = old_state;
		old_state->prev = current_state;
	}
}

//TODO: Test this
void entry_append(entry* e, char** args, size_t args_size){
	// Create array of elements to attach to the entry
	element* elements = elements_create(args, args_size);
	int old_length = e->length;
	e->length = e->length + args_size;
	e->values = realloc(e->values,	(e->length)*sizeof(element)); 
	memcpy(e->values+old_length, elements, sizeof(element)*args_size);

	// TODO: Go to each new element that is a char and attach back links to current element
	for (int i = 0; i < args_size; i++){
		element* current_element = elements+i;
		if (current_element->type == ENTRY){
			entry* forward_link = current_element->entry;
			entry_connect(e, forward_link);
		} 
	}	

	free(elements);
	entry_recalcsmm(e);

}

void _reverse_array(void* array, int length, size_t size){
	for (int i = 0; i < length/2; i++){
		swap(array, i, length-i-1, size);
	}
}

//! Fix error of not mallocing sufficiently!
void entry_push(entry* e, char** args, size_t args_size){
	// Create array of elements to attach to the entry
	element* elements = elements_create(args, args_size);
	_reverse_array((void*)elements, (int)args_size, sizeof(element));
	e->length = e->length + args_size;
	e->values = realloc(e->values,	(e->length)*sizeof(element)); 
	memmove(e->values+args_size, e->values, sizeof(element)*(e->length-args_size));
	memcpy(e->values, elements, sizeof(element)*args_size);

	for (int i = 0; i < args_size; i++){
		element* current_element = elements+i;
		if (current_element->type == ENTRY){
			entry* forward_link = current_element->entry;
			entry_connect(e, forward_link);
		} 
	}	

	free(elements);
	entry_recalcsmm(e);
}

void entry_min(entry* e){
	int min = e->min;
	int forward_size = 0;
	entry** forwards  = get_forward_links(e, &forward_size);

	for (int i = 0; i < forward_size; i++){
		if (forwards[i]->min < min){
			min = forwards[i]->min;
		}
	}

	printf("Minimum value is: %d\n", min);

	if (forward_size > 0){
		free(forwards);
	}

}

void entry_max(entry* e){
	int max = e->max;
	int forward_size = 0;
	entry** forwards  = get_forward_links(e, &forward_size);
	
	for (int i = 0; i < forward_size; i++){
		if (forwards[i]->max > max){
			max = forwards[i]->max;
		}
	}
	
	printf("Maximum value is: %d\n", max);

	if (forward_size > 0){
		free(forwards);
	}
}


void entry_sum(entry* e){
	// _inspect_state();
	int sum = e->sum;
	int forward_size = 0;
	entry** forwards  = get_forward_links(e, &forward_size);

	for (int i = 0; i < forward_size; i++){
		sum += forwards[i]->sum;
	}

	printf("Sum of values is: %d\n", sum);
	if (forward_size > 0){
		free(forwards);
	}
}

// Private method used by entry_len to get the DFS counting length of a general entry
int entry_len(entry* e){
	
	int len = e->length-e->forward_size;
	int forward_size = 0;
	entry** forwards  = get_forward_links(e, &forward_size);

	for (int i = 0; i < forward_size; i++){
		entry* forward = forwards[i];
		len += forward->length-forward->forward_size;
	}

	printf("The number of values in the entry is: %d\n", len);

	if (forward_size > 0){
		free(forwards);
	}
	
	return len;
}

//! Careful with this one! You don't want memory leaks
void entry_free(entry* e){	//! Changed so entry free frees the entry and its forward references -> causes memory issues elsewhere.
	// Free all values pointed to by e
	if (e->values != NULL) free(e->values);
	if (e->forward != NULL) free(e->forward);
	if (e->backward != NULL) free(e->backward);
	if (e->copy_reference != NULL) free(e->copy_reference);
	free(e);
}

// TODO: You must use the entry e as the new entry as the other entries are backward linked to it

// TODO: Change entry_set to have own independent function that does not rely on creating a new entry? and using entry_connect?
// TODO: Create entry_modify() function i.e. starts with args and just changes values, and adds forward references to existing entry. (does not create a new entry)

// Testing function to look at everything in memory
#if (TEST == 1)
	void _inspect_state(){
		entry* cursor = current_state;
		while (cursor != NULL){
			printf(">>> %s (Is simple: %d)\n", cursor->key, cursor->is_simple);
			printf("Values: ");
			for (int i = 0; i < cursor->length; i++){
				element elem = cursor->values[i];
				if (elem.type == ENTRY){
					printf("%s", elem.entry->key);
				} else {
					printf("%d", elem.value);
				}
				printf(" ");
			}
			printf("\n");

			printf("Backward links (%ld): ", cursor->backward_size);

			for (int i = 0; i < cursor->backward_size; i++){
				entry* backward = cursor->backward[i];
				printf("%s", backward->key);
				printf(" ");
			}
			printf("\n");

			printf("Forward links (%ld): ", cursor->forward_size);
			
			for (int i = 0; i < cursor->forward_size; i++){
				entry* forward= cursor->forward[i];
				printf("%s", forward->key);
				printf(" ");
			}
			printf("\n");

			cursor = cursor->next;
		}
	}
#else
	void _inspect_state(){
		return;
	}
#endif


// Removes entry with address rm from an array
entry** _entries_remove(entry** entries, size_t* entries_len, entry* rm){
	
	// printf("entry to remove is %s\n", rm->key);

	int idx = 0;
	// Grab index to remove
	for (; idx < *entries_len; idx++){
		// printf("%s ", entries[idx]->key);
		if (strcmp(entries[idx]->key, rm->key) == 0){
			break;
		}
	}

	// TESTING:
	// printf("***Viewing entries BEFORE removing***\n");
	// for (int i = 0; i < *entries_len; i++){
	// 	printf("%s, ", entries[i]->key);
	// }
	// printf("\n");
	// printf("entries length is: %ld\n", *entries_len);

	// Copy everything beyond index to index position
	if (*entries_len == 1){ //! Edge case causing segfault because we cannot realloc?
		*entries_len = *entries_len - 1;
		// printf("%p\n", entries);
		free(entries); 
		entries = NULL; //! Need to return null or you'll return pointer to garbage;
	} else {
		// printf("%ld %d\n", *entries_len, idx); //! Mixed test case -> why is b's backward entry size of 2? Shouldn't be any forward links to a though in mixed test case
		memmove(entries+idx, entries+idx+1, (*entries_len-(idx+1))*sizeof(entry*));
		*entries_len = *entries_len - 1;
		entries = realloc(entries, (*entries_len)*sizeof(entry*));
	}


	
	// // // TESTING;
	// printf("***Viewing entries AFTER removing***\n");
	// for (int i = 0; i < *entries_len; i++){
	// 	printf("%s, ", entries[i]->key);
	// }
	// printf("\n");

	return entries;
}

entry** _entries_replace(entry** entries, size_t* entries_len, entry* target, entry* replacement){
	int idx = 0;
	// Grab index to remove
	for (; idx < *entries_len; idx++){
		if (entries[idx] == target){
			break;
		}
	}

	// memcpy(entries+idx, replacement, sizeof(entry*));
	entries[idx] = replacement;
	return entries;
}

element* _elements_replace(element* elements, size_t* elements_len, entry* target, entry* replacement){
	int idx = 0;
	// Grab index to remove
	for (; idx < *elements_len; idx++){
		if (elements[idx].type == ENTRY && elements[idx].entry == target){
			break;
		}
	}

	elements[idx].entry = replacement;
	return elements;
}


// When you set an element to a new value -> could lead to links to it being removed
void entry_set(entry* e){
	// Search through current state and see if the entry with key is tehre
	entry* existing = entry_get(e->key);
	if (existing == NULL){
		state_push(e); // TODO: Consider whether you should by default push or append?
	} else {

		//! Fix the segfault error when you do b 2 d
		// printf("Setting general entry with key: %s\n", existing->key);
		// printf("Backward links (%ld): ", existing->backward_size);

		// for (int i = 0; i < existing->backward_size; i++){
		// 	entry* backward = existing->backward[i];
		// 	printf("%s", backward->key);
		// 	printf(" ");
		// }
		// printf("\n");

		// printf("Forward links (%ld): ", existing->forward_size);
		
		// for (int i = 0; i < existing->forward_size; i++){
		// 	entry* forward= existing->forward[i];
		// 	printf("%s", forward->key);
		// 	printf(" ");
		// }
		// printf("\n");
		// END test section

		// make all existing forward entries point back to new entry 
		for (int i = 0; i < existing->forward_size; i++){// TODO: Replace this with _rm_forward_link_to
			entry* forward = existing->forward[i];
			forward->backward = _entries_remove(forward->backward, &forward->backward_size, existing); 
		}

		// _inspect_state();

		// printf("Successfully removed exsiting forwad entries that point to new entry\n");

		// Make all existing back entries point forward to new entry
		//! a->new b - Troublesome section here
		//! O(n^2) if you set the entries.
		for (int i = 0; i < existing->backward_size; i++){
			entry* backward = existing->backward[i];
			backward->forward = _entries_replace(backward->forward, &backward->forward_size, existing, e);
			backward->values = _elements_replace(backward->values, &backward->length, existing, e);
		}

		// printf("--------------\n");
		// Make e take position of existing state
		entry* before = existing->prev;
		entry* after = existing->next;

		e->next = after;
		e->prev = before;

		if (before != NULL){
			before->next = e;
		} else {
			current_state = e;
		}

		if (after != NULL){
			after->prev = e;
		}
		
		// Make new state have back references of previous state
		entry** backward_copy = calloc(existing->backward_size, sizeof(entry*));
		memcpy(backward_copy, existing->backward, existing->backward_size*(sizeof(entry*)));
		e->backward = backward_copy;
		// e->backward_max = existing->backward_max;
		e->backward_size = existing->backward_size;

		
		entry_free(existing);
		// _inspect_state();



		// // Copy e's values to the existing e
		// existing->values = realloc(existing->values, e->length*sizeof(element)); 
		// memcpy(existing->values, e->values, e->length*sizeof(element));
		// existing->length = e->length;
		// //! Need to check if the new entry is simple

		// // Copy e's forward references to the old e's forward references
		// entry** forward_copy = calloc(e->forward_size, sizeof(entry*));
		// memcpy(forward_copy, e->forward, e->forward_size*sizeof(entry*));
		// free(existing->forward);
		// existing->forward = forward_copy;
		// existing->forward_max = e->forward_max;
		// existing->forward_size = e->forward_size; 

		// // Free the old e
		// //! TODO -> make this call the entry_free() method! -> Actually should free this in the main method
		// entry_free(e);
	}

	// If entry is there, grab back entries for that entry and attache to current entry. Free old entry's memory	
}






void entry_reverse(entry* e){
	if (e->is_simple == false){
		printf("Cannot reverse an entry that is not simple!\n");
		return;
	}

	for (int i = 0; i < e->length/2; i++){
		swap(e->values, i, e->length-1-i, sizeof(element));
	}
}

//! entry* get_all_forward_recursive(entry* e)

// Appends entry to the entry array (not directly to entry->next), MUST reassign to return value due to pass by value
entry** _entry_append(entry** list, entry* e, int* list_size){
	*list_size = *list_size + 1;
	if (*list_size == 1){
		list = calloc(1, sizeof(entry*));
	} else {
		list = realloc(list, (*list_size)*sizeof(entry*));
	}
	list[*list_size-1] = e;
	return list;
}

void entry_clear_visits(){
	entry* cursor = current_state;
	while (cursor != NULL){
		cursor->has_visited = false;
		cursor = cursor->next;
	}
}

//! realloc() invalid pointer
//! How to avoid adding the same node over and over again in DFS (same as the copying nodes question) -> check clone graph question on leetcode (but that only works for limited nodes, where each node has one value -> can't use array trick, must use hashmap here)
//! example solution will be to add all elements to the array 
//! Boss solution: Create own set/hashmap data structure that works with any input so you can perform traversals without repetiiton yet also do it in constant time (need to write own hash function though)
entry** _get_forward_links(entry* e, int* size){
	entry** forwards = NULL;
	entry** next_forwards = NULL;
	int next_size = 0;

	// Base case - If the current entry has already been visited i.e. added to forwards list, don't add it again
	e->has_visited = true;

	// Add all forwards list 
	for (int i = 0; i < e->forward_size; i++){
		entry* forward_link = e->forward[i];
		
		if (forward_link->has_visited == true) continue;	

		// Resize forwards and add link to end of list
		forwards = _entry_append(forwards, forward_link, size);	//! Need to get the new forwards value because of pass by value unless you use triple pointer
	
		// DFS forward - Get array containing recursive (only if the entry is not marked as visited)
		next_size = 0; //? next_size resetted for every iteration of the loop
		next_forwards = _get_forward_links(forward_link, &next_size);
	
		// Attach fowards from recursive call to end of current list		
		if (next_size > 0){
			*size += next_size;
			forwards = realloc(forwards, (*size)*sizeof(entry*));		
			memcpy(forwards+*size-next_size, next_forwards, next_size*sizeof(entry*));
			free(next_forwards);
		}
	}
	
	return forwards;
}


//! Calling array should free the array
// Returns array containing all forward_entries, with int telling the length of the entry array
entry** get_forward_links(entry* e, int* size){
	entry** forwards = _get_forward_links(e, size);
	e->has_visited = false;
	for (int i = 0; i < *size; i++){
		forwards[i]->has_visited = false;
	}	
	return forwards;
}

entry** _get_backward_links(entry* e, int* size){
	entry** backwards;
	entry** next_backwards;
	int next_size = 0;

	// Base case - If the current entry has already been visited i.e. added to forwards list, don't add it again
	e->has_visited = true;

	// Add all forwards list 
	for (int i = 0; i < e->backward_size; i++){
		entry* backward_link = e->backward[i];
		
		if (backward_link->has_visited == true) continue;	

		// Resize forwards and add link to end of list
		backwards = _entry_append(backwards, backward_link, size);	//! Need to get the new forwards value because of pass by value unless you use triple pointer
	
		// DFS forward - Get array containing recursive (only if the entry is not marked as visited)
		next_size = 0; //? next_size resetted for every iteration of the loop
		next_backwards = _get_backward_links(backward_link, &next_size);
	
		// Attach fowards from recursive call to end of current list		
		if (next_size > 0){
			*size += next_size;
			backwards = realloc(backwards, (*size)*sizeof(entry*));		
			memcpy(backwards+*size-next_size, next_backwards, next_size*sizeof(entry*));
			free(next_backwards);
		}
	}
	
	return backwards;
}

entry** get_backward_links(entry* e, int* size){
	entry** backwards = _get_backward_links(e, size);
	e->has_visited = false;
	for (int i = 0; i < *size; i++){
		backwards[i]->has_visited = false;
	}
	return backwards;
}

void entry_forward(entry* e){
	int size = 0;
	entry** forward_entries = get_forward_links(e, &size);	

	// TODO: Sort to lexicographical order;
	e->has_visited = false;
	for (int i = 0; i < size; i++){
		printf("%s ", forward_entries[i]->key);
	}

	if (size == 0){
		printf("nil\n\n");
	} else {
		free(forward_entries);
		printf("\n");
	}
}

//! Memory reference issue, list entries can get the entries, but it is terminated by null byte
// TODO: Function is recursive -> keep adding backwards links to char list until no more backwards
//? Does not return correct backward references.
void entry_backward(entry* e){
	int size = 0;
	entry** backward_entries = get_backward_links(e, &size);	
	// Loop through all entries and reset their visited value;

	// Sort to lexicographical order;
	e->has_visited = false;
	for (int i = 0; i < size; i++){
		printf("%s ", backward_entries[i]->key);
		// backward_entries[i]->has_visited = false;
	}

	if (size == 0){
		printf("nil\n\n");
	} else {
		free(backward_entries);
		printf("\n");
	}
}

void entry_type(entry* e){
	if (e->is_simple){
		printf("The entry is simple\n");
	} else {
		printf("The entry is general\n");
	}
}

//! Would this count as quadratic time? Ask about this, if so don't delete the back links
void _rm_forward_links_to(entry* e){
	// Remove each back link to e every for every forward link e has
	// printf("backward links: %d\n", e->forward)
	for (int i = 0; i < e->forward_size; i++){
		entry* forward_link = e->forward[i];
		// printf("forward_link: %s\n", forward_link->key);
		forward_link->backward = _entries_remove(forward_link->backward, &forward_link->backward_size, e);
	}
}

void _rm_backward_links_to(entry* e){
	// Remove forward links to e for every back link e has
	for (int i = 0; i < e->backward_size; i++){
		entry* backward_link = e->backward[i];
		backward_link->forward = _entries_remove(backward_link->forward, &backward_link->forward_size, e);
	}
}

void entry_delete(entry* e){
	_inspect_state();

	if (e->backward_size > 0){
		printf("You cannot delete an entry with backlinks!\n");
		return;
	} else {
		_rm_forward_links_to(e);
		_rm_backward_links_to(e);

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
		if (current_state->key == e->key){
			current_state = e->next;
		}

		// if (before == NULL){
		// 	current_state = e->next;
		// 	if (current_state != NULL){
		// 		current_state->prev = NULL;
		// 	}
		// } else {
		// 	if (after == NULL){
		// 		before->next = NULL;
		// 	} else {
		// 		before->next = after;
		// 		after->prev = before;
		// 	}			
		// } 	

		entry_free(e);
		_inspect_state();
	}
}

int element_compare(const void* e1, const void* e2){
	element* element_1 = (element*) e1;
	element* element_2 = (element*) e2;
	int result = element_1->value - element_2->value; 
	return result;
}

// Use C library's sorting algos
void entry_sort(entry* e){
	qsort(e->values, e->length, sizeof(element), &element_compare); //? Seems like we don't need to add the & sign next to function to make it a function pointer?
}

void entry_unique(entry* e){
	if (e->is_simple == false){
		printf("Cannot make unique an entry that is not simple!\n");
	}

	element* new_values = calloc(e->length, sizeof(element));
	element* last_word = NULL; //! Set it to null or you'll get stack underflow error (last_word == NULl) does not work
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
	new_values = realloc(new_values, new_size*sizeof(element));
	free(e->values); //TODO: Effectively free these values
	e->length = new_size;
	e->values = new_values;

}

void entry_pick(entry* e, int index){	
	if (e->length == 0){
		printf("nil\n");
		return;
	}
	
	if (index < 0 || index >= e->length){
		printf("index out of range\n");
		return;
	}


	element* elem = e->values+index;

	
	if (elem->type == INTEGER){
		#if (TEST == 1)
			printf("Value at index %d in entry with key %s is: %d\n", index, e->key, elem->value);
		#else
			printf("%d\n", elem->value);
		#endif
	} else {
		#if (TEST == 1)
			printf("Value at index %d in entry with key %s is: %s\n", index, e->key, elem->entry->key);
		#else
			printf("%s\n", elem->entry->key);
		#endif
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


void entry_pluck(entry* e, int index){
	// printf("%ld\n", e->length);

	if (index < 0 || index >= e->length){
		printf("index out of range\n");
		return;
	}	

	if (e->length == 0){
		printf("nil\n");
		return;
	}

	

	entry_pick(e, index);
	element* elem_to_remove = e->values+index;


	// Remove backlinks to e for entries that link back to e due to e containing elem_to_remove
	if (elem_to_remove->type == ENTRY){
		entry* forward = elem_to_remove->entry;
		forward->backward =_entries_remove(forward->backward, &forward->backward_size, e); // remove back link
		e->forward = _entries_remove(e->forward, &e->forward_size, elem_to_remove->entry); // remove forwad link
	} else {
		entry_recalcsmm(e);
	}


	
	e->length--;
	memmove(elem_to_remove, elem_to_remove+1, (e->length-index)*sizeof(element)); //! Address sanitizer issue -> use memmove instead?
	e->values = realloc(e->values, e->length*sizeof(element));

	// // Resize values array to fit size.
	// // TODO: Cannot malloc sizeof 0 -> fix this
	// if (e->length == 0){
	// 	// Delete element array entirely.
	// 	free(e->values);
	// } else {
	// 	// Remove element from element array
	// 	memcpy(elem_to_remove, elem_to_remove+1, (e->length-index)*sizeof(element));
	// 	e->values = realloc(e->values, e->length*sizeof(element));
	// }
	
	//! Issue with realloc -> when you to  size 0 is that valid?


	 //? Can realloc be used to shrink the amount of memory allocated? YES
	//  free(e->values+index);; //! Does this cause double free?
	//! Don't forget to realloc the exact amount in bytes, i.e. forgot to multiply by sizeof(element);
	// TODO: Fix memory leak from losing the reference to the element that has been deleted, actuall no because element array here i.e. if you remove the element with realloc it's gone, only do it for element*
}

void entry_pop(entry* e){
	// You cannot have an index out of range error for pop -> always check for length first
	if (e->length == 0){
		printf("nil\n");
		return;
	}
	entry_pluck(e, 0);
}

void list_keys(){
	entry* cursor = current_state;

	if (cursor == NULL){
		printf("no keys\n");
		return;
	}

	while (cursor != NULL){
		printf("%s\n", cursor->key);
		cursor = cursor->next;
	}
}

void list_entries(){
	entry* cursor = current_state;

	if (cursor == NULL){
		printf("no entries\n");
		return;
	}

	while (cursor != NULL){
		printf("%s ", cursor->key);
		entry_tostring(cursor); //? Should this display the links as a letter or the elements in that next link?
		cursor = cursor->next;
	}
}

void list_snapshots(){
	snapshot* cursor = first_snapshot;

	if (cursor == NULL){
		printf("no snapshots\n");
		return;
	}

	while (cursor != NULL){
		printf("%d ", cursor->id);
		cursor = cursor->next;
	}
}

snapshot* snapshot_get(int id){
	snapshot* cursor = first_snapshot;
	while (cursor != NULL){
		if (id == cursor->id){
			return cursor;
		}
		cursor = cursor->next;
	}
	return NULL;
}

// Creates a copy of the entry inside e->copy_reference that links to all forward entries.
entry* _entry_copy(entry* e){

	// Return entry's copy if it has already been copied'
	if (e->copy_reference != NULL){
		return e->copy_reference;
	}

	entry* copy = calloc(1, sizeof(entry));
	element* copy_values = calloc(e->length, sizeof(element));

	// Store copy of e's forward entries (new forward array)
	int forward_copies_size = 0;
	entry** forward_copies = NULL;
	// entry** forward_copies = calloc(e->forward_size, sizeof(entry*)); //! mem leak

	// Copy memory from e for the entry to copy, but clear forward/backward arrays for copies
	memcpy(copy, e, sizeof(entry));
	copy->backward_size = 0;
	copy->forward_size = 0;
	copy->backward = NULL;
	copy->forward = NULL;
	
	// Iterate through all values and make links to copies of forward entries 
	for (int i = 0; i < e->length; i++){
		element* elem = e->values + i;
		element* elem_copy = copy_values + i;
		memcpy(elem_copy, elem, sizeof(element)); //? Copy values over by default, deal with entry case as exception
		if (elem->type == ENTRY){
			printf("%s", elem->entry->key);
			entry* forward_copy = _entry_copy(elem->entry); // TODO: Fix returning of copy reference, nvm it is working. just didn't read right?
			
			// Connect e to copy of forward link in both ways
			forward_copy->backward = _entry_append(forward_copy->backward, e, (int*)&forward_copy->backward_size);
			forward_copies = _entry_append(forward_copies, forward_copy, (int*)&forward_copies_size); //! reassignment needed due to realloc chaning addresses
			elem_copy->type = ENTRY;
			elem_copy->entry = forward_copy; 
		} 
		
	}

	copy->values = copy_values;
	copy->forward = forward_copies;
	copy->forward_size = forward_copies_size;	
	e->copy_reference = copy;

	printf(" ");

	return copy;
}

//? If every thing is O(n) instead of each entry, then iterating through all entries and degrees is O(n)
			
// 	// TODO: For general entries, think about how you would create ensure these entries are linked to the references? 
// 	//? IDEA: When you encounter a forward link, call entry copy recursively and make attach the return value to the forward link
// 	//? IDEA: When the returned entry comes back add its backward link to current entry
// 	//? How to avoid double copying entries? i.e. if we already copied all in one chain, don't copy that chain again -> think of edge case where you have a->b, a->c, b, c->d
// 		//? Store the keys we have already computed? in an array and check in function to see if key is already in (n^2 time though)
// 		//? Store id variable for each entry in the list -> add to struct? and then use marked array i.e. constant time lookup for whether you double copied
// 		//? Apply the same idea to deleting entries.
// 		//! If you go this route, you will need to refactor delete and append commands by implementing a global variable for storing the number of nodes in the current state.
// }




//? Could create pointer to last element and just append to that 
// Sets the correct next and prev pointers for the snapshot to be appended.
void snapshot_append(snapshot* snap){
	if (last_snapshot == NULL){
		first_snapshot = snap;
		last_snapshot = snap;
	} else {
		last_snapshot->next = snap;
		snap->prev = last_snapshot;
		last_snapshot = snap;
	}
}

//?! Are forward and backward entries of a certain entry always going to maintain the same index? probably not? e.g. triangle
snapshot* snapshot_create(entry* entries){
	entry* cursor = entries;
	entry* entries_copy = NULL;
	entry* previous = NULL;

	while (cursor != NULL){
		entry* copy = cursor->copy_reference;
		if (copy == NULL){
			copy = _entry_copy(cursor); //! might have corrupted top size with poor management of entry_copy() -> old
		}

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

	// Set all elements' copy_reference to null after creating snapshot
	cursor = entries;
	while (cursor != NULL){
		// printf("%s ", cursor->key);
		cursor->copy_reference = NULL;
		cursor->has_visited = false;
		cursor = cursor->next;
	}
	// printf("\n");

	snapshot* new_snapshot = calloc(1, sizeof(snapshot));
	// printf("%p\n", entries_copy);
	// printf("%s\n", entries_copy->key);
	new_snapshot->entries = entries_copy;

	if (last_snapshot != NULL){
		new_snapshot->id = last_snapshot->id+1; //TODO: Check if this is the correct id naming system or if you should have global var tracker.
	} else {
		new_snapshot->id = 1;
	}
	
	return new_snapshot;
}

void snapshot_free(entry* entries){
	// Free up all the entries in the snapshot
	entry* cursor = entries;
	entry* old = NULL;
	while (cursor != NULL){
		old = cursor;
		cursor = cursor->next; //! Make sure cursor points a allocated block of memory 
		entry_free(old);
	}
}



//! Bug: If you delete a snapsot when there is only one snapshots, subsequent snapshots added will not be found.
//? error handling done by the main function
void snapshot_drop(snapshot* snap){
	snapshot* before = snap->prev;
	snapshot* after = snap->next;

	// Free up all the entries in the snapshot
	printf("Trying to free entries in the snapshot to be dropped\n");
	snapshot_free(snap->entries); //! Segfault line
	printf("succesfully freed entries in the snapshot to be dropped\n");

	// Get previous snapshot to point to next snapshot and vice versa
	if (before == NULL){
		first_snapshot = after;
		if (first_snapshot == NULL){
			last_snapshot = NULL;
		}
	} else if (snap == last_snapshot) {
		last_snapshot = snap->prev;
		before->next = NULL;
	} else {
		before->next = after;
		after->prev = before;
	}

	// Update first and last snapshot
	//! Last_snapshot needs to be null 

	


	free(snap);

}

void program_clear(){
	snapshot* cursor = last_snapshot; 
	snapshot* old;

	snapshot_free(current_state); 

	while (cursor != NULL){
		old = cursor;
		cursor = cursor->prev;
		snapshot_drop(old);
	}
}

void snapshot_rollback(snapshot* snap){
	snapshot* cursor = last_snapshot; 
	snapshot* old;

	snapshot_free(current_state);

	// Got the snapshot we want (deleting snapshots along the way)
	while (cursor != NULL){
		if (cursor == snap){
			break;
		}
		old = cursor;
		cursor = cursor->prev;
		snapshot_drop(old);
	}

	// Create copy of snapshot we want to rollback to
	snapshot* snap_copy = snapshot_create(cursor->entries);
	current_state = snap_copy->entries;
	free(snap_copy);
	// Clear the current state (//TODO: Implement deep delete function alongside deep copy function)
}

//! When you checkout to an entry and then drop that entry whilst you're in the snapshot -> you get junk values when you list entries.
void snapshot_checkout(snapshot* snap){
	// Free current state
	snapshot_free(current_state);
	snapshot* snap_copy = snapshot_create(snap->entries);
	current_state = snap_copy->entries;
	free(snap_copy); //? don't need id for copy of snapshot
}

snapshot* snapshot_save(){
	snapshot* new_snapshot = snapshot_create(current_state);  //! Snapshots are not freed?
	snapshot_append(new_snapshot);
	return new_snapshot;
}

int snapshot_size(snapshot* snap){
	int size = 0;
	entry* cursor = snap->entries;
	while (cursor != NULL){
		cursor = cursor->next;
		size++;
	}
	return size;
}

void purge(char* key){
	
	entry* original_state = current_state;
	entry* to_delete;

	// Find key in snapshots and delete
	snapshot* snap = first_snapshot;
	while (snap != NULL){
		current_state = snap->entries;
		to_delete = entry_get(key);
		if (to_delete != NULL){
			entry_delete(to_delete);
		}
		snap->entries = current_state; //? Make snapshot possess correct starting entries upon deletion
		snap = snap->next;
	}

	// Restore original state after purging
	// Find key in current database and delete
	current_state = original_state;
	to_delete = entry_get(key);
	if (to_delete != NULL){
		entry_delete(to_delete);
	} 

}

int main(void) {

	char line[MAX_LINE];
	// snapshot* snapshots; // TODO: Add support for creating snapshots of the current state (deep copies) for simple entries;
	current_state = NULL; //? Good practice to always intialise pointers to null to avoid garbage values


	//! I can malloc a struct for test but it is not letting me do it in the function?
	// entry* test = malloc(sizeof(entry));
	// test->key[0] = 'c';

	while (true) {
		printf("> ");
	

		//? Quit if the user does not enter anything as a command -> don't forget to free memory afterwards
		if (NULL == fgets(line, MAX_LINE, stdin)) {
			printf("\n");
			command_bye();
			return 0;
		}


		// Process multiple arguments to the command line
		# if (PRINT_COMMAND == 1)
			printf("%s\n", line);
		#endif
		char* word = strtok(line, " \n\r"); 
		char** args = calloc(MAX_LINE, sizeof(char*));
		// char** args = calloc(MAX_KEY, sizeof(char*)); //! Failing to succesfully allocate sufficient memory to a datatype leads to undefined behaviour!! E.g. corrupted top size can't malloc properly in the future
		size_t args_size = 0;
		while (word != NULL) {
			args[args_size] = word;
			args_size++;
			word = strtok(NULL, " \n\r"); //! Tells the function to use the last string that was inputted into strtok -> returns NULL when it reaches \0 byte in the string
		}
		// Create a key value pair with the first argument as the key
		// Convert numbers beyond the first argument to numbers
		//! Use strcasecmp if you want to have String.equalsIgnoresCase() functionality in C

		// for (int i = 0; i < args_size; i++){
		// 	printf("A%d: [%s] ", i, args[i]);
		// }
		// printf("\n");


		char* command_type = args[0];
		// printf("The command type is %s\n", command_type);
		// printf("value evaluated: %d\n", strcasecmp(command_type, "SET"));

		// Instruction order

		// Set --





		
		if (strcasecmp(command_type, "SET") == 0){
			entry* e = entry_create(args+1, args_size-1); //! TODO: Fix issue within entry_create
			fwrapper_entry(e, &entry_set);
			// entry_set(e);
			// printf("ok\n");
		} else if (strcasecmp(command_type, "PUSH") == 0){
			entry* e = entry_get(args[1]);
			if (e == NULL) {
				printf("no such key\n");
			} else {
				entry_push(e, args+2, args_size-2); //TODO: make it so teh push is not atoi
				printf("ok\n");
			}
		} else if (strcasecmp(command_type, "APPEND") == 0){
			entry* e = entry_get(args[1]); //? +1 so that we don't include the command in the arguments used to build the entry
			if (e == NULL){
				printf("no such key\n");
			} else {
				entry_append(e, args+2, args_size-2);
				printf("ok\n");
			}
		} else if (strcasecmp(command_type, "GET") == 0){
			entry* e = entry_get(args[1]);
			if (e == NULL) {
				printf("no such key\n");
			} else {
				entry_tostring(e);
			}
		} else if (strcasecmp(command_type, "DEL") == 0){
			entry* e = entry_get(args[1]);
			fwrapper_entry(e, &entry_delete);
			// if (e == NULL) {
			// 	printf("no such key\n");
			// } else {
			// 	entry_delete(e);
			// 	printf("ok\n");
			// }
		} else if (strcasecmp(command_type, "MIN") == 0){
			entry* e = entry_get(args[1]);
			fwrapper_entry(e, &entry_min);
			// entry_min(e);
		} else if (strcasecmp(command_type, "MAX") == 0){
			entry* e = entry_get(args[1]);
			fwrapper_entry(e, &entry_max);
			// entry_max(e);
		} else if (strcasecmp(command_type, "SUM") == 0){
			entry* e = entry_get(args[1]); // TODO: Add local sum, max, len so you don't have to sum degrees.
			fwrapper_entry(e, &entry_sum);
			// entry_sum(e);
		} else if (strcasecmp(command_type, "LEN") == 0){
			entry* e = entry_get(args[1]);
			// fwrapper_entry(e, &entry_len);
			if (e == NULL) {
				printf("no such key\n");
			} else {
				entry_len(e);
			}
		} else if (strcasecmp(command_type, "SORT") == 0){
			entry* e = entry_get(args[1]);
			fwrapper_entry(e, &entry_sort);
			// entry_sort(e);
		} else if (strcasecmp(command_type, "REV") == 0){
			entry* e = entry_get(args[1]);
			fwrapper_entry(e, &entry_reverse);
			// entry_reverse(e);
		} else if (strcasecmp(command_type, "UNIQ") == 0){
			entry* e = entry_get(args[1]); //TODO: add input verification and also checking that entry exists
			fwrapper_entry(e, &entry_unique);
			// entry_unique(e);
		} else if (strcasecmp(command_type, "PLUCK") == 0){
			entry* e = entry_get(args[1]);
			if (e == NULL) {
				printf("no such key\n");
			} else {
				if (!string_isnumeric(args[2])){
					printf("Cannot pluck an index that is not numeric!\n");
				} else {
					//! check entry is valid
					int index = atoi(args[2])-1;
					entry_pluck(e, index);	
				}	
			}
		} else if (strcasecmp(command_type, "PICK") == 0){
			entry* e = entry_get(args[1]);
			if (e == NULL) {
				printf("no such key\n");
			} else {
				if (!string_isnumeric(args[2])){
					printf("Cannot pick an index that is not numeric!\n");
				} else {
					int index = atoi(args[2])-1;
					entry_pick(e, index);	
				}
			}
		} else if (strcasecmp(command_type, "POP") == 0){
			entry* e = entry_get(args[1]);
			if (e == NULL){
				printf("no such key\n");
			} else {
				entry_pop(e);
			}
			// fwrapper_entry(e, &entry_pop);
			// entry_pop(e); //! Check for entry valid -> actually have some wrapper function that does that
		} else if (strcasecmp(command_type, "FORWARD") == 0){
			entry* e = entry_get(args[1]);
			fwrapper_entry(e, &entry_forward);
			// entry_forward(e);
		} else if (strcasecmp(command_type, "BACKWARD") == 0){
			entry* e = entry_get(args[1]);
			fwrapper_entry(e, &entry_backward);
			// entry_backward(e);
		} else if (strcasecmp(command_type, "PURGE") == 0){
			char* key = args[1];
			// fwrapper_entry(e, &entry_pop);
			purge(key);
		}  else if (strcasecmp(command_type, "TYPE") == 0){
			entry* e = entry_get(args[1]);
			fwrapper_entry(e, &entry_type);
			if (e == NULL) {
				printf("no such key\n"); //TODO: Use function pointers (create wrapper function) to call any functions that use the get entry method.
			} else {
				entry_type(e);
			}
		} else if (strcasecmp(command_type, "LIST") == 0){
			char* option = args[1];
			if (strcasecmp(option, "KEYS") == 0){
				list_keys();
			} else if (strcasecmp(option, "ENTRIES") == 0){
				list_entries();
			} else if (strcasecmp(option, "SNAPSHOTS") == 0){
				list_snapshots();
			} 
		} else if (strcasecmp(command_type, "SNAPSHOT") == 0){
		 	snapshot* snap = snapshot_save();
			printf("saved as snapshot %d\n", snap->id);
		} else if (strcasecmp(command_type, "DROP") == 0){ //! Segfaults
			if (!string_isnumeric(args[1])){
				printf("You must provide a valid ID for a snapshot!\n");
			} else {
				int id = atoi(args[1]);
				snapshot* snap = snapshot_get(id);
				if (snap == NULL){
					printf("No such snapshot\n");
				} else {
					snapshot_drop(snap);
				}

			}
		} else if (strcasecmp(command_type, "ROLLBACK") == 0){ //! Segfaults
			if (!string_isnumeric(args[1])){
				printf("You must provide a valid ID for a snapshot!\n");
			} else {
				int id = atoi(args[1]);
				snapshot* snap = snapshot_get(id);
				if (snap == NULL){
					printf("Cannot rollback to snapshot with id: %d as this id does not exist in the database\n", id);
				} else {
					snapshot_rollback(snap);
				}
			}
		} else if (strcasecmp(command_type, "CHECKOUT") == 0){
			if (!string_isnumeric(args[1])){
				printf("You must provide a valid ID for a snapshot!\n");
			} else {
				int id = atoi(args[1]);
				snapshot* snap = snapshot_get(id);
				if (snap == NULL){
					printf("Cannot checkout to snapshot with id: %d as this id does not exist in the database\n", id);
				} else {
					snapshot_checkout(snap);
				}
			}
		} else if (strcasecmp(command_type, "HELP") == 0){
		 	command_help();
		} else if (strcasecmp(command_type, "BYE") == 0){
			command_bye();
			program_clear(); //! Does not work -> memory leak and double free
			// snapshot_free(current_state); 
			// Drop all snapshots
			free(args);
			return -1;
			// TODO: Free the memory that has been allocated to snapshots and entries
		}

		printf("\n");
		//! Make sure to free the arguments after you have finished operating on them;
		// isdigit()
		free(args);
		// for (int i = 0; i < args_num; i++){
		// 	printf("Argument %d: %s\n", i, args[i]);
		// }

		//
		// TODO
		//





  	}

	return 0;
}
