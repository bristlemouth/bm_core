#include "resource_trie.h"
#include "bm_os.h"
#include <string.h>

#define increment_past_separator(t) (*t == '/' ? t + 1 : t)
#define is_separator(t) (*t == '/')

typedef bool (*MatchCb)(ResourceTrieElement *current, void *arg);

struct ExactMatchCtx;

typedef BmErr (*PartialMatchCb)(struct ExactMatchCtx *ctx,
                                ResourceTrieElement *child,
                                BmTopicLength shared_len);

typedef struct ExactMatchCtx {
  ResourceTrieElement *current;
  ResourceTrieElement *parent;
  const PartialMatchCb cb;
  bool found;
} ExactMatchCtx;

typedef struct {
  BmTopicLength shared_segments;
  BmTopicLength shared_length;
} SegmentConsumedCtx;

static inline bool topic_has_wildcard(const char *topic) {
  return (strchr(topic, '*') != NULL) || (strchr(topic, '?') != NULL);
}

static ResourceTrieElement *alloc_element(const char *segment,
                                          BmTopicLength segment_length) {
  ResourceTrieElement *element = bm_malloc(sizeof(ResourceTrieElement));
  if (!element) {
    return NULL;
  }

  if (!segment_length) {
    segment_length = strlen(segment);
  }

  uint32_t segment_size = segment_length + 1;
  *element = (ResourceTrieElement){0};
  element->segment = (const char *)bm_malloc(segment_size);
  if (!element->segment) {
    bm_free(element);
    return NULL;
  }
  strncpy((void *)element->segment, segment, segment_length);
  element->resource_id = invalid_resource_id;
  element->segment_length = segment_length;

  return element;
}

static void free_element(ResourceTrieElement *element) {
  bm_free((void *)element->segment);
  bm_free(element);
}

static void add_child(ResourceTrieElement *parent, ResourceTrieElement *child,
                      ResourceTrieElement *check) {
  // If child is direct ll pointer set to child
  if (parent->children == check) {
    parent->children = child;
    return;
  }

  // Iterate through childrent and set new sibling
  ResourceTrieElement *tmp = parent->children;
  while (tmp->sibling != check) {
    tmp = tmp->sibling;
  }

  tmp->sibling = child;
}

static inline void copy_and_increment_segment(char **new, const char *old,
                                              BmTopicLength length) {
  BmTopicLength max_copy = bm_min(length, BM_TOPIC_MAX_LEN);
  strncpy(*new, old, max_copy);
  *new += max_copy;
}

static bool check_and_compress(ResourceTrieRoot *root,
                               ResourceTrieElement *parent) {
  // Compress if possible
  ResourceTrieElement *child = parent->children;

  bool has_children = !child || child->sibling || child->children;
  bool is_root = parent == &root->element;
  if (has_children || is_root) {
    return false;
  }

  // Update the segment topic
  BmTopicLength child_length = child->segment_length;
  BmTopicLength parent_length = parent->segment_length;

  // Add 1 for separator
  uint32_t combined_length = child_length + parent_length + 1;
  char *new_segment = (char *)bm_malloc(combined_length + 1);
  const char *segment_begin = new_segment;

  copy_and_increment_segment(&new_segment, parent->segment, parent_length);
  *new_segment = '/';
  new_segment++;
  copy_and_increment_segment(&new_segment, child->segment, child_length);

  ResourceTrieElement *sibling = parent->sibling;
  *parent = *child;
  // Child segment will be freed in free_element
  parent->segment = segment_begin;
  parent->segment_length = combined_length;
  parent->sibling = sibling;

  free_element(child);

  return true;
}

static void remove_child(ResourceTrieRoot *root, ResourceTrieElement *parent,
                         ResourceTrieElement *current) {

  // If there are children under this element see if element can be compressed
  if (current->children) {
    bool compressed = check_and_compress(root, current);

    // Could not compress, but resource is now invalid
    if (!compressed) {
      current->resource_id = invalid_resource_id;
      current->port_mask = 0;
      current->local_interest = 0;
      current->is_wildcard = 0;
    }
    return;
  }

  // If current is child set to sibling
  if (parent->children == current) {
    parent->children = current->sibling;
    free_element(current);
    return;
  }

  // Current must be a sibling
  ResourceTrieElement *tmp = parent->children;
  while (tmp->sibling != current) {
    tmp = tmp->sibling;
  }
  tmp->sibling = current->sibling;
  free_element(current);
}

static BmTopicLength calculate_shared_segments(const char *topic,
                                               const char *segment,
                                               BmTopicLength *shared_length) {
  BmTopicLength segments = 0;
  BmTopicLength length = 0;
  char t_c = *topic;
  char t_s = *segment;

  while (length < BM_TOPIC_MAX_LEN && t_c && t_s && t_c == t_s) {
    length++;
    if (t_c == '/') {
      segments++;
    }
    t_c = topic[length];
    t_s = segment[length];
  }

  *shared_length = length;

  return segments;
}

static bool segment_consumed(const char *topic,
                             const ResourceTrieElement *current,
                             SegmentConsumedCtx *ctx) {
  BmTopicLength segment_length = current->segment_length;
  const char *segment = current->segment;

  ctx->shared_segments =
      calculate_shared_segments(topic, segment, &ctx->shared_length);
  BmTopicLength topic_span = (BmTopicLength)strcspn(topic, "/");

  // Full segment consumed
  if (segment_length >= topic_span && ctx->shared_length == segment_length) {
    return true;
  }

  // If the length does not span to the next separator
  bool topic_separator = is_separator(&topic[ctx->shared_length]);
  bool segment_separator = is_separator(&segment[ctx->shared_length]);
  if (!topic_separator && !segment_separator && ctx->shared_segments) {
    // Walk back to the next separator
    while (ctx->shared_length > 0 && topic[ctx->shared_length] != '/') {
      ctx->shared_length--;
    }
  }

  return false;
}

static void stack_push(ResourceTrieStack *stack, BmTopicLength *sp,
                       ResourceTrieElement *element, BmTopicLength path_len) {
  stack[*sp].element = element;
  stack[*sp].path_len = path_len;
  (*sp)++;
}

static void stack_pop(ResourceTrieStack *stack, BmTopicLength *sp,
                      ResourceTrieElement **element, BmTopicLength *path_len) {
  *sp = uint_safe_decrement(*sp);
  *element = stack[*sp].element;
  *path_len = stack[*sp].path_len;
}

static bool process(ResourceTrieRoot *root, const char *topic, MatchCb cb,
                    void *arg, bool topic_wildcard, BmTopicLength offset,
                    BmTopicLength topic_length, ResourceTrieElement *current) {
  if (!current) {
    return false;
  }

  // Check match at entries with valid resource_id
  if (current->resource_id != invalid_resource_id) {
    bool matched;
    if (current->is_wildcard) {
      matched = bm_wildcard_match(topic, topic_length, root->match_str, offset);
    } else if (topic_wildcard) {
      matched = bm_wildcard_match(root->match_str, offset, topic, topic_length);
    } else {
      matched = !strncmp(root->match_str, topic, BM_TOPIC_MAX_LEN);
    }

    if (matched && cb(current, arg)) {
      return true;
    }
  }

  return false;
}

static BmErr match_wildcard(ResourceTrieRoot *root, const char *topic,
                            MatchCb cb, void *arg) {

  topic = increment_past_separator(topic);
  BmTopicLength topic_length = (BmTopicLength)strnlen(topic, BM_TOPIC_MAX_LEN);
  bool topic_wildcard = topic_has_wildcard(topic);

  // Push root onto stack
  BmTopicLength sp = 0;
  ResourceTrieStack *stack = root->stack;
  ResourceTrieElement *current = root->element.children;
  BmTopicLength path_length = 0;

  while (current || sp) {
    while (current != NULL) {
      BmTopicLength segment_length = current->segment_length;
      // Add separator between segments
      if (path_length > 0) {
        if (path_length >= BM_TOPIC_MAX_LEN) {
          continue;
        }
        root->match_str[path_length++] = '/';
      }

      // Skip if segment would overflow buffer
      if (path_length + segment_length >= BM_TOPIC_MAX_LEN) {
        continue;
      }
      // Copy segment into match_str at parent offset
      memcpy(&root->match_str[path_length], current->segment, segment_length);
      path_length += segment_length;
      root->match_str[path_length] = '\0';

      if (current->children) {
        stack_push(stack, &sp, current, path_length);

        current = current->children;
      } else {
        // Process leaf node
        if (process(root, topic, cb, arg, topic_wildcard, path_length,
                    topic_length, current)) {
          return BmOK;
        }

        // Remove the added segment from the path
        path_length -= segment_length;
        path_length = uint_safe_decrement(path_length);
        memset(&root->match_str[path_length], 0, segment_length);
        current = current->sibling;
      }
    }

    if (sp > 0) {
      stack_pop(stack, &sp, &current, &path_length);

      // Clear separator and segment
      memset(&root->match_str[path_length], 0, current->segment_length + 1);
      if (process(root, topic, cb, arg, topic_wildcard, path_length,
                  topic_length, current)) {
        return BmOK;
      }
      current = current->sibling;
    }
  }

  return BmOK;
}

static BmErr exact_match(ResourceTrieRoot *root, const char **topic,
                         ExactMatchCtx *ctx) {
  ctx->current = &root->element;
  const char *topic_local = *topic;
  ctx->found = false;

  topic_local = increment_past_separator(topic_local);
  while (*topic_local) {
    ctx->found = false;

    ResourceTrieElement *child = ctx->current->children;
    while (child) {

      SegmentConsumedCtx consumed = {0};
      if (segment_consumed(topic_local, child, &consumed)) {
        topic_local += child->segment_length;
        topic_local = increment_past_separator(topic_local);
        ctx->parent = ctx->current;
        ctx->current = child;
        ctx->found = true;
        break;
      }

      if (!consumed.shared_segments) {
        child = child->sibling;
        continue;
      }

      if (!ctx->cb) {
        break;
      }

      BmErr err = ctx->cb(ctx, child, consumed.shared_length);
      if (err != BmOK) {
        return err;
      }
      topic_local += consumed.shared_length;
      topic_local = increment_past_separator(topic_local);
      ctx->found = true;
      break;
    }

    if (!ctx->found) {
      break;
    }
  }

  *topic = topic_local;
  return BmOK;
}

static ResourceTrieElement *split_element(ResourceTrieElement *parent,
                                          ResourceTrieElement *child,
                                          BmTopicLength split_length) {
  const char *prefix = child->segment;

  // Assign suffix to original segment
  const char *suffix = &prefix[split_length];
  suffix = increment_past_separator(suffix);
  BmTopicLength suffix_len = strlen(suffix) + 1;

  char *tmp_segment = (char *)bm_malloc(suffix_len);
  if (!tmp_segment) {
    return NULL;
  }

  strncpy((void *)tmp_segment, suffix, suffix_len);

  // Get rid of separator if it is the last match lengthed item
  if (prefix[split_length - 1] == '/') {
    split_length--;
  }
  // Create the split element which will have the prefix
  ResourceTrieElement *split = alloc_element(prefix, split_length);
  if (!split) {
    return NULL;
  }

  bm_free((void *)child->segment);
  child->segment = tmp_segment;
  child->segment_length = suffix_len - 1;

  // Preserve child's sibling chain at the parent level
  split->sibling = child->sibling;
  child->sibling = NULL;

  // Set split to point to original child
  add_child(split, child, NULL);

  add_child(parent, split, child);

  return split;
}

static bool concrete_topic_update(ResourceTrieElement *current, void *arg) {
  ResourceTrieElement *new = (ResourceTrieElement *)arg;

  if (current == new) {
    return false;
  }

  new->port_mask |= current->port_mask;
  new->local_interest |= current->local_interest;

  return false;
}

static bool match_topic_update(ResourceTrieElement *current, void *arg) {
  ResourceTrieElement *new = (ResourceTrieElement *)arg;

  // Only non-wildcard topics should be updated
  bool passthrough_topic = current->resource_id == invalid_resource_id;
  if (current == new || current->is_wildcard || passthrough_topic) {
    return false;
  }

  current->port_mask |= new->port_mask;
  current->local_interest |= new->local_interest;

  return false;
}

static BmErr split_topic(ExactMatchCtx *ctx, ResourceTrieElement *child,
                         BmTopicLength shared_len) {
  // Partial segment consumed split topic
  ResourceTrieElement *split = split_element(ctx->current, child, shared_len);
  if (!split) {
    return BmENOMEM;
  }
  ctx->parent = ctx->current;
  ctx->current = split;

  return BmOK;
}

BmErr resource_trie_add(ResourceTrieRoot *root, const char *topic,
                        uint32_t resource_id, uint16_t port_mask,
                        bool local_interest) {
  if (!root || !topic || resource_id > max_resource_id) {
    return BmEINVAL;
  }

  const char *topic_start = topic;
  ExactMatchCtx ctx = {
      .current = NULL,
      .parent = NULL,
      .cb = split_topic,
      .found = false,
  };
  BmErr err = exact_match(root, &topic, &ctx);
  if (err != BmOK) {
    return err;
  }

  bool found = ctx.found;
  ResourceTrieElement *current = ctx.current;
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

  add_child(current, new, NULL);

  new->resource_id = resource_id;
  new->port_mask = port_mask;
  new->local_interest = local_interest;

  // Update elements in the resource trie based on the topic type
  new->is_wildcard = topic_has_wildcard(topic_start);
  MatchCb cb = concrete_topic_update;
  if (new->is_wildcard) {
    cb = match_topic_update;
  }

  return match_wildcard(root, topic_start, cb, new);
}

static bool match_result_update(ResourceTrieElement *current, void *arg) {
  ResourceTrieMatchResult *result = (ResourceTrieMatchResult *)arg;

  if (current->resource_id != invalid_resource_id) {
    result->matches[result->count++] = current;
  }

  if (result->count >= array_size(result->matches)) {
    return true;
  }

  return false;
}

BmErr resource_trie_match(ResourceTrieRoot *root, const char *topic) {
  if (!root || !topic) {
    return BmEINVAL;
  }

  memset(&root->result, 0, sizeof(ResourceTrieMatchResult));

  return match_wildcard(root, topic, match_result_update, &root->result);
}

BmErr resource_trie_remove(ResourceTrieRoot *root, const char *topic) {
  if (!root || !topic) {
    return BmEINVAL;
  }

  ExactMatchCtx ctx = {
      .current = NULL,
      .parent = NULL,
      .cb = NULL,
      .found = false,
  };
  const char *t = topic;
  BmErr err = exact_match(root, &t, &ctx);
  if (err != BmOK) {
    return err;
  }

  bool found = ctx.found;
  ResourceTrieElement *current = ctx.current;
  ResourceTrieElement *parent = ctx.parent;

  // Remove and delete the element if found
  if (found) {
    if (topic_has_wildcard(topic)) {
      resource_trie_match(root, topic);

      uint16_t wildcard_port_mask = 0;
      bool wildcard_local_interest = false;
      ResourceTrieMatchResult *result = &root->result;

      // Check which of the removed wildcard's bits/interest are still covered
      // by other remaining wildcards
      for (uint16_t i = 0; i < result->count; i++) {
        ResourceTrieElement *match = result->matches[i];
        if (!match->is_wildcard || match == current) {
          continue;
        }

        wildcard_port_mask |= match->port_mask;
        wildcard_local_interest |= match->local_interest;
      }

      // Determine which bits are now orphaned, were only held by
      // the removed wildcard and no other wildcard covers them anymore
      uint16_t remove_mask = current->port_mask & ~wildcard_port_mask;
      uint16_t remove_local_interest =
          current->local_interest && !wildcard_local_interest;

      // Search all concrete patterns and strip the bits
      for (uint16_t i = 0; i < result->count; i++) {
        ResourceTrieElement *match = result->matches[i];
        if (match->is_wildcard) {
          continue;
        }

        // Concrete topic will clear bits if
        match->port_mask &= ~remove_mask;
        if (remove_local_interest) {
          match->local_interest = 0;
        }
      }
    }
    remove_child(root, parent, current);
    check_and_compress(root, parent);

    return BmOK;
  }

  return BmENODATA;
}
