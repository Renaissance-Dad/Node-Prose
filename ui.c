#include <stddef.h> // for NULL
#include <stdio.h>  // for snprintf

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
