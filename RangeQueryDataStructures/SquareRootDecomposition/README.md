# Square Root Decomposition

A data structure that partitions an array into blocks of size $\sqrt{N}$ to optimize range operations from $O(N)$ to $O(\sqrt{N})$.

## Files
* `sqrtDecomp.cpp`: Standard implementation for range queries and point updates[cite: 1].
* `lazySqrtDecomp.cpp`: Advanced implementation supporting range updates and range queries using lazy propagation[cite: 1].

## Complexity Analysis

### Standard Sqrt Decomposition (`sqrtDecomp.cpp`)
* **Preprocessing**: $O(N)$[cite: 1]
* **Range Query**: $O(\sqrt{N})$[cite: 1]
* **Point Update**: $O(1)$ or $O(\sqrt{N})$ (depending on summary rebuild requirements)[cite: 1]
* **Space Complexity**: $O(\sqrt{N})$ auxiliary space for block summaries[cite: 1].

### Lazy Sqrt Decomposition (`lazySqrtDecomp.cpp`)
* **Preprocessing**: $O(N)$[cite: 1]
* **Range Query**: $O(\sqrt{N})$[cite: 1]
* **Range Update**: $O(\sqrt{N})$[cite: 1]
* **Space Complexity**: $O(\sqrt{N})$ for block summaries + $O(\sqrt{N})$ for lazy tags[cite: 1].

## Unified Policy Architecture
The lazy implementation utilizes a `Unified Policy` design to bind query and update logic[cite: 1]. This ensures mathematical consistency (e.g., preventing $O(N)$ operations like ADD updates on XOR summaries) and minimizes virtual dispatch overhead[cite: 1].
