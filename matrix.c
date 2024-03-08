//-------------------------------------------------
// Roshini Pulle
// CRUZID: rpulle
// 1877859
// CSE 101 Winter 2023 
// PA4
// Implementation for ADT functions 
//------------------------------------------------
// took TA help and lab session help
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "Matrix.h"

// enums ----------------------------------------------------------------------
typedef enum
{
    SUM,
    DIFF,
} OP_t;

// structs --------------------------------------------------------------------

// private Entry type
typedef struct EntryObj *Entry;

// private EntryObj type
typedef struct EntryObj
{
    int col;
    double val;
} EntryObj;

// private MatrixObj type
typedef struct MatrixObj
{
    int rows;
    int NNZCount;
    List *entries;
} MatrixObj;

// Helper Function Declaration ------------------------------------------------
void insertInAscOrder(List L, Entry X);
Matrix sdHelper(Matrix A, Matrix B, OP_t operation);
double dot(List P, List Q);

// Constructors-Destructors ---------------------------------------------------

// newEntry()
// Returns reference to new Entry object. Initializes col and val fields.
Entry newEntry(int col, double val)
{
    Entry E = malloc(sizeof(EntryObj));
    E->col = col;
    E->val = val;
    return E;
}

// freeEntry()
// Frees heap memory pointed to by *pE, sets *pE to NULL.
void freeEntry(Entry *pE)
{
    if (pE != NULL && *pE != NULL)
    {
        free(*pE);
        *pE = NULL;
    }
}

// newMatrix()
// Returns a reference to a new nXn Matrix object in the zero state.
Matrix newMatrix(int n)
{
    Matrix M;
    M = malloc(sizeof(MatrixObj));
    M->rows = n;
    M->NNZCount = 0;
    M->entries = malloc(sizeof(List) * (n + 1));

    for (int i = 1; i <= n; i++)
    {
        M->entries[i] = newList();
    }

    return M;
}

// freeMatrix()
// Frees heap memory associated with *pM, sets *pM to NULL.
void freeMatrix(Matrix *pM)
{
    if (pM == NULL || *pM == NULL)
    {
        return;
    }
    Matrix M = *pM;
    for (int i = 1; i <= M->rows; i++)
    {
        List row = M->entries[i];
        if (row != NULL)
        {
            for (moveFront(row); index(row) >= 0; moveNext(row))
            {
                Entry E = get(row);
                freeEntry(&E);
            }
            freeList(&row);
            M->entries[i] = NULL;
        }
    }
    free(M->entries);
    M->entries = NULL;
    free(*pM);
    *pM = NULL;
}


// Access functions -----------------------------------------------------------

// size()
// Return the size of square Matrix M.
int size(Matrix M)
{
    if (M == NULL)
    {
        fprintf(stderr, "Matrix Error: calling size() on NULL Matrix reference\n");
        exit(EXIT_FAILURE);
    }

    return M->rows;
}

// NNZ()
// Return the number of non-zero elements in M.
int NNZ(Matrix M)
{
    if (M == NULL)
    {
        fprintf(stderr, "Matrix Error: calling NNZ() on NULL Matrix reference\n");
        exit(EXIT_FAILURE);
    }

    return M->NNZCount;
}

// equals()
// Return true (1) if matrices A and B are equal, false (0) otherwise.
int equals(Matrix A, Matrix B)
{
    if (A == NULL)
    {
        fprintf(stderr, "Matrix Error: calling equals() on NULL Matrix reference\n");
        exit(EXIT_FAILURE);
    }
    if (B == NULL)
    {
        fprintf(stderr, "Matrix Error: calling equals() on NULL Matrix reference\n");
        exit(EXIT_FAILURE);
    }

    if (A->rows != B->rows || A->NNZCount != B->NNZCount)
    {
        return 0;
    }

    for (int i = 1; i <= A->rows; i++)
    {
        List rowA = A->entries[i];
        List rowB = B->entries[i];
       
        for(moveFront(rowA),moveFront(rowB); index(rowA) >=0 && index(rowA) >=0; moveNext(rowA),moveNext(rowB)){
            Entry entryA = get(rowA);
            Entry entryB = get(rowB);
            if (entryA->col != entryB->col || entryA->val != entryB->val)
            {
                return 0;
        }
            }
        
        if (index(rowA) > -1 || index(rowB) > -1)
        {
            return 0;
        }
    }

    return 1;
}

// Manipulation procedures ----------------------------------------------------

// makeZero()
// Re-sets M to the zero Matrix state.
void makeZero(Matrix M)
{
    if (M == NULL)
    {
        fprintf(stderr, "Matrix Error: calling makeZero() on NULL Matrix reference\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 1; i <= M->rows; i++)
    {
        List row = M->entries[i];
        if (row != NULL)
        {
          
          for(moveFront(row); index(row) >= 0; moveNext(row)){
            Entry E = get(row);
                freeEntry(&E);
          }
        }
        clear(M->entries[i]);
    }

    M->NNZCount = 0;
}

// changeEntry()
// Changes the ith row, jth column of M to the value x.
// Pre: 1<=i<=size(M), 1<=j<=size(M)
void changeEntry(Matrix M, int i, int j, double x)
{
    if (M == NULL)
    {
        fprintf(stderr, "Matrix Error: calling changeEntry() on NULL Matrix reference\n");
        exit(EXIT_FAILURE);
    }
    if (i < 1 || i > size(M))
    {
        fprintf(stderr, "Matrix Error: calling changeEntry() on i < 1 or i > size(M)\n");
        exit(EXIT_FAILURE);
    }
    if (j < 1 || j > size(M))
    {
        fprintf(stderr, "Matrix Error: calling changeEntry() on j < 1 or j > size(M)\n");
        exit(EXIT_FAILURE);
    }

    List row = M->entries[i];
    Entry E;
    for(moveFront(row); index(row) >= 0; moveNext(row)){
      E = (Entry)get(row);
        if (E->col == j){
            if (x != 0.0){
                E->val = x;
            }
            else{
                freeEntry(&E);
                delete (M->entries[i]);
                M->NNZCount -= 1;
            }
            return;
        }
    }

    if (x != 0.0)
    {
        // insert in asc order (sorted)
        E = newEntry(j, x);
        insertInAscOrder(M->entries[i], E);
        M->NNZCount += 1;
    }
}

// Matrix Arithmetic operations -----------------------------------------------

// copy()
// Returns a reference to a new Matrix object having the same entries as A.
Matrix copy(Matrix A)
{
    if (A == NULL)
    {
        fprintf(stderr, "Matrix Error: calling copy() on NULL Matrix reference\n");
        exit(EXIT_FAILURE);
    }
    Matrix copyA = newMatrix(size(A));
    Entry E;
    for (int i = 1; i <= copyA->rows; i++)
    {
        List row = A->entries[i];
        for(moveFront(row); index(row) >= 0; moveNext(row)){
          E = get(row);
            changeEntry(copyA, i, E->col, E->val);
        }
    }
    return copyA;
}

// transpose()
// Returns a reference to a new Matrix object representing the transpose of A.
Matrix transpose(Matrix A)
{
    if (A == NULL)
    {
        fprintf(stderr, "Matrix Error: calling transpose() on NULL Matrix reference\n");
        exit(EXIT_FAILURE);
    }
    Matrix transA = newMatrix(size(A));
    Entry E;
    for (int i = 1; i <= size(A); i++)
    {
        List row = A->entries[i];
        for(moveFront(row); index(row) >=0; moveNext(row)){
          E = get(row);
          changeEntry(transA, E->col, i, E->val);
        }
    }
    return transA;
}

// scalarMult()
// Returns a reference to a new Matrix object representing xA.
Matrix scalarMult(double x, Matrix A)
{
    if (A == NULL)
    {
        fprintf(stderr, "Matrix Error: calling scalarMult() on NULL Matrix reference\n");
        exit(EXIT_FAILURE);
    }
    Matrix ax = newMatrix(size(A));
    Entry E;
    for (int i = 1; i <= size(A); i++) {
        List row = A->entries[i];
        for(moveFront(row); index(row) >= 0; moveNext(row)){
           E = get(row);
            changeEntry(ax, i, E->col, x * E->val);
        }
    }

    return ax;
}

// sum()
// Returns a reference to a new Matrix object representing A+B.
// pre: size(A)==size(B)
Matrix sum(Matrix A, Matrix B)
{
    if (A == NULL)
    {
        fprintf(stderr, "Matrix Error: calling sum() on NULL Matrix reference\n");
        exit(EXIT_FAILURE);
    }
    if (B == NULL)
    {
        fprintf(stderr, "Matrix Error: calling sum() on NULL Matrix reference\n");
        exit(EXIT_FAILURE);
    }
    if (size(A) != size(B)) {
        fprintf(stderr, "Matrix Error: calling sum() on size(A) != size(B)\n");
        exit(EXIT_FAILURE);
    }
  
    return sdHelper(A, B, SUM);
}

// diff()
// Returns a reference to a new Matrix object representing A-B.
// pre: size(A)==size(B)
Matrix diff(Matrix A, Matrix B)
{
    if (A == NULL)
    {
        fprintf(stderr, "Matrix Error: calling diff() on NULL Matrix reference\n");
        exit(EXIT_FAILURE);
    }
    if (B == NULL)
    {
        fprintf(stderr, "Matrix Error: calling diff() on NULL Matrix reference\n");
        exit(EXIT_FAILURE);
    }
    if (size(A) != size(B)) {
        fprintf(stderr, "Matrix Error: calling diff() on size(A) != size(B)\n");
        exit(EXIT_FAILURE);
    }
    return sdHelper(A, B, DIFF);
}

// product()
// Returns a reference to a new Matrix object representing AB
// pre: size(A)==size(B)
Matrix product(Matrix A, Matrix B)
{
    if (A == NULL)
    {
        fprintf(stderr, "Matrix Error: calling diff() on NULL Matrix reference\n");
        exit(EXIT_FAILURE);
    }
    if (B == NULL)
    {
        fprintf(stderr, "Matrix Error: calling diff() on NULL Matrix reference\n");
        exit(EXIT_FAILURE);
    }
    if (size(A) != size(B)) {
        fprintf(stderr, "Matrix Error: calling diff() on size(A) != size(B)\n");
        exit(EXIT_FAILURE);
    }
    
    Matrix result = newMatrix(size(A));
    Matrix transposeB = transpose(B);

    for (int i = 1; i <= size(A); i++) {
        for (int j = 1; j <= size(A); j++) {
            double dotProduct = dot(A->entries[i], transposeB->entries[j]);
            changeEntry(result, i, j, dotProduct);
        }
    }
    freeMatrix(&transposeB);
    return result;
}

// Other Functions ------------------------------------------------------------

// printMatrix()
// Prints a string representation of Matrix M to filestream out. Zero rows
// are not printed. Each non-zero row is represented as one line consisting
// of the row number, followed by a colon, a space, then a space separated
// list of pairs "(col, val)" giving the column numbers and non-zero values
// in that row. The double val will be rounded to 1 decimal point.
void printMatrix(FILE *out, Matrix M)
{
    if (M == NULL)
    {
        fprintf(stderr, "Matrix Error: calling printMatrix() on NULL Matrix reference\n");
        exit(EXIT_FAILURE);
    }
    Entry E;
    for (int i = 1; i <= size(M); i++){
        List row = M->entries[i];
        if (length(row) > 0)
        {
            fprintf(out, "%d: ", i);
          for(moveFront(row);index(row)>=0;moveNext(row)){
             E = get(row);
            fprintf(out, "(%d, %.1f) ", E->col, E->val);
          }
            fprintf(out, "\n");
        }
    }
}

// Helper Functions -----------------------------------------------------------

// Insert Entry X into List L and maintain ascending order
void insertInAscOrder(List L, Entry X)
{
    if (L == NULL)
    {
        fprintf(stderr, "Matrix Error: calling insertInAscOrder() on NULL List reference\n");
        exit(EXIT_FAILURE);
    }

    Entry E;
    int len = length(L);

    if (len == 0)
    {
        append(L, X);
    }
    else{
      for(moveFront(L);index(L)>=0;moveNext(L)){
        E = (Entry)get(L);
            if (E->col > X->col)
            {
                insertBefore(L, X);
                break;
            }
      }
        if (length(L) == len)
        {
            append(L, X);
        }
    }
}

// A helper function to either calculate the sum or difference between matrices
// got help from a student to get pseudocode at lab
Matrix sdHelper(Matrix A, Matrix B, OP_t operation) {
    int n = size(A);
    Matrix result;

    // Same matrix
    if (A == B) {
        if (operation == SUM) {
            result = scalarMult(2, A);
        }
        else {
            result = newMatrix(n);
        }
    }
    else {
        result = newMatrix(n);

        List rowA, rowB;
        Entry entryA, entryB;
        double x;
        for (int i = 1; i <= n; i++) {
            rowA = A->entries[i];
            rowB = B->entries[i];

            moveFront(rowA);
            moveFront(rowB);

             for(moveFront(rowA),moveFront(rowB);index(rowA) >= 0 && index(rowB) >= 0;) {
                entryA = get(rowA);
                entryB = get(rowB);

                if (entryA->col == entryB->col) {
                    if (operation == SUM) {
                        x = entryA->val + entryB->val;
                    }
                    else {
                        x = entryA->val - entryB->val;
                    }
                    changeEntry(result, i, entryA->col, x);
                    moveNext(rowA);
                    moveNext(rowB);
                }
                  else if (entryA->col < entryB->col) {
                    changeEntry(result, i, entryA->col, entryA->val);
                    moveNext(rowA);
                }
                else if (entryA->col > entryB->col) {
                    x = entryB->val;
                    if (operation == DIFF) {
                        x = -1 * x;
                    }
                    changeEntry(result, i, entryB->col, x);
                    moveNext(rowB);
                }
            }
      
            for(;index(rowA) >= 0;) {
                entryA = get(rowA);
                changeEntry(result, i, entryA->col, entryA->val);
                moveNext(rowA);
            }

            for(;index(rowB) >= 0;) {
                entryB = get(rowB);
                x = entryB->val;
                if (operation == DIFF) {
                    x = -1 * x;
                }
                changeEntry(result, i, entryB->col, x);
                moveNext(rowB);
            }
    }
  }

    return result;
}

// Dot product of two lists of entries
double dot(List P, List Q) {
    if (P == NULL) {
        fprintf(stderr, "Matrix Error: calling Dot() on NULL List reference\n");
        exit(EXIT_FAILURE);
    }
    if (Q == NULL) {
        fprintf(stderr, "Matrix Error: calling Dot() on NULL List reference\n");
        exit(EXIT_FAILURE);
    }

    double result = 0;

    Entry entryP, entryQ;

    for(moveFront(P),moveFront(Q); index(P) >=0 && index(Q) >=0;){
      entryP = get(P);
        entryQ = get(Q);

        if (entryP->col == entryQ->col) {
            result += entryP->val * entryQ->val;
            moveNext(P);
            moveNext(Q);
        }
        else if (entryP->col > entryQ->col) {
            moveNext(Q);
        }
        else if (entryP->col < entryQ->col) {
            moveNext(P);
        }
      }

    return result;
}
