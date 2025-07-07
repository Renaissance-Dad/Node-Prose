#include <stddef.h> // for NULL
#include <stdio.h>  // for snprintf
#include <string.h>
#include <float.h>

#include "raylib.h"
#include "raymath.h"
#include "raygui.h"

#include "core.h"        // contains types and extern globalFont
#include "nodetypes.h"
#include "ui.h"          // contains prototypes for this file


// CORE FUNCTIONS
// Draw simple menu bar with Save, Load, Run
void DrawMenuBar(ScreenSettings *screen){
    static bool viewModeModalOpen = false;
    // Draw responsive menubar in lightpeach 
    Color LIGHTPEACH = { 255, 241, 232, 255 };
    int menuBarHeight = CLAMP(screen->height / 18, 40, 70);
    Rectangle menuBar = { 0, 0, (float)screen->width, (float)menuBarHeight };
    DrawRectangleRec(menuBar, Fade(LIGHTPEACH, 0.90f));
   
    // Scale padding and button size based on screen width
    float scale = screen->width / 1280.0f;  // Use 1280 as baseline
    int buttonWidth = (int)(120 * scale);
    int buttonHeight = menuBarHeight - 4;
    int buttonY = (menuBarHeight - buttonHeight) / 2;
    int padding = 4;

    // Set text alignment and padding (1 REM = font size)
    int fontSize = CLAMP((int)(menuBarHeight * 0.5f), 10, 32);
    GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(BUTTON, TEXT_PADDING, fontSize);
    
    // View Mode Button
    if (GuiButton((Rectangle){ padding + 3 * (buttonWidth + padding), buttonY, buttonWidth, buttonHeight }, "#116# View")) {
        viewModeModalOpen = true;
    }

    // Draw GUI Buttons
    GuiButton((Rectangle){ padding, buttonY, buttonWidth, buttonHeight }, "#4# Save");
    GuiButton((Rectangle){ padding + buttonWidth + padding, buttonY, buttonWidth, buttonHeight }, "#3# Load");
    GuiButton((Rectangle){ padding + 2 * (buttonWidth + padding), buttonY, buttonWidth, buttonHeight }, "#131# Run");
    
    if (viewModeModalOpen) {
        Rectangle modalBounds = {
            screen->width / 2 - 140,
            screen->height / 2 - 80,
            280,
            130
        };

        GuiWindowBox(modalBounds, "Select View Mode");

        Rectangle nodeBtn = { modalBounds.x + 20, modalBounds.y + 40, 240, 30 };
        Rectangle scriptBtn = { modalBounds.x + 20, modalBounds.y + 80, 240, 30 };

        if (GuiButton(nodeBtn, "Node View")) {
            screen->currentView = VIEW_MODE_NODE;
            viewModeModalOpen = false;
        }

        if (GuiButton(scriptBtn, "Script View")) {
            screen->currentView = VIEW_MODE_SCRIPT;
            viewModeModalOpen = false;
        }
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mouse = GetMousePosition();
            Rectangle closeBtn = {
                modalBounds.x + modalBounds.width - 20,
                modalBounds.y,
                20,
                20
            };

            if (CheckCollisionPointRec(mouse, closeBtn)) {
                viewModeModalOpen = false;
            }
        }  
    }
}

// Draw dotted background
void DrawBackground(const ScreenSettings *screen){
    Color dotColor = DARKGRAY;
    int spacing = 20;
    int dotRadius = 2;

    for (int y = spacing; y < screen->height; y += spacing)
    {
        for (int x = spacing; x < screen->width; x += spacing)
        {
            DrawCircle(x, y, dotRadius, dotColor);
        }
    }
}

// Function that draws all the nodes in the linked-list
void DrawAllNodes(Node *head, Context *context) {
    if (!head) return;

    Node *current = head;

    while (current != NULL) {
        //ok so it doesn't draw the last node...
        if (context && current == context->draggedNode) {
            current = current->nextZ;
            continue;
        }
        
        DrawSingleNode(current);
        

        current = current->nextZ;
    }
}

// draw single node
void DrawSingleNode(Node *node){
    if (!node || node->type == NODE_COUNT) return;
    
    
    // Calculate the visual bounds of the node
    Rectangle nodeRect = {
        node->position.x,
        node->position.y,
        (float) node->width,
        (float)(node->isExpanded ? node->height * 5 : node->height)
    };

    // Set border color: red if this is the head node
    Color borderColor = (node->nextZ == NULL) ? RED : DARKGRAY;

    // Draw background and border
    DrawRectangleRec(nodeRect, WHITE);
    DrawRectangleLinesEx(nodeRect, 2, borderColor);

    // Draw connectors
    for (int c = 0; c < MAX_CONNECTORS; c++) {
        Connector conn = node->connectors[c];
        if (conn.radius <= 0) continue;

        bool isConnected = false;

        if (conn.type == CONNECTOR_INPUT && conn.with.from != NULL && conn.with.from->type != NODE_COUNT) {
            isConnected = true;
        }
        if (conn.type == CONNECTOR_OUTPUT && conn.with.to != NULL && conn.with.to->type != NODE_COUNT) {
            isConnected = true;
        }

        if (isConnected) {
            // Solid blue if connected
            DrawCircleV(conn.center, conn.radius + 2, BLUE);
        } else {
            // White fill if not connected
            DrawCircleV(conn.center, conn.radius, WHITE);
            DrawCircleLines((int)conn.center.x, (int)conn.center.y, conn.radius, DARKGRAY);
        }
    }

    // Draw node title
    const char *title = GetNodeTypeName(node->type);
    int fontSize = GuiGetStyle(DEFAULT, TEXT_SIZE);
    int radius = node->connectors[0].radius;
    int padding = 4;    

    Vector2 titlePos = {
        node->position.x + 4 * padding + 2 * radius,
        node->position.y + (node->height - fontSize) / 2 - 2
    };

    GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiLabel((Rectangle){titlePos.x, titlePos.y, node->width, fontSize + 4}, title);

    // Icons
    DrawNodeDeleteIcon(node, 16.0f);
    DrawNodeExpandIcon(node, 16.0f);
    DrawNodeCogIcon(node, 16.0f);

    // Expanded node special visuals
    if (node->isExpanded) {
        if (node->type == NODE_DEFAULT) {
            // Draw text area box
            float boxHeight = node->height * 3.0f;
            float pad = 8.0f;
            Rectangle textArea = {
                node->position.x + pad,
                node->position.y + node->height + pad,
                node->width - 2 * pad,
                boxHeight
            };
            DrawRectangleLinesEx(textArea, 1.5f, DARKGRAY);

            // Draw Node ID at bottom inside the node
            char labelBuffer[32];
            snprintf(labelBuffer, sizeof(labelBuffer), "Node ID: %s", node->id);

            Vector2 textSize = MeasureTextEx(globalFont, labelBuffer, fontSize, 1);
            Vector2 drawPos = {
                node->position.x + (node->width - textSize.x) / 2.0f,
                node->position.y + node->height * 5 - fontSize - 6
            };

            DrawTextEx(globalFont, labelBuffer, drawPos, (float)fontSize, 1, GRAY);
        }
    } 
}

// Draw Live Bezier
void DrawLiveBezier(Context *context) {
    if (!context->connecting) return;

    Vector2 start = context->connectionStart;
    Vector2 end = GetMousePosition();

    context->bezier[0] = start;
    context->bezier[1] = (Vector2){start.x + 50, start.y};
    context->bezier[2] = (Vector2){end.x - 50, end.y};
    context->bezier[3] = end;

    DrawSplineBezierCubic(context->bezier, 4, 3.0f, RED);
    
    // 🔴 Draw red highlight dot on origin connector
    if (context->connectingFromNode && context->connectingFromConnectorIndex >= 0) {
        Connector conn = context->connectingFromNode->connectors[context->connectingFromConnectorIndex];
        float outerRadius = conn.radius + 2.5f;

        DrawCircleV(conn.center, outerRadius, RED);  // solid red dot
    }
    
    // 🔴 Draw red dot on hovered input connector during live connect
    if (context->hoveredInputNode && context->hoveredInputConnectorIndex >= 0) {
        Connector conn = context->hoveredInputNode->connectors[context->hoveredInputConnectorIndex];
        float outerRadius = conn.radius + 2.5f;

        DrawCircleV(conn.center, outerRadius, RED); // solid red dot
    }
    
    // Folded-in cancellation logic (end of frame)
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        bool overInput = (context->hoveredInputNode && context->hoveredInputConnectorIndex >= 0);
        bool overOrigin = false;

        if (context->connectingFromNode) {
            Rectangle originBounds = {
                context->connectingFromNode->position.x,
                context->connectingFromNode->position.y,
                (float)context->connectingFromNode->width,
                (float)(context->connectingFromNode->isExpanded
                        ? context->connectingFromNode->height * 5
                        : context->connectingFromNode->height)
            };
            overOrigin = CheckCollisionPointRec(GetMousePosition(), originBounds);
        }

        if (!overInput && !overOrigin) {
            TraceLog(LOG_INFO, "Live connection cancelled: clicked outside input and origin node.");
            context->connecting = false;
            context->connectingFromNode = NULL;
            context->connectingFromConnectorIndex = -1;
            context->hoveredInputNode = NULL;
            context->hoveredInputConnectorIndex = -1;
        }
    }
}

// draws permanent bezier connections
void DrawPermanentConnections(Context *context) {
    for (int i = 0; i < context->bezierCount; i++) {
        BezierCurve *curve = &context->permanentBeziers[i];

        if (curve->fromNode == context->draggedNode || curve->toNode == context->draggedNode) {
            continue;  // skip if connected to dragged node
        }

        Vector2 *points = curve->points;

        DrawSplineBezierCubic(points, 4, 3.0f, BLUE);
    }
}

// draw permanent connections
void DrawPermanentConnectionsForNode(Node *node, Context *context) {
    for (int i = 0; i < context->bezierCount; i++) {
        BezierCurve *curve = &context->permanentBeziers[i];

        if (curve->fromNode == node || curve->toNode == node) {
            Vector2 *points = curve->points;

            DrawSplineBezierCubic(points, 4, 3.0f, BLUE);
        }
    }
}

// function that draws the topnode and the permanent beziers connected to it
void DrawTopNodeAndConnections(Node *head, Context *context) {
    if (context->draggedNode) {
        DrawSingleNode(context->draggedNode);
        DrawPermanentConnectionsForNode(context->draggedNode, context);
    }
}

// function that draws scene outline
void DrawSceneOutlines(Context *context) {
    Vector2 mouse = GetMousePosition();
    bool cursorOverridden = false;

    for (int i = 0; i < context->sceneList.count; i++) {
        SceneOutline *scene = &context->sceneList.scenes[i];

        // === Label Setup ===
        int fontSize = 12;
        float iconSize = 16.0f;
        float iconPadding = 4.0f; 
        float labelHeight = 20.0f;
        char labelBuffer[64];
        snprintf(labelBuffer, sizeof(labelBuffer), "Scene: %s",
                 scene->name[0] != '\0' ? scene->name : "Unnamed");

        Vector2 textSize = MeasureTextEx(globalFont, labelBuffer, fontSize, 1);
        float labelPadding = 12.0f;
        float labelWidth = textSize.x + labelPadding;
        float minSceneWidth = labelWidth + iconSize + 2 * iconPadding;

        Rectangle labelBar = {
            scene->bounds.x,
            scene->bounds.y,
            labelWidth,
            labelHeight
        };

        // === Dragging via label ===
        if (!context->isResizingScene && !context->isResizingSceneVertically &&
            !context->draggedScene && CheckCollisionPointRec(mouse, labelBar) && (!context->draggedNode)) {
            SetMouseCursor(MOUSE_CURSOR_RESIZE_ALL);
            cursorOverridden = true;
        }

        if (!context->draggedScene && CheckCollisionPointRec(mouse, labelBar) &&
            IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            context->draggedScene = scene;
            context->sceneDragOffset = Vector2Subtract(mouse, (Vector2){scene->bounds.x, scene->bounds.y});
        }

        if (context->draggedScene == scene && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            Vector2 newPos = Vector2Subtract(mouse, context->sceneDragOffset);
            Rectangle newBounds = {
                newPos.x,
                newPos.y,
                scene->bounds.width,
                scene->bounds.height
            };

            // Prevent overlap with other scenes
            bool overlaps = false;
            for (int i = 0; i < context->sceneList.count; i++) {
                SceneOutline *other = &context->sceneList.scenes[i];
                
                if (other == scene) continue;

                if (CheckCollisionRecs(newBounds, other->bounds)) {
                    overlaps = true;
                    break;
                }
            }

            if (!overlaps) {
                Vector2 oldPos = { scene->bounds.x, scene->bounds.y };
                Vector2 delta = Vector2Subtract(newPos, oldPos);

                // Move the scene
                scene->bounds.x = newPos.x;
                scene->bounds.y = newPos.y;

                // Move all contained nodes
                for (int n = 0; n < scene->nodeCount; n++) {
                    Node *node = scene->containedNodes[n];
                    node->position = Vector2Add(node->position, delta);
                    UpdateConnectorPositions(node);
                }

                // Move all connected bezier curves of scene nodes
                for (int b = 0; b < context->bezierCount; b++) {
                    BezierCurve *curve = &context->permanentBeziers[b];

                    for (int n = 0; n < scene->nodeCount; n++) {
                        Node *node = scene->containedNodes[n];

                        if (curve->fromNode == node || curve->toNode == node) {
                            for (int j = 0; j < 4; j++) {
                                curve->points[j] = Vector2Add(curve->points[j], delta);
                            }
                            break; // Only process once per curve
                        }
                    }
                }
            }
        }

        if (context->draggedScene == scene && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            context->draggedScene = NULL;
        }

        // === Resize logic (unchanged) ===
        float resizeMargin = 12.0f;
        float resizeVerticalPadding = 32.0f;
        float bottomResizeMargin = 12.0f;
        float bottomHorizontalPadding = 32.0f;

        Rectangle rightEdge = {
            scene->bounds.x + scene->bounds.width - resizeMargin / 2,
            scene->bounds.y + resizeVerticalPadding,
            resizeMargin,
            scene->bounds.height - 2 * resizeVerticalPadding
        };
        bool hoveringRight = CheckCollisionPointRec(mouse, rightEdge);

        if (!context->isDragging && !context->isDrawingScene && !context->draggedNode 
            && !context->draggedScene && hoveringRight){
            SetMouseCursor(MOUSE_CURSOR_RESIZE_EW);
            cursorOverridden = true;
        }

        if (!context->isResizingScene && hoveringRight && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            context->isResizingScene = true;
            context->resizingScene = scene;
            context->resizeStartX = mouse.x;
            context->initialSceneWidth = scene->bounds.width;
        }

        Rectangle bottomEdge = {
            scene->bounds.x + bottomHorizontalPadding,
            scene->bounds.y + scene->bounds.height - bottomResizeMargin / 2,
            scene->bounds.width - 2 * bottomHorizontalPadding,
            bottomResizeMargin
        };
        bool hoveringBottom = CheckCollisionPointRec(mouse, bottomEdge);

        if (!context->isDragging && !context->isDrawingScene &&
            !context->draggedNode && !context->draggedScene && hoveringBottom) {
            SetMouseCursor(MOUSE_CURSOR_RESIZE_NS);
            cursorOverridden = true;
        }

        if (!context->isResizingSceneVertically && hoveringBottom && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            context->isResizingSceneVertically = true;
            context->resizingScene = scene;
            context->resizeStartY = mouse.y;
            context->initialSceneHeight = scene->bounds.height;
        }

        Rectangle cornerEdge = {
            scene->bounds.x + scene->bounds.width - bottomHorizontalPadding / 2,
            scene->bounds.y + scene->bounds.height - resizeVerticalPadding / 2,
            bottomHorizontalPadding,
            resizeVerticalPadding
        };
        bool hoveringCorner = CheckCollisionPointRec(mouse, cornerEdge);

        if (!context->isDragging && !context->isDrawingScene &&
            !context->draggedNode && !context->draggedScene && hoveringCorner) {
            SetMouseCursor(MOUSE_CURSOR_RESIZE_NWSE);
            cursorOverridden = true;
        }

        if (!context->isResizingScene && !context->isResizingSceneVertically &&
            hoveringCorner && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && (!context->draggedNode)) {
            context->isResizingScene = true;
            context->isResizingSceneVertically = true;
            context->resizingScene = scene;
            context->resizeStartX = mouse.x;
            context->resizeStartY = mouse.y;
            context->initialSceneWidth = scene->bounds.width;
            context->initialSceneHeight = scene->bounds.height;
        }

        if (context->resizingScene == scene && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            if (context->isResizingScene) {
                float deltaX = mouse.x - context->resizeStartX;
                scene->bounds.width = fmaxf(context->initialSceneWidth + deltaX, minSceneWidth);
            }
            if (context->isResizingSceneVertically) {
                float deltaY = mouse.y - context->resizeStartY;
                scene->bounds.height = fmaxf(context->initialSceneHeight + deltaY, 40);
            }
        }

        if ((context->isResizingScene || context->isResizingSceneVertically) &&
            context->resizingScene == scene && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            context->isResizingScene = false;
            context->isResizingSceneVertically = false;
            context->resizingScene = NULL;
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        }

        // === Draw Scene Border and Label ===
        DrawRectangleLinesEx(scene->bounds, 2, DARKGREEN);
        DrawRectangleRec(labelBar, DARKGREEN);

        Vector2 textPos = {
            labelBar.x + 6,
            labelBar.y + (labelBar.height - textSize.y) / 2
        };
        DrawTextEx(globalFont, labelBuffer, textPos, fontSize, 1, WHITE);

        // === Icons and behaviors ===
        Scene_ShrinkClick(scene, context);
        Scene_DeleteClick(scene, context);
    }

    // === Live Drawing Preview ===
    if (context->isDrawingScene) {
        Vector2 mouse = GetMousePosition();
        Vector2 start = context->sceneStartPos;

        Vector2 topLeft = {
            fminf(start.x, mouse.x),
            fminf(start.y, mouse.y)
        };
        Vector2 size = {
            fabsf(mouse.x - start.x),
            fabsf(mouse.y - start.y)
        };

        Rectangle previewBounds = {topLeft.x, topLeft.y, size.x, size.y};

        bool overlaps = false;
        for (int i = 0; i < context->sceneList.count; i++) {
            if (CheckCollisionRecs(previewBounds, context->sceneList.scenes[i].bounds)) {
                overlaps = true;
                break;
            }
        }

        // Blink red border if invalid placement
        Color previewColor;
        if (overlaps) {
            int blink = (int)(GetTime() * 4) % 2; // Fast blink
            previewColor = blink ? RED : BLANK;
        } else {
            previewColor = Fade(DARKGREEN, 0.5f);
        }

        DrawRectangleLinesEx(previewBounds, 2, previewColor);
    }

    // === Cursor Reset ===
    if (!cursorOverridden && !context->isResizingScene &&
        !context->isResizingSceneVertically && !context->draggedScene) {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }
}

// DRAW helper functions
void ShrinkSceneToFitContent(SceneOutline *scene){
    if (!scene || scene->nodeCount == 0) return;

    const float paddingX = 20.0f;
    const float paddingY = 20.0f;

    float minX = FLT_MAX, minY = FLT_MAX;
    float maxX = -FLT_MAX, maxY = -FLT_MAX;

    for (int i = 0; i < scene->nodeCount; i++) {
        Node *node = scene->containedNodes[i];
        if (!node || node->type == NODE_COUNT) continue;

        float nodeHeight = node->isExpanded ? node->height * 5 : node->height;

        float left = node->position.x;
        float top = node->position.y;
        float right = left + node->width;
        float bottom = top + nodeHeight;

        if (left < minX) minX = left;
        if (top < minY) minY = top;
        if (right > maxX) maxX = right;
        if (bottom > maxY) maxY = bottom;
    }

    scene->bounds.x = minX - paddingX;
    scene->bounds.y = minY - (2*paddingY);
    scene->bounds.width = (maxX - minX) + 2 * paddingX;
    scene->bounds.height = (maxY - minY) + 2 * paddingY;
}

// DECORATORS
// Draw expanded node
void DrawNodeExpandIcon(Node *node, float size) {
    float padding = 4.0f;

    // Position next to delete icon (to the left of it)
    Rectangle iconBounds = {
        node->position.x + node->width - (2 * (size + padding)) + (padding/2),
        node->position.y + padding,
        size,
        size
    };

    DrawRectangleRec(iconBounds, ORANGE);
    
    // Choose icon based on expansion state
    int icon = node->isExpanded ? ICON_ARROW_UP_FILL : ICON_ARROW_DOWN_FILL;
    
    GuiDrawIcon(icon, (int)iconBounds.x, (int)iconBounds.y, 1, WHITE);
}

// Draw cog Icon
void DrawNodeCogIcon(Node *node, float size) {
    float padding = 4.0f;

    Rectangle iconBounds = {
        node->position.x + padding,
        node->position.y + padding,  
        size,
        size
    };

    DrawRectangleRec(iconBounds, ORANGE);
    GuiDrawIcon(ICON_GEAR, (int)iconBounds.x, (int)iconBounds.y, 1, WHITE);
}

// Draw X icon
void DrawNodeDeleteIcon(Node *node, float size) {
    float padding = 4.0f;

    Rectangle iconBounds = {
        node->position.x + node->width - size - padding,
        node->position.y + padding,
        size,
        size
    };

    DrawRectangleRec(iconBounds, ORANGE);

    // Draw the raygui icon centered
    GuiDrawIcon(ICON_CROSS_SMALL, (int)iconBounds.x, (int)iconBounds.y, 1, WHITE);
}

//INIT FUNCTIONS
void CreateInitialScene(Context *context) {
    if (context->sceneList.count < MAX_SCENES) {
        SceneOutline scene = {
            .bounds = (Rectangle){ 150, 150, 300, 200 }
        };
        strncpy(scene.name, "Intro", sizeof(scene.name));
        context->sceneList.scenes[context->sceneList.count++] = scene;
    }
}

//HELPER FUNCTIONS
// Helper function, give it the enum type (which defaults to an int) and get the string
const char* GetNodeTypeName(int type) {
    static const char* names[] = {
        "Dialogue Node", "Stack Node", "Random Node", "Random Bag",
        "User Choice", "Skill Gate", "Go To", "If/Else"
    };
    return (type >= 0 && type < NODE_COUNT) ? names[type] : "Unknown";
}

// RandomID
void GenerateRandomID(char *buffer, int length) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < length; i++) {
        buffer[i] = charset[GetRandomValue(0, (int)(sizeof(charset) - 2))];
    }
    buffer[length] = '\0';
}
