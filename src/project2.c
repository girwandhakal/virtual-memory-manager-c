// Name: Girwan Dhakal
// CWID: 12274204
#include<stdio.h>
#include<string.h>
#include<stdint.h>
#include<stdbool.h>
#include<stdlib.h>
#include<limits.h>
#define backing_store "BACKING_STORE.bin"
#define PAGETABLE_SIZE 256
#define TLBSIZE 16

int cur_frame = -1; // current frame pointer
int tlb_head = 0; // head of the TLB table to implement FIFO
int count = 0; // number of translated addresses
int pageFault = 0; // number of page faults
int tlbHits = 0; // number of tlb hits

double getTlbHitRate(int tlbHits,int count)
{
    return (double)tlbHits / count ;
}

double getPageFaultRate(int pageFault, int count)
{
    return (double)pageFault / count;
}

int getPageNumber(int num) // calculates page number from virtual address
{
    return (num >> 8) & 0xFF;
}

int getOffset(int num) // calculates offset from virtual address
{
    return num & 0xFF;
}

int getStoredValue(int LA) // returns the value of the signed byte stored at a certain virtual address LA
{
    int8_t value = 0;
    FILE* ptr2;
    ptr2 = fopen(backing_store, "rb");
    if(ptr2 == NULL)
    {
        printf("file failed to open \n");
    }
    if(fseek(ptr2, getPageNumber(LA)*256 + getOffset(LA), SEEK_SET) != 0)
    {
        printf("failed to seek \n");
    }
    if(fread(&value, sizeof(int8_t), 1, ptr2) != 1)
    {
        printf("Failed to read \n");
    }
    fclose(ptr2);
    return value;
}

typedef struct memoryFrames{
    int8_t data[256];
    int occupied;
}memoryFrames;

typedef struct physicalMemory{
    memoryFrames* frame;
}physicalMemory;

physicalMemory* createMemory(int size)
{
    physicalMemory* PM = (physicalMemory*) malloc(sizeof(physicalMemory));
    if(PM == NULL)
    {
        printf("Error creating page table \n");
    }

    PM->frame = (memoryFrames*) malloc(size * sizeof(memoryFrames));
    if (PM->frame == NULL) {
        printf("Error creating memory frames\n");
        free(PM);
        return NULL;
    }
    int i;

    for(i = 0; i < size; i++)
    {
        PM->frame[i].occupied = 0;
    }

    return PM;
}

typedef struct pagetable_entry{ // defines each page table entry
    int validBit;
    int frame;
    int agingRegister;
} pagetable_entry;

typedef struct pagetable{ // defines the pagetable
    pagetable_entry entries[PAGETABLE_SIZE];
} pagetable;

pagetable* createPageTable() //allocates memory for the page table and initializes the attribute values
{
    pagetable* PT = (pagetable*) malloc(sizeof(pagetable));
    if(PT == NULL)
    {
        printf("Error creating page table \n");
    }
    int i;
    for(i = 0; i < PAGETABLE_SIZE; i++)
    {
        PT->entries[i].validBit = 0;
        PT->entries[i].frame = 0;
        PT->entries[i].agingRegister = 0;
    }
    return PT;
}

// LRU Functions

pagetable_entry* findVictim(pagetable* PT) // find the victim page
{
    int smallestAge = INT_MAX;
    pagetable_entry* victim;
    int i;
    for(i = 0; i < PAGETABLE_SIZE; i++)
    {
        if(PT->entries[i].agingRegister < smallestAge)
        {
            victim = &PT->entries[i];
            smallestAge = victim->agingRegister;
        }
    }
    return victim;
}

int findHighestSequenceNumber(pagetable* PT)
{
    int max = 0;
    int i;
    for(i = 0 ; i < PAGETABLE_SIZE; i++)
    {
        if(PT->entries[i].agingRegister > max)
        {
            max = PT->entries[i].agingRegister;
        }
    }
    return max;
}

int allocateFrame(physicalMemory* PM, pagetable* PT, pagetable_entry* victim, int LA, int size) // LA is logical address 
{
    int i;
    i = 0;
    while(i < size && PM->frame[i].occupied == 1 ) // find free frame from memory
    {
        i++;
    }
    if(i >=size) // no free frames left
        {
        int8_t temp[256];
        FILE* fd = fopen(backing_store, "rb");
        if(fd == NULL)
        {
            printf("File failed to open \n");
            return 1;
        }
        if (fseek(fd, getPageNumber(LA) * 256, SEEK_SET) != 0) {
            printf("fseek failed");
            return 0; // Failed to seek in the file
        }

        // Read the frame from the backing store file into memory
        if (fread(&temp, sizeof(int8_t), 256, fd) != 256) {
            printf("fread failed");
            return 0; // Failed to read from the file
        }
        int j;
        for(j = 0; j < 256; j++)
        {
            PM->frame[victim->frame].data[j] = temp[j];
        }
        fclose(fd);
        //memcpy(&MEMORY[victim->frame * 256], addr, 256);
        victim->validBit = 0;  // victim frame is now overwritten ,thus not in memory anymore
        PT->entries[getPageNumber(LA)].frame = victim->frame;
        PT->entries[getPageNumber(LA)].validBit = 1;
        PT->entries[getPageNumber(LA)].agingRegister = findHighestSequenceNumber(PT);
    }
    else
    {
        PM->frame[i].data[getOffset(LA)] = getStoredValue(LA);
        PT->entries[getPageNumber(LA)].frame = i;
        PM->frame[i].occupied = 1;
    }
    return 1;
}

int storeInMemory(int MEMORY[], int LA, pagetable* PT) // stores the signed byte in a free frame and returns the physical address
{
    int base = 0;
    int PA;
    bool found = false;
    while(!found)
    {
        cur_frame = (cur_frame + 1) % 256; 
        if(MEMORY[(cur_frame * 256) + getOffset(LA)] == NULL) // frame and offset is free
        {
            MEMORY[(cur_frame*256) + getOffset(LA)] = getStoredValue(LA);
            PT->entries[getPageNumber(LA)].frame = cur_frame;
            PT->entries[getPageNumber(LA)].validBit = 1;
            PA = (cur_frame*256) + getOffset(LA);
            found = true;
        }
    }
    return PA; // physical address
}

// TLB section

typedef struct TLB_entry
{
    int pageNumber;
    int frameNumber;
}TLB_entry;

typedef struct TLB
{
    TLB_entry entries[TLBSIZE];
}TLB;

TLB* createTLB()
{
    TLB* tlb = (TLB*)malloc(sizeof(TLB));
    int i;
    for(i = 0; i < TLBSIZE; i++)
    {
        tlb->entries[i].pageNumber = -1;
        tlb->entries[i].frameNumber = -1; 
    }
    return tlb;
}

int getFrameNumber(TLB* tlb, int LA)
{
    int i;
    for(i = 0; i < TLBSIZE; i++)
    {
        if(tlb->entries[i].pageNumber == getPageNumber(LA))
        {
            return tlb->entries[i].frameNumber;
        }
    }
    return -1;
}

void updateTLB(TLB* tlb, int LA, int frame)
{
    tlb->entries[tlb_head].frameNumber = frame;
    tlb->entries[tlb_head].pageNumber = getPageNumber(LA);
    tlb_head = (tlb_head + 1) % TLBSIZE;
}

int main(int argc, char* argv[])
{
// dealing with command line arguments
    if(argc != 3)
    {
        printf("Usage:.\a.exe addresfile.txt frame_number \n");
        return 1;
    }

    int frame_num = atoi(argv[2]);
    char* addressFile = argv[1];

// initializing and creating page table, tlb and memory
    pagetable* PT = createPageTable(); // this is the page table each page table index will contain the frame number
    TLB* tlb = createTLB(); //creates TLB

    physicalMemory* PM = createMemory(frame_num);

    int LA, PA, frameNumber;

// implementation starts
    FILE* ptr1;
    ptr1 = fopen(addressFile,"r");
    int count = 0;
    while(fscanf(ptr1, "%d", &LA) == 1)
    {
        frameNumber = getFrameNumber(tlb, LA);
        if(frameNumber >= 0) //tlb hit
        {
           PM->frame[frameNumber].data[getOffset(LA)] = getStoredValue(LA);//acesses the frame number for current page number using page table
           printf("Virtual address: %d Physical address: %d Value: %d\n", LA,(frameNumber*256) + getOffset(LA) ,PM->frame[frameNumber].data[getOffset(LA)]);
           updateTLB(tlb, LA, frameNumber);
           tlbHits++;
        }
        else // tlb miss
        {
        if(PT->entries[getPageNumber(LA)].validBit == 0) //if the page number is not in memory ; page fault
        {

           pagetable_entry* victim = findVictim(PT); //victim frame

           allocateFrame(PM,PT,victim, LA, frame_num); //allocates new frame gets rid of victim

            printf("Virtual address: %d Physical address: %d Value: %d\n", LA, PT->entries[getPageNumber(LA)].frame * 256 + getOffset(LA),PM->frame[PT->entries[getPageNumber(LA)].frame].data[getOffset(LA)]);
            updateTLB(tlb, LA, PT->entries[getPageNumber(LA)].frame);
            pageFault++;
        }
        else // if page is in memory
        {         
           PM->frame[PT->entries[getPageNumber(LA)].frame].data[getOffset(LA)] = getStoredValue(LA);//acccesses the frame number for current page number using page table
           printf("Virtual address: %d Physical address: %d Value: %d\n", LA,(PT->entries[getPageNumber(LA)].frame*256) + getOffset(LA) ,PM->frame[PT->entries[getPageNumber(LA)].frame].data[getOffset(LA)]);
           updateTLB(tlb, LA, PT->entries[getPageNumber(LA)].frame);
           PT->entries[getPageNumber(LA)].agingRegister++; // increment aging if page was accessed
        }
        }
        count++;
    }
    fclose(ptr1);

    printf("Number of Translated Addresses = %d\n", count);
    printf("Page Faults = %d \n", pageFault);
    printf("Page Fault Rate = %lf\n", getPageFaultRate(pageFault, count));
    printf("TLB Hits = %d \n", tlbHits);
    printf("TLB Hit Rate = %lf \n", getTlbHitRate(tlbHits, count));

}
