---
title: Arrays
summary: An array is a row of same-sized boxes side by side in memory. Learn indexing, how arrays decay to pointers, and why out-of-bounds access is undefined behaviour.
minutes: 20
difficulty: easy
id: c-memory/arrays
mode: harness
---
An **array** is a block of identical boxes sitting right next to each other in memory, with no gaps. This is called **contiguous** memory. Picture a row of terraced houses where each house holds one `int`.

```c
int scores[4] = {90, 75, 60, 88};
```

If `scores` starts at address 1000 and an `int` takes 4 bytes, the elements live at 1000, 1004, 1008 and 1012.

## Indexing

`scores[i]` means "element number `i`". Counting starts at **0**, so the valid indices here are 0, 1, 2 and 3.

```c
printf("%d\n", scores[0]);   // 90
scores[3] = 100;             // change the last one
```

Because elements are evenly spaced, the computer finds `scores[i]` with simple arithmetic: start address plus `i` times the element size. That is why indexing is fast.

## Arrays decay to pointers

When you use an array's name in most expressions, it turns into a pointer to its first element. This is called **decay**. These two lines are equivalent:

```c
int *p = scores;       // same as &scores[0]
printf("%d\n", p[2]);  // 60, pointers can be indexed too
printf("%d\n", *(p + 2));  // also 60
```

`p + 2` means "two elements further along", not "two bytes". C scales by the element size for you. And `p[2]` is just shorthand for `*(p + 2)`.

## Passing arrays to functions

Decay has a big consequence: a function that receives an array only receives a pointer to the first element. It has **no idea how long the array is**. So you pass the length separately:

```c
int first_or_zero(const int *arr, size_t len) {
    if (len == 0) return 0;
    return arr[0];
}

int main(void) {
    int nums[3] = {5, 6, 7};
    printf("%d\n", first_or_zero(nums, 3));
    return 0;
}
```

`size_t` is the standard unsigned integer type for sizes and counts. `const int *` promises the function will only read the elements, not change them.

Inside the array's own scope, `sizeof(nums) / sizeof(nums[0])` gives the element count. Inside the function it does not work, because there `arr` is only a pointer.

## Out of bounds is undefined behaviour

```c
int scores[4] = {90, 75, 60, 88};
printf("%d\n", scores[4]);   // WRONG: only 0..3 exist
```

C does not check. Reading or writing past the end is **undefined behaviour**: the program might print garbage, crash, or appear to work today and break tomorrow. It is the source of countless real-world security bugs. The loop condition for an array of `len` elements is `i < len`, never `i <= len`.

## Common mistakes

- Off-by-one: looping to `i <= len`.
- Trying to learn an array's length inside a function with `sizeof`.
- Forgetting to initialise the running maximum or sum before the loop.

Your turn: sum an array and find its largest element.
