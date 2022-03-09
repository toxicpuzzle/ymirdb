/**
 * comp2017 - assignment 2
 * <your name>
 * <your unikey>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <ctype.h>
#include <limits.h>

#include "ymirdb.h"

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
void _inspect_state();

void command_bye() {
	printf("bye\n");
}

void command_help() {
	printf("%s\n", HELP);
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
char* entry_tostring(entry* e){
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
	return string;
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
	e->values = calloc(args_size-1, sizeof(element));
	memcpy(e->key, key, strlen(key)+1);

	// Set elements for entry
	e->values = elements_create(args+1, args_size-1);
	e->length = args_size-1;

	return e;
}

void state_append(entry* e){
	if (current_state == NULL){
		current_state = e;
	} else {
		// Go to the end of the current list
		entry* cursor = current_state;
		while (cursor->next != NULL){
			cursor = cursor->next;
		}
		cursor->next = e;
		e->prev = cursor;
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
}

//! Fix error of not mallocing sufficiently!
void entry_push(entry* e, char** args, size_t args_size){
	// Create array of elements to attach to the entry
	element* elements = elements_create(args, args_size);
	e->length = e->length + args_size;
	e->values = realloc(e->values,	(e->length)*sizeof(element)); 
	memcpy(e->values+args_size, e->values, sizeof(element)*(e->length-args_size));
	memcpy(e->values, elements, sizeof(element)*args_size);
}


//TODO: Make sure these functions are running within qudratic time constraints -> e.g. using forwad_max to keep track of largest max far in forward chain.
void entry_min(entry* e){
	int min = INT_MAX;
	int forward_size = 0;
	// _inspect_state();
	entry** forwards  = get_forward_links(e, &forward_size);

	// _inspect_state();
	// Look through own values to find value smaller than min
	for (int i = 0; i < e->length; i++){
		if (e->values[i].type == INTEGER && e->values[i].value < min){
			min = e->values[i].value;
		}
	}

	for (int i = 0; i < forward_size; i++){
		entry* forward = forwards[i];
		// printf("Looking at key: %s\n", forward->key);
		for (int j = 0; j < forward->length; j++){
			element current_element = forward->values[j];
			if (current_element.type == INTEGER && current_element.value < min){
				min = current_element.value;
			}
		}		
	}

	printf("Minimum value is: %d\n", min);
	//? free(forwards); -> Investigate why putting free forwards here leads to double free? i.e. freed in earlier calls and then tried to reference again in later calls?
	if (forward_size > 0){
		free(forwards);
	}
	// free(forwards);
}

void entry_max(entry* e){
	int max = INT_MIN;
	int forward_size = 0;
	entry** forwards  = get_forward_links(e, &forward_size);

	for (int i = 0; i < e->length; i++){
		if (e->values[i].type == INTEGER && e->values[i].value > max){
			max = e->values[i].value;
		}
	}

	for (int i = 0; i < forward_size; i++){
		entry* forward = forwards[i];
		// printf("Looking at key: %s\n", forward->key);
		for (int j = 0; j < forward->length; j++){
			element current_element = forward->values[j];
			if (current_element.type == INTEGER && current_element.value > max){
				max = current_element.value;
			}
		}		
	}

	printf("Maximum value is: %d\n", max);
	if (forward_size > 0){
		free(forwards);
	}
	// free(forwards);
}


int sum_calculate(entry* e){
	int sum = 0;
	for (int i = 0; i < e->length; i++){
		element* curr_elem = e->values+i;
		if (curr_elem->type == INTEGER){
			sum += curr_elem->value;
		} else {
			sum += sum_calculate(curr_elem->entry);
		}
	}

	return sum;
}

void entry_sum(entry* e){
	int sum = sum_calculate(e);
	printf("Sum of values is: %d\n", sum);
}

// Private method used by entry_len to get the DFS counting length of a general entry
int _entry_len(entry* e){
	int num_list_size = 0;
	e->has_visited = true;

	for (int i = 0; i < e->length; i++){
		if (e->values[i].type == INTEGER){
			num_list_size++;
		} else if (e->values[i].type == ENTRY && e->values[i].entry->has_visited == false){
			num_list_size += _entry_len(e->values[i].entry);
		}
	}
	
	return num_list_size;
}

// TODO: Specialised DFS function to count only simple entries towards the final size, can't just get forward links
void entry_len(entry* e){
	int length = _entry_len(e);
	printf("The number of values in the entry is: %d\n", length);
}

//! Careful with this one! You don't want memory leaks
void entry_free(entry* e){	
	// Free all values pointed to by e
	free(e->values);
	// for (int i = 0; i < e->length; i++){
	// 	free(e->values+i);
	// }
	free(e);
}

// TODO: You must use the entry e as the new entry as the other entries are backward linked to it

// TODO: Change entry_set to have own independent function that does not rely on creating a new entry? and using entry_connect?
// TODO: Create entry_modify() function i.e. starts with args and just changes values, and adds forward references to existing entry. (does not create a new entry)

// Testing function to look at everything in memory
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


// Removes entry with address rm from an array
entry** _entries_remove(entry** entries, size_t* entries_len, entry* rm){
	int idx = 0;
	// Grab index to remove
	for (; idx < *entries_len; idx++){
		if (entries[idx] == rm){
			break;
		}
	}

	// TESTING:
	// printf("***Viewing entries BEFORE removing***\n");
	// for (int i = 0; i < *entries_len; i++){
	// 	printf("%s, ", entries[i]->key);
	// }
	// printf("\n");


	// Copy everything beyond index to index position
	if (*entries_len == 1){ //! Edge case causing segfault because we cannot realloc?
		*entries_len = *entries_len - 1;
		free(entries);
	} else {
		memcpy(entries+idx, entries+idx+1, (*entries_len-(idx+1))*sizeof(entry*));
		*entries_len = *entries_len - 1;
		entries = realloc(entries, (*entries_len)*sizeof(entry*));
	}


	
	// // TESTING;
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
		state_append(e); // TODO: Consider whether you should by default push or append?
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
			_entries_remove(forward->backward, &forward->backward_size, existing); 
		}

		// _inspect_state();

		// printf("Successfully removed exsiting forwad entries that point to new entry\n");

		// Make all existing back entries point forward to new entry
		//! a->new b - Troublesome section here
		for (int i = 0; i < existing->backward_size; i++){
			entry* backward = existing->backward[i];
			_entries_replace(backward->forward, &backward->forward_size, existing, e);
			_elements_replace(backward->values, &backward->length, existing, e);
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
		e->backward_max = existing->backward_max;
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




void swap(void* a1, int idx1, int idx2, size_t size_each_elem){
	void* temp = calloc(1, size_each_elem);
	memcpy(temp, a1+idx1*size_each_elem, size_each_elem); 
	memcpy(a1+idx1*size_each_elem, a1+idx2*size_each_elem, size_each_elem);
	memcpy(a1+idx2*size_each_elem, temp, size_each_elem);
	free(temp);
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
	entry** forwards;
	entry** next_forwards;
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
		}
	}
	
	return forwards;
}


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
	// Loop through all entries and reset their visited value;

	// Sort to lexicographical order;
	e->has_visited = false;
	for (int i = 0; i < size; i++){
		printf("%s ", forward_entries[i]->key);
		// forward_entries[i]->has_visited = false;
	}

	if (size == 0){
		printf("nil\n\n");
	} else {
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
	for (int i = 0; i < e->forward_size; i++){
		entry* forward_link = e->forward[i];
		_entries_remove(forward_link->backward, &forward_link->backward_size, e);
	}
}

void _rm_backward_links_to(entry* e){
	// Remove forward links to e for every back link e has
	for (int i = 0; i < e->backward_size; i++){
		entry* backward_link = e->backward[i];
		_entries_remove(backward_link->forward, &backward_link->forward_size, e);
	}
}



// TODO: Deleting element at the end of the list seems to not effectively delete the element. -> Fixed
void entry_delete(entry* e){
	// _inspect_state();

	//! backlinks not working
	if (e->backward_size > 0){
		printf("You cannot delete an entry with backlinks!\n");
		return;
	} else {
		_rm_forward_links_to(e);
		_rm_backward_links_to(e);

		if (e->prev == NULL){
			current_state = e->next;
		} else {
			entry* before = e->prev;
			entry* after = e->next;
			if (after == NULL){
				before->next = NULL;
			} else {
				before->next = after;
				after->prev = before;
			}			
		} 	

		entry_free(e);
		// _inspect_state();
	}
}

int element_compare(const void* e1, const void* e2){
	element* element_1 = (element*) e1;
	element* element_2 = (element*) e2;
	return element_1->value - element_2->value;
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
	element* last_word;
	element* cursor = e->values;

	int new_size = 0;
	for (int i = 0; i < e->length; i++){
		printf("Printing cursor: %d\n", cursor->value);
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
	element* elem = e->values+index;
	if (elem->type == INTEGER){
		printf("Value at index %d in entry with key %s is: %d\n", index, e->key, elem->value);
	} else {
		printf("Value at index %d in entry with key %s is: %s\n", index, e->key, elem->entry->key);
	}
}

void entry_pluck(entry* e, int index){
	entry_pick(e, index);
	// entry_delete(e);
	element* elem_to_remove = e->values+index;
	// _inspect_state();

	// Remove backlinks to e for entries that link back to e due to e containing elem_to_remove
	if (elem_to_remove->type == ENTRY){
		_entries_remove(elem_to_remove->entry->backward, &elem_to_remove->entry->backward_size, e);
		_entries_remove(e->forward, &e->forward_size, elem_to_remove->entry);
	}

	// _inspect_state();

	// Remove element from element array
	e->length--;
	memcpy(elem_to_remove, elem_to_remove+1, (e->length-index)*sizeof(element));
	e->values = realloc(e->values, e->length*sizeof(element));

	// Remove e's forward reference to element 


	// Remove the value at the index
	// free(e->values+index);
	 //? Can realloc be used to shrink the amount of memory allocated? YES
	//! Don't forget to realloc the exact amount in bytes, i.e. forgot to multiply by sizeof(element);
	// TODO: Fix memory leak from losing the reference to the element that has been deleted, actuall no because element array here i.e. if you remove the element with realloc it's gone, only do it for element*
}

void entry_pop(entry* e){
	if (e->length > 1){ // TODO: Check if you can pop an entry with only 1 value?
		entry_pluck(e, 0);
	} else {
		printf("Cannot pop an entry that has only 1 or 0 values \n");
	}
}

void list_keys(){
	entry* cursor = current_state;
	while (cursor != NULL){
		printf("%s\n", cursor->key);
		cursor = cursor->next;
	}
}

void list_entries(){
	entry* cursor = current_state;
	while (cursor != NULL){
		printf("%s ", cursor->key);
		entry_tostring(cursor); //? Should this display the links as a letter or the elements in that next link?
		cursor = cursor->next;
	}
}

void list_snapshots(){
	snapshot* cursor = first_snapshot;
	int counter = 0;
	while (cursor != NULL){
		printf("%d ", cursor->id);
		cursor = cursor->next;
		counter++;
	}
	if (counter > 0){
		printf("\n");
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

entry* entry_copy(entry* e){
	if (e->is_simple){
		entry* copy = calloc(1, sizeof(entry));
		element* copy_values = calloc(e->length, sizeof(element));
		
		// Copy memory from e for the entry to copy
		memcpy(copy, e, sizeof(entry));

		// Create deep copies of values array
		memcpy(copy_values, e->values, sizeof(element)*e->length);
		copy->values = copy_values;
		return copy;
	} else {
		printf("Cannot copy an entry that is not simple!\n");
		// TODO: Ensure forward and backward references are copied over to the new array.
	}

	return NULL;
	
	
	// TODO: For general entries, think about how you would create ensure these entries are linked to the references? 
	//? IDEA: When you encounter a forward link, call entry copy recursively and make attach the return value to the forward link
	//? IDEA: When the returned entry comes back add its backward link to current entry
	//? How to avoid double copying entries? i.e. if we already copied all in one chain, don't copy that chain again -> think of edge case where you have a->b, a->c, b, c->d
		//? Store the keys we have already computed? in an array and check in function to see if key is already in (n^2 time though)
		//? Store id variable for each entry in the list -> add to struct? and then use marked array i.e. constant time lookup for whether you double copied
		//? Apply the same idea to deleting entries.
		//! If you go this route, you will need to refactor delete and append commands by implementing a global variable for storing the number of nodes in the current state.
}


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
	// snapshot* cursor = first_snapshot;
	
	// if (cursor == NULL){
	// 	first_snapshot = snap;
	// 	return;
	// }

	// while (cursor->next != NULL){
	// 	cursor = cursor->next;
	// }
	// cursor->next = snap;

}

snapshot* snapshot_create(entry* entries){
	entry* cursor = entries;
	entry* entries_copy = NULL;
	entry* previous = NULL;
	// TODO: Set pointer to start of copied entries
	while (cursor != NULL){
		entry* copy = entry_copy(cursor); //! might have corrupted top size with poor management of entry_copy()
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

	snapshot* new_snapshot = calloc(1, sizeof(snapshot));
	new_snapshot->entries = entries_copy;
	if (last_snapshot != NULL){
		new_snapshot->id = last_snapshot->id+1; //TODO: Check if this is the correct id naming system or if you should have global var tracker.
	} else {
		new_snapshot->id = 0;
	}
	return new_snapshot;
}

void entries_free(entry* entries){
	// Free up all the entries in the snapshot
	entry* cursor = entries;
	entry* old;
	while (cursor != NULL){
		old = cursor;
		cursor = cursor->next;
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
	entries_free(snap->entries); //! Segfault line
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

	free(snap);

}

void snapshot_rollback(snapshot* snap){
	snapshot* cursor = last_snapshot; 
	snapshot* old;

	entries_free(current_state);

	while (cursor != NULL){
		if (cursor == snap){
			break;
		}
		old = cursor;
		cursor = cursor->prev;
		snapshot_drop(old);
	}

	// Clear the current state (//TODO: Implement deep delete function alongside deep copy function)
	current_state = cursor->entries;
}

//! When you checkout to an entry and then drop that entry whilst you're in the snapshot -> you get junk values when you list entries.
void snapshot_checkout(snapshot* snap){
	current_state = snap->entries; //TODO: Deep copy over the snapshot into the current state?
}

void snapshot_save(){
	// TODO: Deep copy current state into a snapshot
	snapshot* new_snapshot = snapshot_create(current_state); //! Error coming from snapshot_create();
	snapshot_append(new_snapshot);
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
	entry_delete(entry_get(key));
	// Find all the elements with the same key in snapshots
	entry* original_state = current_state;
	snapshot* cursor = first_snapshot;
	while (cursor != NULL){
		// Make create generalised entry_get/delete for snapshots -> don't need to set them as current state?
		current_state = cursor->entries;
		entry_delete(entry_get(key)); //! Side effect -> changes current_state.
		cursor->entries = current_state;
		cursor = cursor->next;
	}
	current_state = original_state;
}
void args_get_values(){

}



int main(void) {

	char line[MAX_LINE];
	// snapshot* snapshots; // TODO: Add support for creating snapshots of the current state (deep copies) for simple entries;
	current_state = NULL; //? Good practice to always intialise pointers to null to avoid garbage values


	//! I can malloc a struct for test but it is not letting me do it in the function?
	entry* test = malloc(sizeof(entry));
	test->key[0] = 'c';

	while (true) {
		printf("> ");
	

		//? Quit if the user does not enter anything as a command -> don't forget to free memory afterwards
		if (NULL == fgets(line, MAX_LINE, stdin)) {
			printf("\n");
			command_bye();
			return 0;
		}


		// Process multiple arguments to the command line
		char* word = strtok(line, " \n\r"); 
		char** args = calloc(MAX_KEY, sizeof(char*)); //! Failing to succesfully allocate sufficient memory to a datatype leads to undefined behaviour!! E.g. corrupted top size can't malloc properly in the future
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
		
		if (strcasecmp(command_type, "SET") == 0){
			entry* e = entry_create(args+1, args_size-1); //! TODO: Fix issue within entry_create
			entry_set(e);
		} else if (strcasecmp(command_type, "PUSH") == 0){
			entry* e = entry_get(args[1]);
			if (e == NULL) {
				printf("No entry with key: %s was found!\n", args[1]);
			} else {
				entry_push(e, args+2, args_size-2); //TODO: make it so teh push is not atoi
			}
		} else if (strcasecmp(command_type, "APPEND") == 0){
			entry* e = entry_get(args[1]); //? +1 so that we don't include the command in the arguments used to build the entry
			if (e == NULL){
				printf("No entry with key: %s was found!\n", args[1]);
			} else {
				entry_append(e, args+2, args_size-2);
			}
		} else if (strcasecmp(command_type, "GET") == 0){
			entry* e = entry_get(args[1]);
			if (e == NULL) {
				printf("No entry with key: %s was found!\n", args[1]);
			} else {
				entry_tostring(e);
			}
		} else if (strcasecmp(command_type, "DEL") == 0){
			entry* e = entry_get(args[1]);
			if (e == NULL) {
				printf("No entry with key: %s was found!\n", args[1]);
			} else {
				entry_delete(e);
			}
		} else if (strcasecmp(command_type, "MIN") == 0){
			entry* e = entry_get(args[1]);
			entry_min(e);
		} else if (strcasecmp(command_type, "MAX") == 0){
			entry* e = entry_get(args[1]);
			entry_max(e);
		} else if (strcasecmp(command_type, "SUM") == 0){
			entry* e = entry_get(args[1]);
			entry_sum(e);
		} else if (strcasecmp(command_type, "LEN") == 0){
			entry* e = entry_get(args[1]);
			entry_len(e);
		} else if (strcasecmp(command_type, "SORT") == 0){
			entry* e = entry_get(args[1]);
			entry_sort(e);
		} else if (strcasecmp(command_type, "REV") == 0){
			entry* e = entry_get(args[1]);
			entry_reverse(e);
		} else if (strcasecmp(command_type, "UNIQ") == 0){
			entry* e = entry_get(args[1]); //TODO: add input verification and also checking that entry exists
			entry_unique(e);
		} else if (strcasecmp(command_type, "PLUCK") == 0){
			entry* e = entry_get(args[1]);
			if (!string_isnumeric(args[2])){
				printf("Cannot pluck an index that is not numeric!\n");
			} else {
				int index = atoi(args[2]);
				entry_pluck(e, index);	
			}
		} else if (strcasecmp(command_type, "PICK") == 0){
			entry* e = entry_get(args[1]);
			if (!string_isnumeric(args[2])){
				printf("Cannot pick an index that is not numeric!\n");
			} else {
				int index = atoi(args[2]);
				entry_pick(e, index);	
			}
		} else if (strcasecmp(command_type, "POP") == 0){
			entry* e = entry_get(args[1]);
			entry_pop(e);
		} else if (strcasecmp(command_type, "FORWARD") == 0){
			entry* e = entry_get(args[1]);
			entry_forward(e);
		} else if (strcasecmp(command_type, "BACKWARD") == 0){
			entry* e = entry_get(args[1]);
			entry_backward(e);
		} else if (strcasecmp(command_type, "PURGE") == 0){
			char* key = args[1];
			purge(key);
		}  else if (strcasecmp(command_type, "TYPE") == 0){
			entry* e = entry_get(args[1]);
			if (e == NULL) {
				printf("No entry with key: %s was found!\n", args[1]); //TODO: Use function pointers (create wrapper function) to call any functions that use the get entry method.
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
		 	snapshot_save();
		} else if (strcasecmp(command_type, "DROP") == 0){ //! Segfaults
			if (!string_isnumeric(args[1])){
				printf("You must provide a valid ID for a snapshot!\n");
			} else {
				int id = atoi(args[1]);
				snapshot* snap = snapshot_get(id);
				snapshot_drop(snap);
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
			entries_free(current_state);
			return -1;
			// TODO: Free the memory that has been allocated to snapshots and entries
		}

		//! Make sure to free the arguments after you have finished operating on them;
		// isdigit()
		
		// for (int i = 0; i < args_num; i++){
		// 	printf("Argument %d: %s\n", i, args[i]);
		// }

		//
		// TODO
		//





  	}

	return 0;
}
