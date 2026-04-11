#include <iostream>
#include <limits>
#include <stdexcept>

using namespace std;

/**
 * @struct node
 * @brief Represents a single materialized node in the dynamic sparse segment tree.
 * * Unlike static trees, nodes are allocated on-demand to save memory. 
 * This implementation follows the "Updated-Self" philosophy: the 'value' field 
 * always contains the ground truth for this node's range, while 'lazyValue' 
 * acts as a pending update strictly for its descendants.
 * * @tparam T Type of the summarized value (e.g., long long for range sum).
 * @tparam U Type of the update value (e.g., int for add/assign).
 */

template<typename T, typename U>
class node
{
    public:
        node *left = nullptr;
        node *right = nullptr;
        T value;
        U lazyValue;
        bool isLazy = false;

        node() : value(T()), lazyValue(U())
        {

        }

        ~node()
        {
            delete left;
            delete right;
        }
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
        virtual T apply(const T &value, const U &updateVal, long long low, long long high) = 0;
        
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
        T apply(const T &value, const U &updateVal, long long low, long long high) override
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
        T apply(const T &value, const U &updateVal, long long low, long long high) override
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
        T apply(const T &value, const U &updateVal, long long low, long long high) override
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
        T apply(const T &value, const U &updateVal, long long low, long long high) override
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
 * @brief A Dynamic Sparse Segment Tree with Lazy Propagation.
 * * This tree manages massive virtual ranges (up to 10^18) by only allocating 
 * memory for indices that have been updated or queried. It uses the 
 * Strategy Pattern to decouple the core tree logic from mathematical 
 * operations like Sum, Min, or Max.
 * * @tparam T The data type of the range result.
 * @tparam U The data type of the update operation.
 */

template<typename T, typename U>
class segTree
{
    private:
        node<T, U> *tree_ = nullptr;
        MergeStrategy<T> *mergeStrat_ = nullptr;
        UpdateStrategy<T, U> *updateStrat_ = nullptr;
        long long maxRange;
        long long numElem = 0; // For diagnostic purpose

        void pushLazyUpdates(node<T, U> *cur, long long low, long long high);
        void updateElem(node<T, U>*  &cur, long long low, long long high, long long i, long long j, const U &val);
        T searchElem(node<T, U> *cur, long long low, long long high, long long i, long long j);

    public:
        segTree(long long range, MergeStrategy<T> *merge, UpdateStrategy<T,U> *updt);
        ~segTree();
        
        void update(long long i, long long j, const U &val);
        T search(long long i, long long j);
        long long getNodeCount() const { return numElem; }
};

template<typename T, typename U>
segTree<T,U>::segTree(long long range, MergeStrategy<T> *merge, UpdateStrategy<T, U> *updt) : 
              maxRange(range),
              mergeStrat_(merge),
              updateStrat_(updt),
              numElem(0)
{

}

template<typename T, typename U>
segTree<T,U>::~segTree()
{
    delete tree_;
    delete mergeStrat_;
    delete updateStrat_;
}

/**
 * @brief Materializes child nodes and propagates pending lazy updates.
 * * Acts as the "Node Factory." It ensures that if a node has a pending 
 * update, that update is inherited by its children. If children do 
 * not exist, they are instantiated at this stage.
 * * @param cur Pointer to the current parent node.
 * @param low The lower bound of the parent's current range.
 * @param high The upper bound of the parent's current range.
 */

template<typename T, typename U>
void segTree<T, U>::pushLazyUpdates(node<T, U> *cur, long long low, long long high)
{
    // We are following lazy to children philosophy
    // Meaning a node which is tagged as lazy is acutally updated but it still need to pass
    // lazy updates to its children

    if(cur && cur->isLazy)
    {
        // First create children if not there already

        if(cur->left == nullptr)
        {
            cur->left = new node<T, U>();
            numElem++;
        }
            

        if(cur->right == nullptr)
        {
            cur->right = new node<T, U>();
            numElem++;
        }
            

        // Update children, as children will not become lazy to their children but direct chilren will
        // be up to date.
        long long mid = low + (high - low) / 2;

        cur->left->value = updateStrat_->apply(cur->left->value, cur->lazyValue, low, mid);
        cur->right->value = updateStrat_->apply(cur->right->value, cur->lazyValue, mid + 1, high);

        // Now update their lazy value. necessary for cascading updates
        cur->left->lazyValue = updateStrat_->combine(cur->left->lazyValue, cur->lazyValue);
        cur->right->lazyValue = updateStrat_->combine(cur->right->lazyValue, cur->lazyValue);

        // Make children lazy to their children
        cur->left->isLazy = true;
        cur->right->isLazy = true;

        // Clear lazy flag for cur node as it has successfully passed lazyness to their children
        cur->isLazy = false;
        cur->lazyValue = updateStrat_->identity();
    }

}

/**
 * @brief Recursively performs a range update on the sparse tree.
 * * Implements the "Lazy Stop" optimization: if the current range is 
 * fully contained within the target range, the node is updated and 
 * tagged, but recursion stops to prevent unnecessary node allocation.
 * * @param cur Reference to the pointer of the current node (allows in-place allocation).
 * @param low Current range lower bound.
 * @param high Current range upper bound.
 * @param i Target update range start (inclusive).
 * @param j Target update range end (inclusive).
 * @param val The update value (e.g., the value to add or assign).
 */

template<typename T, typename U>
void segTree<T, U>::updateElem(node<T, U>* &cur, long long low, long long high, long long i, long long j, const U &val)
{
    // First check if cur is nullptr then create it as we are creating the path while updting

    if(!cur)
    {
        numElem++;
        cur = new node<T,U>();
    }
        

    // if no overlap
    if(high < i || j < low)
        return;

    // if total overlap
    if(low >=i && high <=j)
    {
        // make node lazy to its children but updated self
        cur->value = updateStrat_->apply(cur->value, val, low, high);
        cur->lazyValue = updateStrat_->combine(cur->lazyValue, val);
        cur->isLazy = true;
        return;
    }

    // partial overlap
    else
    {
        pushLazyUpdates(cur, low, high);
    }

    long long mid = low + (high - low) / 2;

    updateElem(cur->left, low, mid, i, j, val);
    updateElem(cur->right, mid + 1, high, i, j, val);

    // push will gurantee that both children exist so don`t need null checks
    cur->value = mergeStrat_->merge(cur->left->value, cur->right->value);

}

/**
 * @brief Recursively queries a range for its summarized value.
 * * This method is "Memory-Safe": it will not create new nodes for 
 * empty (nullptr) ranges, instead returning the identity value. It 
 * only triggers a push if it hits a lazy node during a partial overlap.
 * * @param cur Pointer to the current node.
 * @param low Current range lower bound.
 * @param high Current range upper bound.
 * @param i Target query range start (inclusive).
 * @param j Target query range end (inclusive).
 * @return The summarized result for the requested range.
 */

template<typename T, typename U>
T segTree<T, U>::searchElem(node<T, U> *cur, long long low, long long high, long long i, long long j)
{
    if(cur == nullptr)
        return mergeStrat_->identity();

    // No overlap
    if(high < i || low > j)
        return mergeStrat_->identity();

    // Total overlap
    if(low >= i && high <=j)
    {
        return cur->value;
    }

    // The remianing case is no overlap, hence push the lazy updates down.
    pushLazyUpdates(cur, low, high);

    long long mid = low + (high - low) / 2;

    T left = searchElem(cur->left, low, mid, i, j);
    T right = searchElem(cur->right, mid + 1, high, i, j);

    return mergeStrat_->merge(left, right);
}

/**
 * @brief Performs a range update [i, j] with value val.
 * @throws std::out_of_range if indices are outside [0, maxRange - 1].
 */

template<typename T, typename U>
void segTree<T, U>::update(long long i, long long j, const U &val)
{
    if( i < 0 || i > maxRange - 1 || j < 0 || j > maxRange - 1 || i > j)
        throw std::out_of_range("Index is out of bounds!");

    updateElem(tree_, 0, maxRange -1, i, j, val);

}

/**
 * @brief Performs a range query [i, j].
 * @return The merged result of the range.
 * @throws std::out_of_range if indices are outside [0, maxRange - 1].
 */

template<typename T, typename U>
T segTree<T, U>::search(long long i , long long j)
{
    if( i < 0 || i > maxRange - 1 || j < 0 || j > maxRange - 1 || i > j)
        throw std::out_of_range("Index is out of bounds!");

    return searchElem(tree_, 0, maxRange -1, i, j);
}