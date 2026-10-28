#ifndef POLYPEN_MODEL_EDGE_H
#define POLYPEN_MODEL_EDGE_H

#include <stddef.h>

#include "model/point.h"

typedef enum {
  EDGE_CONSTRAINT_NONE = 0,
  EDGE_CONSTRAINT_LENGTH = 1 << 0,
  EDGE_CONSTRAINT_PARALLEL = 1 << 1
} EdgeConstraintType;

typedef enum { EDGE_TYPE_STRAIGHT, EDGE_TYPE_BEZIER, EDGE_TYPE_ARC } EdgeType;

typedef struct {
  Point control1;
  Point control2;
} CubicBezier;

typedef enum { ARC_SIDE_LEFT, ARC_SIDE_RIGHT } ArcSide;

typedef struct {
  ArcSide side;
  double cx;
  double cy;
  double radius;
  double start_angle;
  double end_angle;
} ArcGeometry;

typedef struct {
  double length;
} LengthConstraint;

typedef struct {
  struct EdgeNode *target;
  int group_id;
  int relative_sign;
} ParallelConstraint;

typedef struct {
  Point *start;
  Point *end;
  EdgeType type;
  EdgeConstraintType constraint_type;
  int constraint_mask;
  LengthConstraint length;
  ParallelConstraint parallel;
  union {
    CubicBezier bezier;
    ArcGeometry arc;
  } geometry;
} Edge;

typedef struct EdgeNode {
  Edge edge;
  struct EdgeNode *next;
} EdgeNode;

typedef struct {
  EdgeNode *head;
  size_t count;
} EdgeList;

EdgeList *edge_list_create(void);
void edge_list_destroy(EdgeList *list);
int edge_list_append(EdgeList *list, Point *start, Point *end);
int edge_set_bezier(Edge *edge, Point control1, Point control2);
int edge_set_arc(Edge *edge, ArcSide side);

int edge_set_constraint(Edge *edge, int constraint_flag);
void edge_set_length_constraint(Edge *edge, double target_length);
void edge_remove_length_constraint(Edge *edge);
void edge_set_parallel_constraint(EdgeNode *e1, EdgeNode *e2, int group_id);
void edge_remove_parallel_constraint(EdgeNode *edge_node);
void edge_remove_all_constraints(EdgeNode *edge_node);

void edge_list_solve_constraints(EdgeList *list, Point *fixed_point);

int edge_get_tangent_at_point(const EdgeNode *edge_node, const Point *p, gboolean incoming, double *out_dx, double *out_dy, double *out_c1_handle_len);
int edge_update_arc_geometry(EdgeNode *arc_node);


#endif
