#include <iostream>
#include <vector>
#include <memory>
#include <stdexcept>

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
 * @class segTreeStrategy
 * @brief Abstract base strategy class for lazy segment tree operations
 * 
 * Defines the interface for different segment tree operations (sum, min, max, gcd, etc.)
 * and how lazy updates are applied and combined.
 * 
 * @tparam T Type of values in the segment tree
 * @tparam U Type of lazy updates (can be different from T)
 */
template <typename T, typename U>
class segTreeStrategy
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
         * @brief Applies lazy update to a node value
         * @param value Current value at the node
         * @param lazyVal The lazy update to apply
         * @param low Left boundary of the range
         * @param high Right boundary of the range
         * @return Updated value after applying lazy update
         */
        virtual T applyUpdates(const T &value, const U &lazyVal, int low, int high) = 0;
        
        /**
         * @brief Combines two lazy updates (when multiple updates are pending)
         * @param oldLazy Previous lazy update at this node
         * @param newLazy New lazy update to apply
         * @return Combined lazy update
         */
        virtual U combineLazy(const U &oldLazy, const U &newLazy) = 0;
        
        /**
         * @brief Returns identity element for values (e.g., 0 for sum, INT_MAX for min)
         * @return Identity/default value
         */
        virtual T valueIdentity() = 0;
        
        /**
         * @brief Returns identity element for lazy updates (e.g., 0 for add, no-op for assign)
         * @return Identity lazy value
         */
        virtual U lazyIdentity() = 0;
        
        virtual ~segTreeStrategy() {}
};

/**
 * @class sumSegTreeStrategy
 * @brief Strategy for range sum queries with range add updates
 * 
 * Supports efficient range sum queries and range add operations using lazy propagation.
 * 
 * @tparam T Type of values (typically int, long long)
 * @tparam U Type of lazy updates (value to add to a range)
 * 
 * Operations:
 * - merge(a, b): Returns a + b (sum operation)
 * - applyUpdates: Adds lazyVal to each element in range
 * - combineLazy: Adds multiple pending updates together
 */
template <typename T, typename U>
class sumSegTreeStrategy : public segTreeStrategy<T, U>
{
    public:
        T merge(const T &a, const T &b)
        {
            return a + b;
        }

        T applyUpdates(const T &value, const U &lazyVal, int low, int high)
        {
            // For sum segment tree the update is to add the lazy value to the current value
            return value + lazyVal * (high - low + 1);
        }

        U combineLazy(const U &oldLazy, const U &newLazy)
        {
            // For sum segment tree the lazy values are combined by addition
            return oldLazy + newLazy;
        }

        T valueIdentity()
        {
            return T();        
        }

        U lazyIdentity()
        {
            return U();
        }
};

/**
 * @class assignSegTreeStrategy
 * @brief Strategy for range sum queries with range assignment updates
 * 
 * Supports efficient range sum queries and range assignment (set) operations using lazy propagation.
 * Newer assignments overwrite older pending updates.
 * 
 * @tparam T Type of values (typically int, long long)
 * @tparam U Type of lazy updates (value to assign to a range)
 * 
 * Operations:
 * - merge(a, b): Returns a + b (sum of values)
 * - applyUpdates: Sets each element in range to lazyVal
 * - combineLazy: Overwrites with newer assignment value
 */
template <typename T, typename U>
class assignSegTreeStrategy : public segTreeStrategy<T, U>
{
    public:
        T merge(const T &a, const T &b)
        {
            return a + b;
        }

        T applyUpdates(const T &value, const U &lazyVal, int low, int high)
        {
            // For assignment, set all elements in range to lazyVal
            // So the sum becomes lazyVal * (number of elements)
            return lazyVal * (high - low + 1);
        }

        U combineLazy(const U &oldLazy, const U &newLazy)
        {
            // For assignment, newer assignment overwrites the older one
            return newLazy;
        }

        T valueIdentity()
        {
            return T();        
        }

        U lazyIdentity()
        {
            return U();
        }
};

/**
 * @class segTree
 * @brief Lazy Segment Tree implementation for efficient range queries and updates
 * 
 * A generic segment tree using lazy propagation to handle range queries and range updates
 * efficiently. Supports various operations through the strategy pattern.
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
        unique_ptr<segTreeStrategy<T, U>> strategy_;  ///< Strategy defining merge and update operations
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
        void updateElem(int low, int high, int i, int j, const T &val, int pos);

    public:
        /**
         * @brief Constructor: Initializes the segment tree
         * @param inArr Reference to input array
         * @param strategy Unique pointer to strategy defining operations
         */
        segTree(vector<T> &inArr, unique_ptr<segTreeStrategy<T, U>> strategy);
        
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
        void update(int i, int j, const T &val);
};

template <typename T, typename U>
T segTree<T, U>::build(vector<T> &inArr, int low, int high, int pos)
{
    if(low == high)
    {
        tree_[pos].value = inArr[low];
        tree_[pos].lazy = strategy_->lazyIdentity();
        tree_[pos].isLazy = false;
        return tree_[pos].value;
    }

    int mid = low + (high - low) / 2;

    T left = build(inArr, low, mid, 2 * pos + 1);
    T right = build(inArr, mid +1, high, 2 * pos + 2);

    tree_[pos].value = strategy_->merge(left, right);

    return tree_[pos].value;
}

template <typename T, typename U>
segTree<T, U>::segTree(vector<T> &inArr, unique_ptr<segTreeStrategy<T, U>> strategy):
         strategy_(move(strategy)),
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
        // propogate the lazy value to the current node
        tree_[pos].value = strategy_->applyUpdates(tree_[pos].value, tree_[pos].lazy, low, high);

        // if its not a leaf node then propogate the lazy value to its children
        if(low != high)
        {
            tree_[2 * pos + 1].lazy = strategy_->combineLazy(tree_[2 * pos + 1].lazy, tree_[pos].lazy);
            tree_[2 * pos + 1].isLazy = true;

            tree_[2 * pos + 2].lazy = strategy_->combineLazy(tree_[2 * pos + 2].lazy, tree_[pos].lazy);
            tree_[2 * pos + 2].isLazy = true;
        }

        // clear the lazy value for the current node
        tree_[pos].lazy = strategy_->lazyIdentity();
        tree_[pos].isLazy = false;
    }
}

template <typename T, typename U>
T segTree<T, U>::rangeSearch(int low, int high, int i , int j, int pos)
{
    // The very first thing is to propogate the
    // lazy value if there is any pending update for the current node
    applyAndPushUpdates(low, high, pos);

    // check if total overlap then return
    if(low >= i && high <= j)
        return tree_[pos].value;

    // if no overlap return identity value
    if(high < i || low > j)
        return strategy_->valueIdentity();

    // partial overlap need to check both sides
    int mid = low + (high - low) / 2;
    T left = rangeSearch(low, mid, i, j, 2 * pos + 1);
    T right = rangeSearch(mid + 1 , high , i, j, 2 * pos + 2);

    return strategy_->merge(left, right);
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
void segTree<T, U>::updateElem(int low, int high, int i , int j, const T &val, int pos)
{
    // The very first thing is to propogate the
    // lazy value if there is any pending update for the current node
    applyAndPushUpdates(low, high, pos);

    // This is total overlap
    if(low >= i && high <= j)
    {
        tree_[pos].value = strategy_->applyUpdates(tree_[pos].value, val, low, high);

        if(low != high)
        {
            tree_[2 * pos + 1].lazy = strategy_->combineLazy(tree_[2 * pos + 1].lazy, val);
            tree_[2 * pos + 1].isLazy = true;

            tree_[2 * pos + 2].lazy = strategy_->combineLazy(tree_[2 * pos + 2].lazy, val);
            tree_[2 * pos + 2].isLazy = true;
        }
        return;
    }

    // No overlap
    if(i <j && (j < low || i > high))
        return;

    int mid = low + (high - low) / 2;

    // Partial overlap
    updateElem(low, mid, i , j, val, 2 * pos + 1);
    updateElem(mid + 1, high, i , j, val, 2 * pos + 2);

    tree_[pos].value = strategy_->merge(tree_[2 * pos + 1].value, tree_[2 * pos + 2].value);
}

template <typename T, typename U>
void segTree<T, U>::update(int i, int j, const T &val)
{
    if( i < 0 || i > numElem - 1 || j < 0 || j > numElem - 1 || i > j)
        throw std::out_of_range("Index is out of bounds!");

    updateElem(0, numElem -1, i , j , val, 0);
}

/*
// ============ EXAMPLE USAGE ============
int main() {
    // Example 1: Range Sum Query with Range Add Updates
    {
        vector<int> arr = {1, 2, 3, 4, 5};
        
        // Create segment tree with sum strategy (range add updates)
        segTree<int, int> tree(arr, make_unique<sumSegTreeStrategy<int, int>>());
        
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
        
        // Create segment tree with min strategy (range assignment)
        segTree<int, int> minTree(arr, make_unique<assignSegTreeStrategy<int, int>>());
        
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
