# Virtual Memory Manager

## Overview

This project includes two implementations of a virtual memory manager, designed to simulate memory management mechanisms commonly used in computer systems. Both projects manage virtual addresses, map them to physical addresses, and simulate page table and Translation Lookaside Buffer (TLB) operations. 

- **Project 1**: Basic Virtual Memory Manager with a FIFO-based TLB implementation.
- **Project 2**: Advanced Virtual Memory Manager with an LRU-based page replacement policy and aging register for page management.

## Project 1: Basic Virtual Memory Manager

### Description

Project 1 demonstrates a basic virtual memory management system using FIFO (First In, First Out) for TLB (Translation Lookaside Buffer) replacement. It includes the following features:
- Page Table Management
- TLB Management with FIFO Replacement
- Basic Physical Memory Simulation

### Key Components

- **Page Table**: Maps virtual pages to physical frames.
- **TLB**: Caches recent translations to speed up address translation, using FIFO replacement.
- **Physical Memory**: Simulates memory as an array of bytes.

### How to Run

1. **Compile the Code:**
   ```bash
   gcc project1.c
    ```
2. **Run the Program:**
    ```bash
    ./project1 addressfile.txt
    ```


***

## Project 2: Advanced Virtual Memory Manager

### Description

Project 2 extends Project 1 by implementing an advanced virtual memory management system using an LRU (Least Recently Used) page replacement policy. It includes:

* Page Table Management with Aging Register
* TLB Management with FIFO Replacement
* Enhanced Physical Memory Simulation with LRU and Aging

### Key Components

* **Page Table**: Includes an aging register to track page usage.
* **TLB**: Caches recent translations using FIFO replacement.
* **Physical Memory**: Enhanced with frame allocation and page replacement based on LRU policy.

### How to Run

1. **Compile the Code:**
   ```bash
   gcc project2.c
    ```
2. **Run the Program:**
    ```bash
    ./project2 addressfile.txt frame_number
    ```

***

### Metrics

    Page Fault Rate: The percentage of addresses that result in a page fault.
    TLB Hit Rate: The percentage of addresses that result in a TLB hit.