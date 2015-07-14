







/*
 * DEPRECATED
 */


size_t construct_jmptblOLD(SYMBOL_T symbol_count, SYMBOL_T *alpha_map, STATE_T **jmp_tbl_p)
{
    const int null_state = 0;
    const int entry_state = 1;
    STATE_T original_sink_state = 0, old_state = 0;;
    STATE_T trans_table_state_count = 0;

    if (symbol_count < 2)
        return 0; /* Error - need null symbol + 1 more */

    /* DEBUG */
    for (map<transition, state>::const_iterator i = trans_table.begin(); i != trans_table.end(); ++i)
    {
        cout << "Trans[" << (i->first).first << ", " << (i->first).second << "] = " << i->second << endl;

    }

    /* test to see if we have exceeded the limit of the jmp_tbl size
     * Allocate the space needed for our jump table and set each byte to the existing sink state.
     * We will eventually swap the existing sink state with the zero'th (existing entry state).
     */
    size_t trans_table_size = trans_table.size();




// TODO for future use this is how they access the trans table: //trans_table[from][to];



    /* Since we donn't know what symbols from the entire alphabet are present, we assume the sink state to defined:
     * a contiguous series of trans_table elements of the same src state where each dst state is itself (src). And
     * that the contiguous region is capped on either end by the next states, respectively.
     */
    size_t j = 0, k = 0;
    char sink_found = FALSE;
    int first_run = 1;
    /* Test if all input symbols for a given state map to itself. If so, it is the  existing sink   */
    for (map<transition, state>::const_iterator i = trans_table.begin(); i != trans_table.end(); ++i) {
        if (first_run) {
            --first_run;
            old_state = (i->first).first;
            ++trans_table_state_count; /* First state */
        }
        if ((i->first).first != old_state) {
            /* entering a new state */
            ++trans_table_state_count;
            if (!sink_found) {
                if (!j) {
                    original_sink_state = old_state;
                    sink_found = TRUE;
                    /* since 0 is a valid state, we need to use a flag to mark this event
                     * let the loop keep running afterwards so we can determine the number of states
                     */

                } /* else j != 0 */
                j = 0;
            }
        }
        old_state = (i->first).first;
        if ((i->first).first != i->second)
            ++j;
    }
    /* We evaluate whether or not we have found the sink at the start of every state change  when walking the
     * transition table, therefore, the last set of table elements that formed last src state might have been
     * the sink.
     */
    if (!sink_found && j) {
        return 0;
    }
    /* DEBUG */
    cout << "SINK: " << (size_t) original_sink_state << endl;


    size_t jmp_tbl_size = symbol_count * trans_table_state_count  * sizeof(STATE_T);
    *jmp_tbl_p = (STATE_T *) calloc(symbol_count * trans_table_state_count, sizeof(STATE_T));
    STATE_T *jmp_tbl = *jmp_tbl_p;
    if ( !jmp_tbl) {
        return 0; /* Error */
    }

    /* i := ((src_state, symbol), dst_state)*/
    j = 0;
    for (map<transition, state>::const_iterator i = trans_table.begin(); i != trans_table.end(), k < jmp_tbl_size; ++i, ++k, j = 0) {
        //for (i = 0; i < trans_table_size; ++i ){

        while ((k < jmp_tbl_size) && (j < symbol_count) && (alpha_map[(i->first).second] != k % symbol_count)) {
            jmp_tbl[k] = original_sink_state;
            ++k; ++j;
        }
        if (j >= symbol_count) {
            /* Error - invalid symbol encountered */
            k = jmp_tbl_size + 1;
            break;
        } else if (k >= jmp_tbl_size) {
            /* Error - missing symbol encountered */
            k = jmp_tbl_size + 1;
            break;
        } else {
            /* assert (alpha_map[(i->first).second] == k % symbol_count); */
            jmp_tbl[k] = (STATE_T) i->second;
            continue;
        }
    }
    if (k != jmp_tbl_size) {
        /* Error  handler */
        free(jmp_tbl);
        return 0;
    }






/* DEBUG */

    for (k = 0; k < jmp_tbl_size; ++k) {
        cout << "jmp_tbl1[" <<  (k / symbol_count) << ", " << (k % symbol_count) << "] = " << (int) jmp_tbl[k] << endl;
    }


    /* XOR swap entry state and existing sink state, for each input symbol entry */
    for (k=0; k < symbol_count; ++k) {
        jmp_tbl[(null_state * symbol_count) + k]          ^= jmp_tbl[(original_sink_state * symbol_count) + k];
        jmp_tbl[(original_sink_state * symbol_count) + k] ^= jmp_tbl[(null_state * symbol_count) + k];
        jmp_tbl[(null_state * symbol_count) + k]          ^= jmp_tbl[(original_sink_state * symbol_count) + k];

    }

    /* XOR swap entry state (now in the original sink state) and existing state #1, for each input symbol
     * entry
     */
    for (k=0; k < symbol_count; ++k) {

        jmp_tbl[(original_sink_state * symbol_count) + k]   ^= jmp_tbl[(entry_state * symbol_count) + k];
        jmp_tbl[(entry_state * symbol_count) + k]           ^= jmp_tbl[(original_sink_state * symbol_count) + k];
        jmp_tbl[(original_sink_state * symbol_count) + k]   ^= jmp_tbl[(entry_state * symbol_count) + k];

    }


/* DEBUG */

    for (k = 0; k < jmp_tbl_size; ++k) {
        cout << "jmp_tbl_postswap[" <<  (k / symbol_count) << ", " <<  (k % symbol_count) << "] = " << (int) jmp_tbl[k] << endl;
    }


    /* The following has occured:
     * 0 (entry)    -> 1
     * 1            -> sink
     * sink         -> 0
     *
     * Now update all of the index values to match the state label changes made.
     */
    for (k = 0; k < jmp_tbl_size; ++k) {
        if (jmp_tbl[k] ==  null_state)
            jmp_tbl[k] = entry_state;
        else if (jmp_tbl[k] == entry_state)
            jmp_tbl[k] =  original_sink_state;
        else if (jmp_tbl[k] == original_sink_state)
            jmp_tbl[k] = null_state;

    }


/* DEBUG */

    for (k = 0; k < jmp_tbl_size; ++k) {
        cout << "jmp_tbl[" << (k / symbol_count) << ", " << (k % symbol_count) << "] = " << (int) jmp_tbl[k] << endl;
    }

    return jmp_tbl_size;
}








ORIGINAL README.txt CONTENTS
============================
A C++ implementation of a regular expression recognizer. The recognizer constructs an NFA from a regular expression (with a simplified syntax), converts it into a DFA, and simulates the DFA on the input string.

The main function is in regex_parse.cpp

To compile/build, run:
g++ -ansi -Wall -pedantic -o regex *.cpp

The code is ANSI C++ and should compile cleanly on any standard C++ compiler.


This code is in the public domain
Eli Bendersky (eliben@gmail.com)
