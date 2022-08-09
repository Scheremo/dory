/*
 * directional_allocator.h
 * Alessio Burrello <alessio.burrello@unibo.it>
 * Luka Macan <luka.macan@unibo.it>
 *
 * Copyright (C) 2019-2020 University of Bologna
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. 
 */


/* Directional allocator
 *
 *  Allocates memory depending on the direction argument:
 *    - direction == 1: allocates from the beginning of the memory
 *    - direction == 0: allocates from the ending of the memory
 */


#define NULL (void *)0


static void *directional_mem_begin = NULL;
static void *directional_mem_end = NULL;


static void directional_allocator_init(void *begin, int size) {
    directional_mem_begin = begin;
    directional_mem_end = begin + size;
}

static void *dmalloc(int size, int direction) {
    void *retval = NULL;
    if (directional_mem_begin + size < directional_mem_end) {
        if (direction == 1) {
            retval = directional_mem_begin;
            directional_mem_begin += size;
        } else {
            directional_mem_end -= size;
            retval = directional_mem_end;
        }
    }
    return retval;
}

static void dfree(int size, int direction) {
    if (direction == 1)
        directional_mem_begin -= size;
    else
        directional_mem_end += size;
}
