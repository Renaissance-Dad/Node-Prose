#pragma once
#include "nodetypes.h"

extern Font globalFont;

// macro function for integers - inline use
#ifndef CLAMP
#define CLAMP(value, min, max) ((value) < (min) ? (min) : ((value) > (max) ? (max) : (value)))
#endif

// Defines
#define MAX_CONNECTORS 12 //amount of connectors per node
#define MAX_NODES 250     //amount of nodes in the graph
#define MAX_BEZIERS 300   //amount of permanent bezier curves 
#define MAX_SCENES 50     //amount of scenes inside a project
#define MAX_SCENE_NODES 32

typedef struct {
    Rectangle bounds;
    char name[32];
    Node *containedNodes[MAX_SCENE_NODES];
    Node *previousNodes[MAX_SCENE_NODES];
    int previousNodeCount;
    int nodeCount;
} SceneOutline;

typedef struct {
    SceneOutline scenes[MAX_SCENES];
    int count;
} SceneList;

typedef enum {
    VIEW_MODE_NODE,
    VIEW_MODE_SCRIPT
} ViewMode;

typedef struct {
    Vector2 points[4];  // Cubic Bézier: start, control1, control2, end
    Vector2 relativeposition[2];   // xy vs corner startnode, xy vs corner endnode
    Node *fromNode;                // Pointer to the node where connection starts
    Node *toNode;                  // Pointer to the node where connection ends
} BezierCurve;

// Enum to track screen mode
typedef enum {
    SCREEN_SMALL,
    SCREEN_LARGE
} ScreenSize;

// Struct to hold screen dimensions
typedef struct {
    int width;
    int height;
    ScreenSize currentSize;
    ViewMode currentView;
} ScreenSettings;

// Enum for the connector types "radio buttons"
typedef enum {
    CONNECTOR_INPUT,
    CONNECTOR_OUTPUT
} ConnectorType;

// Struct for easy click detection of radio-buttons
typedef struct {
    Vector2 center;
    float radius;
    ConnectorType type;
    NodeType parentType;
    Connection with;
} Connector;

// Global context that is shared between functions
typedef struct {
    // nodes
    Node *nodes; 
    // dragging
    bool isDragging;
    Node *draggedNode;
    Vector2 dragOffset;  // difference between mouse position and node corner
    double dragStartTime;
    Node *dragCandidateNode;
    // Double-click tracking
    double lastClickTime;
    // bring to frong
    Node *bringToFront;
    // bezier curves
    bool connecting;
    Vector2 connectionStart;
    Vector2 bezier[4];
    Node *connectingFromNode;
    int connectingFromConnectorIndex;
    Node *hoveredInputNode;
    int hoveredInputConnectorIndex;
    BezierCurve permanentBeziers[MAX_BEZIERS];
    int bezierCount;
    // Draw scene context variables
    SceneList sceneList;
    bool isDrawingScene;
    Vector2 sceneStartPos;
    SceneOutline *draggedScene;
    Vector2 sceneDragOffset;
    bool isResizingScene;
    bool isResizingSceneVertically;
    SceneOutline *resizingScene;
    float initialSceneWidth;
    float initialSceneHeight;
    float resizeStartX;
    float resizeStartY;    
    // Panning
    bool isPanning;
    Vector2 panStartMouse;
    Vector2 panStartOffset;
    // head linked list
    Node **head; 
} Context;

//ffwd declaration for behavioral node functions
typedef void (*BehaviorFn)(Node *node, Context *context); 

//each node will have a stack of behaviour functions
typedef struct {
    BehaviorFn stack[8];
    int top;
} BehaviorStack;

//wrapper struct for the node
typedef struct Node {
    Vector2 position;                      //screen position
    int width;                             //screen width
    int height;                            //screen height
    Node* nextZ;                           //pointer to next node on z-level
    bool isExpanded;                       //nodes can be expanded or compacted
    Connector connectors[MAX_CONNECTORS];  //list of connectors on the node for nested hitbox detection
    BehaviorStack behavior;                //per-node stack of behaviour functions
    char id[9];                            //random_ID 
    NodeType type;                         //type of the node, used for union
    union {                                //contains all the data in a union
        DefaultNode defaultNode;
        StackNode stackNode;
        RandomNode randomNode;
        RandomBagNode randomBagNode;
        UserChoiceNode userChoiceNode;
        SkillGateNode skillGateNode;
        GoToNode goToNode;
        ConditionalNode conditionalNode;
    } data;
} Node;



// Event Handlers
void HandleScreenToggle(ScreenSettings *screen);

// Dispatcher function
void DispatchNodeBehaviors(Node *head, Context *context);

// Behaviour functions
void Behavior_Drag(Node *node, Context *context);
void Behavior_DeleteIcon(Node *node, Context *context);
void Behavior_ExpandIcon(Node *node, Context *context);
void Behavior_CogIcon(Node *node, Context *context);
void Behavior_FocusOnClick(Node *node, Context *context);
void Behavior_ConnectorClick(Node *node, Context *context);
void Scene_ShrinkClick(SceneOutline *scene, Context *context);
void Scene_DeleteClick(SceneOutline *scene, Context *context);

// General Behavior functions
void HandleNodeCreationClick(Context *context, Node **head, Node *nodes);
void Behavior_PanCanvas(Context *context);
void Behavior_DrawSceneOutline(Context *context);
void UpdateSceneNodeMembership(Context *context);



//  node functions
Node* CreateNodeAt(Vector2 position, Node *head, Node *nodes, Context *context);
void BringNodeToTop(Node *target, Context *context);
void DeleteNodeFromList(Node *target, Context *context);

// Node draw decorations
void UpdateConnectorPositions(Node *node);

// Helper function
bool IsMouseDoubleClick(Context *context);
void RegisterBasicConnectors(Node *node);

// DEBUG
void Debug_PrintNodes(Node *nodes, Node *head);
void Debug_ContextAfterDelete(Node *nodes, Context *context);