# LinP

The fastest and most lightweight linearizability tester you will ever find.

## Histories

Histories are text files that provide a **data type** as header and **operations** on a single object of the stated data type in the following rows. Yes, we assume all operations to be completed by the end of the history.

**Data types** are prefixed with `#` followed by any of the supported tags:

- `set`
- `stack`
- `queue`
- `priorityqueue`

**Operations** are denoted by method, value, start time, and end time in that order. Refer to examples in `testcases` directory for supported methods for a given data type.

The input history must be _unambiguous_.

### Example

```
# stack
push 1 1 2
peek 2 3 4
pop 2 5 6
pop 1 7 8
```

## Usage

```bash
-bash-4.2$ ./linp [-txvh] <history_file>
```

### Options

- `-t`: report time taken in seconds
- `-x`: exclude peek operations (chooses faster algo if possible)
- `-v`: print verbose information
- `-h`: include header
- `--help`: show help message

### Output

The standard output shall be in the form:

```
"%d %f\n", <linearizability>, <time taken>
```

_linearizability_ prints `1` when input history is linearizable, `0` otherwise.

```bash
-bash-4.2$ ./build/linp -t testcases/priorityqueue/lin_simple_0.log
1 1.8e-05
```

## Time Complexity

| Data Type      | Time Complexity |
| -------------- | --------------- |
| Set            | $O(n)$          |
| Stack          | $O(n\log{n})$   |
| Queue          | $O(n\log{n})$   |
| Priority Queue | $O(n\log{n})$   |
