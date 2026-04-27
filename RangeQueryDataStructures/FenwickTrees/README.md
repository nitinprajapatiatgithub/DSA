# Advanced C++ Fenwick Tree (Binary Indexed Tree) Suite

This repository contains a highly optimized, production-grade **Fenwick Tree** (Binary Indexed Tree) implementation in modern C++. Similar to the Segment Tree suite, this architecture heavily utilizes the **Strategy Pattern** (Dependency Inversion) to decouple the internal bitwise traversal logic from the mathematical operations (Sum, Multiplication, XOR, etc.). 

This approach ensures maximum reusability while maintaining the incredibly low constant-factor overhead that makes Fenwick Trees the go-to structure in competitive programming and high-performance systems.

## 📂 Repository Structure & Implementations

| File | Type | Complexity | Description & Best Use Case |
| :--- | :--- | :--- | :--- |
| `fenwickTree.cpp` | **Standard BIT** | Build: $O(N \log N)$<br>Update/Query: $O(\log N)$<br>Space: $O(N)$ | The classical implementation using point updates to build the initial tree. Safe, easy to trace, and reliable for online dynamic arrays. |
| `fenwickTreeWithLinearBuildTime.cpp` | **Optimized Linear Build** | Build: $O(N)$<br>Update/Query: $O(\log N)$<br>Space: $O(N)$ | An advanced bottom-up construction approach. Instead of updating each element individually, it pushes cumulative sums to immediate parent nodes `i + (i & -i)`. **Crucial for performance when initializing massive arrays or frequency tables.** |

## 🚀 Architectural Design: The Strategy Pattern

Unlike basic BIT implementations that are hardcoded for prefix sums, this library uses abstract strategy classes to support generic range operations:

1. **`MergeStrategy<T>`**: Defines how two elements combine (e.g., `sumMergeStrategy`) and provides the identity element.
2. **`UpdateStrategy<T, U>`**: Defines how an update mutates the tree's values.
3. **`InverseStrategy<T>`**: *Exclusive to Fenwick Trees.* Because BITs natively only answer **Prefix Queries** $[0, R]$, range queries $[L, R]$ require mathematically "subtracting" the prefix $[0, L-1]$ from $[0, R]$. This strategy defines how to perform that inversion (e.g., standard subtraction for sums, XOR for bitwise arrays).

### Usage Example
```cpp
#include "fenwickTreeWithLinearBuildTime.cpp"

// 1. Instantiate strategies
auto mergeStrat = make_unique<sumMergeStrategy<int>>();
auto updateStrat = make_unique<sumUpdateStrategy<int, int>>();
auto inverseStrat = make_unique<sumInverseStrategy<int>>();

// 2. Initialize Tree in O(N) Time
vector<int> arr = {1, 2, 3, 4, 5};
fenwickTree<int, int> ft(arr, move(mergeStrat), move(updateStrat), move(inverseStrat));

// 3. Point Update (Add 10 to index 2) -> {1, 2, 13, 4, 5}
ft.update(2, 10);

// 4. Prefix & Range Queries
int prefixSum = ft.queryPrefix(2);  // Returns 16 (1 + 2 + 13)
int rangeSum = ft.rangeQuery(1, 4); // Returns 24 (2 + 13 + 4 + 5)
