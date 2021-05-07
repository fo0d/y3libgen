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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <ctype.h>

#include "dbgspy.h"
#include "hash.h"
#include "list.h"
#include "str.h"

/*
This function initializes DBGSpy.

file = name of dbgspy.out file.
*/

struct y3_dbg *
y3_dbgspy_init(char *file, int *error) {
  struct y3_dbg *T;
  *error = Y3_DBGSPY_SUCCESS;

  T = (struct y3_dbg *) malloc(sizeof(struct y3_dbg));

  if (file == NULL) {
    char *p = file;
    for (; p && *p; p++) {
      if (!isascii(*p)) {
        *error = Y3_DBGSPY_INVALID_OUTPUT_FILENAME_ERROR;
        return NULL; /* and raise exception */
      }
    }
  }

  if (y3_dbgspy_openoutfile(T, file)) {
    *error = Y3_DBGSPY_CANNOT_OPEN_OUTPUT_FILE_ERROR;
    return NULL; /* and raise exception */
  }
  if (fstat(T->fd, &T->st)) {
    *error = Y3_DBGSPY_CANNOT_GET_OUTPUT_FILE_STATS_ERROR;
    return NULL; /* and raise exception */
  }

  T->target = T->context = NULL;

  T->n_targets = 0;
  T->n_contexts = 0;

  return T;
}


/*
This function looks through the hash table of targets
in T and matches it to an id.

If 'name' does not exist -1 is returned.
*/

int
y3_dbgspy_get_target_id(struct y3_dbg *T, char *name, int *error) {
  struct y3_list *link;
  *error = Y3_DBGSPY_SUCCESS;

  if (T == NULL) {
    *error = Y3_DBGSPY_NOT_INITIALIZED_ERROR;
    return -1;
  }

  if (name == NULL) {
    *error = Y3_DBGSPY_SPECIFIED_TARGET_NAME_IS_NULL_ERROR;
    return -1;
  }

  if (T->target) {
    link = y3_list_link_id(T->target, name);
    if (link != NULL) {
      return ((DBGT) link->data)->id;
    }
  }

  *error = Y3_DBGSPY_TARGET_DNE_ERROR;
  return -1;
}

struct y3_dbg_target *
y3_dbgspy_get_target(struct y3_dbg *T, char *name, int *error) {
  struct y3_list *link;
  struct y3_dbg_target *t;
  *error = Y3_DBGSPY_SUCCESS;

  if (T == NULL) {
    *error = Y3_DBGSPY_NOT_INITIALIZED_ERROR;
    return NULL;
  }

  if (name == NULL) {
    *error = Y3_DBGSPY_SPECIFIED_TARGET_NAME_IS_NULL_ERROR;
    return NULL;
  }

  if (T->target) {
    link = y3_list_link_id(T->target, name);
    if (link) {
      t = (DBGT) link->data;
      return t;
    }
  }

  *error = Y3_DBGSPY_UNKNOWN_ERROR;
  return NULL;
}

struct y3_dbg_context *
y3_dbgspy_get_context(struct y3_dbg *T, char *name, int *error) {
  struct y3_list *link;
  struct y3_dbg_context *t;
  *error = Y3_DBGSPY_SUCCESS;

  if (T == NULL) {
    *error = Y3_DBGSPY_NOT_INITIALIZED_ERROR;
    return NULL;
  }

  if (name == NULL) {
    *error = Y3_DBGSPY_SPECIFIED_CONTEXT_NAME_IS_NULL_ERROR;
    return NULL;
  }

  if (T->context) {
    link = y3_list_link_id(T->context, name);
    if (link) {
      t = (DBGC) link->data;
      return t;
    }
  }

  *error = Y3_DBGSPY_UNKNOWN_ERROR;
  return NULL;
}

struct y3_dbg_element *
y3_dbgspy_get_element_in_state(struct y3_dbg_xstate *S, char *name, int *error) {
  struct y3_list *link;
  *error = Y3_DBGSPY_SUCCESS;

  link = y3_list_link_id(S->element, name);

  if (link != NULL) {
    return (DBGV) link->data;
  }

  *error = Y3_DBGSPY_VAR_DNE_ERROR;
  return NULL;
}

/*
This function adds space for a new context,
adds room for elements to be used by y3_dbgspy_new_context_elements().
*/

struct y3_dbg_context *
y3_dbgspy_new_context(struct y3_dbg *T, char *name, int *error) {
  char _name[Y3_DBGSPY_MAX_NAME_LEN];
  char *_name_a;
  int context_id;
  size_t name_len = 0;
  struct y3_list *link;
  struct y3_dbg_context *context;
  *error = Y3_DBGSPY_SUCCESS;

  if (T == NULL) {
    *error = Y3_DBGSPY_NOT_INITIALIZED_ERROR;
    return NULL;
  }

  context_id = T->n_contexts;

  T->n_contexts++;

  if (name == NULL) {
    y3_str_i64tos(context_id, _name, Y3_DBGSPY_MAX_NAME_LEN, &name_len);
    name_len++;
    _name_a = (char *) malloc(name_len);
    memmove(_name_a, _name, name_len);
    name = _name_a;
  } else {
    name_len = strlen(name) + 1;
    _name_a = (char *) malloc(name_len);

    memmove(_name_a, name, name_len);
    name = _name_a;
  }

  if (T->context == NULL) {
    T->context = y3_list_create_custom_hs((DBGC) malloc(G_SIZE), name, (void *) -1, Y3_LIST_INITIAL_NODE_COUNT, 1,
                                          y3_hash_new_table(
                                              Y3_LIST_INITIAL_NODE_COUNT,
                                              (void *) -1,
                                              y3_hash_string5,
                                              y3_hash_string_less,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL)
    );
  } else {
    if (y3_list_link_id(T->context, name)) {
      *error = Y3_DBGSPY_DUPLICATE_VARIABLE_NAME_ERROR;
      return NULL;
    }

    T->context = y3_list_insert(T->context, (DBGC) malloc(G_SIZE), context_id, 0, 2, name);
  }

  link = y3_list_link_id(T->context, name);
  context = (DBGC) link->data;
  context->id = context_id;
  context->n_elements = 0;
  context->element = NULL;
  context->name = (char *) malloc(++name_len);
  memmove(context->name, name, name_len);

  return context;
}

/*
This function stores user defined element name, value and type,
into the specified dbgspy context.

The format for '...' is "name", val, value_size_in_bytes, "type", is_ptr

n = specifies how many unique element names are specified.

As an example of a unique context would be a user defined scope in C

 such as

 {
      .... code....
      .... code....
      .... code....
 }
*/

void
y3_dbgspy_new_context_elements(struct y3_dbg *T, char *name, int *error, int n, ...) {
  int i;
  size_t n_len, val_nbytes;
  struct y3_list *link;
  struct y3_dbg_context *g;
  struct y3_dbg_element *v;
  char *p_name, *t_name;
  void *value;
  va_list params;
  *error = Y3_DBGSPY_SUCCESS;


  if (T == NULL) {
    *error = Y3_DBGSPY_NOT_INITIALIZED_ERROR;
    return;
  }

  link = y3_list_link_id(T->context, name);

  if (link == NULL) {
    *error = Y3_DBGSPY_CONTEXT_DNE_ERROR;
    return;
  }

  g = (DBGC) link->data;

  va_start(params, n);

  for (i = 0; i < n; i++) {
    p_name = va_arg(params, char *);
    if (i == 0) {
      g->element = y3_list_create_custom_hs((DBGV) malloc(V_SIZE), p_name, (int *) -1, Y3_LIST_INITIAL_NODE_COUNT, 1,
                                            y3_hash_new_table(
                                                Y3_LIST_INITIAL_NODE_COUNT,
                                                (int *) -1,
                                                y3_hash_string5,
                                                y3_hash_string_less,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL)
      );
    } else {
      if (y3_list_link_id(g->element, p_name) != NULL) {
        va_arg(params, void *);
        va_arg(params, char *);
        if (va_arg(params, int))
          va_arg(params, size_t);
        fprintf(stderr, "Skipping Global element %s (already exists)...\n", p_name);
        continue;
      }
      g->element = y3_list_insert(g->element, (DBGV) malloc(V_SIZE), i, 0, 2, p_name);
    }
    link = y3_list_link_id(g->element, p_name);
    v = (DBGV) link->data;
    value = va_arg(params, void *);
    t_name = va_arg(params, char *);
    v->is_ptr = va_arg(params, int);

    if (v->is_ptr) {
      val_nbytes = va_arg(params, size_t);
      v->val = (void *) malloc(val_nbytes);
      memmove(v->val, value, val_nbytes);
    } else {
      v->val = value;
    }

    n_len = strlen(p_name) + 1;
    v->name = (char *) malloc(n_len);
    memmove(v->name, p_name, n_len);

    n_len = strlen(t_name) + 1;
    v->type.name = (char *) malloc(n_len);
    memmove(v->type.name, t_name, n_len);
  }

  va_end(params);
}

/*
This function adds a target to the dbgspy target collection.
The input format expected is the following,

T = the DBGSpy target collection.
name = unique name from user (used as unique target id)
n = number of paramers for ... argument
... = element names and values of the form 'name' 'value' 'type' 'value size in bytes (if type = ptr)'
*/

struct y3_dbg_target *
y3_dbgspy_target(struct y3_dbg *T, char *_name, int *error, int n, ...) {
  int i, id;
  size_t n_len, val_nbytes;
  void *value;
  char *name;
  struct y3_list *link;
  struct y3_dbg_target *t;
  struct y3_dbg_element *v;
  struct y3_dbg_xstate *s;
  int new_target = 0;
  va_list params;
  char *p_name, *t_name;
  *error = Y3_DBGSPY_SUCCESS;

  if (T == NULL) {
    *error = Y3_DBGSPY_NOT_INITIALIZED_ERROR;
    return NULL;
  }

  if (n < 0) {
    *error = Y3_DBGSPY_TARGET_VARIABLE_COUNT_LESS_THAN_ZERO_ERROR;
    return NULL;
  }

  if (_name == NULL) {
    *error = Y3_DBGSPY_SPECIFIED_TARGET_NAME_IS_NULL_ERROR;
    return NULL;
  }

  va_start(params, n);

  n_len = strlen(_name) + 1;
  name = (char *) malloc(n_len);
  memmove(name, _name, n_len);

  /*
  * Each state gets its own copy of elements, since
  * new element definitions get created for that specific n'th state
  * for the existing target.
  */
  if ((id = y3_dbgspy_get_target_id(T, name, error)) < 0) { // new target
    if (*error != Y3_DBGSPY_SUCCESS) {
      if (*error == Y3_DBGSPY_TARGET_DNE_ERROR) {
        *error = Y3_DBGSPY_SUCCESS;
      } else {
        va_end(params);
        return NULL;
      }
    }
    new_target = 1;
    id = T->n_targets;
    if (T->target == NULL) {
      T->target =
          y3_list_create_custom_hs((DBGT) malloc(T_SIZE), name, (void *) -1, Y3_LIST_INITIAL_NODE_COUNT, 1,
                                   y3_hash_new_table(
                                       Y3_LIST_INITIAL_NODE_COUNT,
                                       (void *) -1,
                                       y3_hash_string5,
                                       y3_hash_string_less,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL)
          );
    } else {
      T->target = y3_list_insert(T->target, (DBGT) malloc(T_SIZE), id, 0, 2, name);
    }
    link = y3_list_link_id(T->target, name);
    t = (DBGT) link->data;
    t->n_states = 0;
    t->id = id;
    t->n_len = strlen(name) + 1;
    t->n_elements = n;
    t->name = (char *) malloc(t->n_len);
    memmove(t->name, name, t->n_len);
    t->xstate = y3_list_create((DBGS) malloc(S_SIZE), t->n_states, (void *) -1, Y3_LIST_INITIAL_NODE_COUNT, 1);
    link = y3_list_link_id(t->xstate, t->n_states);
    s = (DBGS) link->data;
    s->id = t->n_states;

    s->element = NULL;
  } else { // new state of elements(s) for existing target
    printf("target name[%s] exists\n", name);
    link = y3_list_link_id(T->target, name);
    t = (DBGT) link->data;

    /*
        In case where the number of elements to monitor (spy on)
        has changed, but the target already exists.

        The target context has to be cleaned and reset before
        continuing, with y3_dbgspy_clean_target(),
        therefore an exception has to be raised.
    */

    if (n > t->n_elements) {
      *error = Y3_DBGSPY_TARGET_VARIABLE_COUNT_CHANGED_ERROR;
      return NULL;
    }

    t->n_states++;

    t->xstate = y3_list_insert(t->xstate, (DBGS) malloc(S_SIZE), t->n_states, 0, 2, t->n_states);
    link = y3_list_link_id(t->xstate, t->n_states);
    s = (DBGS) link->data;
    s->element = NULL;
    s->id = t->n_states;
  }

  link = y3_list_link_id(t->xstate, t->n_states);
  s = (DBGS) link->data;
  printf("adding state [%d] to target [%s] id [%d]\n", t->n_states, name, id);

  for (i = 0; i < n; i++) {
    p_name = va_arg(params, char *);
    if (s->element == NULL) {
      s->element = y3_list_create_custom_hs((DBGV) malloc(V_SIZE), p_name, (int *) -1, Y3_LIST_INITIAL_NODE_COUNT, 1,
                                            y3_hash_new_table(
                                                Y3_LIST_INITIAL_NODE_COUNT,
                                                (int *) -1,
                                                y3_hash_string5,
                                                y3_hash_string_less,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL)
      );
    } else {
      if (y3_dbgspy_get_element_in_state(s, p_name, error) != NULL) {
        if (*error != Y3_DBGSPY_SUCCESS) {
          return NULL;
        }
        va_arg(params, void *);
        va_arg(params, char *);
        if (va_arg(params, int))
          va_arg(params, size_t);
        fprintf(stderr, "Skipping element %s (already exists)...\n", p_name);
        continue;
      }
      s->element = y3_list_insert(s->element, (DBGV) malloc(V_SIZE), i, 0, 2, p_name);
    }
    value = (void *) va_arg(params, void *);
    t_name = va_arg(params, char *);
    n_len = strlen(p_name) + 1;
    v = y3_dbgspy_get_element_in_state(s, p_name, error);
    if (*error != Y3_DBGSPY_SUCCESS) {
      return NULL;
    }
    v->name = (char *) malloc(n_len);
    memmove(v->name, p_name, n_len);
    n_len = strlen(t_name) + 1;
    v->type.name = (char *) malloc(n_len);
    memmove(v->type.name, t_name, n_len);
    v->is_ptr = va_arg(params, int);

    if (v->is_ptr) {
      val_nbytes = va_arg(params, size_t);
      v->val = (void *) malloc(val_nbytes);
      memmove(v->val, value, val_nbytes);
    } else {
      v->val = value;
    }
  }

  printf("\nshowing all added elements\n");

  link = y3_list_link_id(t->xstate, t->n_states);
  s = (DBGS) link->data;
  for (link = s->element; link; link = link->next) {
    v = (DBGV) link->data;
    printf("element [%d] name [%s]\n", link->id, v->name);
  }

  va_end(params);

  if (new_target) {
    T->n_targets++;
  }

  printf("TOTAL TARGETS SO FAR [%d]\n", T->n_targets);

  return t;
}

void *
y3_dbgspy_target_add_element(struct y3_dbg_target *t, int *error, int n, ...) {
  int i;
  size_t n_len, val_nbytes;
  void *value;
  struct y3_list *link;
  struct y3_dbg_element *v;
  struct y3_dbg_xstate *s;
  va_list params;
  char *p_name, *t_name;

  *error = Y3_DBGSPY_SUCCESS;

  link = y3_list_link_id(t->xstate, t->n_states);
  s = (DBGS) link->data;
  printf("adding element(s) to state [%d] of target [%s] id [%d]\n", t->n_states, t->name, t->id);

  va_start(params, n);

  for (i = 0; i < n; i++) {
    p_name = va_arg(params, char *);
    if (s->element == NULL) {
      s->element = y3_list_create_custom_hs((DBGV) malloc(V_SIZE), p_name, (int *) -1, Y3_LIST_INITIAL_NODE_COUNT, 1,
                                            y3_hash_new_table(
                                                Y3_LIST_INITIAL_NODE_COUNT,
                                                (int *) -1,
                                                y3_hash_string5,
                                                y3_hash_string_less,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL)
      );
    } else {
      if (y3_dbgspy_get_element_in_state(s, p_name, error) != NULL) {
        if (*error != Y3_DBGSPY_SUCCESS) {
          return NULL;
        }
        va_arg(params, void *);
        va_arg(params, char *);
        if (va_arg(params, int))
          va_arg(params, size_t);
        fprintf(stderr, "Skipping element %s (already exists)...\n", p_name);
        continue;
      }
      s->element = y3_list_insert(s->element, (DBGV) malloc(V_SIZE), i, 0, 2, p_name);
    }
    value = (void *) va_arg(params, void *);
    t_name = va_arg(params, char *);
    n_len = strlen(p_name) + 1;
    v = y3_dbgspy_get_element_in_state(s, p_name, error);
    if (*error != Y3_DBGSPY_SUCCESS) {
      return NULL;
    }
    v->name = (char *) malloc(n_len);
    memmove(v->name, p_name, n_len);
    n_len = strlen(t_name) + 1;
    v->type.name = (char *) malloc(n_len);
    memmove(v->type.name, t_name, n_len);
    v->is_ptr = va_arg(params, int);

    if (v->is_ptr) {
      val_nbytes = va_arg(params, size_t);
      v->val = (void *) malloc(val_nbytes);
      memmove(v->val, value, val_nbytes);
    } else {
      v->val = value;
    }
  }

  va_end(params);

  printf("\nshowing all added elements\n");

  link = y3_list_link_id(t->xstate, t->n_states);
  s = (DBGS) link->data;
  for (link = s->element; link; link = link->next) {
    v = (DBGV) link->data;
    printf("element [%d] name [%s]\n", link->id, v->name);
  }

  return t;
}

void *
y3_dbgspy_get_target_element_val(struct y3_dbg *T, char *target_name, char *element_name, int *error) {
  struct y3_list *link;
  struct y3_dbg_target *t;
  struct y3_dbg_element *v;
  struct y3_dbg_xstate *s;
  *error = Y3_DBGSPY_SUCCESS;

  link = y3_list_link_id(T->target, target_name);

  if (link == NULL) {
    *error = Y3_DBGSPY_TARGET_DNE_ERROR;
    return NULL;
  }

  t = (DBGT) link->data;
  link = y3_list_link_id(t->xstate, t->n_states);
  s = (DBGS) link->data;
  v = y3_dbgspy_get_element_in_state(s, element_name, error);
  if (*error != Y3_DBGSPY_SUCCESS) {
    return NULL;
  }
  return v->val;
}

struct y3_dbg_element *
y3_dbgspy_get_target_element_inst(struct y3_dbg *T, char *target_name, char *element_name, int *error) {
  struct y3_list *link;
  struct y3_dbg_target *t;
  struct y3_dbg_element *v;
  struct y3_dbg_xstate *s;
  *error = Y3_DBGSPY_SUCCESS;

  link = y3_list_link_id(T->target, target_name);

  if (link == NULL) {
    *error = Y3_DBGSPY_TARGET_DNE_ERROR;
    return NULL;
  }

  t = (DBGT) link->data;
  link = y3_list_link_id(t->xstate, t->n_states);
  s = (DBGS) link->data;
  v = y3_dbgspy_get_element_in_state(s, element_name, error);
  if (*error != Y3_DBGSPY_SUCCESS) {
    return NULL;
  }
  return v;
}

/*
The user is responsible for the memory allocated to the data residing in the memory space
pointed to by *element_data if "is_ptr" is non-zero.
*/
void
y3_dbgspy_set_target_element_val(
    struct y3_dbg *T,
    char *target_name,
    char *element_name,
    void *element_data,
    int is_ptr,
    int *error
) {
  struct y3_list *link;
  struct y3_dbg_target *t;
  struct y3_dbg_element *v;
  struct y3_dbg_xstate *s;
  *error = Y3_DBGSPY_SUCCESS;

  link = y3_list_link_id(T->target, target_name);

  if (link == NULL) {
    *error = Y3_DBGSPY_TARGET_DNE_ERROR;
    return;
  }

  t = (DBGT) link->data;
  link = y3_list_link_id(t->xstate, t->n_states);
  s = (DBGS) link->data;
  v = y3_dbgspy_get_element_in_state(s, element_name, error);
  if (*error != Y3_DBGSPY_SUCCESS) {
    return;
  }
  v->is_ptr = is_ptr;
  v->val = element_data;
}

void *
y3_dbgspy_get_context_element_val(struct y3_dbg *T, char *context_name, char *element_name, int *error) {
  struct y3_list *link;
  struct y3_dbg_context *t;
  struct y3_dbg_element *v;
  *error = Y3_DBGSPY_SUCCESS;

  link = y3_list_link_id(T->context, context_name);

  if (link == NULL) {
    *error = Y3_DBGSPY_TARGET_DNE_ERROR;
    return NULL;
  }

  t = (DBGC) link->data;
  link = y3_list_link_id(t->element, element_name);
  v = (DBGV) link->data;
  return v->val;
}

struct y3_dbg_element *
y3_dbgspy_get_context_element_inst(struct y3_dbg *T, char *context_name, char *element_name, int *error) {
  struct y3_list *link;
  struct y3_dbg_context *t;
  struct y3_dbg_element *v;
  *error = Y3_DBGSPY_SUCCESS;

  link = y3_list_link_id(T->context, context_name);
  if (link == NULL) {
    *error = Y3_DBGSPY_TARGET_DNE_ERROR;
    return NULL;
  }
  t = (DBGC) link->data;
  link = y3_list_link_id(t->element, element_name);
  v = (DBGV) link->data;
  return v;
}

/*
The user is responsible for the memory allocated to the data residing in the memory space
pointed to by *element_data if "is_ptr" is non-zero.
*/
void
y3_dbgspy_set_cotext_element_val(
    struct y3_dbg *T,
    char *context_name,
    char *element_name,
    void *element_data,
    int is_ptr,
    int *error
) {
  struct y3_list *link;
  struct y3_dbg_context *t;
  struct y3_dbg_element *v;
  *error = Y3_DBGSPY_SUCCESS;

  link = y3_list_link_id(T->context, context_name);

  if (link == NULL) {
    *error = Y3_DBGSPY_TARGET_DNE_ERROR;
    return;
  }

  t = (DBGC) link->data;
  link = y3_list_link_id(t->element, element_name);
  v = (DBGV) link->data;
  v->is_ptr = is_ptr;
  v->val = element_data;
}

/*
This function creates or opens the dbgspy.out file.
*/

#define FILE_MODE    (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

int
y3_dbgspy_openoutfile(struct y3_dbg *T, char *file) {
  int FILE_FLAGS = O_CREAT | O_WRONLY;

#ifdef _DOSLIKE
  FILE_FLAGS = O_CREAT|O_WRONLY|O_BINARY;
#endif

  if ((T->fd = open(file, FILE_FLAGS)) < 0) {
    return -1;
  }

#ifdef _DOSLIKE
  (void)setmode(T->fd, O_BINARY);
  if(chmod(file, S_IREAD | S_IWRITE))
      return -1;
#else
  if (fchmod(T->fd, FILE_MODE)) {
    return -1;
  }
#endif

  return 0;
}

