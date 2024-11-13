#ifndef HEADER_H
#define HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define max_characters 30  // Maximum number of characters a block can hold

// Header structure
typedef struct Header
{
    int last_block_address;       // Address of the last block, no need for the first block as the structure is an array (contiguous)
    int last_block_free_position; // Required for shifts in ordered insertion
    int num_characters_inserted;  // Needed for comparison with the number of deleted characters
    int num_characters_deleted;    // If > 50% => file reorganization is needed
} Header;

// Block structure
typedef struct Block
{
    char array[max_characters + 1];  // Variable size, so block is an array of characters
} Block;

// TOVS File structure
typedef struct TOVS
{
    FILE * file;
    Header header;
} TOVS;

// Semi-record structures
typedef char SemiRecord[max_characters + 1];  // String that will hold the data for read_string and write_string
typedef struct Record
{
    int key;       // Key of 3 characters
    int deleted;   // Boolean: 3 characters
    char data[max_characters - 6]; // Max size of the record is the block (6 = length size + key size + delete flag size + 1)
} Record;

// Function prototypes
int GetHeader(TOVS *file, int index);
void SetHeader(TOVS *file, int index, int value);
void WriteDirect(TOVS *file, int index, Block buffer);
void ReadDirect(TOVS *file, int index, Block *buffer);
void AllocateBlock(TOVS *file);
void NumToString(int num, int max, char *str);
void CopyString(char *src, int start, int max, char *dest);
void SplitString(char *s, int i, int max);
void SemiToRecord(SemiRecord se, Record *rec);
void RecordToSemi(Record rec, SemiRecord se);
void RetrieveSe(TOVS *file, int *i, int *j, SemiRecord se);
void DisplayFile(TOVS *f);
void DisplayHeader(TOVS *f);
void DisplayBlock(TOVS *f);
void DisplayOverlap(TOVS *file);
TOVS *OpenFile(char file_name[30], char open_mode);
void CloseFile(TOVS *file);
void SearchTOVS(TOVS *file, int key, int *i, int *j, int *found);
void LogicalDeletion(TOVS *file, int key);
void PhysicalDeletion(TOVS *file, int key);
void InsertAtPosition(TOVS *file, int *i, int *j, SemiRecord SR);
void InsertTOVS(TOVS *file, Record R);
void ReorganizeTOVS(TOVS *file, char *new_file);
void PerformOperation(TOVS *file, int choice);
void FileManipulationMenu(TOVS *file);
void MainMenu();

#endif // HEADER_H