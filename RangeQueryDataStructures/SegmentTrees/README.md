# Advanced C++ Segment Tree Suite

This repository contains a modular, production-grade suite of **Segment Tree** implementations in modern C++. The architecture heavily relies on the **Strategy Pattern** (Dependency Inversion) to decouple the physical tree traversal logic from the mathematical operations (Sum, Min, Max, Assignment, Addition). This allows for extreme reusability across a wide variety of advanced algorithmic challenges and system design applications.

## 📂 Repository Structure & Implementations

| File | Type | Complexity & Memory | Description & Best Use Case |
| :--- | :--- | :--- | :--- |
| `segmentTree.cpp` | **Recursive Static** | Update: $O(\log N)$<br>Space: $O(4N)$ | The classical array-based implementation. Nodes are updated via top-down recursion. Best used when the array size is fixed, memory is not strictly constrained, and only point updates are required. |
| `iterativeSegmentTree.cpp` | **Iterative (Bottom-Up)** | Update: $O(\log N)$<br>Space: $O(2N)$ | A highly optimized, non-recursive approach utilizing flat arrays and bitwise arithmetic (`ind >>= 1`). It builds and queries from the leaves up to the root. Extremely cache-friendly and fast due to low constant factors. Best for raw execution speed when only point updates are needed. |
| `lazySegmentTree.cpp` | **Lazy Propagation** | Update: $O(\log N)$<br>Space: $O(4N)$ | Extends the static tree using `segTreeNode` to support range updates. Employs a "lazy tag" philosophy to defer updates until a node is explicitly visited. Essential for applying operations across subarrays without $O(N)$ penalties. |
| `dynamicLazySegmentTree.cpp` | **Sparse / Dynamic Lazy** | Update: $O(\log(\text{MaxRange}))$<br>Space: $O(Q \log(\text{MaxRange}))$ | A pointer-based tree where nodes (`node*`) are instantiated on-demand. Handles massive virtual coordinate spaces (e.g., $10^{18}$) by skipping empty ranges. Includes lazy propagation built-in. Crucial for online queries on massive domains where coordinate compression isn't feasible. |
| `persistentSegmentTree.cpp` | **Persistent** | Update: $O(\log N)$<br>Space: $O(N + Q \log N)$ | Implements "Path Copying" using `std::shared_ptr`. Instead of mutating nodes, updates create new branches, safely preserving all previous versions of the tree. Includes specialized logic like `queryKth` for dual-root subtraction. Best for "Time Travel" queries, prefix-sum tree logic, and offline 2D range queries. |

## 🚀 Architectural Design: The Strategy Pattern

To avoid duplicating tree traversal logic for RMQ (Range Minimum Query) and RSQ (Range Sum Query), this suite utilizes abstract strategy classes:

1. **`MergeStrategy<T>`**: Defines how two child nodes combine to form a parent (e.g., `SumMain`, `MinMain`, `MaxMain`), and provides the identity element.
2. **`UpdateStrategy<T, U>`**: Defines how an update modifies an existing node and how lazy tags combine (e.g., `AddUpdate`, `SumAssignUpdate`, `MinAssignUpdate`).

### Example Initialization (Lazy Tree)
```cpp
// 1. Instantiate strategies using unique_ptr for clear ownership
vector<int> arr = {1, 2, 3, 4, 5};
auto mergeStrat = make_unique<SumMain<int>>();
auto updateStrat = make_unique<AddUpdate<int, int>>();

// 2. Initialize Tree
segTree<int, int> tree(arr, move(mergeStrat), move(updateStrat));

// 3. Range Update & Query
tree.update(1, 3, 5);     // Adds 5 to indices 1 through 3
int result = tree.search(0, 4); // Retrieves the total sum
