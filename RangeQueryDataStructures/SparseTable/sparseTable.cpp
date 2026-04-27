#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <limits>

/**
 * @brief Policy for Range Minimum Query.
 * * This strategy utilizes the idempotent property of the 'min' operation
 * to allow O(1) range queries by overlapping two power-of-two segments.
 */
template <typename T>
struct MinPolicy {
    using value_type = T;
    static constexpr bool is_idempotent = true;
    
    static T merge(const T& a, const T& b) {
        return std::min(a, b);
    }

    static T identity() {
        return T(0);
    }
};

/**
 * @brief Policy for Range Sum Query.
 * * This strategy handles non-idempotent operations where overlapping segments
 * would yield incorrect results. It uses a O(log N) bit-stitching approach.
 */
template <typename T>
struct SumPolicy {
    using value_type = T;
    static constexpr bool is_idempotent = false;
    
    static T merge(const T& a, const T& b) {
        return a + b;
    }
    
    static T identity() {
        return T(0);
    }
};

/**
 * @class SparseTable
 * @brief A generic, static data structure for efficient range queries.
 * * @tparam Policy A strategy class defining the merge operation and its properties.
 * * Architecture:
 * - Build Time: O(N log N)
 * - Space Complexity: O(N log N)
 * - Query Time: O(1) for idempotent ops, O(log N) for non-idempotent ops.
 */
template <typename Policy>
class SparseTable {
public:
    using T = typename Policy::value_type;

    /**
     * @brief Constructor that initializes and builds the sparse table.
     * @param data The input vector of elements to process.
     */
    explicit SparseTable(const std::vector<T>& data);

    /**
     * @brief Performs a range query on the interval [L, R].
     * @param L The starting index (inclusive).
     * @param R The ending index (inclusive).
     * @return The result of the range operation defined by the Policy.
     */
    T query(std::size_t L, std::size_t R) const;

private:
    std::size_t n;            ///< Number of elements in the input array.
    int max_j;                ///< Maximum power of 2 exponent (floor(log2(n))).
    std::vector<std::vector<T>> st; ///< Sparse Table 2D structure.
    std::vector<int> logs;    ///< Precomputed logarithms for O(1) query lookups.

    /**
     * @brief Precomputes floor(log2(i)) for all i up to n.
     * Essential for achieving O(1) performance in idempotent queries.
     */
    void precompute_logs();

    /**
     * @brief Computes the DP states for the sparse table.
     * Combines intervals of length 2^(j-1) to form intervals of length 2^j.
     */
    void buildTable(const std::vector<T>& data);
};

template<typename Policy>
void SparseTable<Policy>::precompute_logs() {
    logs.assign(n + 1, 0);
    for (std::size_t i = 2; i <= n; i++) {
        logs[i] = logs[i / 2] + 1;
    }
}

template<typename Policy>
void SparseTable<Policy>::buildTable(const std::vector<T>& data) {
    // Allocate outer vector (rows)
    st.assign(n, std::vector<T>(max_j + 1));

    // Base Case: Intervals of length 2^0 = 1
    for (std::size_t i = 0; i < n; i++) {
        st[i][0] = data[i];
    }

    // Inductive Step: Build intervals of length 2^j
    for (int j = 1; j <= max_j; j++) {
        std::size_t half_len = (1ULL << (j - 1));
        std::size_t full_len = (1ULL << j);

        // Safe bounds check to prevent unsigned overflow
        for (std::size_t i = 0; i <= n - full_len; i++) {
            st[i][j] = Policy::merge(st[i][j - 1], st[i + half_len][j - 1]);
        }
    }
}

template <typename Policy>
SparseTable<Policy>::SparseTable(const std::vector<T>& data) {
    n = data.size();
    if (n == 0) return;

    precompute_logs();
    max_j = logs[n];
    buildTable(data);
}

template<typename Policy>
typename Policy::value_type SparseTable<Policy>::query(std::size_t L, std::size_t R) const {
    // Basic safety check for interval length
    if (L > R) return Policy::is_idempotent ? T{} : Policy::identity();
    
    std::size_t range_len = R - L + 1;

    if constexpr (Policy::is_idempotent) {
        // O(1) Overlap Strategy
        int k = logs[range_len];
        return Policy::merge(st[L][k], st[R - (1ULL << k) + 1][k]);
    } else {
        // O(log N) Bit-Stitching Strategy
        T res = Policy::identity();
        for (int j = max_j; j >= 0; --j) {
            if ((1ULL << j) <= (R - L + 1)) {
                res = Policy::merge(res, st[L][j]);
                L += (1ULL << j);
                if (L > R) break;
            }
        }
        return res;
    }
}
