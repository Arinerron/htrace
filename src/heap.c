#include "options.h"
#include "heap.h"
#include "logging.h"
#include "debugger.h"

uint64_t MALLOC_COUNT = 0;
uint64_t FREE_COUNT = 0;
uint64_t REALLOC_COUNT = 0;

uint64_t break_ats[MAX_BREAK_ATS];

// initialize the chunk meta if first time
void chunk_init() {
    // XXX deprecated, TODO delete this
}



// see if it's time to pause
void check_oid(uint64_t oid, int prepend_newline) {
    // TODO
    if (!args_parsed_yet) {
        //parse_arguments(); // TODO
        args_parsed_yet = 1;
    }

    // try reading from params second
    int should_break = 0;
    for (int i = 0; i < MAX_BREAK_ATS; i++) {
        if (break_ats[i] == oid) {
            should_break = 1;
        }
    }

    // now actually break if necessary
    if (should_break) {
        if (prepend_newline) log("\n"); // XXX: this hack is because malloc/realloc need a newline before paused msg
        log(COLOR_ERROR "    [   PROCESS PAUSED   ]\n");
        log(COLOR_ERROR "    |   * attaching GDB via: " COLOR_ERROR_BOLD "/usr/bin/gdb -p %d\n" COLOR_RESET, CHILD_PID);
        if (prepend_newline) log("    "); // XXX/HACK: see above

        // launch gdb
        _remove_breakpoints(CHILD_PID);
        ptrace(PTRACE_DETACH, CHILD_PID, NULL, SIGSTOP);

        char buf[10+1];
        snprintf(buf, 10, "%d", CHILD_PID);
        char *gdb_path = "/usr/bin/gdb";
        char *args[] = {gdb_path, "-p", buf, NULL};
        if (execv(args[0], args) == -1) {
            fatal("failed to execute debugger %s: %s (errno %d)", args[0], strerror(errno), errno);
            abort();
        }
        //raise(SIGSTOP);
    }
}


// returns the current operation ID
uint64_t get_oid() {
    uint64_t oid = MALLOC_COUNT + FREE_COUNT + REALLOC_COUNT;
    ASSERT(oid < (uint64_t)0xFFFFFFFFFFFFFFF0LLU, "ran out of oids"); // avoid overflows
    return oid;
}


void show_stats() {
    uint64_t unfreed_sum = 0;

    /* // TODO: convert to BST code
    Chunk cur_chunk;
    int _prefix = 0; // hack for getting newlines right
    for (int i = 0; i < MAX_CHUNKS; i++) {
        cur_chunk = chunk_meta[i];
        if (cur_chunk.state == STATE_MALLOC) {
            if (OPT_VERBOSE) {
                if (!_prefix) {
                    _prefix = 1;
                    log("\n");
                }
                log(COLOR_ERROR "* chunk malloc'd in operation " SYM COLOR_ERROR " was never freed\n", cur_chunk.ops[STATE_MALLOC]);
            }
            unfreed_sum += CHUNK_SIZE(cur_chunk.size);
        }
    }
    */

    if (unfreed_sum && OPT_VERBOSE) log(COLOR_LOG "------\n");
    log(COLOR_LOG "Statistics:\n");
    log("... total mallocs: " CNT "\n", MALLOC_COUNT);
    log("... total frees: " CNT "\n", FREE_COUNT);
    log("... total reallocs: " CNT "\n" COLOR_RESET, REALLOC_COUNT);

    if (unfreed_sum) {
        log(COLOR_ERROR "... total bytes lost: " SZ_ERR "\n", SZ_ARG(unfreed_sum));
    }

    log("%s", COLOR_RESET);
}
