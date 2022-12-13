#ifndef __DB_H__
#define __DB_H__
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 *  Input Buffer
 *
 */
typedef struct
{
    char *buffer;
    size_t buffer_length;
    ssize_t input_length;
} InputBuffer;

/**
 *  state of query execution .
 *
 */
typedef enum
{
    EXECUTE_SUCCESS,
    EXECUTE_DUPLICATE_KEY,
} ExecuteResult;

/**
 *  对第一个字符为 '.' 原始输入命令的解析
 */
typedef enum
{
    META_COMMAND_SUCCESS,             /* .tree .btree .create  */
    META_COMMAND_UNRECOGNIZED_COMMAND /* error query */
} MetaCommandResult;

/**
 * recg InputBuffer's type
 *            if true -> Statement
 *
 */
typedef enum
{
    PREPARE_SUCCESS,
    PREPARE_NEGATIVE_ID,
    PREPARE_STRING_TOO_LONG,
    PREPARE_SYNTAX_ERROR,
    PREPARE_UNRECOGNIZED_STATEMENT
} PrepareResult;

/**
 * type of statement
 *                    SELECT
 *                    or
 *                    INSERT
 *
 */
typedef enum
{
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;

#define COLUMN_USERNAME_SIZE 32 /* length of username */
#define COLUMN_EMAIL_SIZE 255   /* length of email */

/**
 *  struct for
 *      [<4>(id) | <33>(username) | <256>(email)]
 *
 * +-----+-----------+---------------------+
 * | id  | username  |       email         |
 * +-----+-----------+---------------------+
 * | 1   |    Tom    | alskdjflaj@asd.com  |
 * +-----+-----------+---------------------+
 * | 2   |    Bob    | qlkjelk@aasdf.com   |
 * +-----+-----------+---------------------+
 */
typedef struct
{
    uint32_t id;                             /* 4 bytes */
    char username[COLUMN_USERNAME_SIZE + 1]; /* add one space for \0 */
    char email[COLUMN_EMAIL_SIZE + 1];       /* add one space for \0 */
} Row;

/**
 *  struct for Operation.
 *
 */
typedef struct
{
    StatementType type; /* SELECT or INSERT */
    Row row_to_insert;  /* only used by insert statement */
} Statement;

/**
 *  Get member's offset in struct
 *
 */
#define size_of_attribute(Struct, Attribute) sizeof(((Struct *)0)->Attribute)

/**
 *  struct member's Size and Offset.
 *
 */
#define ID_SIZE size_of_attribute(Row, id)
#define USERNAME_SIZE size_of_attribute(Row, username)
#define EMAIL_SIZE size_of_attribute(Row, email)
#define ID_OFFSET 0
#define USERNAME_OFFSET ID_OFFSET + ID_SIZE
#define EMAIL_OFFSET USERNAME_OFFSET + USERNAME_SIZE
#define ROW_SIZE (ID_SIZE + USERNAME_SIZE + EMAIL_SIZE)

#define PAGE_SIZE 4096      /* Common pagesize in most platform */
#define TABLE_MAX_PAGES 100 /* max page num of a table */

/**
 *
 * Pager:
 *    ┌─────────┐
 *    │  FILE   │
 *    │  int    │
 *    │  int    │
 *    │         │      Page[100]  :  TABLE_MAX_PAGES
 *    │  void*  │        ┌───┬───┬───┬───┬───┬───┐
 *    └─────────┘  ───>  |   |   |   |   |   |   |
 *                       └───┴───┴───┴───┴───┴───┘
 */
typedef struct
{
    int file_descriptor;
    uint32_t file_length;
    uint32_t num_pages;
    void *pages[TABLE_MAX_PAGES];
} Pager;

typedef struct
{
    Pager *pager;
    uint32_t root_page_num;
} Table;

typedef struct
{
    Table *table;
    uint32_t page_num;
    uint32_t cell_num;
    bool end_of_table; // Indicates a position one past the last element
} Cursor;

static void print_row(Row *row);

void print_row(Row *row)
{
    printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}

typedef enum
{
    NODE_INTERNAL,
    NODE_LEAF
} NodeType;

/*
 * Common Node Header Layout
 */
#define NODE_TYPE_SIZE sizeof(uint8_t)
#define NODE_TYPE_OFFSET 0
#define IS_ROOT_SIZE sizeof(uint8_t)
#define IS_ROOT_OFFSET NODE_TYPE_SIZE
#define PARENT_POINTER_SIZE sizeof(uint32_t)
#define PARENT_POINTER_OFFSET (IS_ROOT_OFFSET + IS_ROOT_SIZE)
#define COMMON_NODE_HEADER_SIZE \
    (NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE) /* 1 + 1 + 4 */

/*
 * Internal Node Header Layout
 */
#define INTERNAL_NODE_NUM_KEYS_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_NUM_KEYS_OFFSET COMMON_NODE_HEADER_SIZE
#define INTERNAL_NODE_RIGHT_CHILD_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_RIGHT_CHILD_OFFSET \
    (INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE)
#define INTERNAL_NODE_HEADER_SIZE \
    (COMMON_NODE_HEADER_SIZE + INTERNAL_NODE_NUM_KEYS_SIZE + INTERNAL_NODE_RIGHT_CHILD_SIZE) /* 6 + 4 + 4 */

/*
 * Internal Node Body Layout
 */
#define INTERNAL_NODE_KEY_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_CHILD_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_CELL_SIZE \
    (INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE)
/* Keep this small for testing */
#define INTERNAL_NODE_MAX_CELLS 3

/*
 * Leaf Node Header Layout
 */
#define LEAF_NODE_NUM_CELLS_SIZE sizeof(uint32_t)
#define LEAF_NODE_NUM_CELLS_OFFSET COMMON_NODE_HEADER_SIZE
#define LEAF_NODE_NEXT_LEAF_SIZE sizeof(uint32_t)
#define LEAF_NODE_NEXT_LEAF_OFFSET \
    (LEAF_NODE_NUM_CELLS_OFFSET + LEAF_NODE_NUM_CELLS_SIZE)
#define LEAF_NODE_HEADER_SIZE \
    (COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE + LEAF_NODE_NEXT_LEAF_SIZE) /* 6 + 4 + 4 */

/*
 * Leaf Node Body Layout
 */
#define LEAF_NODE_KEY_SIZE sizeof(uint32_t)
#define LEAF_NODE_KEY_OFFSET 0
#define LEAF_NODE_VALUE_SIZE ROW_SIZE
#define LEAF_NODE_VALUE_OFFSET \
    (LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE)                         /* Leaf Node Key Size */
#define LEAF_NODE_CELL_SIZE (LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE) /* Leaf Cell = Key + Value 293 + 4 */
#define LEAF_NODE_SPACE_FOR_CELLS (PAGE_SIZE - LEAF_NODE_HEADER_SIZE)   /* 4096 - Leaf Node Header */
#define LEAF_NODE_MAX_CELLS \
    ((LEAF_NODE_SPACE_FOR_CELLS) / (LEAF_NODE_CELL_SIZE)) /* How many Cells can */
#define LEAF_NODE_RIGHT_SPLIT_COUNT ((LEAF_NODE_MAX_CELLS + 1) / 2)
#define LEAF_NODE_LEFT_SPLIT_COUNT \
    ((LEAF_NODE_MAX_CELLS + 1) - LEAF_NODE_RIGHT_SPLIT_COUNT)

#endif