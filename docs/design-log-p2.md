# Design Log — Project 2

## Growth factor and amortized cost

For the Conversation array, a doubling strategy was used. The capacity changes from zero to one when the first message is added,
then doubling after the array reaches capacity. When capacity is reached, the current elements are copied into a larger allocated Message array. 
Then the old array is deleted. This allows Conversation to keep storing more messages without requiring a fixed conversation size. As capacity increases, 
resizing happens less and less frequently relative to the growth of the array. For n insertions the number of elements copied follows the geometric series 
1 + 2 + 4 ... + 2^m, where 2^m is less than n. The sum of that series is 2^(m+1) - 1, which is less than 2n. This means the total amount of copying across n appends 
is O(n). Therefore, when that work is spread across all n appends, the amortized cost of each append is O(1).


## Rule of Five evidence

Within Conversation there is a dynamically allocated Message array, which Conversation must handle correctly during its Rule of Five operations.
The destructor releases the array using delete[]. The copy constructor allocates another array and then copies each Message, which makes a new copy. 
Copy assignment checks for self-assignment, then releases the existing allocation and then creates a new deep copy. 
The move constructor and move assignment have different behavior; rather than copying the messages they control the original pointer, capacity, and size.
The source object was then reset to a null pointer and capacity and size set to zero. My tests confirm that the copies have different addresses, 
while the move tests preserve the original address and leave the source empty. 

## Sentinel scanner: bounded pending_ proof

The scanner only needs to keep the characters that could still become the beginning of the sentinel. If the sentinel length is A while the buffer is length B, 
then if no sentinel is found, the scanner will release B - A + 1 characters. This leaves A - 1 characters in the pending buffer. If B is already smaller than A, 
then the buffer is already smaller than that limit. pending_ starts empty, so it starts below the A - 1 limit. After every feed, it will either contain A - 1 characters, 
less than A - 1 characters, or zero characters if the sentinel was found. Therefore, pending_ will never contain more than sentinel_.size() - 1 characters after a feed. 
This suffices because a sentinel that crosses boundaries of the chunks can only begin in the last A - 1 characters of the previous chunk. 


## What I would change differently

I would change the design of the sentinel to separate out the conversation control from the conversation text. I think a better idea would be to design a client that emits structured events.
Then the harness would stop based on the event type instead of just interpreting the text. This could eliminate sentinel collisions and make the interface easier to build off of.
I would also make debugging easier so that the program could distinguish between the failures in the text stream and the logical errors. I would make the harness more reusable,
and future versions could add additional event types without inventing more special strings and needing to update the scanner every time. 
If I were allowed to redesign the project, I would change the testing expectations to be something that was explicitly taught before assigning a project on them.