#ifndef NODE_TYPES_H
#define NODE_TYPES_H

#include "raylib.h"  // For Vector2
#include <stdbool.h>

typedef struct Node Node;

// Basic building block to draw connection between two nodes
typedef struct {
    Node *from;
    Node *to;
} Connection;

// Enum with all the node types
typedef enum {
    NODE_DEFAULT,
    NODE_STACK,
    NODE_RANDOM,
    NODE_RANDOM_BAG,
    NODE_USER_CHOICE,
    NODE_SKILL_GATE,
    NODE_GO_TO,
    NODE_CONDITIONAL,
    NODE_COUNT //last of the enums used for counting in loops and checking bounds
} NodeType;

// A simple default node with no branching or logic
typedef struct {
    char *text;  // text of the default node
    Connection next; // has the pointer to the next node
} DefaultNode;

// A control node. Represents a stack node. A stack node has no text. Narrative strands (nodes) are removed from the top upon completion. 
typedef struct {
    int stackindex; //index to navigate the stack
    Connection next[10]; // has a group of pointers that need to be chosen first. 
} StackNode;

// A control node. Chooses one random narrative strand (node) from a list of nodes.
typedef struct {
    unsigned int seed;  // For reproducibility
} RandomNode;

// A control node not that dissimilar from RandomNode. Narrative strands are still random but once picked, they are omitted from future choices.
// This makes it easier to hit a "rare" narrative strand as it removes chance.
typedef struct {
    unsigned int seed;  // For reproducibility
} RandomBagNode;

// A hybrid node. This node often has text (in the form of a question) and allows the user to advance different narrative strands. 
typedef struct {
    int choices;
} UserChoiceNode;

// A control node. The narrative strand is locked behind a skillcheck. Failing allows you to pick another option.  
typedef struct {
    Connection *passConnection;
    int requiredSkillId;
} SkillGateNode;

// A control node. This node jumps to another node (like a goto label)
typedef struct {
    Connection *target;   // pointer to destination node
} GoToNode;

// A control node. This node evaluates a condition and follows pass/fail paths. Works like skillG
typedef struct {
    Connection *passConnection;
    Connection *failConnection;
    int conditionId;      // external condition evaluator
    // must be a bool function pointer....
} ConditionalNode;

#endif // NODE_TYPES_H