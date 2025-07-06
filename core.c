#define RAYGUI_IMPLEMENTATION
#define RAYGUI_SUPPORT_ICONS
#include "raylib.h"
#include "raygui.h"
#include "raymath.h"

#include "core.h"
#include "ui.h"


Font globalFont;

int main(void)
{
    ScreenSettings screen = {.width = 1280, .height = 720, .currentSize = SCREEN_SMALL, .currentView = VIEW_MODE_NODE};

    InitWindow(screen.width, screen.height, "Node Prose - node based videogame narrative authoring tool");
    globalFont = LoadFont("resources/LSANSD.ttf");
    GuiSetFont(globalFont);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
    EnableCursor();
    SetTargetFPS(60);
    
    Node nodes[MAX_NODES] = {0};  // zero-initialize the node array
    for (int i = 0; i < MAX_NODES; i++) {
        nodes[i].type = NODE_COUNT;  // mark all as unused
    }
    
    Node *head = &nodes[0];  
    
    //intitial clean context
    Context context = {
        .nodes = nodes,
        .isDragging = false,
        .draggedNode = NULL,
        .dragOffset = {0},
        .lastClickTime = 0,
        .head = &head,
        .dragStartTime = 0,
        .dragCandidateNode = NULL,
        .bringToFront = NULL,
        .connecting = false,
        .connectionStart = {0},
        .bezier = {{0},{0},{0},{0}},
        .connectingFromNode = NULL,
        .connectingFromConnectorIndex = -1,
        .hoveredInputNode = NULL,
        .hoveredInputConnectorIndex = -1,
        .bezierCount = 0
    };
    
    //initial node for testing
        head->position = (Vector2){ 200, 200 };
        head->width = 200;
        head->height = 60;
        head->nextZ = NULL;
        head->isExpanded = false;
        head->type = NODE_DEFAULT;
        strcpy(head->id, "AAAA0001");
        RegisterBasicConnectors(head);
        

    while (!WindowShouldClose())
    {
        HandleScreenToggle(&screen);
        HandleNodeCreationClick(&context, &head, nodes);
        Behavior_PanCanvas(&context);
        
        
        
        DispatchNodeBehaviors(head, &context);
        
        

        BeginDrawing();
            ClearBackground(ORANGE);
            DrawBackground(&screen);
            DrawMenuBar(&screen);
            
            if (screen.currentView == VIEW_MODE_NODE) {
            
                DrawAllNodes(head, &context);
                DrawPermanentConnections(&context);
                DrawTopNodeAndConnections(head, &context);
                DrawLiveBezier(&context);
            
            } else if (screen.currentView == VIEW_MODE_SCRIPT) {
                DrawText("SCRIPT VIEW (not implemented yet)", 50, 100, 28, DARKGRAY);
            }
        EndDrawing();
    }

    UnloadFont(globalFont);
    CloseWindow();
    return 0;
}





//Screen F1 toggle function
void HandleScreenToggle(ScreenSettings *screen){
    if (IsKeyPressed(KEY_F1))
    {
        if (screen->currentSize == SCREEN_SMALL) {
            screen->width = 1920;
            screen->height = 1080;
            screen->currentSize = SCREEN_LARGE;
        } else {
            screen->width = 1280;
            screen->height = 720;
            screen->currentSize = SCREEN_SMALL;
        }

        SetWindowSize(screen->width, screen->height);

        Vector2 screenCenter = {
            (GetMonitorWidth(0) - screen->width) / 2.0f,
            (GetMonitorHeight(0) - screen->height) / 2.0f
        };
        SetWindowPosition((int)screenCenter.x, (int)screenCenter.y);
    }
}





// specific behaviour function for dragging 
void Behavior_Drag(Node *node, Context *context) {
    if (node->type == NODE_COUNT) return;
    
     // 🛑 Don't allow dragging during connector click
    if (context->connecting) return;

    Rectangle bounds = {
        node->position.x,
        node->position.y,
        node->width,
        (float)(node->isExpanded ? node->height * 5 : node->height)
    };

    Vector2 mouse = GetMousePosition();
    double now = GetTime();

    if (!context->isDragging) {
        // Step 1: Mouse pressed on a node — mark candidate
        if (CheckCollisionPointRec(mouse, bounds) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            context->dragCandidateNode = node;
            context->dragStartTime = now;
            context->bringToFront = node;
        }

        // Step 2: Candidate delay logic
        if (context->dragCandidateNode != NULL &&
            context->dragCandidateNode == node &&
            IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            
            if ((now - context->dragStartTime) >= 0.09) { // 90ms delay
                context->isDragging = true;
                context->draggedNode = node;
                context->dragOffset = Vector2Subtract(mouse, node->position);
                context->dragCandidateNode = NULL;
            }
        }

        // Step 3: Cancel candidate if mouse released early
        if (context->dragCandidateNode != NULL &&
            context->dragCandidateNode == node &&
            IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            
            context->dragCandidateNode = NULL;
        }
    }

    // Step 4: Active dragging
    if (context->isDragging && context->draggedNode == node) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            node->position = Vector2Subtract(mouse, context->dragOffset);

            // 🔁 Update connector positions to follow the node
            UpdateConnectorPositions(node);
            
            // 🔁 Update all bezier curves that connect to this node
           
            for (int j = 0; j < MAX_BEZIERS; j++) {

                BezierCurve *curve = &context->permanentBeziers[j];

            
                    
                // If this node is the FROM node in a curve
                if (node == curve->fromNode) {
                    Vector2 start = Vector2Subtract(mouse, context->dragOffset);
                    Vector2 start2 = Vector2Add(start, curve->relativeposition[0]);
                    curve->points[0] = start2;
                    curve->points[1] = (Vector2){start2.x + 50, start2.y};
                    
                    
                    

                } 
                
                if (node == curve->toNode) {
                    
                    
                    
                    Vector2 end = Vector2Subtract(mouse, context->dragOffset);
                    Vector2 end2 = Vector2Add(end, curve->relativeposition[1]);
                    curve->points[2] = (Vector2){end2.x - 50, end2.y};
                    curve->points[3] = end2;
                    
                    
                    
                    
                } 
            }

            SetMouseCursor(MOUSE_CURSOR_RESIZE_ALL);
        } else {
            context->isDragging = false;
            context->draggedNode = NULL;
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        }
    }
}

// Dispatch of behaviours
void DispatchNodeBehaviors(Node *head, Context *context) {
    BehaviorFn behaviors[] = {
        Behavior_Drag,
        Behavior_FocusOnClick,
        Behavior_ConnectorClick,
        Behavior_DeleteIcon,
        Behavior_ExpandIcon,
        Behavior_CogIcon
    };
    const int behaviorCount = sizeof(behaviors) / sizeof(behaviors[0]);

    Node *current = head;
    while (current != NULL) {
        // 💥 SKIP deleted nodes before running any behavior
        if (current->type != NODE_COUNT) {
            for (int i = 0; i < behaviorCount; i++) {
                behaviors[i](current, context);
            }
        }
        current = current->nextZ;
    }
    
    if (context->bringToFront != NULL) {
        BringNodeToTop(context->bringToFront, context);
        context->bringToFront = NULL;
    }
}

// Doubleclick helper function
bool IsMouseDoubleClick(Context *context) {
    double currentTime = GetTime();
    bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    if (clicked && (currentTime - context->lastClickTime) < 0.25) {
        context->lastClickTime = 0; // prevent triple click
        return true;
    }

    if (clicked) {
        context->lastClickTime = currentTime;
    }

    return false;
}

//Wrapper for creation new node
void HandleNodeCreationClick(Context *context, Node **head, Node *nodes) {
    if (IsMouseDoubleClick(context)) {
        Vector2 mousePos = GetMousePosition();

        // 🔍 Find the topmost node (tail of the Z-stack)
        Node *topNode = *head;
        while (topNode && topNode->nextZ) {
            topNode = topNode->nextZ;
        }

        // ✅ Skip creation if mouse is inside the top node's bounds
        if (topNode) {
            Rectangle bounds = {
                topNode->position.x,
                topNode->position.y,
                topNode->width,
                topNode->isExpanded ? topNode->height * 5 : topNode->height
            };

            if (CheckCollisionPointRec(mousePos, bounds)) {
                return; // 🔁 Mouse is over top node, abort creation
            }
        }

        // 👍 Safe to create
        *head = CreateNodeAt(mousePos, *head, nodes, context);
        SetMousePosition((int)(mousePos.x + 20), (int)(mousePos.y + 20));
    }
}

//Create new node logic
Node* CreateNodeAt(Vector2 position, Node *head, Node *nodes, Context *context) {
    for (int i = 0; i < MAX_NODES; i++) {
        Node *slot = &nodes[i];

        if (slot->type == NODE_COUNT) {  // safely detect unused slot
            memset(slot, 0, sizeof(Node));  // clear all fields just to be safe

            slot->position = position;
            slot->width = 200;
            slot->height = 60;
            slot->type = NODE_DEFAULT;
            slot->isExpanded = false;
            GenerateRandomID(slot->id, 8);
            
            RegisterBasicConnectors(slot);

            // Insert at front of the list
            slot->nextZ = head;
            return slot;  // new head
        }
    }

    return head; // fallback: no available slot
}





// delete node behavior function
void Behavior_DeleteIcon(Node *node, Context *context) {
    float size = 16.0f;
    float padding = 4.0f;

    Rectangle iconBounds = {
        node->position.x + node->width - size - padding,
        node->position.y + padding,
        size,
        size
    };

    Vector2 mouse = GetMousePosition();

    if (CheckCollisionPointRec(mouse, iconBounds) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        // Signal deletion — set type to NODE_COUNT to mark unused
        DeleteNodeFromList(node, context);
        
    }
}

//delete node logic function
void DeleteNodeFromList(Node *target, Context *context) {
    if (!target || !context || !context->head || !(*context->head) || !context->nodes) return;

    Node *head = *context->head;
    Node *nodes = context->nodes;

    // === 1. Remove from Z-stack linked list ===
    if (head == target) {
        *context->head = target->nextZ;
    } else {
        Node *prev = head;
        while (prev->nextZ && prev->nextZ != target) {
            prev = prev->nextZ;
        }
        if (prev->nextZ == target) {
            prev->nextZ = target->nextZ;
        } else {
            return; // Not found
        }
    }

    // === 2. Cancel drag if active ===
    if (context->draggedNode == target) {
        context->isDragging = false;
        context->draggedNode = NULL;
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }

    // === 3. Remove Bézier connections involving this node ===
    for (int i = 0; i < context->bezierCount; i++) {
        BezierCurve *curve = &context->permanentBeziers[i];
        if (curve->fromNode == target || curve->toNode == target) {
            // Shift remaining left
            for (int j = i; j < context->bezierCount - 1; j++) {
                context->permanentBeziers[j] = context->permanentBeziers[j + 1];
            }
            context->bezierCount--;
            i--; // Recheck current index
        }
    }

    // === 4. Nullify references TO this node from other nodes ===
    for (int i = 0; i < MAX_NODES; i++) {
        Node *n = &nodes[i];
        if (n->type == NODE_COUNT) continue;

        for (int c = 0; c < MAX_CONNECTORS; c++) {
            if (n->connectors[c].with.from == target) {
                n->connectors[c].with.from = NULL;
            }
            if (n->connectors[c].with.to == target) {
                n->connectors[c].with.to = NULL;
            }
        }
    }

    // === 5. Clear the node’s own connectors ===
    for (int c = 0; c < MAX_CONNECTORS; c++) {
        target->connectors[c].with.from = NULL;
        target->connectors[c].with.to = NULL;
    }

    // === 6. Mark node as unused ===
    target->type = NODE_COUNT;
    target->nextZ = NULL;
    target->width = 0;
    target->height = 0;
    target->position = (Vector2){0, 0};
    target->behavior.top = 0;
    memset(target->connectors, 0, sizeof(target->connectors));
}

// Gear icon behavior function
void Behavior_CogIcon(Node *node, Context *context) {
    float size = 16.0f;
    float padding = 4.0f;

    Rectangle iconBounds = {
        node->position.x + padding,
        node->position.y + padding, 
        size,
        size
    };

    Vector2 mouse = GetMousePosition();

    if (CheckCollisionPointRec(mouse, iconBounds) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        // Cycle node type
        node->type = (node->type + 1) % NODE_COUNT;
    }
}

//exand node behavioral function
void Behavior_ExpandIcon(Node *node, Context *context) {
    float size = 16.0f;
    float padding = 4.0f;

    Rectangle iconBounds = {
        node->position.x + node->width - 2 * (size + padding),
        node->position.y + padding,
        size,
        size
    };

    Vector2 mouse = GetMousePosition();

    if (CheckCollisionPointRec(mouse, iconBounds) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        node->isExpanded = !node->isExpanded;
    }
}

//behavior function to focus on click
void Behavior_FocusOnClick(Node *node, Context *context) {
    Rectangle bounds = {
        node->position.x,
        node->position.y,
        node->width,
        (float)(node->isExpanded ? node->height * 5 : node->height)
    };

    if (CheckCollisionPointRec(GetMousePosition(), bounds) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        context->bringToFront = node;
    }
}

// Bring node to top function
void BringNodeToTop(Node *target, Context *context) {
    if (!target || !context || !context->head || !(*context->head)) return;

    Node *head = *context->head;

    if (head == target && target->nextZ == NULL) return;  // already last

    // Unlink target
    if (head == target) {
        *context->head = target->nextZ;
    } else {
        Node *prev = head;
        while (prev->nextZ && prev->nextZ != target) {
            prev = prev->nextZ;
        }
        if (prev->nextZ == target) {
            prev->nextZ = target->nextZ;
        } else {
            return; // not found (shouldn't happen)
        }
    }

    // Find tail and insert at the end
    Node *tail = *context->head;
    if (!tail) {
        *context->head = target;
        target->nextZ = NULL;
        return;
    }

    while (tail->nextZ) {
        tail = tail->nextZ;
    }

    tail->nextZ = target;
    target->nextZ = NULL;
}



// register connectors
void RegisterBasicConnectors(Node *node) {
    float radius = 6.0f;
    float padding = 4.0f;

    // Input on the left side
    node->connectors[0] = (Connector){
        .center = {
            node->position.x + padding + radius,
            node->position.y + node->height / 2
        },
        .radius = radius,
        .type = CONNECTOR_INPUT,
        .parentType = node->type,
        .with = {0}
    };

    // Output on the right side
    node->connectors[1] = (Connector){
        .center = {
            node->position.x + node->width - padding - radius,
            node->position.y + node->height / 2
        },
        .radius = radius,
        .type = CONNECTOR_OUTPUT,
        .parentType = node->type,
        .with = {0}
    };
}

//update node connector positions
void UpdateConnectorPositions(Node *node) {
    for (int i = 0; i < MAX_CONNECTORS; i++) {
        if (node->connectors[i].radius > 0) {
            if (node->connectors[i].type == CONNECTOR_INPUT) {
                node->connectors[i].center = (Vector2){
                    node->position.x + 6 + 4,
                    node->position.y + node->height / 2
                };
            } else if (node->connectors[i].type == CONNECTOR_OUTPUT) {
                node->connectors[i].center = (Vector2){
                    node->position.x + node->width - 6 - 4,
                    node->position.y + node->height / 2
                };
            }
        }
    }
}

// Behaviour on connector click
void Behavior_ConnectorClick(Node *node, Context *context) {
    Vector2 mouse = GetMousePosition();

    for (int c = 0; c < MAX_CONNECTORS; c++) {
        Connector *conn = &node->connectors[c];
        if (conn->radius <= 0) continue;

        Rectangle hitbox = {
            conn->center.x - conn->radius,
            conn->center.y - conn->radius,
            conn->radius * 2,
            conn->radius * 2
        };

        // === HOVER HIGHLIGHT for INPUT CONNECTOR while connecting ===
        if (context->connecting && conn->type == CONNECTOR_INPUT && CheckCollisionPointRec(mouse, hitbox)) {
            context->hoveredInputNode = node;
            context->hoveredInputConnectorIndex = c;
        } else if (conn->type == CONNECTOR_INPUT && context->hoveredInputNode == node && context->hoveredInputConnectorIndex == c) {
            // Reset if no longer hovered
            context->hoveredInputNode = NULL;
            context->hoveredInputConnectorIndex = -1;
        }

        // === FINISHING a connection at an INPUT ===
        if (context->connecting &&
            conn->type == CONNECTOR_INPUT &&
            IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
            CheckCollisionPointRec(mouse, hitbox)) {

            Node *from = context->connectingFromNode;
            int fromIndex = context->connectingFromConnectorIndex;

            if (from && fromIndex >= 0) {
                Connector *fromConn = &from->connectors[fromIndex];

                // Store connection
                fromConn->with.to = node;
                conn->with.from = from;
                
                // === 2. Store permanent Bézier for visual link ===
                if (context->bezierCount < MAX_BEZIERS) {
                    
                    context->permanentBeziers[context->bezierCount++] = (BezierCurve){
                        .points = {
                            context->bezier[0],
                            context->bezier[1],
                            context->bezier[2],
                            context->bezier[3]
                        },
                        .fromNode = from,
                        .toNode = node,
                        .relativeposition = {
                            {
                                context->bezier[0].x - from->position.x,
                                context->bezier[0].y - from->position.y
                            },{
                                context->bezier[3].x - node->position.x,
                                context->bezier[3].y - node->position.y
                            }
                        }
                    };
                }

            }

            // Clear connection state
            context->connecting = false;
            context->connectingFromNode = NULL;
            context->connectingFromConnectorIndex = -1;
            return;
        }

        // === STARTING a connection from OUTPUT ===
        if (!context->connecting &&
            conn->type == CONNECTOR_OUTPUT &&
            IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
            CheckCollisionPointRec(mouse, hitbox)) {

            context->connecting = true;
            context->connectingFromNode = node;
            context->connectingFromConnectorIndex = c;
            context->connectionStart = conn->center;

            Vector2 start = conn->center;
            Vector2 end = mouse;
            context->bezier[0] = start;
            context->bezier[1] = (Vector2){start.x + 50, start.y};
            context->bezier[2] = (Vector2){end.x - 50, end.y};
            context->bezier[3] = end;

            return;
        }
    }
}





//full canvas panning
void Behavior_PanCanvas(Context *context) {
    Vector2 mouse = GetMousePosition();

    if (!context->isPanning) {
        // Start panning on Middle Mouse Button
        if (IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON)) {
            context->isPanning = true;
            context->panStartMouse = mouse;
            SetMouseCursor(MOUSE_CURSOR_RESIZE_ALL);
        }
    } else {
        // While holding the middle button
        if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
            Vector2 delta = Vector2Subtract(mouse, context->panStartMouse);

            Node *current = *(context->head);
            while (current != NULL) {
                current->position = Vector2Add(current->position, delta);
                UpdateConnectorPositions(current);
                current = current->nextZ;
            }

            // Also move bezier anchors
            for (int i = 0; i < context->bezierCount; i++) {
                for (int j = 0; j < 4; j++) {
                    context->permanentBeziers[i].points[j] = Vector2Add(context->permanentBeziers[i].points[j], delta);
                }
            }

            context->panStartMouse = mouse;
        } else {
            context->isPanning = false;
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        }
    }
}







// DEBUG FUNCTIONS 
/*

use:

Debug_PrintNodes(nodes, head);

in main()

*/
void Debug_PrintNodes(Node *nodes, Node *head) {
    printf("---- NODE LIST DEBUG ----\n");

    Node *current = head;
    int guard = 0;

    while (current != NULL && guard++ < MAX_NODES) {
        // Find index of this node in the array
        int index = -1;
        for (int i = 0; i < MAX_NODES; i++) {
            if (&nodes[i] == current) {
                index = i;
                break;
            }
        }

        printf("Node #%d (array index: %d) | Type: %d | Pos: (%.1f, %.1f)\n",
               guard, index, current->type, current->position.x, current->position.y);

        current = current->nextZ;
    }

    if (guard >= MAX_NODES) {
        printf("Warning: node traversal exceeded MAX_NODES — possible cyclic or corrupted list!\n");
    }

    printf("-------------------------\n");
}

void Debug_ContextAfterDelete(Node *nodes, Context *context) {
    printf("==== Context After Deletion ====\n");

    printf("IsDragging: %s\n", context->isDragging ? "true" : "false");
    printf("Dragged Node: %p\n", (void*)context->draggedNode);
    printf("Drag Offset: (%.1f, %.1f)\n", context->dragOffset.x, context->dragOffset.y);
    printf("Last Click Time: %.2f\n", context->lastClickTime);

    if (context->head && *(context->head)) {
        printf("Head Node: %p\n", (void*)*(context->head));
    } else {
        printf("Head Node: NULL\n");
    }

    Debug_PrintNodes(nodes, *(context->head));
}
