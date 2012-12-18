/*
 * runtime.c
 *
 * Copyright 2008-2010 Apple, Inc. Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "Block_private.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "config.h"

#ifdef HAVE_AVAILABILITY_MACROS_H
#include <AvailabilityMacros.h>
#endif /* HAVE_AVAILABILITY_MACROS_H */

#ifdef HAVE_TARGET_CONDITIONALS_H
#include <TargetConditionals.h>
#endif /* HAVE_TARGET_CONDITIONALS_H */

#if defined(HAVE_OSATOMIC_COMPARE_AND_SWAP_INT) && defined(HAVE_OSATOMIC_COMPARE_AND_SWAP_LONG)

#ifdef HAVE_LIBKERN_OSATOMIC_H
#include <libkern/OSAtomic.h>
#endif /* HAVE_LIBKERN_OSATOMIC_H */

#elif defined(__WIN32__) || defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS 1
#include <windows.h>

static __inline bool OSAtomicCompareAndSwapLong(long oldl, long newl, long volatile *dst) {
    /* fixme barrier is overkill -- see objc-os.h */
    long original = InterlockedCompareExchange(dst, newl, oldl);
    return (original == oldl);
}

static __inline bool OSAtomicCompareAndSwapInt(int oldi, int newi, int volatile *dst) {
    /* fixme barrier is overkill -- see objc-os.h */
    int original = InterlockedCompareExchange(dst, newi, oldi);
    return (original == oldi);
}

/*
 * Check to see if the GCC atomic built-ins are available.  If we're on
 * a 64-bit system, make sure we have an 8-byte atomic function
 * available.
 *
 */

#elif defined(HAVE_SYNC_BOOL_COMPARE_AND_SWAP_INT) && defined(HAVE_SYNC_BOOL_COMPARE_AND_SWAP_LONG)

static __inline bool OSAtomicCompareAndSwapLong(long oldl, long newl, long volatile *dst) {
  return __sync_bool_compare_and_swap(dst, oldl, newl);
}

static __inline bool OSAtomicCompareAndSwapInt(int oldi, int newi, int volatile *dst) {
  return __sync_bool_compare_and_swap(dst, oldi, newi);
}

#else
#error unknown atomic compare-and-swap primitive
#endif /* HAVE_OSATOMIC_COMPARE_AND_SWAP_INT && HAVE_OSATOMIC_COMPARE_AND_SWAP_LONG */


/*
 * Globals:
 */

static const int _Block_copy_flag = BLOCK_NEEDS_FREE;
static const int _Byref_flag_initial_value = BLOCK_NEEDS_FREE | 2;

static const int WANTS_ONE = (1 << 16);

/*
 * Internal Utilities:
 */

static int latching_incr_int(int *where) {
    while (1) {
        int old_value = *(volatile int *)where;
        if ((old_value & BLOCK_REFCOUNT_MASK) == BLOCK_REFCOUNT_MASK) {
            return BLOCK_REFCOUNT_MASK;
        }
        if (OSAtomicCompareAndSwapInt(old_value, old_value+1, (volatile int *)where)) {
            return old_value+1;
        }
    }
}

static int latching_decr_int(int *where) {
    while (1) {
        int old_value = *(volatile int *)where;
        if ((old_value & BLOCK_REFCOUNT_MASK) == BLOCK_REFCOUNT_MASK) {
            return BLOCK_REFCOUNT_MASK;
        }
        if ((old_value & BLOCK_REFCOUNT_MASK) == 0) {
            return 0;
        }
        if (OSAtomicCompareAndSwapInt(old_value, old_value-1, (volatile int *)where)) {
            return old_value-1;
        }
    }
}


/*
 * GC support stub routines:
 */
#if 0
#pragma mark GC Support Routines
#endif /* if 0 */


static void *_Block_alloc_default(const unsigned long size, const bool initialCountIsOne, const bool isObject) {
    return malloc(size);
}

static void _Block_assign_default(void *value, void **destptr) {
    *destptr = value;
}

static void _Block_retain_object_default(const void *ptr) {
    if (!ptr) return;
}

static void _Block_release_object_default(const void *ptr) {
    if (!ptr) return;
}

static void _Block_assign_weak_default(const void *ptr, void *dest) {
    *(void **)dest = (void *)ptr;
}

static void _Block_memmove_default(void *dst, void *src, unsigned long size) {
    memmove(dst, src, (size_t)size);
}


/*
 * GC support callout functions - initially set to stub routines:
 */

static void *(*_Block_allocator)(const unsigned long, const bool isOne, const bool isObject) = _Block_alloc_default;
static void (*_Block_deallocator)(const void *) = (void (*)(const void *))free;
static void (*_Block_assign)(void *value, void **destptr) = _Block_assign_default;
static void (*_Block_retain_object)(const void *ptr) = _Block_retain_object_default;
static void (*_Block_release_object)(const void *ptr) = _Block_release_object_default;
static void (*_Block_assign_weak)(const void *dest, void *ptr) = _Block_assign_weak_default;
static void (*_Block_memmove)(void *dest, void *src, unsigned long size) = _Block_memmove_default;



/*
 * Internal Support routines for copying:
 */

#if 0
#pragma mark Copy/Release support
#endif /* if 0 */

/* Copy, or bump refcount, of a block.  If really copying, call the copy helper if present. */
static void *_Block_copy_internal(const void *arg, const int flags) {
    struct Block_layout *aBlock;

    //printf("_Block_copy_internal(%p, %x)\n", arg, flags);	
    if (!arg) return NULL;
    
    
    // The following would be better done as a switch statement
    aBlock = (struct Block_layout *)arg;
    if (aBlock->flags & BLOCK_NEEDS_FREE) {
        // latches on high
        latching_incr_int(&aBlock->flags);
        return aBlock;
    }
    else if (aBlock->flags & BLOCK_IS_GLOBAL) {
        return aBlock;
    }

    // Its a stack block.  Make a copy.
    struct Block_layout *result = malloc(aBlock->descriptor->size);
    if (!result) return (void *)0;
    memmove(result, aBlock, aBlock->descriptor->size); // bitcopy first
    // reset refcount
    result->flags &= ~(BLOCK_REFCOUNT_MASK);    // XXX not needed
    result->flags |= BLOCK_NEEDS_FREE | 1;
    if (result->flags & BLOCK_HAS_COPY_DISPOSE) {
        //printf("calling block copy helper %p(%p, %p)...\n", aBlock->descriptor->copy, result, aBlock);
        (*aBlock->descriptor->copy)(result, aBlock); // do fixup
    }
    return result;
}


/*
 * Runtime entry points for maintaining the sharing knowledge of byref data blocks.
 *
 * A closure has been copied and its fixup routine is asking us to fix up the reference to the shared byref data
 * Closures that aren't copied must still work, so everyone always accesses variables after dereferencing the forwarding ptr.
 * We ask if the byref pointer that we know about has already been copied to the heap, and if so, increment it.
 * Otherwise we need to copy it and update the stack forwarding pointer
 * XXX We need to account for weak/nonretained read-write barriers.
 */

static void _Block_byref_assign_copy(void *dest, const void *arg, const int flags) {
    struct Block_byref **destp = (struct Block_byref **)dest;
    struct Block_byref *src = (struct Block_byref *)arg;
        
    //printf("_Block_byref_assign_copy called, byref destp %p, src %p, flags %x\n", destp, src, flags);
    //printf("src dump: %s\n", _Block_byref_dump(src));
    if ((src->forwarding->flags & BLOCK_REFCOUNT_MASK) == 0) {
        //printf("making copy\n");
        // src points to stack
        struct Block_byref *copy = (struct Block_byref *)_Block_allocator(src->size, false, false);
        copy->flags = src->flags | _Byref_flag_initial_value; // non-GC one for caller, one for stack
        copy->forwarding = copy; // patch heap copy to point to itself (skip write-barrier)
        src->forwarding = copy;  // patch stack to point to heap copy
        copy->size = src->size;
        if (src->flags & BLOCK_HAS_COPY_DISPOSE) {
            // Trust copy helper to copy everything of interest
            // If more than one field shows up in a byref block this is wrong XXX
            copy->byref_keep = src->byref_keep;
            copy->byref_destroy = src->byref_destroy;
            (*src->byref_keep)(copy, src);
        }
        else {
            // just bits.  Blast 'em using _Block_memmove in case they're __strong
            _Block_memmove(
                (void *)&copy->byref_keep,
                (void *)&src->byref_keep,
                src->size - sizeof(struct Block_byref_header));
        }
    }
    // already copied to heap
    else if ((src->forwarding->flags & BLOCK_NEEDS_FREE) == BLOCK_NEEDS_FREE) {
        latching_incr_int(&src->forwarding->flags);
    }
    // assign byref data block pointer into new Block
    _Block_assign(src->forwarding, (void **)destp);
}

// Old compiler SPI
static void _Block_byref_release(const void *arg) {
    struct Block_byref *shared_struct = (struct Block_byref *)arg;
    int refcount;

    // dereference the forwarding pointer since the compiler isn't doing this anymore (ever?)
    shared_struct = shared_struct->forwarding;
    
    //printf("_Block_byref_release %p called, flags are %x\n", shared_struct, shared_struct->flags);
    // To support C++ destructors under GC we arrange for there to be a finalizer for this
    // by using an isa that directs the code to a finalizer that calls the byref_destroy method.
    if ((shared_struct->flags & BLOCK_NEEDS_FREE) == 0) {
        return; // stack or GC or global
    }
    refcount = shared_struct->flags & BLOCK_REFCOUNT_MASK;
    if (refcount <= 0) {
        printf("_Block_byref_release: Block byref data structure at %p underflowed\n", arg);
    }
    else if ((latching_decr_int(&shared_struct->flags) & BLOCK_REFCOUNT_MASK) == 0) {
        //printf("disposing of heap based byref block\n");
        if (shared_struct->flags & BLOCK_HAS_COPY_DISPOSE) {
            //printf("calling out to helper\n");
            (*shared_struct->byref_destroy)(shared_struct);
        }
        _Block_deallocator((struct Block_layout *)shared_struct);
    }
}


/*
 *
 * API supporting SPI
 * _Block_copy, _Block_release, and (old) _Block_destroy
 *
 */

#if 0
#pragma mark SPI/API
#endif /* if 0 */

void * __attribute__((weak)) _Block_copy(const void *arg) {
    return _Block_copy_internal(arg, WANTS_ONE);
}


// API entry point to release a copied Block
void __attribute__((weak)) _Block_release(void *arg) {
    struct Block_layout *aBlock = (struct Block_layout *)arg;
    int32_t newCount;
    if (!aBlock) return;
    newCount = latching_decr_int(&aBlock->flags) & BLOCK_REFCOUNT_MASK;
    if (newCount > 0) return;
    // Hit zero
    if (aBlock->flags & BLOCK_NEEDS_FREE) {
        if (aBlock->flags & BLOCK_HAS_COPY_DISPOSE)(*aBlock->descriptor->dispose)(aBlock);
        _Block_deallocator(aBlock);
    }
    else if (aBlock->flags & BLOCK_IS_GLOBAL) {
        ;
    }
    else {
        printf("Block_release called upon a stack Block: %p, ignored\n", (void *)aBlock);
    }
}



// Old Compiler SPI point to release a copied Block used by the compiler in dispose helpers
static void _Block_destroy(const void *arg) {
    struct Block_layout *aBlock;
    if (!arg) return;
    aBlock = (struct Block_layout *)arg;
    _Block_release(aBlock);
}



/*
 *
 * SPI used by other layers
 *
 */

// SPI
unsigned long int __attribute__((weak)) Block_size(void *arg) {
    return ((struct Block_layout *)arg)->descriptor->size;
}


#if 0
#pragma mark Compiler SPI entry points
#endif /* if 0 */

    
/*******************************************************

Entry points used by the compiler - the real API!


A Block can reference four different kinds of things that require help when the Block is copied to the heap.
1) C++ stack based objects
2) References to Objective-C objects
3) Other Blocks
4) __block variables

In these cases helper functions are synthesized by the compiler for use in Block_copy and Block_release, called the copy and dispose helpers.  The copy helper emits a call to the C++ const copy constructor for C++ stack based objects and for the rest calls into the runtime support function _Block_object_assign.  The dispose helper has a call to the C++ destructor for case 1 and a call into _Block_object_dispose for the rest.

The flags parameter of _Block_object_assign and _Block_object_dispose is set to
	* BLOCK_FIELD_IS_OBJECT (3), for the case of an Objective-C Object,
	* BLOCK_FIELD_IS_BLOCK (7), for the case of another Block, and
	* BLOCK_FIELD_IS_BYREF (8), for the case of a __block variable.
If the __block variable is marked weak the compiler also or's in BLOCK_FIELD_IS_WEAK (16).

So the Block copy/dispose helpers should only ever generate the four flag values of 3, 7, 8, and 24.

When  a __block variable is either a C++ object, an Objective-C object, or another Block then the compiler also generates copy/dispose helper functions.  Similarly to the Block copy helper, the "__block" copy helper (formerly and still a.k.a. "byref" copy helper) will do a C++ copy constructor (not a const one though!) and the dispose helper will do the destructor.  And similarly the helpers will call into the same two support functions with the same values for objects and Blocks with the additional BLOCK_BYREF_CALLER (128) bit of information supplied.

So the __block copy/dispose helpers will generate flag values of 3 or 7 for objects and Blocks respectively, with BLOCK_FIELD_IS_WEAK (16) or'ed as appropriate and always 128 or'd in, for the following set of possibilities:
	__block id                   128+3
        __weak block id              128+3+16
	__block (^Block)             128+7
	__weak __block (^Block)      128+7+16
        
The implementation of the two routines would be improved by switch statements enumerating the eight cases.

********************************************************/

/*
 * When Blocks or Block_byrefs hold objects then their copy routine helpers use this entry point
 * to do the assignment.
 */
void  __attribute__((weak)) _Block_object_assign(void *destAddr, const void *object, const int flags) {
    //printf("_Block_object_assign(*%p, %p, %x)\n", destAddr, object, flags);
    if ((flags & BLOCK_BYREF_CALLER) == BLOCK_BYREF_CALLER) {
        if ((flags & BLOCK_FIELD_IS_WEAK) == BLOCK_FIELD_IS_WEAK) {
            _Block_assign_weak(object, destAddr);
        }
        else {
            // do *not* retain or *copy* __block variables whatever they are
            _Block_assign((void *)object, destAddr);
        }
    }
    else if ((flags & BLOCK_FIELD_IS_BYREF) == BLOCK_FIELD_IS_BYREF)  {
        // copying a __block reference from the stack Block to the heap
        // flags will indicate if it holds a __weak reference and needs a special isa
        _Block_byref_assign_copy(destAddr, object, flags);
    }
    // (this test must be before next one)
    else if ((flags & BLOCK_FIELD_IS_BLOCK) == BLOCK_FIELD_IS_BLOCK) {
        // copying a Block declared variable from the stack Block to the heap
        _Block_assign(_Block_copy_internal(object, flags), destAddr);
    }
    // (this test must be after previous one)
    else if ((flags & BLOCK_FIELD_IS_OBJECT) == BLOCK_FIELD_IS_OBJECT) {
        //printf("retaining object at %p\n", object);
        _Block_retain_object(object);
        //printf("done retaining object at %p\n", object);
        _Block_assign((void *)object, destAddr);
    }
}

// When Blocks or Block_byrefs hold objects their destroy helper routines call this entry point
// to help dispose of the contents
// Used initially only for __attribute__((NSObject)) marked pointers.
void __attribute__((weak)) _Block_object_dispose(const void *object, const int flags) {
    //printf("_Block_object_dispose(%p, %x)\n", object, flags);
    if (flags & BLOCK_FIELD_IS_BYREF)  {
        // get rid of the __block data structure held in a Block
        _Block_byref_release(object);
    }
    else if ((flags & (BLOCK_FIELD_IS_BLOCK|BLOCK_BYREF_CALLER)) == BLOCK_FIELD_IS_BLOCK) {
        // get rid of a referenced Block held by this Block
        // (ignore __block Block variables, compiler doesn't need to call us)
        _Block_destroy(object);
    }
    else if ((flags & (BLOCK_FIELD_IS_WEAK|BLOCK_FIELD_IS_BLOCK|BLOCK_BYREF_CALLER)) == BLOCK_FIELD_IS_OBJECT) {
        // get rid of a referenced object held by this Block
        // (ignore __block object variables, compiler doesn't need to call us)
        _Block_release_object(object);
    }
}


/*
 * Debugging support:
 */
#if 0
#pragma mark Debugging
#endif /* if 0 */


const char * __attribute__((weak)) _Block_dump(const void *block) {
    struct Block_layout *closure = (struct Block_layout *)block;
    static char buffer[512];
    char *cp = buffer;
    if (closure == NULL) {
        sprintf(cp, "NULL passed to _Block_dump\n");
        return buffer;
    }
    if (! (closure->flags & BLOCK_HAS_DESCRIPTOR)) {
        printf("Block compiled by obsolete compiler, please recompile source for this Block\n");
        exit(1);
    }
    cp += sprintf(cp, "^%p (new layout) =\n", (void *)closure);
    cp += sprintf(cp, "flags:");
    if (closure->flags & BLOCK_HAS_DESCRIPTOR) {
        cp += sprintf(cp, " HASDESCRIPTOR");
    }
    if (closure->flags & BLOCK_NEEDS_FREE) {
        cp += sprintf(cp, " FREEME");
    }
    if (closure->flags & BLOCK_IS_GC) {
        cp += sprintf(cp, " ISGC");
    }
    if (closure->flags & BLOCK_HAS_COPY_DISPOSE) {
        cp += sprintf(cp, " HASHELP");
    }
    if (closure->flags & BLOCK_HAS_CTOR) {
        cp += sprintf(cp, " HASCTOR");
    }
    cp += sprintf(cp, "\nrefcount: %u\n", closure->flags & BLOCK_REFCOUNT_MASK);
    cp += sprintf(cp, "invoke: %p\n", (void *)(uintptr_t)closure->invoke);
    {
        struct Block_descriptor *dp = closure->descriptor;
        cp += sprintf(cp, "descriptor: %p\n", (void *)dp);
        cp += sprintf(cp, "descriptor->reserved: %lu\n", dp->reserved);
        cp += sprintf(cp, "descriptor->size: %lu\n", dp->size);

        if (closure->flags & BLOCK_HAS_COPY_DISPOSE) {
            cp += sprintf(cp, "descriptor->copy helper: %p\n", (void *)(uintptr_t)dp->copy);
            cp += sprintf(cp, "descriptor->dispose helper: %p\n", (void *)(uintptr_t)dp->dispose);
        }
    }
    return buffer;
}


const char *  __attribute__((weak)) _Block_byref_dump(struct Block_byref *src) {
    static char buffer[256];
    char *cp = buffer;
    cp += sprintf(cp, "byref data block %p contents:\n", (void *)src);
    cp += sprintf(cp, "  forwarding: %p\n", (void *)src->forwarding);
    cp += sprintf(cp, "  flags: 0x%x\n", src->flags);
    cp += sprintf(cp, "  size: %d\n", src->size);
    if (src->flags & BLOCK_HAS_COPY_DISPOSE) {
        cp += sprintf(cp, "  copy helper: %p\n", (void *)(uintptr_t)src->byref_keep);
        cp += sprintf(cp, "  dispose helper: %p\n", (void *)(uintptr_t)src->byref_destroy);
    }
    return buffer;
}

