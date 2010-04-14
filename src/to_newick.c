/* 

Copyright (c) 2009 Thomas Junier and Evgeny Zdobnov, University of Geneva
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
* Neither the name of the University of Geneva nor the names of its
    contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "rnode.h"
#include "list.h"
#include "concat.h"
#include "common.h"
#include "rnode_iterator.h"
#include "hash.h"

/* returns the length part of a node, e.g. ":12.345" */

char *length(struct rnode *node)
{
	char * result = malloc(sizeof(char));
	if (NULL == result) return NULL;
	*result = '\0';

	if (NULL != node->parent) {
		if (strlen(node->edge_length_as_string) > 0) {
			result = append_to(result, ":");
			result = append_to(result,
				       node->edge_length_as_string);	
		}
	}

	return result;
}

char *subtree(struct rnode *node)
{
	char * result = malloc(sizeof(char));
	if (NULL == result) return NULL;
	*result = '\0';

	if (is_leaf(node)) {
		result = append_to(result, node->label);
		char *length_s = length(node);
		if (NULL == length_s) return NULL;
		result = append_to(result, length_s);
		free(length_s);
	} else {
		struct rnode *child;
		struct list_elem *elem;
		char * child_node_s;

		result = append_to(result, "(");

		/* first child */
		elem = node->children->head;
		child = elem->data;
		child_node_s = subtree(child);
		if (NULL == child_node_s) return NULL;
		result = append_to(result, child_node_s);
		free(child_node_s);
		/* other children, comma-separated */
		for (elem = elem->next; elem != NULL; elem = elem->next) {
			result = append_to(result, ",");
			child = elem->data;
			child_node_s = subtree(child);
			if (NULL == child_node_s) return NULL;
			result = append_to(result, child_node_s);
			free(child_node_s);
		}
		result = append_to(result, ")");
		if (NULL != node->label) {
			result = append_to(result, node->label);
		}
		char *length_s = length(node);
		if (NULL == length_s) return NULL;
		result = append_to(result, length_s);
		free(length_s);
	}
	return result;
}

char *to_newick(struct rnode *node)
{
	char *result;
	result = subtree(node);
	if (NULL == result) return NULL;
	result = append_to(result, ";");
	return result;
}

int dump_newick(struct rnode *node)
{
	struct rnode_iterator *it;
	struct rnode *current;
	struct hash *seen;
	char *key;

	it = create_rnode_iterator(node);
	if (NULL == it) {
		perror(NULL);
		return FAILURE;
	}
	seen = create_hash(10000);	// TODO: allow to pass hint
	if (NULL == seen) {
		perror(NULL);
		return FAILURE;
	}

	/* the starting node (root) starts out as 'seen' */
	key = make_hash_key(node);
	if (! hash_set(seen, key, node)) {
		perror(NULL);
		return FAILURE;
	}
	free(key);

	printf("(");
	while ((current = rnode_iterator_next(it)) != NULL) {
		// printf ("%s ", current->label);
		if (is_leaf(current)) {
			/* leaf: just print label */
			printf("%s", current->label);
			if (strcmp("", current->edge_length_as_string) != 0)
					printf(":%s", current->edge_length_as_string);
		}
		else {
			/* inner node: behaviour depends on whether we've
			 * already 'seen' this node or not. */
			key = make_hash_key(current);
			if (NULL == hash_get(seen, key)) {
				/* not seen: print '(' */
				if(! hash_set(seen, key, current)) {
					perror(NULL);
					return FAILURE;
				}
				printf("(");
			} else {
				if (NULL == get_next_unvisited_child(it)) {
					printf(")%s", current->label);
					if (strcmp("",
					current->edge_length_as_string) != 0)
						printf(":%s", current->edge_length_as_string);
				}
				else
					printf(",");
			}
			free(key);
		}
		//printf("\n");
	}
	printf(";\n");

	/* See why rnode_iterator_next() returned NULL */
	switch (it->status) {
		case RNODE_ITERATOR_END:
			break;	/* Ok */
		case RNODE_ITERATOR_ERROR:
			return FAILURE;
		default:
			assert(0);	/* programmer error */
	}


	destroy_rnode_iterator(it);
	return SUCCESS;
}
