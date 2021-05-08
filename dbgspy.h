/*
D-Bug-Spy (DBGSpy) -- A live execution monitor implemented as a
                                                                                        (High/Application)-Level Debugger library.
*/

/* ********************************************************* *
 * By Yuriy Y. Yermilov aka (binaryONE) cyclone.yyy@gmail.com
 *
 * website: code.computronium.io
 *
 * THIS SOFTWARE IS PROVIDED BY THE OWNER ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE OWNER
 * PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ********************************************************** */
/*
DBGSpy is NOT a source level debugger, it is a programming interface
that is implemented as a live execution monitor of a given program.
It was designed for the programmer to quickly and easily monitor
code as it is being run.

It is a '(High/Application) Level Debugger library'.

The output in the [dbgspy.out] file is in the form of a simple
interpreted syntax, or straight output from the user setup functions.
The reason for making it so, was to allow easy parsing/filtering/editing
etc... to be performed by any text-processing tool available.

The session [id]# is needed so that a single [dbgspy.out] file
can be used multiple times for the same program (even different
ones, however that's highly not-recomended except for very
special cases). This is used to id-entify the debugging
log entries for each session. Each [session] in essence represents
a single run of a program.


TODO (work in progress):

A java [GUI front-end] is added for watching, graphing, editing, saving
etc... of everything within [dbgspy.out] file. The GUI is written in
java to maintain a unanimous cross-platform look and capabilities.
The GUI front-end is suplementary as everything can be done in console.

What is recorded, flagged or monitored by the dbgspy interface is
up to the user.

By default the dbgspy is in interactive mode, which starts from the first
function that calls the dbgspy service. When the service does what it was
setup to do, it then waits for further input from the user. The input can
be any of the defined commands. If the user decides to quit the
interactive session, dbgspy will return control to the program instruction
immediately following it. During the program run-time, dbgspy will be
activating everytime a function desiring its service calls it.

*/

/*
All functions in implementation return 0 on success NOT 0 on failure,
unless otherwise specified.

All memory is dynamically allocated/deallocated.

A 'target' in dbgspy is used and meant as a synonym for the word 'function'.

'target' names and 'id's are unique, just like functions are unique.
*/

#include <sys/stat.h>

/*
Records for data target.
*/

struct y3_dbg_type
{
  char* name;                 // type name is unique
  int (*proc_my_type)(void*); // for extensibility
};

struct y3_dbg_byte
{
  char b1, b2, b3, b4;
};

struct y3_dbg_element
{
  char* name;
  void* val;
  //
  // Each element can have a state
  //

  //
  // If this flag is set to 1 then the value is dereferenced as a pointer of
  // type. Otherwise the value is used absolutely.
  //
  int is_ptr;
  //
  // These are handlers for accessing the complex user defined types or
  // primitive types
  //
  struct y3_dbg_type type;
};

struct y3_dbg_xstate
{
  int id;
  int n_elements;
  struct y3_list* element;
};

/*
A list of 'actions' flags are provided below,
they can be used together or alone.
*/

struct y3_dbg_actions
{
  unsigned int show_all : 1;
  unsigned int show_selected : 1;
  unsigned int show_allstates : 1;
  unsigned int show_contexts : 1;
  unsigned int show_allcontexts : 1;
};

/*
Targets are kept as an array since they will not be
deleted, also it is natural to hash them.
*/

struct y3_dbg_target
{
  int id;       /* unique id # */
  size_t n_len; /* length of function name */
  char* name;   /* name of function (target) */
  /*
  Each target is backed up as many times as it is
  referenced in y3_dbgspy_target(). This is done
  so at any point execution can be rolled back
  to a certain state.

  The only things that change during execution are
  the element values.

  However they too are recorded and their values are
  also recorded for that particular state.

  A state is created for each function call, and each
  state sets the element values for that function

    at the time is it
  called.
  */

  struct y3_list* xstate;
  int n_elements;
  int n_states;
};

struct y3_dbg_context
{
  struct y3_dbg_context* parent;
  struct y3_list* element;
  char* name;
  int id;
  int n_elements;
};

struct y3_dbg
{
  struct y3_list* context;
  struct y3_list* target;
  struct y3_dbg_actions flags; /* flags on how to treat this target */
  int n_targets;
  int n_contexts;
  short session_id; /* identification # of the current session */
  struct stat st;   /* dbgspy.out file statistics */
  int fd;           /* dbgspy.out file descriptor */
  int written;      /* bytes written to file in this session */
};

#define Y3_DBGSPY_PTR 1
#define Y3_DBGSPY_VAL 0

#define Y3_DBGSPY_MAX_NAME_LEN 1024

typedef enum
{
  Y3_DBGSPY_SUCCESS,
  Y3_DBGSPY_UNKNOWN_ERROR,
  Y3_DBGSPY_TARGET_VARIABLE_COUNT_LESS_THAN_ZERO_ERROR,
  Y3_DBGSPY_TARGET_VARIABLE_COUNT_CHANGED_ERROR,
  Y3_DBGSPY_TARGET_DNE_ERROR,
  Y3_DBGSPY_SPECIFIED_TARGET_NAME_IS_NULL_ERROR,
  Y3_DBGSPY_SPECIFIED_CONTEXT_NAME_IS_NULL_ERROR,
  Y3_DBGSPY_CONTEXT_DNE_ERROR,
  Y3_DBGSPY_DUPLICATE_VARIABLE_NAME_ERROR,
  Y3_DBGSPY_VAR_DNE_ERROR,
  Y3_DBGSPY_TYPE_MISSMATCH_ERROR,
  Y3_DBGSPY_NOT_INITIALIZED_ERROR,
  Y3_DBGSPY_INVALID_OUTPUT_FILENAME_ERROR,
  Y3_DBGSPY_CANNOT_OPEN_OUTPUT_FILE_ERROR,
  Y3_DBGSPY_CANNOT_GET_OUTPUT_FILE_STATS_ERROR,
} Y3_DBGSPY_ERRORS;

typedef struct y3_dbg_target* DBGT;
typedef struct y3_dbg_context* DBGC;
typedef struct y3_dbg_xstate* DBGS;
typedef struct y3_dbg_element* DBGV;

#define T_SIZE sizeof(struct y3_dbg_target)
#define G_SIZE sizeof(struct y3_dbg_context)
#define S_SIZE sizeof(struct y3_dbg_xstate)
#define V_SIZE sizeof(struct y3_dbg_element)

struct y3_dbg*
y3_dbgspy_init(char* file, int* error);
struct y3_dbg_target*
y3_dbgspy_target(struct y3_dbg* T, char* name, int* error, int n, ...);
int
y3_dbgspy_openoutfile(struct y3_dbg* T, char* name);
int
y3_dbgspy_get_target_id(struct y3_dbg* T, char* name, int* error);
struct y3_dbg_context*
y3_dbgspy_new_context(struct y3_dbg* T, char* name, int* error);
void
y3_dbgspy_new_context_elements(struct y3_dbg* T,
                               char* name,
                               int* error,
                               int n,
                               ...);
struct y3_dbg_element*
y3_dbgspy_get_element_in_state(struct y3_dbg_xstate* T,
                               char* name,
                               int* error);
void*
y3_dbgspy_get_target_element_val(struct y3_dbg* T,
                                 char* targ_name,
                                 char* element_name,
                                 int* error);
struct y3_dbg_element*
y3_dbgspy_get_target_element_inst(struct y3_dbg* T,
                                  char* targ_name,
                                  char* element_name,
                                  int* error);
void
y3_dbgspy_set_target_element_val(struct y3_dbg* T,
                                 char* targ_name,
                                 char* element_name,
                                 void* var_data,
                                 int is_ptr,
                                 int* error);
struct y3_dbg_target*
y3_dbgspy_get_target(struct y3_dbg* T, char* name, int* error);
struct y3_dbg_context*
y3_dbgspy_get_context(struct y3_dbg* T, char* name, int* error);
void
y3_dbgspy_set_cotext_element_val(struct y3_dbg* T,
                                 char* context_name,
                                 char* element_name,
                                 void* var_data,
                                 int is_ptr,
                                 int* error);
struct y3_dbg_element*
y3_dbgspy_get_context_element_inst(struct y3_dbg* T,
                                   char* context_name,
                                   char* element_name,
                                   int* error);
void*
y3_dbgspy_get_context_element_val(struct y3_dbg* T,
                                  char* context_name,
                                  char* element_name,
                                  int* error);
void*
y3_dbgspy_target_add_element(struct y3_dbg_target* T,
                             int* error,
                             int n,
                             ...);
