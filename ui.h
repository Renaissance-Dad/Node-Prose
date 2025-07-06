#ifndef UI_H
#define UI_H

#include "raylib.h"  // For Vector2
#include <stdbool.h>

//CORE DRAW FUNCTIONS
void DrawMenuBar(ScreenSettings *screen);
void DrawBackground(const ScreenSettings *screen);
void DrawAllNodes(Node *head, Context *context);
void DrawSingleNode(Node *node);
void DrawLiveBezier(Context *context);
void DrawPermanentConnections(Context *context);
void DrawPermanentConnectionsForNode(Node *node, Context *context);
void DrawTopNodeAndConnections(Node *head, Context *context);

// DRAW DECORATORS
void DrawNodeExpandIcon(Node *node, float size);
void DrawNodeCogIcon(Node *node, float size);
void DrawNodeDeleteIcon(Node *node, float size);

// HELPER FUNCTIONS
const char* GetNodeTypeName(int type); //feed it an enum and get the string 
void GenerateRandomID(char *buffer, int length);

#endif 