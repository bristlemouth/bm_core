#include "resource_trie.h"
#include "bm_os.h"
#include <string.h>

#define increment_past_separator(t) (*t == '/' ? t + 1 : t)

static ResourceTrieElement *alloc_element(const char *segment,
                                          BmTopicLength segment_length) {
  ResourceTrieElement *element = bm_malloc(sizeof(ResourceTrieElement));
  if (!element) {
    return NULL;
  }

  if (!segment_length) {
    segment_length = strlen(segment) + 1;
  }
  element->segment = (const char *)bm_malloc(segment_length);
  if (!element->segment) {
    bm_free(element);
    return NULL;
  }

  memcpy((void *)element->segment, segment, segment_length);
  element->children_count = 0;
  element->children = NULL;
  element->local_interest = 0;
  element->resource_id = invalid_resource_id;

  return element;
}

static void free_element(ResourceTrieElement *element) {
  bm_free((void *)element->segment);
  bm_free(element);
}

static BmErr add_child(ResourceTrieElement *parent,
                       ResourceTrieElement *child) {
  uint8_t count = parent->children_count;
  uint16_t new_count = count + 1;

  ResourceTrieElement **new = (ResourceTrieElement **)bm_malloc(new_count);
  if (!new) {
    return BmENOMEM;
  }

  memcpy(*new, parent->children, count);
  new[count] = child;
  bm_free(parent->children);
  parent->children = new;
  parent->children_count = new_count;

  return BmOK;
}

static BmTopicLength common_prefix_length(const char *topic,
                                          const char *segment) {
  BmTopicLength length = 0;
  char t_c = *topic;
  char t_s = *segment;

  while (length < BM_TOPIC_MAX_LEN && t_c && t_s && t_c == t_s) {
    length++;
    t_c = topic[length];
    t_s = segment[length];
  }

  return length;
}

static ResourceTrieElement *split_element(ResourceTrieElement *parent,
                                          ResourceTrieElement *child,
                                          uint8_t child_idx,
                                          BmTopicLength split_length) {
  // Allocate new element and assign prefix string
  ResourceTrieElement *split = alloc_element(child->segment, split_length);
  if (!split) {
    return NULL;
  }

  // Set split to point to original child
  if (add_child(split, child) != BmOK) {
    bm_free(split);
    return NULL;
  }

  // Assign suffix to original segment
  const char *suffix = &child->segment[split_length];
  suffix = increment_past_separator(suffix);
  BmTopicLength suffix_len = strlen(suffix) + 1;

  char *tmp_segment = (char *)bm_malloc(suffix_len);
  if (!tmp_segment) {
    free_element(split);
    return NULL;
  }

  memcpy((void *)tmp_segment, suffix, suffix_len);
  tmp_segment[suffix_len - 1] = '\0';
  bm_free((void *)child->segment);
  child->segment = tmp_segment;

  // Set the parents child to now point to the split
  parent->children[child_idx] = split;

  return split;
}

BmErr resource_trie_add(ResourceTrieRoot *root, const char *topic,
                        uint32_t resource_id, uint16_t port_mask,
                        bool local_interest) {
  if (!root || !topic || resource_id > max_resource_id) {
    return BmEINVAL;
  }

  ResourceTrieElement *current = &root->element;
  bool found = false;

  topic = increment_past_separator(topic);

  while (*topic) {
    found = false;

    for (uint8_t i = 0; i < current->children_count; i++) {
      ResourceTrieElement *child = current->children[i];

      BmTopicLength seg_len =
          (BmTopicLength)strnlen(child->segment, BM_TOPIC_MAX_LEN);
      BmTopicLength shared_len = common_prefix_length(topic, child->segment);

      if (!shared_len) {
        continue;
      }

      // Full segment consumed
      if (shared_len == seg_len) {
        topic += seg_len;
        topic = increment_past_separator(topic);
        current = child;
        found = true;
        break;
      }

      // Partial segment consumed split topic
      ResourceTrieElement *split = split_element(current, child, i, shared_len);
      if (!split) {
        return BmENOMEM;
      }

      topic += shared_len;
      topic = increment_past_separator(topic);
      current = split;
      found = true;
    }

    if (!found) {
      break;
    }
  }

  // Update variables if found
  if (found) {
    current->resource_id = resource_id;
    current->port_mask = port_mask;
    current->local_interest = local_interest;
    return BmOK;
  }

  // If not found:
  // - create a new element
  // - add it as a child to the current element
  // - assign associated variables
  ResourceTrieElement *new = alloc_element(topic, 0);
  if (!new) {
    return BmENOMEM;
  }

  if (add_child(current, new) != BmOK) {
    free_element(new);
    return BmENOMEM;
  }

  new->resource_id = resource_id;
  new->port_mask = port_mask;
  new->local_interest = local_interest;

  return BmOK;
  ;
}

static void stack_push(ResourceTrieStack *stack, BmTopicLength *sp,
                       ResourceTrieElement *element, const char *remaining) {
  stack[*sp].element = element;
  stack[*sp].remaining = remaining;
  (*sp)++;
}

static void stack_pop(ResourceTrieStack *stack, BmTopicLength *sp,
                      ResourceTrieElement **element, const char **remaining) {
  (*sp)--;
  *element = stack[*sp].element;
  *remaining = stack[*sp].remaining;
}

BmErr resource_trie_match(ResourceTrieRoot *root, const char *topic,
                          ResourceTrieMatchResult *result) {
  if (!root || !topic || !result) {
    return BmEINVAL;
  }

  result->count = 0;
  topic = increment_past_separator(topic);

  // Add root to stack
  BmTopicLength sp = 0;
  ResourceTrieStack *stack = root->stack;
  stack_push(stack, &sp, &root->element, topic);

  ResourceTrieElement *current;
  const char *remaining;

  while (sp > 0 && result->count < result->capacity) {
    stack_pop(stack, &sp, &current, &remaining);

    // If no string is remaining, this is a match
    if (*remaining == '\0') {
      if (current->resource_id != invalid_resource_id) {
        result->matches[result->count++] = current;
      }
      continue;
    }

    for (uint8_t i = 0; i < current->children_count; i++) {
      if (sp >= resource_trie_max_depth) {
        return BmENOMEM;
      }

      ResourceTrieElement *child = current->children[i];
      const char *segment = child->segment;

      bool found = true;
      const char *rest = remaining;
      BmTopicLength rem_len = (BmTopicLength)strnlen(rest, BM_TOPIC_MAX_LEN);

      // Perform wildcard match per topic segment if an element is compressed
      while (*segment) {
        BmTopicLength seg_len = (BmTopicLength)strcspn(segment, "/");

        if (!bm_wildcard_match(rest, rem_len, segment, seg_len)) {
          found = false;
          break;
        }

        segment += seg_len;
        segment = increment_past_separator(segment);

        while (*rest != '/' && *rest) {
          rest++;
          rem_len--;
        }
        if (*rest == '/') {
          rest++;
          rem_len--;
        }
      }

      if (found) {
        stack_push(stack, &sp, child, rest);
      }
    }
  }

  return BmOK;
}

BmErr resource_trie_remove(ResourceTrieRoot *root, const char *topic);
