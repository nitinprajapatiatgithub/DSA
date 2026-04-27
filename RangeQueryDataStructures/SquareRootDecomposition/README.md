# Square Root Decomposition

Square Root Decomposition is a technique that divides an array into blocks of size approximately $\sqrt{N}$. This allows for a balance between update and query times, making it a flexible "middle-ground" data structure.

### Complexity
* **Build Time**: $O(N)$.
* **Space Complexity**: $O(N)$ to store the original array and $O(\sqrt{N})$ for block summaries.
* **Update Time**:
    * **Reversible Ops** (e.g., Sum): $O(1)$.
    * **Idempotent Ops** (e.g., Min): $O(\sqrt{N})$.
* **Query Time**: $O(\sqrt{N})$.

### Use Cases
* When Segment Tree implementation is too complex for a specific merge operation.
* **Mo's Algorithm**: Processing offline range queries by sorting them in a specific block-based order.
