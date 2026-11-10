Write a program that keeps a list of whole numbers and obeys commands. The input is any number of commands, one per line, until the input ends:

- `push N` puts the number `N` at the front of the list.
- `pop` removes the front number and prints it. If the list is empty, it prints `empty` instead.
- `reverse` turns the list around, so the back becomes the front.
- `print` prints the numbers from front to back on one line, separated by single spaces. An empty list prints `empty`.
- `count` prints how many numbers the list holds.

For example, the commands `push 1`, `push 2`, `push 3`, `print`, `reverse`, `print`, `pop`, `count` produce:

```
3 2 1
1 2 3
1
2
```

Store the numbers in a linked list of heap nodes, with no upper limit on its length. Other command series are tried, including long ones and ones that pop from an empty list. Start from the empty file: choose your own node type, functions and `main`.
