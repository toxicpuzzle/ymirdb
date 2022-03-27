comp2017 - assignment 2
Tim Yang
yyan0195

Overview of memory model

Ymirdb is created using a linked-list memory model, where all entries in each state are referenced by a pointer
to the latest entry inserted into the database. Each entry contains dynamic arrays storing the its forward and 
backward references, which are modified everytime an operation such as pluck, set, pop, purge, del, append, push 
is performed. Snapshots are stored using a similar linked-list model, where each snapshot is referenced by a pointer
to the latest snapshot created.

Implementation of complex functions

To compute the results for sum, len, max, min, and copy all entries in a snapshot by only visiting each entry at most
once, DFS is employed. For example, the recursive call to _calculate_sum() calculates the sum of all combined forward
entries (including repeated sums from repeated forward entries) for any general entry, and the current local sum 
(values directly referenced by the current entry) is added to this value and stored in the current entry as (sum_forward)
so that other entries that would later call _calculate_sum() on itself would avoid repeat calculations. The other entries
know that an entry has a valid sum_forward value if the has_visited attribute is flagged as true. This DFS method is 
generalised to implement the len, max, and snapshot-related functions.

> Copying snapshots

Each entry stores a copy_reference pointer to an entry struct, where a reference to the copied object is temporarily 
stored. This reference is used by the copying algorithm to ensure forward and backward links of all copied objects
are linked together. 

test