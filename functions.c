#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "header.h"

#define max_characters 30  // Maximum number of characters a block can hold

// Retrieves header fields based on index
int GetHeader(TOVS *file, int index) {
    if (index == 1) { return file->header.last_block_address; }
    else if (index == 2) { return file->header.last_block_free_position; }
    else if (index == 3) { return file->header.num_characters_inserted; }
    else { return file->header.num_characters_deleted; }
}

// Updates header fields based on index
void SetHeader(TOVS *file, int index, int value) {
    if (index == 1) { file->header.last_block_address = value; }
    else if (index == 2) { file->header.last_block_free_position = value; }
    else if (index == 3) { file->header.num_characters_inserted = value; }
    else { file->header.num_characters_deleted = value; }
}

// Writes to the i-th block
void WriteDirect(TOVS *file, int index, Block buffer) {
    fseek(file->file, sizeof(Header) + (sizeof(Block) * (index - 1)), SEEK_SET);
    fwrite(&buffer, sizeof(Block), 1, file->file);
}

// Reads from the i-th block
void ReadDirect(TOVS *file, int index, Block *buffer) {
    fseek(file->file, sizeof(Header) + (sizeof(Block) * (index - 1)), SEEK_SET);
    fread(buffer, sizeof(Block), 1, file->file);
}

// Allocates a new block by updating the header
void AllocateBlock(TOVS *file) {
    SetHeader(file, 1, GetHeader(file, 1) + 1);
}

// Converts a number to a string, padded with zeros to a specified length
void NumToString(int num, int max, char *str) {
    char str_num[4];  // Integer num is 3 characters long
    sprintf(str_num, "%d", num);
    int padding = max - strlen(str_num);
    sprintf(str, "%s", "");
    while (padding > 0) { sprintf(str, "%s0", str); padding--; }
    sprintf(str, "%s%s", str, str_num);
}

// Copies a substring from a string starting at position i up to max characters
void CopyString(char *src, int start, int max, char *dest) {
    sprintf(dest, "%s", "");
    while (start < strlen(src) && max > 0) {
        sprintf(dest, "%s%c", dest, src[start]);
        start++;
        max--;
    }
}

// Splits a string at a specified position
void SplitString(char *s, int i, int max) {
    char right_part[max_characters + 1];
    char left_part[max_characters + 1];
    CopyString(s, 0, i, left_part);
    CopyString(s, i + max, strlen(s), right_part);
    sprintf(s, "%s%s", left_part, right_part);
}

// Converts a semi-record to a record
void SemiToRecord(SemiRecord se, Record *rec) {
    char str[max_characters + 1];
    // the key
    sprintf(str, "%s", "");
    CopyString(se, 3, 3, str);
    rec->key = atoi(str);
    // the boolean
    sprintf(str, "%c", se[6]);
    rec->deleted = atoi(str);
    // the data
    CopyString(se, 7, strlen(se) - 7, rec->data);
}

// Converts a record to a semi-record
void RecordToSemi(Record rec, SemiRecord se) {
    char str[4];
    int length = strlen(rec.data);
    sprintf(se, "%s", "");
    // writing the data length to the semi-record
    NumToString(length, 3, str);
    sprintf(se, "%s%s", se, str);
    // writing the key to the semi-record
    NumToString(rec.key, 3, str);
    sprintf(se, "%s%s", se, str);
    // writing the deleted flag
    NumToString(rec.deleted, 1, str);
    sprintf(se, "%s%s", se, str);
    // writing the data
    sprintf(se, "%s%s", se, rec.data);
}

// Retrieves a semi-record from the file
void RetrieveSe(TOVS *file, int *i, int *j, SemiRecord se) {
    Block buf;
    char length[4]; // will hold the data length
    int k;
    sprintf(length, "%s", "");
    sprintf(se, "%s", "");
    ReadDirect(file, (*i), &buf);
    // retrieve the data length, int in length, char in se
    for (k = 0; k < 3; k++) {
        if ((*j) < max_characters) {
            sprintf(length, "%s%c", length, buf.array[*j]);
            sprintf(se, "%s%c", se, buf.array[*j]);
            (*j)++;
        } else {
            (*i)++;
            ReadDirect(file, (*i), &buf);
            sprintf(length, "%s%c", length, buf.array[0]);
            sprintf(se, "%s%c", se, buf.array[0]);
            (*j) = 1;
        }
    }
    // retrieve the key, boolean, and data in the variable se
    for (k = 0; k < (atoi(length) + 4); k++) {
        if ((*j) < max_characters) {
            sprintf(se, "%s%c", se, buf.array[*j]);
            (*j)++;
        } else {
            (*i)++;
            ReadDirect(file, (*i), &buf);
            sprintf(se, "%s%c", se, buf.array[0]);
            (*j) = 1;
        }
    }
}

// Display functions
void DisplayFile(TOVS *f) {
    int i = 1;
    int j = 0;
    Record rec;
    SemiRecord se;
    char length[4];
    printf("\n\n==================== Records ====================\n\n");
    printf(" Key   | Deleted | Data\n");
    printf("-------|---------|---------------------------------\n");
    while (i <= GetHeader(f, 1)) {
        RetrieveSe(f, &i, &j, se);
        SemiToRecord(se, &rec);
        NumToString(rec.key, 3, length);
        printf(" %-5s | %-7d | %s\n", length, rec.deleted, rec.data);
        if ((i == GetHeader(f, 1)) && (j == GetHeader(f, 2))) 
            break;
    }
    printf("===============================================\n\n");
}

void DisplayHeader(TOVS *f) {
    printf("\n\n================= File Characteristics =================\n\n");
    printf(" -> Last block address            : %d\n", GetHeader(f, 1));
    printf(" -> Free position in last block   : %d\n", GetHeader(f, 2));
    printf(" -> Number of characters inserted : %d\n", GetHeader(f, 3));
    printf(" -> Number of characters deleted  : %d\n", GetHeader(f, 4));
    printf("=======================================================\n\n");
}

void DisplayBlock(TOVS *f) {
    Block buf;
    int i;
    printf("\n\n=================== Block Contents ===================\n");
    if (GetHeader(f, 1) > 0) {
        for (i = 1; i <= GetHeader(f, 1); i++) {
            ReadDirect(f, i, &buf);
            printf(" Block %d contents : %s\n", i, buf.array);
        }
    } else {
        printf(" No blocks to display.\n");
    }
    printf("=====================================================\n\n");
}

void DisplayOverlap(TOVS *file) {
    int i = 1;
    int i1 = 1;  // i1 will compare with i
    int j = 0;
    int j1 = 0;
    Record rec;
    SemiRecord se;
    char length[4];
    printf("\n\n Overlaps:\n\n");
    while (i <= GetHeader(file, 1)) {
        RetrieveSe(file, &i1, &j1, se); // if i1 != i => block change, hence overlap
        if (i1 != i) {
            SemiToRecord(se, &rec);        // convert string to record
            NumToString(rec.key, 3, length);  // generate data length
            // display only if the record is not logically deleted
            if (rec.deleted == 0) {
                printf("between block %d and %d: %s | %d | %s\n", i, i1, length, rec.deleted, rec.data);
            }
            if (j1 == max_characters) {
                i1++;
                j1 = 0;
            }
            // i and j receive values of i1 and j1 for comparison in the next record
            i = i1;
            j = j1;
        }
        if ((i == GetHeader(file, 1)) && (j == GetHeader(file, 2))) break;
    }
}

// Opens a file with specified name and mode
TOVS *OpenFile(char file_name[30], char open_mode) {
    TOVS *file = (TOVS *)malloc(sizeof(TOVS));
    // If opened in new mode
    if ((open_mode == 'n') || (open_mode == 'N')) {
        file->file = fopen(file_name, "wb+");
        if (file->file != NULL) {
            printf("File opened successfully\n\n");
            // Initialize the header
            file->header.last_block_address = 0;
            file->header.last_block_free_position = 0;
            file->header.num_characters_inserted = 0;
            file->header.num_characters_deleted = 0;
            rewind(file->file);    // Position at the start of the file
            fwrite(&(file->header), sizeof(Header), 1, file->file);
        } else {
            printf("File creation failed\n");
        }
    }
    // If opened in existing mode
    else if ((open_mode == 'a') || (open_mode == 'A')) {
        file->file = fopen(file_name, "rb+");
        if (file->file != NULL) {
            printf("File opened successfully\n");
            rewind(file->file);
            fread(&(file->header), sizeof(Header), 1, file->file);
            DisplayHeader(file);
        } else {
            printf("Failed to open file; create the file first\n");
        }
    }
    return file;
}

void CloseFile(TOVS *file) {
    rewind(file->file);
    fwrite(&(file->header), sizeof(Header), 1, file->file);
    fclose(file->file);
    free(file);
}

// File manipulation --------------------------------------------------------------------------------------------------- 
void SearchTOVS(TOVS *file, int key, int *i, int *j, int *found) {
    (*found) = 0;
    (*i) = 1;
    (*j) = 0;
    int i1 = 1, j1 = 0;
    SemiRecord SR;
    Record R;
    if (GetHeader(file, 1) > 0)  // search only if the file is non-empty
    {
        sprintf(SR, "%s", "");
        while (!(*found) && (*i) <= GetHeader(file, 1)) {
            RetrieveSe(file, &i1, &j1, SR); // retrieve the record into SR and position to the beginning of the next record
            SemiToRecord(SR, &R);          // string record -> struct record
            if (!R.deleted && R.key >= key) break; // either the key does not exist, or we've found it
            if (!(*found)) {
                (*i) = i1;
                (*j) = j1;
            }
            if (((*i) == GetHeader(file, 1)) && (*j) == GetHeader(file, 2)) break;  // reached end without finding anything
        }
        if ((!R.deleted) && (R.key == key)) { (*found) = 1; }
    }
}

void LogicalDeletion(TOVS *file, int key) {
    Block buf;
    int i, j, found, k;
    char size[4];
    SearchTOVS(file, key, &i, &j, &found);
    if (found) {
        // retrieve size of data to update the GetHeader
        ReadDirect(file, i, &buf);
        sprintf(size, "%s", "");
        for (k = 0; k < 3; k++) {
            if (j < max_characters) { sprintf(size, "%s%c", size, buf.array[j]); j++; }
            else {
                i++;
                ReadDirect(file, i, &buf);
                sprintf(size, "%s%c", size, buf.array[0]);
                j = 1;
            }
        }
        // modify the deleted field; at this point, j is positioned at the first character of the key
        if (j + 3 < max_characters) {
            buf.array[j + 3] = '1';
            WriteDirect(file, i, buf);
        } else {
            // increment i to read the next block
            ReadDirect(file, i + 1, &buf);
            buf.array[(j + 3) % max_characters] = '1';  // modulo to reposition at beginning of block
            WriteDirect(file, i + 1, buf);
        }
        SetHeader(file, 4, GetHeader(file, 4) + atoi(size) + 7); // atoi(size) + 7 = length of logically deleted string
    }
}

void PhysicalDeletion(TOVS *file, int key) {
    int i, j, i1, j1, found, stop, k;
    char size[4];
    SemiRecord sr, part1, part2;
    Block buf;
    SearchTOVS(file, key, &i, &j, &found);
    printf("\nSearch result for key = %d\n i = %d,  j = %d, found = %d ", key, i, j, found);
    if (found) {
        i1 = i;  // i1 and j1 are used as traversal variables in buf
        j1 = j;
        stop = 0;
        RetrieveSe(file, &i1, &j1, sr); // position i and j at the beginning of the records
        printf("\n\nResult of retrieve_se\nsr = %s,  i1 = %d,  j1 = %d", sr, i1, j1);
        CopyString(sr, 0, 3, size);     // copy and retrieve the size of the record to delete
        printf("\n\nResult of copy_string\nsize = %s", size);
        while (!stop) {
            RetrieveSe(file, &i1, &j1, sr);  // retrieve the records to shift one by one
            printf("\n\nResult of retrieve_se inside while loop\nsr = %s,  i1 = %d,  j1 = %d", sr, i1, j1);
            CopyString(sr, 0, max_characters - j, part1);
            printf("\n\nResult of copy_string\npart1 = %s", part1);
            CopyString(sr, strlen(part1), strlen(sr), part2); // verify for strlen(sr)
            printf("\n\nResult of copy_string\npart2 = %s", part2);
            ReadDirect(file, i, &buf);
            buf.array[j] = '\0';           // clear string from j onwards
            sprintf(buf.array, "%s%s", buf.array, part1);
            WriteDirect(file, i, buf);
            j = j + strlen(part1);      // if another insert occurs in this block, it joins right after part1
            if (strlen(part2) != 0)     // strlen(part2) = 0 if and only if strlen(sr) <= max_characters - j
            {
                i = i1;
                j = strlen(part2);
                ReadDirect(file, i, &buf);
                for (k = 0; k < j; k++) { buf.array[k] = part2[k]; }  // shift string to start of block
                WriteDirect(file, i, buf);
            }
            if ((i1 >= GetHeader(file, 1)) && (j1 >= GetHeader(file, 2))) { stop = 1; }  // end of file, shifts completed
        }
        // update the GetHeader
        SetHeader(file, 1, i);
        SetHeader(file, 2, j);
        SetHeader(file, 3, GetHeader(file, 3) - (atoi(size) + 7));
    }
}

void InsertAtPosition(TOVS *file, int *i, int *j, SemiRecord SR) {
    Block buf;
    SemiRecord part1, part2, removed_part;   // part1, part2: used to split the string to insert
    // removed_part: holds the part of the block to shift for insertion, reinserted throughout traversal
    if ((*i) <= GetHeader(file, 1)) {
        sprintf(buf.array, "%s", "");
        sprintf(part2, "%s", "");
        sprintf(removed_part, "%s", "");
        ReadDirect(file, (*i), &buf);
        CopyString(SR, 0, max_characters - (*j), part1);  // copy into part1 the characters of SR that fit in block i
        CopyString(SR, strlen(part1), strlen(SR) - strlen(part1), part2); // rest of record to insert
        CopyString(buf.array, (*j), max_characters - (*j), removed_part);
        sprintf(part2, "%s%s", part2, removed_part);  // part2 now contains the string to insert
        buf.array[(*j)] = '\0';
        sprintf(buf.array, "%s%s", buf.array, part1);  // insert part1 into buf
        (*j) = (*j) + strlen(part1);
        WriteDirect(file, (*i), buf);
        if ((*i) == GetHeader(file, 1)) { SetHeader(file, 2, *j); }
        if (strlen(part2) != 0) {
            if (*j >= max_characters) { (*i)++; (*j) = (*j) % max_characters; }
            InsertAtPosition(file, i, j, part2);
        }
    } else {
        AllocateBlock(file);
        sprintf(buf.array, "%s", SR);
        (*j) = strlen(SR);
        SetHeader(file, 2, *j);
        WriteDirect(file, (*i), buf);
    }
}



void InsertTOVS(TOVS *file, Record R) {
    int i, j, found;
    SemiRecord SR;
    SearchTOVS(file, R.key, &i, &j, &found);
    if (!found) {
        RecordToSemi(R, SR);
        InsertAtPosition(file, &i, &j, SR);
        SetHeader(file, 3, GetHeader(file, 3) + strlen(SR));
    }
}

void ReorganizeTOVS(TOVS *file, char *new_file) {
    int i = 1; 
    int i1 = 1;
    int j = 0; 
    int j1 = 0;
    SemiRecord sr;
    Record rec;
    TOVS *new_f = OpenFile(new_file, 'N');  // create new file
    while (i <= GetHeader(file, 1)) {
        sprintf(sr, "%s", "");  // initialize sr as empty to avoid any issues
        RetrieveSe(file, &i, &j, sr);  // retrieve string from the old file
        SemiToRecord(sr, &rec);  // transform string into record
        if (rec.deleted == 0) {  // if deleted field is 0 <=> not logically deleted <=> write into new file
            InsertAtPosition(new_f, &i1, &j1, sr);
            SetHeader(new_f, 3, GetHeader(new_f, 3) + strlen(sr));  // update inserted character count
        }
        if ((i == GetHeader(file, 1)) && j == GetHeader(file, 2)) break;  // exit loop if end of old file is reached
    }
    DisplayHeader(new_f);
    DisplayFile(new_f);
    CloseFile(new_f);  // Close the new file
}

// Perform chosen operation
void PerformOperation(TOVS *file, int choice) {
    int key, num_records, i, j, found;
    Record record;
    switch (choice) {
        case 1:
            system("cls");
            printf("\n+-------------------------------------------------------------+\n");
            printf("|                          INSERTION                          |\n");
            printf("+-------------------------------------------------------------+\n");
            printf(" Number of records to insert: ");
            scanf("%d", &num_records);
            for (int k = 0; k < num_records; k++) {
                printf("\n Record %d: key = ", k + 1);
                scanf("%d", &record.key);
                printf("       data = ");
                scanf("%s", record.data);
                record.deleted = 0;
                InsertTOVS(file, record);
            }
            break;
        case 2:
            system("cls");
            printf("\n+-------------------------------------------------------------+\n");
            printf("|                          SEARCH                             |\n");
            printf("+-------------------------------------------------------------+\n");
            printf(" Enter the key to search for: ");
            scanf("%d", &key);
            SearchTOVS(file, key, &i, &j, &found);
            if (!found)
                printf(" Result: The key %d does not exist in the file\n", key);
            else
                printf(" Result: block %d, position %d\n", i, j);
            break;
        case 3:
            system("cls");
            printf("\n+-------------------------------------------------------------+\n");
            printf("|                     LOGICAL DELETION                        |\n");
            printf("+-------------------------------------------------------------+\n");
            printf(" Enter the key to logically delete: ");
            scanf("%d", &key);
            LogicalDeletion(file, key);
            break;
        case 4:
            system("cls");
            printf("\n+-------------------------------------------------------------+\n");
            printf("|                    PHYSICAL DELETION                        |\n");
            printf("+-------------------------------------------------------------+\n");
            printf(" Enter the key to physically delete: ");
            scanf("%d", &key);
            PhysicalDeletion(file, key);
            break;
        case 5:
            system("cls");
            printf("\n+-------------------------------------------------------------+\n");
            printf("|                          DISPLAY                            |\n");
            printf("+-------------------------------------------------------------+\n");
            DisplayHeader(file);
            DisplayFile(file);
            DisplayBlock(file);
            DisplayOverlap(file);
            break;
        case 6:
            system("cls");
            printf("\n+-------------------------------------------------------------+\n");
            printf("|                     REORGANIZATION                          |\n");
            printf("+-------------------------------------------------------------+\n");
            int k1 = GetHeader(file, 4);
            float k2 = GetHeader(file, 3);
            if (k1 / k2 >= 0.5) ReorganizeTOVS(file, "reorganized_file");
            break;
    }
    system("pause");
}

// File manipulation menu
void FileManipulationMenu(TOVS *file) {
    int sub_choice;
    while (1) {
        system("cls");
        printf("\n+-------------------------------------------------------------+\n");
        printf("|                    FILE MANIPULATION MENU                   |\n");
        printf("+-------------------------------------------------------------+\n");
        printf("| [1] : Insert                                               |\n");
        printf("| [2] : Search                                               |\n");
        printf("| [3] : Logical deletion                                     |\n");
        printf("| [4] : Physical deletion                                    |\n");
        printf("| [5] : Display                                              |\n");
        printf("| [6] : Reorganization                                       |\n");
        printf("| [0] : Return to main menu                                  |\n");
        printf("+-------------------------------------------------------------+\n");
        printf(" Your choice: ");
        scanf("%d", &sub_choice);
        if (sub_choice == 0) {
            CloseFile(file);
            return;  // Return to main menu
        }
        PerformOperation(file, sub_choice);
    }
}

// Main menu
void MainMenu() {
    char filename[30];
    int choice;
    TOVS *file;
    while (1) {
        system("cls");
        printf("\n+-------------------------------------------------------------+\n");
        printf("|                          MAIN MENU                          |\n");
        printf("+-------------------------------------------------------------+\n");
        printf("| [1] : Create a new file                                     |\n");
        printf("| [2] : Open an existing file                                 |\n");
        printf("| [0] : Exit program                                          |\n");
        printf("+-------------------------------------------------------------+\n");
        printf(" Your choice: ");
        scanf("%d", &choice);
        switch (choice) {
            case 0:
                return;  // Exit program
            case 1:
                system("cls");
                printf("\n+-------------------------------------------------------------+\n");
                printf("|                        FILE CREATION                        |\n");
                printf("+-------------------------------------------------------------+\n");
                printf(" Please enter the name of your file: ");
                scanf("%s", filename);
                file = OpenFile(filename, 'N');
                FileManipulationMenu(file);
                break;
            case 2:
                system("cls");
                printf("\n+-------------------------------------------------------------+\n");
                printf("|                         OPEN FILE                           |\n");
                printf("+-------------------------------------------------------------+\n");
                printf(" Please enter the name of your file: ");
                scanf("%s", filename);
                file = OpenFile(filename, 'A');
                FileManipulationMenu(file);
                break;
        }
    }
}


