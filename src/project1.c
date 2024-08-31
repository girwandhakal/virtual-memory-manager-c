// Name: Girwan Dhakal
// CWID: 12274204
#include<stdio.h>
#include<string.h>
#include<stdint.h>
#include<stdbool.h>
#include<stdlib.h>
#define backing_store "BACKING_STORE.bin"
#define PAGETABLE_SIZE 256
#define TLBSIZE 16

int cur_frame = -1; // current frame pointer to allocate free frames
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

typedef struct pagetable_entry{ // defines each page table entry
    int presentBit;
    int validBit;
    int frame;
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
        PT->entries[i].presentBit = 0;
        PT->entries[i].validBit = 0;
        PT->entries[i].frame = -1;
    }
    return PT;
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
    if(argc != 2)
    {
        printf("Usage:.\a.exe addressfile.txt \n");
        return 1;
    }
    char* addressFile = argv[1];
    pagetable* PT = createPageTable(); // this is the page table each page table index will contain the frame number
    TLB* tlb = createTLB(); //creates TLB

    int PM[65536]; // this is physical memory; each index of the physical memory will contain a signed byte.
    int i;
    for(i = 0; i < 65536; i++) // initialize the mem
    {
        PM[i] = NULL; 
    }
    int LA, PA, frameNumber;
    FILE* ptr1;
    ptr1 = fopen(addressFile,"r");
    if (ptr1 == NULL) {
    printf("Error opening file %s\n", addressFile);
    // Additional error handling code, if needed
    return 1; // Return an error code to indicate failure
}
    int count = 0;
    while(fscanf(ptr1, "%d", &LA) == 1)
    {
        frameNumber = getFrameNumber(tlb, LA);
        if(frameNumber >= 0) //tlb hit
        {
           PM[(frameNumber*256) + getOffset(LA)] = getStoredValue(LA);//acesses the frame number for current page number using page table
           printf("Virtual address: %d Physical address: %d Value: %d\n", LA,(frameNumber*256) + getOffset(LA) ,PM[(frameNumber*256) + getOffset(LA)]);
           updateTLB(tlb, LA, frameNumber);
           tlbHits++;
        }
        else // tlb miss
        {
        if(PT->entries[getPageNumber(LA)].frame == -1) //if the page number is not found in page table
        {
            int PA = storeInMemory(PM, LA, PT); //physical address
            PT->entries[getPageNumber(LA)].frame = cur_frame; // gets value from backing store and stores in memory, updates page table with the frame number
            printf("Virtual address: %d Physical address: %d Value: %d\n", LA, PA, PM[PA]);
            updateTLB(tlb, LA, PT->entries[getPageNumber(LA)].frame);
            pageFault++;
        }
        else // if page is in page table
        {         
           PM[(PT->entries[getPageNumber(LA)].frame*256) + getOffset(LA)] = getStoredValue(LA);//acccesses the frame number for current page number using page table
           printf("Virtual address: %d Physical address: %d Value: %d\n", LA,(PT->entries[getPageNumber(LA)].frame*256) + getOffset(LA) ,PM[(PT->entries[getPageNumber(LA)].frame*256) + getOffset(LA)]);
           updateTLB(tlb, LA, PT->entries[getPageNumber(LA)].frame);
        }
        }
        count++;
    }
    fclose(ptr1);
    //printf("For virtual address: %d   Page Number: %d, Offset: %d  \n",62615, getPageNumber(62615), getOffset(62615));
    printf("Number of Translated Addresses = %d\n", count);
    printf("Page Faults = %d \n", pageFault);
    printf("Page Fault Rate = %lf\n", getPageFaultRate(pageFault, count));
    printf("TLB Hits = %d \n", tlbHits);
    printf("TLB Hit Rate = %lf \n", getTlbHitRate(tlbHits, count));

}
