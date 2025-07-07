#define RAYGUI_IMPLEMENTATION
#define RAYGUI_SUPPORT_ICONS
#include "raylib.h"
#include "raygui.h"
#include "raymath.h"
#include <float.h>

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
        .bezierCount = 0,
        .sceneList = {.count = 0},          // no scenes yet
        .isDrawingScene = false,            // not currently drawing
        .sceneStartPos = {0, 0},            // initial mouse origin (irrelevant at boot)
        .draggedScene = NULL,
        .sceneDragOffset = {0, 0},
        .isResizingScene = false,
        .isResizingSceneVertically = false,
        .resizingScene = NULL,
        .initialSceneWidth = 0,
        .initialSceneHeight = 0,
        .resizeStartX = 0,
        .resizeStartY = 0
    };
    
    CreateInitialScene(&context);
    
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
        Behavior_DrawSceneOutline(&context); 
        
        
        
        DispatchNodeBehaviors(head, &context);
        
        UpdateSceneNodeMembership(&context);
        
        

        BeginDrawing();
            ClearBackground(ORANGE);
            DrawBackground(&screen);
            
            if (screen.currentView == VIEW_MODE_NODE) {
            
                DrawSceneOutlines(&context);
                DrawAllNodes(head, &context);
                DrawPermanentConnections(&context);
                DrawTopNodeAndConnections(head, &context);
                DrawLiveBezier(&context);
            
            } else if (screen.currentView == VIEW_MODE_SCRIPT) {
                DrawText("SCRIPT VIEW (not implemented yet)", 50, 100, 28, DARKGRAY);
            }
           
           
            DrawMenuBar(&screen);
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

// Draw Scene behaviour
void Behavior_DrawSceneOutline(Context *context) {
    Vector2 mouse = GetMousePosition();

    // === Begin Scene Draw on Right-Click ===
    if (!context->isDrawingScene && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        // Pre-check if mouse is inside any existing scene → block drawing start
        for (int i = 0; i < context->sceneList.count; i++) {
            if (CheckCollisionPointRec(mouse, context->sceneList.scenes[i].bounds)) {
                return; // 🚫 don't start drawing inside a scene
            }
        }

        context->sceneStartPos = mouse;
        context->isDrawingScene = true;
    }

    // === Finalize Scene Creation ===
    if (context->isDrawingScene && IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) {
        Vector2 end = mouse;
        Vector2 topLeft = {
            fminf(context->sceneStartPos.x, end.x),
            fminf(context->sceneStartPos.y, end.y)
        };
        Vector2 size = {
            fabsf(end.x - context->sceneStartPos.x),
            fabsf(end.y - context->sceneStartPos.y)
        };

        Rectangle newBounds = {topLeft.x, topLeft.y, size.x, size.y};

        // Prevent overlap with existing scenes
        bool overlapsExisting = false;
        for (int i = 0; i < context->sceneList.count; i++) {
            if (CheckCollisionRecs(newBounds, context->sceneList.scenes[i].bounds)) {
                overlapsExisting = true;
                break;
            }
        }

        if (!overlapsExisting && context->sceneList.count < MAX_SCENES) {
            context->sceneList.scenes[context->sceneList.count++] = (SceneOutline){
                .bounds = newBounds
            };
        }

        context->isDrawingScene = false;
    }
}

// specific behaviour function for dragging
void Behavior_Drag(Node *node, Context *context) {
    if (node->type == NODE_COUNT) return;

    // 🛑 Don’t allow dragging during connector click
    if (context->connecting) return;

    float nodeHeight = node->isExpanded ? node->height * 5 : node->height;

    Rectangle bounds = {
        node->position.x,
        node->position.y,
        node->width,
        nodeHeight
    };

    Vector2 mouse = GetMousePosition();
    double now = GetTime();

    // Step 1: Begin drag
    if (!context->isDragging) {
        if (CheckCollisionPointRec(mouse, bounds) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            context->dragCandidateNode = node;
            context->dragStartTime = now;
            context->bringToFront = node;
        }

        if (context->dragCandidateNode == node && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            if ((now - context->dragStartTime) >= 0.09) {
                context->isDragging = true;
                context->draggedNode = node;
                context->dragOffset = Vector2Subtract(mouse, node->position);
                context->dragCandidateNode = NULL;
            }
        }

        if (context->dragCandidateNode == node && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            context->dragCandidateNode = NULL;
        }
    }

    // Step 2: Active dragging
    if (context->isDragging && context->draggedNode == node) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            node->position = Vector2Subtract(mouse, context->dragOffset);
            UpdateConnectorPositions(node);

            // Update connected bezier curves
            for (int j = 0; j < MAX_BEZIERS; j++) {
                BezierCurve *curve = &context->permanentBeziers[j];

                if (node == curve->fromNode) {
                    Vector2 start = node->position;
                    Vector2 start2 = Vector2Add(start, curve->relativeposition[0]);
                    curve->points[0] = start2;
                    curve->points[1] = (Vector2){ start2.x + 50, start2.y };
                }

                if (node == curve->toNode) {
                    Vector2 end = node->position;
                    Vector2 end2 = Vector2Add(end, curve->relativeposition[1]);
                    curve->points[2] = (Vector2){ end2.x - 50, end2.y };
                    curve->points[3] = end2;
                }
            }

            // 🔁 Scene-aware directional snap logic
            for (int i = 0; i < context->sceneList.count; i++) {
                SceneOutline *scene = &context->sceneList.scenes[i];
                Rectangle sceneBounds = scene->bounds;

                Rectangle nodeBounds = {
                    node->position.x,
                    node->position.y,
                    (float)node->width,
                    nodeHeight
                };

                bool topLeftInside = CheckCollisionPointRec(node->position, sceneBounds);
                bool stillIntersecting = CheckCollisionRecs(nodeBounds, sceneBounds);

                if (!topLeftInside && stillIntersecting) {
                    Vector2 offset = {0};

                    // Snap horizontally (left side)
                    if (node->position.x < sceneBounds.x &&
                        nodeBounds.x + nodeBounds.width > sceneBounds.x) {
                        offset.x = -(nodeBounds.x + nodeBounds.width - sceneBounds.x + 1);
                    }

                    // Snap vertically (top side)
                    if (node->position.y < sceneBounds.y &&
                        nodeBounds.y + nodeBounds.height > sceneBounds.y) {
                        offset.y = -(nodeBounds.y + nodeBounds.height - sceneBounds.y + 1);
                    }

                    if (offset.x != 0 || offset.y != 0) {
                        node->position.x += offset.x;
                        node->position.y += offset.y;
                        UpdateConnectorPositions(node);
                    }
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

// behavior for scene magic wand click
void Scene_ShrinkClick(SceneOutline *scene, Context *context) {
    float iconSize = 16.0f;
    float iconPadding = 4.0f;

    Rectangle wandBounds = {
        scene->bounds.x + scene->bounds.width - (2 * (iconSize + iconPadding)),
        scene->bounds.y + iconPadding,
        iconSize,
        iconSize
    };

    Vector2 mouse = GetMousePosition();

       
    // === Hover feedback ===
    bool isHovered = CheckCollisionPointRec(mouse, wandBounds);
    Color bg = isHovered ? LIME : DARKGREEN;
    if (context->draggedNode || context->draggedScene != NULL) bg = DARKGREEN;
    DrawRectangleRec(wandBounds, bg);
    GuiDrawIcon(ICON_LASER, (int)wandBounds.x, (int)wandBounds.y, 1, WHITE);



    // === Click logic ===
    if (isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
    !context->draggedNode && !context->draggedScene) {
        ShrinkSceneToFitContent(scene);
    }
}

void Scene_DeleteClick(SceneOutline *scene, Context *context) {
    float iconSize = 16.0f;
    float iconPadding = 4.0f;
    
    Rectangle xBounds = {
        scene->bounds.x + scene->bounds.width - iconSize - iconPadding,
        scene->bounds.y + iconPadding,
        iconSize,
        iconSize
    };  

    Vector2 mouse = GetMousePosition();
    
    // === Hover feedback ===
    bool isHovered = CheckCollisionPointRec(mouse, xBounds);
    Color bg = isHovered ? LIME : DARKGREEN;
     if (context->draggedNode || context->draggedScene != NULL) bg = DARKGREEN;
    DrawRectangleRec(xBounds, bg);
    GuiDrawIcon(ICON_CROSS_SMALL, (int)xBounds.x, (int)xBounds.y, 1, WHITE);

    
    if (isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
    !context->draggedNode && !context->draggedScene) {
        for (int i = 0; i < context->sceneList.count; i++) {
            if (&context->sceneList.scenes[i] == scene) {
                for (int j = i; j < context->sceneList.count - 1; j++) {
                    context->sceneList.scenes[j] = context->sceneList.scenes[j + 1];
                }
                context->sceneList.count--;
                break;
            }
        }
    }
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

            // Move nodes
            Node *current = *(context->head);
            while (current != NULL) {
                current->position = Vector2Add(current->position, delta);
                UpdateConnectorPositions(current);
                current = current->nextZ;
            }

            // Move bezier anchors and relative positions
            for (int i = 0; i < context->bezierCount; i++) {
                BezierCurve *curve = &context->permanentBeziers[i];

                // Move anchor points
                for (int j = 0; j < 4; j++) {
                    curve->points[j] = Vector2Add(curve->points[j], delta);
                }

                /*
                // Move relative positions too
                for (int j = 0; j < 2; j++) {
                    curve->relativeposition[j].x += delta.x;
                    curve->relativeposition[j].y += delta.y;
                }
                */
            }

            // Move scene outlines
            for (int i = 0; i < context->sceneList.count; i++) {
                context->sceneList.scenes[i].bounds.x += delta.x;
                context->sceneList.scenes[i].bounds.y += delta.y;
            }

            context->panStartMouse = mouse;
        } else {
            context->isPanning = false;
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        }
    }
}

//function that adds nodes into scene visually
void UpdateSceneNodeMembership(Context *context) {
    if (!context || !context->nodes) return;
    
    
    if (context->isDragging || context->draggedNode != NULL) {
        return;
    }

    const float paddingX = 20.0f;
    const float paddingY = 20.0f;

    for (int s = 0; s < context->sceneList.count; s++) {
        SceneOutline *scene = &context->sceneList.scenes[s];

        // Skip active resizing (we still want to allow node dragging logic)
        if (context->resizingScene == scene) continue;

        // Backup previous state
        memcpy(scene->previousNodes, scene->containedNodes, sizeof(scene->containedNodes));
        scene->previousNodeCount = scene->nodeCount;
        scene->nodeCount = 0;

        Rectangle sceneBounds = scene->bounds;

        for (int i = 0; i < MAX_NODES; i++) {
            Node *node = &context->nodes[i];
            if (node->type == NODE_COUNT) continue;

            // Compute bounding box of this node
            float nodeHeight = node->isExpanded ? node->height * 5 : node->height;
            Rectangle nodeBounds = {
                node->position.x,
                node->position.y,
                (float)node->width,
                nodeHeight
            };

            bool topLeftInside = CheckCollisionPointRec(node->position, sceneBounds);
            bool intersects = CheckCollisionRecs(nodeBounds, sceneBounds);

            // Check previous membership
            bool wasInScene = false;
            for (int j = 0; j < scene->previousNodeCount; j++) {
                if (scene->previousNodes[j] == node) {
                    wasInScene = true;
                    break;
                }
            }

            // === CASE A: Node was NOT in scene, but now is (top-left inside)
            if (!wasInScene && topLeftInside) {
                // Expand bounds immediately to fit node fully
                float requiredLeft   = fminf(sceneBounds.x, nodeBounds.x - paddingX);
                float requiredTop    = fminf(sceneBounds.y, nodeBounds.y - paddingY);
                float requiredRight  = fmaxf(sceneBounds.x + sceneBounds.width, nodeBounds.x + nodeBounds.width + paddingX);
                float requiredBottom = fmaxf(sceneBounds.y + sceneBounds.height, nodeBounds.y + nodeBounds.height + paddingY);

                sceneBounds.x = requiredLeft;
                sceneBounds.y = requiredTop;
                sceneBounds.width = requiredRight - requiredLeft;
                sceneBounds.height = requiredBottom - requiredTop;

                if (scene->nodeCount < MAX_SCENE_NODES) {
                    scene->containedNodes[scene->nodeCount++] = node;
                }
            }

            // === CASE B: Node was already in scene
            else if (wasInScene) {
                if (topLeftInside) {
                    // Only expand bounds if not actively dragging this node
                    if (context->draggedNode != node) {
                        float requiredRight  = nodeBounds.x + nodeBounds.width + paddingX;
                        float requiredBottom = nodeBounds.y + nodeBounds.height + paddingY;

                        float currentRight = sceneBounds.x + sceneBounds.width;
                        float currentBottom = sceneBounds.y + sceneBounds.height;

                        if (requiredRight > currentRight) {
                            sceneBounds.width = requiredRight - sceneBounds.x;
                        }

                        if (requiredBottom > currentBottom) {
                            sceneBounds.height = requiredBottom - sceneBounds.y;
                        }
                    }

                    if (scene->nodeCount < MAX_SCENE_NODES) {
                        scene->containedNodes[scene->nodeCount++] = node;
                    }
                }
                // Optional: if top-left is now outside → drop node from scene
            }

            // === CASE C: Node intersects but was never in the scene
            else if (!wasInScene && !topLeftInside && intersects) {
                // Push scene to include this node (move top/left if needed)
                float requiredLeft   = fminf(sceneBounds.x, nodeBounds.x - paddingX);
                float requiredTop    = fminf(sceneBounds.y, nodeBounds.y - paddingY);
                float requiredRight  = fmaxf(sceneBounds.x + sceneBounds.width, nodeBounds.x + nodeBounds.width + paddingX);
                float requiredBottom = fmaxf(sceneBounds.y + sceneBounds.height, nodeBounds.y + nodeBounds.height + paddingY);

                sceneBounds.x = requiredLeft;
                sceneBounds.y = requiredTop;
                sceneBounds.width = requiredRight - requiredLeft;
                sceneBounds.height = requiredBottom - requiredTop;

                // Re-check top-left inclusion
                if (CheckCollisionPointRec(node->position, sceneBounds)) {
                    if (scene->nodeCount < MAX_SCENE_NODES) {
                        scene->containedNodes[scene->nodeCount++] = node;
                    }
                }
            }

            // Else: not in scene, not intersecting → ignore
        }

        scene->bounds = sceneBounds;
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
