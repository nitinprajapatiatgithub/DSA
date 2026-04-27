# Sparse Table

A Sparse Table is a static data structure that allows for efficient range queries on an array. It is particularly well-suited for situations where the underlying data does not change (static) and requires extremely fast query times.

### Complexity
* **Build Time**: $O(N \log N)$ to precompute the table states.
* **Space Complexity**: $O(N \log N)$ to store the 2D sparse table.
* **Query Time**:
    * **Idempotent Operations** (e.g., Min, Max, GCD): $O(1)$.
    * **Non-Idempotent Operations** (e.g., Sum): $O(\log N)$.

### Use Cases
* **Static RMQ**: Finding the minimum or maximum in a range where no updates occur.
* **LCA**: Finding the Lowest Common Ancestor in a tree by reducing it to a static RMQ problem.
