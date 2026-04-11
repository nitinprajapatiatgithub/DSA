#include <iostream>
#include <vector>
#include <memory>
#include <stdexcept>
#include <limits>
#include <algorithm>

using namespace std;

/**
 * @struct segTreeNode
 * @brief Represents a single node in the lazy segment tree
 * 
 * @tparam T Type of value stored in the node
 * @tparam U Type of lazy update stored in the node
 * 
 * @member value The actual value stored at this node (e.g., sum, min, max)
 * @member lazy The pending lazy update that hasn't been propagated yet
 * @member isLazy Flag indicating if this node has a pending lazy update
 */
template <typename T, typename U>
class segTreeNode
{
    public:
        T value;
        U lazy;
        bool isLazy;

        segTreeNode(): value(T()), lazy(U()), isLazy(false) {}
};


/**
 * @class MergeStrategy
 * @brief Abstract base strategy class for main segment tree operations (merge and identity)
 * 
 * Defines the interface for different segment tree operations (sum, min, max, gcd, etc.)
 * 
 * @tparam T Type of values in the segment tree
 */
template <typename T>
class MergeStrategy
{
    public:
        /**
         * @brief Merges two values from child nodes
         * @param a First value
         * @param b Second value
         * @return Merged value (operation-specific: sum, min, max, gcd, etc.)
         */
        virtual T merge(const T &a, const T &b) = 0;
        
        /**
         * @brief Returns identity element for values (e.g., 0 for sum, INT_MAX for min)
         * @return Identity/default value
         */
        virtual T identity() = 0;
        
        virtual ~MergeStrategy() {}
};

/**
 * @class UpdateStrategy
 * @brief Abstract base strategy class for update operations and lazy propagation
 * 
 * Defines the interface for how updates are applied and combined.
 * 
 * @tparam T Type of values in the segment tree
 * @tparam U Type of lazy updates (can be different from T)
 */
template <typename T, typename U>
class UpdateStrategy
{
    public:
        /**
         * @brief Applies update to a node value
         * @param value Current value at the node
         * @param updateVal The update to apply
         * @param low Left boundary of the range
         * @param high Right boundary of the range
         * @return Updated value after applying update
         */
        virtual T apply(const T &value, const U &updateVal, int low, int high) = 0;
        
        /**
         * @brief Combines two lazy updates (when multiple updates are pending)
         * @param oldLazy Previous lazy update at this node
         * @param newLazy New lazy update to apply
         * @return Combined lazy update
         */
        virtual U combine(const U &oldLazy, const U &newLazy) = 0;
        
        /**
         * @brief Returns identity element for lazy updates (e.g., 0 for add, no-op for assign)
         * @return Identity lazy value
         */
        virtual U identity() = 0;
        
        virtual ~UpdateStrategy() {}
};

/**
 * @class SumMain
 * @brief Main strategy for range sum queries
 * 
 * @tparam T Type of values (typically int, long long)
 */
template <typename T>
class SumMain : public MergeStrategy<T>
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
 * @class MinMain
 * @brief Main strategy for range minimum queries
 * 
 * @tparam T Type of values (typically int, long long)
 */
template <typename T>
class MinMain : public MergeStrategy<T>
{
    public:
        T merge(const T &a, const T &b) override
        {
            return min(a, b);
        }

        T identity() override
        {
            return numeric_limits<T>::max();
        }
};

/**
 * @class MaxMain
 * @brief Main strategy for range maximum queries
 * 
 * @tparam T Type of values (typically int, long long)
 */
template <typename T>
class MaxMain : public MergeStrategy<T>
{
    public:
        T merge(const T &a, const T &b) override
        {
            return max(a, b);
        }

        T identity() override
        {
            return numeric_limits<T>::min();
        }
};

/**
 * @class AddUpdate
 * @brief Update strategy for range add operations
 * 
 * Supports adding a value to each element in a range.
 * Works with sum, min, max main strategies.
 * 
 * @tparam T Type of values
 * @tparam U Type of updates (value to add)
 */
template <typename T, typename U>
class AddUpdate : public UpdateStrategy<T, U>
{
    public:
        T apply(const T &value, const U &updateVal, int low, int high) override
        {
            return value + static_cast<T>(updateVal) * (high - low + 1);
        }

        U combine(const U &oldLazy, const U &newLazy) override
        {
            return oldLazy + newLazy;
        }

        U identity() override
        {
            return U();
        }
};

/**
 * @class SumAssignUpdate
 * @brief Update strategy for range assignment on sum main strategy
 * 
 * @tparam T Type of values
 * @tparam U Type of updates (value to assign)
 */
template <typename T, typename U>
class SumAssignUpdate : public UpdateStrategy<T, U>
{
    public:
        T apply(const T &value, const U &updateVal, int low, int high) override
        {
            return static_cast<T>(updateVal) * (high - low + 1);
        }

        U combine(const U &oldLazy, const U &newLazy) override
        {
            return newLazy;
        }

        U identity() override
        {
            return U();
        }
};

/**
 * @class MinAssignUpdate
 * @brief Update strategy for range assignment on min main strategy
 * 
 * @tparam T Type of values
 * @tparam U Type of updates (value to assign)
 */
template <typename T, typename U>
class MinAssignUpdate : public UpdateStrategy<T, U>
{
    public:
        T apply(const T &value, const U &updateVal, int low, int high) override
        {
            return static_cast<T>(updateVal);
        }

        U combine(const U &oldLazy, const U &newLazy) override
        {
            return newLazy;
        }

        U identity() override
        {
            return U();
        }
};

/**
 * @class MaxAssignUpdate
 * @brief Update strategy for range assignment on max main strategy
 * 
 * @tparam T Type of values
 * @tparam U Type of updates (value to assign)
 */
template <typename T, typename U>
class MaxAssignUpdate : public UpdateStrategy<T, U>
{
    public:
        T apply(const T &value, const U &updateVal, int low, int high) override
        {
            return static_cast<T>(updateVal);
        }

        U combine(const U &oldLazy, const U &newLazy) override
        {
            return newLazy;
        }

        U identity() override
        {
            return U();
        }
};

/**
 * @class segTree
 * @brief Lazy Segment Tree implementation for efficient range queries and updates
 * 
 * A generic segment tree using lazy propagation to handle range queries and range updates
 * efficiently. Supports various operations through separate main and update strategies.
 * 
 * Time Complexity:
 * - build(): O(n)
 * - search(i, j): O(log n)
 * - update(i, j, val): O(log n)
 * 
 * Space Complexity: O(n)
 * 
 * @tparam T Type of values stored in the tree
 * @tparam U Type of lazy updates (can be different from T)
 */
template <typename T, typename U>
class segTree
{
    private:
        vector<segTreeNode<T, U>> tree_;           ///< Array representing the segment tree with lazy values
        unique_ptr<MergeStrategy<T>> mergeStrat_;      ///< Strategy defining merge operation
        unique_ptr<UpdateStrategy<T, U>> updateStrat_; ///< Strategy defining update operations
        int numElem;                                ///< Number of elements in original array

        /**
         * @brief Applies pending lazy updates and propagates to children
         * @param low Left boundary of current node's range
         * @param high Right boundary of current node's range
         * @param pos Position of current node in tree array
         */
        void applyAndPushUpdates(int low, int high, int pos);
        
        /**
         * @brief Recursively builds the segment tree from input array
         * @param inArr Input array
         * @param low Left boundary of current range
         * @param high Right boundary of current range
         * @param pos Position of current node in tree array
         * @return Value at current node after building subtree
         */
        T build(vector<T> &inArr, int low, int high, int pos);
        
        /**
         * @brief Recursively searches for range query result with lazy propagation
         * @param low Left boundary of current node's range
         * @param high Right boundary of current node's range
         * @param i Query range start
         * @param j Query range end
         * @param pos Position of current node in tree array
         * @return Result of query operation on range [i, j]
         */
        T rangeSearch(int low, int high, int i , int j, int pos);
        
        /**
         * @brief Recursively updates a range with lazy propagation
         * @param low Left boundary of current node's range
         * @param high Right boundary of current node's range
         * @param i Update range start
         * @param j Update range end
         * @param val Value to apply in the update (operation-specific)
         * @param pos Position of current node in tree array
         */
        void updateElem(int low, int high, int i, int j, const U &val, int pos);

    public:
        /**
         * @brief Constructor: Initializes the segment tree
         * @param inArr Reference to input array
         * @param mergeStrat Unique pointer to merge strategy defining merge operation
         * @param updateStrat Unique pointer to update strategy defining update operations
         */
        segTree(vector<T> &inArr, unique_ptr<MergeStrategy<T>> mergeStrat, unique_ptr<UpdateStrategy<T, U>> updateStrat);
        
        /**
         * @brief Performs a range query
         * @param i Left boundary of query range (0-indexed, inclusive)
         * @param j Right boundary of query range (0-indexed, inclusive)
         * @return Result of query operation on range [i, j]
         * @throws std::out_of_range if indices are out of bounds
         */
        T search(int i , int j);
        
        /**
         * @brief Performs a range update with lazy propagation
         * @param i Left boundary of update range (0-indexed, inclusive)
         * @param j Right boundary of update range (0-indexed, inclusive)
         * @param val Value to apply in the update (operation-specific)
         * @throws std::out_of_range if indices are out of bounds
         */
        void update(int i, int j, const U &val);
};

template <typename T, typename U>
T segTree<T, U>::build(vector<T> &inArr, int low, int high, int pos)
{
    if(low == high)
    {
        tree_[pos].value = inArr[low];
        tree_[pos].lazy = updateStrat_->identity();
        tree_[pos].isLazy = false;
        return tree_[pos].value;
    }

    int mid = low + (high - low) / 2;

    T left = build(inArr, low, mid, 2 * pos + 1);
    T right = build(inArr, mid +1, high, 2 * pos + 2);

    tree_[pos].value = mergeStrat_->merge(left, right);

    return tree_[pos].value;
}

template <typename T, typename U>
segTree<T, U>::segTree(vector<T> &inArr, unique_ptr<MergeStrategy<T>> mergeStrat, unique_ptr<UpdateStrategy<T, U>> updateStrat):
         mergeStrat_(move(mergeStrat)),
         updateStrat_(move(updateStrat)),
         numElem(inArr.size())
{
    tree_.assign(4 * numElem, segTreeNode<T, U>());

    // Now build the tree 
    build(inArr, 0, numElem -1, 0);   
}

template <typename T, typename U>
void segTree<T, U>::applyAndPushUpdates(int low, int high, int pos)
{
    if(tree_[pos].isLazy)
    {
        // // propagate the lazy value to the current node
        // tree_[pos].value = updateStrat_->apply(tree_[pos].value, tree_[pos].lazy, low, high);

        // if its not a leaf node then propagate the lazy value to its children
        if(low != high)
        {
            // update child node as we now following laxy to children philosophy
            // Which means once we push updates to children they will be updated
            // to self but lazy to their children

            int mid = low + (high - low) / 2;
            tree_[2 * pos + 1].value = updateStrat_->apply(tree_[2 * pos + 1].value, tree_[pos].lazy, low, mid);
            tree_[2 * pos + 2].value = updateStrat_->apply(tree_[2 * pos + 2].value, tree_[pos].lazy, mid + 1, high);

            // Push laziness down
            tree_[2 * pos + 1].isLazy = true;
            tree_[2 * pos + 2].isLazy = true;

            // Combine chidren lazy values for cascading updates
            tree_[2 * pos + 1].lazy = updateStrat_->combine(tree_[2 * pos + 1].lazy, tree_[pos].lazy);
            tree_[2 * pos + 2].lazy = updateStrat_->combine(tree_[2 * pos + 2].lazy, tree_[pos].lazy);

        }

        // Make cur node as non-lazy as it has pushed lazyness
        tree_[pos].isLazy = false;
        tree_[pos].lazy = updateStrat_->identity();
    }
}

template <typename T, typename U>
T segTree<T, U>::rangeSearch(int low, int high, int i , int j, int pos)
{

    // check if total overlap then return
    if(low >= i && high <= j)
        return tree_[pos].value;

    // if no overlap return identity value
    if(high < i || low > j)
        return mergeStrat_->identity();

    // partial overlap need to check both sides

    // The very first thing is to propagate the
    // lazy value if there is any pending update for the children
    // in case of partial overlap
    applyAndPushUpdates(low, high, pos);

    int mid = low + (high - low) / 2;
    T left = rangeSearch(low, mid, i, j, 2 * pos + 1);
    T right = rangeSearch(mid + 1 , high , i, j, 2 * pos + 2);

    return mergeStrat_->merge(left, right);
}

template <typename T, typename U>
T segTree<T, U>::search(int i , int j)
{
    if( (i < 0 || i > numElem -1) 
        || (j < 0 || j > numElem -1) 
        || (i > j))
    {
        throw std::out_of_range("Index is out of bounds!");
    }

    return rangeSearch(0, numElem -1, i, j, 0);
}

template <typename T, typename U>
void segTree<T, U>::updateElem(int low, int high, int i , int j, const U &val, int pos)
{

    // This is total overlap
    if(low >= i && high <= j)
    {
        // Node will always be updated to self and lazy to children
        tree_[pos].value = updateStrat_->apply(tree_[pos].value, val, low, high);

        // Make node lazy to children
        tree_[pos].isLazy = true;
        tree_[pos].lazy = updateStrat_->combine(tree_[pos].lazy, val);

        return;
    }

    // No overlap
    if(j < low || i > high)
        return;

    // Parital overlap
    // The very first thing is to propagate the
    // lazy value if there is any pending update for children
    applyAndPushUpdates(low, high, pos);

    int mid = low + (high - low) / 2;

    // Partial overlap
    updateElem(low, mid, i , j, val, 2 * pos + 1);
    updateElem(mid + 1, high, i , j, val, 2 * pos + 2);

    tree_[pos].value = mergeStrat_->merge(tree_[2 * pos + 1].value, tree_[2 * pos + 2].value);
}

template <typename T, typename U>
void segTree<T, U>::update(int i, int j, const U &val)
{
    if( i < 0 || i > numElem - 1 || j < 0 || j > numElem - 1 || i > j)
        throw std::out_of_range("Index is out of bounds!");

    updateElem(0, numElem -1, i , j , val, 0);
}

// ============ EXAMPLE USAGE ============
/*
int main() {
    // Example 1: Range Sum Query with Range Add Updates
    {
        vector<int> arr = {1, 2, 3, 4, 5};
        
        // Create segment tree with sum main and add update
        segTree<int, int> tree(arr, make_unique<SumMain<int>>(), make_unique<AddUpdate<int, int>>());
        
        // Query: sum of range [0, 4]
        int result1 = tree.search(0, 4);  // Output: 15
        cout << "Sum [0, 4]: " << result1 << endl;
        
        // Update: add 5 to range [1, 3]
        tree.update(1, 3, 5);
        
        // Query again after update: sum of range [0, 4]
        int result2 = tree.search(0, 4);  // Output: 30 (1 + (2+5) + (3+5) + (4+5) + 5)
        cout << "Sum [0, 4] after update: " << result2 << endl;
    }
    
    // Example 2: Range Min Query with Range Assign Updates
    {
        vector<int> arr = {10, 20, 30, 40, 50};
        
        // Create segment tree with min main and assign update
        segTree<int, int> minTree(arr, make_unique<MinMain<int>>(), make_unique<MinAssignUpdate<int, int>>());
        
        // Query: min of range [1, 3]
        int minResult = minTree.search(1, 3);  // Output: 20
        cout << "Min [1, 3]: " << minResult << endl;
        
        // Update: set all elements in range [0, 2] to 5
        minTree.update(0, 2, 5);
        
        // Query again: min of range [0, 4]
        int minResult2 = minTree.search(0, 4);  // Output: 5
        cout << "Min [0, 4] after assignment: " << minResult2 << endl;
    }
    
    return 0;
}
*/
