TODO
====
GENERAL
-------
- create sub-project makefiles
- MAYBE implement a zero'th policy that is always "(\0)*"


OMNIUS
------
- remove symbol count from communication, et al.
- add memuse statistics to process struct and update it upon alloc/dealloc
- should the msg queue be cleared on startup?

OMNIUS-CLI
----------




REGEX_PARSE
-----------
- put checks in to ensure that the FSM transition table generated has at most max(STATE) states before converting to a jmp table.



NOTES
=====
FSM
---
If you are going to change the integer representation of the null sink state to anything but zero beware that the entire 
system relies on computation that uses the value zero both explicitly and implicitly.


The size of the jump table can have up to 256 (sizeof(STATE_T) src state entry groupings. This can change by modifying
the STATE_T macros.


The alphabet for the FSMs is fixed at {0,1, ... , 254, 255} (sizeof(SYMBOL_T)). This can be expanded by modifying
the SYMBOL_T macros, and the code that parses the regex to handle the larger symbol size.

Jump Table
----------
jmp_tbl entries are indecies back into the table. This was used instead of memory addresses to economize space and
reduce the number of calculations to fewer places.

A regular expression can be turned into an recognizer FSM, which can then be expressed as a directed graph, where the
nodes are the states, and the edges are transitions ofr a given symbol. And ever node other than a single null-sink,
is an accepting state, awaiting the (possibly) next input.

We can represent this as an enormous jmp table. The table is composed of records for each state and each record is
composed of value's, indexed by a symbol (where the set of symbols runs contiguous 0 through some N(=|symbols explicitly
used index|, and the zero'th symbol is reserved as known transition to the null sink state). Each value is an integer 
record index back into the table, for the respective symbol.

For example,the regular expression (RW+(RR)*)+
We first condense the alphabet mapping it back to the integers.
NULL => 0
R    => 1
W    => 2

Now we can describe the graph we could generate:
STATE    SYM     NXT_STATE
0        0       0
0        1       0
0        2       0
1        0       0
1        1       1
1        2       0
2        0       0
2        1       3
2        2       2
3        0       0
3        1       4
3        2       2
4        0       0
4        1       3
4        2       0

Now we use the third column to construct our jmp table by simply laying it out in a contiguous piece of memory. The symbol's are encoded into the positioning.

The above FSM would have a symbolcount of 3, however, the input symbol alphabet omnius operates over is really [0-255] (but it will be constrained by the regex_parse engine only acceptingthe subset [A-Za-z] it's alphabet). 

A ragasm object uses this jmp table along with the current state and the next symbol as input to obtain the index
of the next state and set the current state to that.


Memory
------
Memory is safeguarded against reading old values from previous allocations by setting up the following guarantees: 
Since you can only read allocated nodes, and allocated nodes are guaranteed to be zero'd out, you cannot read residual 
data in memory leftover from previous allocations at the same region.


Host-Side
---------
The host computer uses a custom device driver to communicates with the device. A host-side binary that wants to use omnius must be compiled against a shim which exports functions for communicating with the device driver, as well as providing memory interception and modification . The developer 


