#include <iostream>
#include <vector>
#include <memory>
#include <stdexcept>

using namespace std;

/**
 * @class mergeStrategy
 * @brief Abstract strategy interface for merging segment tree values.
 *
 * Defines the merge behavior used by the tree to combine two child values
 * and the identity element for the merge operation.
 *
 * @tparam T Type of values stored in the tree.
 */
template <typename T>
class mergeStrategy
{
    public:
        virtual T merge(const T &a, const T &b) = 0;
        virtual T identity() = 0;
        virtual ~mergeStrategy() {}
};

/**
 * @class updateStrategy
 * @brief Abstract strategy interface for updating segment tree values.
 *
 * Defines how an update value is applied to an existing node value and
 * provides an identity element for update combination.
 *
 * @tparam T Type of values stored in the tree.
 * @tparam U Type of update values.
 */
template <typename T, typename U>
class updateStrategy
{
    public:
        virtual T apply(const T &a, const U &b) = 0;
        virtual U identity() = 0;
        virtual ~updateStrategy() {}
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
class sumMergeStrategy : public mergeStrategy<T>
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
class sumUpdateStrategy : public updateStrategy<T, U>
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
class sumAssignStrategy : public updateStrategy<T, U>
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

template <typename T, typename U>
class segTree
{
    private:
        vector<T> tree_;
        unique_ptr<mergeStrategy<T>> mergeStrat_;
        unique_ptr<updateStrategy<T, U>> updateStrat_;
        int numElem;

        /**
         * @brief Recursively build the segment tree from input data.
         *
         * @param inArr Input array to build from
         * @param low Current segment left index
         * @param high Current segment right index
         * @param pos Current node index in the tree array
         * @return Value stored at the current node after building
         */
        T build(const vector<T> &inArr, int low, int high, int pos);

        /**
         * @brief Recursively search for the merge result over a query range.
         *
         * Propagates tree values through partial overlaps and uses the merge
         * strategy to combine child results.
         *
         * @param low Current segment left index
         * @param high Current segment right index
         * @param i Query range left index
         * @param j Query range right index
         * @param pos Current node index in the tree array
         * @return Merged result for the query range inside this segment
         */
        T rangeSearch(int low, int high, int i , int j, int pos);

        /**
         * @brief Recursively update a single index value in the tree.
         *
         * Applies the update at the leaf and recomputes parent values using the
         * merge strategy while traversing back up the recursion.
         *
         * @param low Current segment left index
         * @param high Current segment right index
         * @param ind Index to update
         * @param val Update value to apply
         * @param pos Current node index in the tree array
         */
        void updateElem(int low, int high, int ind, const U &val, int pos);

    public:
        /**
         * @brief Construct a segment tree from an input array.
         *
         * @param inArr Input array used to initialize leaf values
         * @param mergeStrat Strategy object used to merge child nodes
         * @param updateStrat Strategy object used to apply update values
         */
        segTree(const vector<T> &inArr, unique_ptr<mergeStrategy<T>> mergeStrat, unique_ptr<updateStrategy<T, U>> updateStrat);

        /**
         * @brief Query the merge result over a half-open range [i, j].
         *
         * @param i Query range left index
         * @param j Query range right index
         * @return Merged result for the range
         * @throws std::out_of_range if query indices are invalid
         */
        T search(int i , int j);

        /**
         * @brief Update a single element in the segment tree.
         *
         * The update is applied at the leaf node, and all affected parents
         * are recomputed using the merge strategy.
         *
         * @param i Index to update
         * @param val Update value to apply at index i
         * @throws std::out_of_range if the index is invalid
         */
        void update(int i, const U &val);
};

template <typename T, typename U>
T segTree<T, U>::build(const vector<T> &inArr, int low, int high, int pos)
{
    if(low == high)
    {
        tree_[pos] = inArr[low];
        return tree_[pos];
    }

    int mid = low + (high - low) / 2;

    T left = build(inArr, low, mid, 2 * pos + 1);
    T right = build(inArr, mid +1, high, 2 * pos + 2);

    tree_[pos] = mergeStrat_->merge(left, right);

    return tree_[pos];
}

template <typename T, typename U>
segTree<T, U>::segTree(const vector<T> &inArr, unique_ptr<mergeStrategy<T>> mergeStrat, unique_ptr<updateStrategy<T, U>> updateStrat):
         mergeStrat_(move(mergeStrat)),
         updateStrat_(move(updateStrat)),
         numElem(static_cast<int>(inArr.size()))
{
    if(numElem == 0)
        return;

    tree_.assign(4 * numElem, mergeStrat_->identity());

    // Now build the tree 
    build(inArr, 0, numElem -1, 0);   
}

template <typename T, typename U>
T segTree<T, U>::rangeSearch(int low, int high, int i , int j, int pos)
{
    // check if total overlap then return
    if(low >= i && high <= j)
        return tree_[pos];

    // if no overlap return identity value
    if(high < i || low > j)
        return mergeStrat_->identity();

    // partial overlap need to check both sides
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
void segTree<T, U>::updateElem(int low, int high, int ind, const U &val, int pos)
{
    // This is total overlap
    if(low == high)
    {
        tree_[pos] = updateStrat_->apply(tree_[pos], val);
        return;
    }

    // No overlap
    if(ind < low || ind > high)
        return;

    int mid = low + (high - low) / 2;

    // Partial overlap
    if (ind <= mid)
        updateElem(low, mid, ind, val, 2 * pos + 1);
    else
        updateElem(mid + 1, high, ind, val, 2 * pos + 2);

    tree_[pos] = mergeStrat_->merge(tree_[2 * pos + 1], tree_[2 * pos + 2]);
}

template <typename T, typename U>
void segTree<T, U>::update(int ind, const U &val)
{
    if( ind < 0 || ind > numElem - 1)
        throw std::out_of_range("Index is out of bounds!");

    updateElem(0, numElem -1, ind, val, 0);
}

/* Leaving below code as commented as it provides usage guidance. */
/*
class NumArray {

    private:
        unique_ptr<segTree<int, int>> tree;

    public:

        NumArray(const vector<int>& nums) {
            tree = make_unique<segTree<int, int>>(nums,
                make_unique<sumMergeStrategy<int>>(),
                make_unique<sumUpdateStrategy<int, int>>());
        }
    
        void update(int index, int val) {
            tree->update(index, val);
        }
    
        int sumRange(int left, int right) {
            return tree->search(left, right);
        }
};
*/
/**
 * Your NumArray object will be instantiated and called as such:
 * NumArray* obj = new NumArray(nums);
 * obj->update(index,val);
 * int param_2 = obj->sumRange(left,right);
 */
