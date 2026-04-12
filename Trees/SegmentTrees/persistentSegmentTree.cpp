#include <iostream>
#include <vector>
#include <memory>

using namespace std;

/**
 * @struct node
 * @brief Represents a single node in the persistent segment tree.
 * * Each node is immutable once created. Updates to the tree result in new 
 * nodes being created along the update path, while unaffected branches 
 * are shared with previous versions to optimize memory.
 * * @tparam T Type of the summarized value (e.g., int for frequency).
 * @tparam U Type of the update value.
 */
template<typename T, typename U>
class node {
public:
    shared_ptr<node<T, U>> left = nullptr;
    shared_ptr<node<T, U>> right = nullptr;
    T value;

    node() : value(T()) {}
    node(T val) : value(val) {}

    // Persistence Secret: The Clone Constructor
    node(shared_ptr<node<T, U>> other) {
        if (other) {
            value = other->value;
            left = other->left;
            right = other->right;
        } else {
            value = T();
            left = right = nullptr;
        }
    }

    // Note: In persistence, we usually don't delete children in the destructor
    // because they are shared across multiple roots. Use a memory pool or 
    // smart pointers for production-grade memory management.
    ~node() {} 
};

/**
 * @class MergeStrategy
 * @brief Abstract strategy interface for merging segment tree values.
 *
 * Defines the merge behavior used by the tree to combine two child values
 * and the identity element for the merge operation.
 *
 * @tparam T Type of values stored in the tree.
 */
template <typename T>
class MergeStrategy
{
    public:
        virtual T merge(const T &a, const T &b) = 0;
        virtual T identity() = 0;
        virtual ~MergeStrategy() {}
};

/**
 * @class UpdateStrategy
 * @brief Abstract strategy interface for updating segment tree values.
 *
 * Defines how an update value is applied to an existing node value and
 * provides an identity element for update combination.
 *
 * @tparam T Type of values stored in the tree.
 * @tparam U Type of update values.
 */
template <typename T, typename U>
class UpdateStrategy
{
    public:
        virtual T apply(const T &a, const U &b) = 0;
        virtual U identity() = 0;
        virtual ~UpdateStrategy() {}
};

/**
 * @class sumMergeStrategy
 * @brief Merge strategy for range sum operations.
 *
 * Implements sum behavior for segment tree nodes and returns 0 as the
 * identity element.
 *
 * @tparam T Type of values stored in the tree.
 */
template <typename T>
class sumMergeStrategy : public MergeStrategy<T>
{
    public:
        T merge(const T &a, const T &b) override
        {
            return a + b;
        }

        T identity() override
        {
            return T();
        }
};

/**
 * @class sumUpdateStrategy
 * @brief Update strategy for point add operations.
 *
 * Applies an additive update to a single tree value.
 *
 * @tparam T Type of values stored in the tree.
 * @tparam U Type of update values.
 */
template <typename T, typename U>
class sumUpdateStrategy : public UpdateStrategy<T, U>
{
    public:
        T apply(const T &a, const U &b) override
        {
            return a + static_cast<T>(b);
        }

        U identity() override
        {
            return U();
        }
};

/**
 * @class sumAssignStrategy
 * @brief Update strategy for point assignment operations.
 *
 * Replaces the existing value with the new update value.
 *
 * @tparam T Type of values stored in the tree.
 * @tparam U Type of update values.
 */
template <typename T, typename U>
class sumAssignStrategy : public UpdateStrategy<T, U>
{
    public:
        T apply(const T &a, const U &b) override
        {
            return static_cast<T>(b);
        }

        U identity() override
        {
            return U();
        }
};


/**
 * @class persistentSegTree
 * @brief A Persistent Segment Tree implementation using Path Copying.
 *
 * This tree maintains a history of versions. Every update creates a new
 * entry point (root) while sharing common nodes with previous versions
 * to achieve O(log N) space per update.
 *
 * @tparam T Data type of the node value.
 * @tparam U Data type of the update operation.
 */
template<typename T, typename U>
class persistentSegTree {
private:
    // We store all version roots here
    vector< shared_ptr<node<T, U>> > roots;

    unique_ptr<MergeStrategy<T>> mergeStrat_ ;
    unique_ptr<UpdateStrategy<T, U>> updateStrat_;

    long long maxRange;

    // Notice the return type node* and the 'prev' parameter
    shared_ptr<node<T, U>> updateElem(shared_ptr<node<T, U>> prev, long long low, long long high, long long i, const U &val);
    T searchElem(shared_ptr<node<T, U>> cur, long long low, long long high, long long i, long long j);

public:

    /**
     * @brief Constructor for persistent segment tree.
     * @param range The maximum range [0, range-1] the tree will cover.
     * @param merge Pointer to the merge strategy.
     * @param updt Pointer to the update strategy.
     */
    persistentSegTree(long long range, unique_ptr<MergeStrategy<T>> merge, unique_ptr<UpdateStrategy<T, U>> updt) 
        : maxRange(range), mergeStrat_(move(merge)), updateStrat_(move(updt)) {
        // Version 0: Empty tree
        roots.push_back(nullptr);
    }

    /**
     * @brief Creates a new tree version by updating a single point.
     * @param i The index to update.
     * @param val The value to apply at index i.
     */
    void update(long long i, const U &val) {
        // We always build the new version based on the CURRENT latest version
        shared_ptr<node<T, U>> latestRoot = roots.back();
        shared_ptr<node<T, U>> newRoot = updateElem(latestRoot, 0, maxRange - 1, i, val);
        roots.push_back(newRoot);
    }

    /**
     * @brief Queries a range value for a specific historical version.
     * @param version The version index to search.
     * @param i Start of the range.
     * @param j End of the range.
     * @return The summarized result for the range.
     */
    T search(int version, long long i, long long j) {
        if (version >= roots.size()) throw out_of_range("Version doesn't exist");
        return searchElem(roots[version], 0, maxRange - 1, i, j);
    }

    T queryKth(shared_ptr<node<T, U>> lowVerTree, shared_ptr<node<T, U>> highVerTree, long long low, long long high, long long k);
    
    int getLatestVersion() { return roots.size() - 1; }

    shared_ptr<node<T, U>> getVersiondTree(long long ver){return roots[ver];}
};

/**
 * @brief Recursively builds a new path to implement persistence.
 * @param prev Pointer to the node in the previous version.
 * @param low Current range lower bound.
 * @param high Current range upper bound.
 * @param i Target update index.
 * @param val Update value.
 * @return Pointer to the newly created (cloned) node.
 */
template<typename T, typename U>
shared_ptr<node<T, U>> persistentSegTree<T, U>::updateElem(shared_ptr<node<T, U>> prev, long long low, long long high, long long i, const U &val) {
    // 1. Path Copying: Create a NEW node that clones the PREVIOUS node
    shared_ptr<node<T, U>> newNode = make_shared<node<T, U>>(prev);

    // 2. Base Case: Point update at the leaf
    if (low == high) {
        newNode->value = updateStrat_->apply(newNode->value, val);
        return newNode;
    }

    long long mid = low + (high - low) / 2;
    if (i <= mid) {
        // Go left: newNode gets a NEW left child, but keeps the OLD right child (Shared!)
        newNode->left = updateElem(newNode->left, low, mid, i, val);
    } else {
        // Go right: newNode gets a NEW right child, but keeps the OLD left child (Shared!)
        newNode->right = updateElem(newNode->right, mid + 1, high, i, val);
    }

    // 3. Re-calculate the value of the NEW node based on its children
    T leftVal = newNode->left ? newNode->left->value : mergeStrat_->identity();
    T rightVal = newNode->right ? newNode->right->value : mergeStrat_->identity();
    newNode->value = mergeStrat_->merge(leftVal, rightVal);

    return newNode;
}

template<typename T, typename U>
T persistentSegTree<T, U>::searchElem(shared_ptr<node<T, U>> cur, long long low, long long high, long long i, long long j) {
    if (!cur || high < i || low > j) return mergeStrat_->identity();
    if (low >= i && high <= j) return cur->value;

    long long mid = low + (high - low) / 2;
    return mergeStrat_->merge(searchElem(cur->left, low, mid, i, j),
                             searchElem(cur->right, mid + 1, high, i, j));
}

/**
     * @brief Performs a dual-root walk to find the k-th smallest element.
     * * Used for MKTHNUM problems by calculating (highVerTree - lowVerTree)
     * frequencies on the fly.
     * * @param lowVerTree Root of the version before the range started.
     * @param highVerTree Root of the latest version of the range.
     * @param low Current rank lower bound.
     * @param high Current rank upper bound.
     * @param k The k-th element to find.
     * @return The rank of the k-th smallest element.
     */
template<typename T, typename U>
T persistentSegTree<T, U>::queryKth(shared_ptr<node<T, U>> leftRoot, shared_ptr<node<T, U>> rightRoot, long long low, long long high, long long k) {
    if (low == high) return low; // We found the Rank

    // The core MKTHNUM logic: Subtract frequencies of the left children
    int leftCount = (rightRoot && rightRoot->left ? rightRoot->left->value : 0) - 
                    (leftRoot && leftRoot->left ? leftRoot->left->value : 0);

    int mid = low + (high - low) / 2;
    if (k <= leftCount) {
        // The k-th element is in the left half
        return queryKth(leftRoot ? leftRoot->left : nullptr, 
                        rightRoot ? rightRoot->left : nullptr, 
                        low, mid, k);
    } else {
        // The k-th element is in the right half, adjust k
        return queryKth(leftRoot ? leftRoot->right : nullptr, 
                        rightRoot ? rightRoot->right : nullptr, 
                        mid + 1, high, k - leftCount);
    }
}