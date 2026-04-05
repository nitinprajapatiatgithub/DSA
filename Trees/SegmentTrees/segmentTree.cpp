#include <iostream>
#include <vector>
#include <memory>
#include <stdexcept>

using namespace std;

template <typename T>
class segTreeStrategy
{
    public:
        virtual T merge(const T &a, const T &b) = 0;
        virtual T identity() = 0;
        virtual ~segTreeStrategy() {}
};

template <typename T>
class sumSegTreeStrategy : public segTreeStrategy<T>
{
    public:
        T merge(const T &a, const T &b)
        {
            return a + b;
        }

        T identity()
        {
            return 0;
        }
};

template <typename T>
class segTree
{
    private:
        vector<T> tree_;
        unique_ptr<segTreeStrategy<T>> strategy_;
        int numElem;

        T build(vector<T> &inArr, int low, int high, int pos);
        T rangeSearch(int low, int high, int i , int j, int pos);
        void updateElem(int low, int high, int ind, const T &val, int pos);

    public:
        segTree(vector<T> &inArr, unique_ptr<segTreeStrategy<T>> strategy);
        T search(int i , int j);
        void update(int i, const T &val);
};

template <typename T>
T segTree<T>::build(vector<T> &inArr, int low, int high, int pos)
{
    if(low == high)
    {
        tree_[pos] = inArr[low];
        return tree_[pos];
    }

    int mid = low + (high - low) / 2;

    T left = build(inArr, low, mid, 2 * pos + 1);
    T right = build(inArr, mid +1, high, 2 * pos + 2);

    tree_[pos] = strategy_->merge(left, right);

    return tree_[pos];
}

template <typename T>
segTree<T>::segTree(vector<T> &inArr, unique_ptr<segTreeStrategy<T>> strategy):
         strategy_(move(strategy)),
         numElem(inArr.size())
{
    tree_.assign(4 * numElem, strategy_->identity());

    // Now build the tree 
    build(inArr, 0, numElem -1, 0);   
}

template <typename T>
T segTree<T>::rangeSearch(int low, int high, int i , int j, int pos)
{
    // check if total overlap then return
    if(low >= i && high <= j)
        return tree_[pos];

    // if no overlap return identity value
    if(high < i || low > j)
        return strategy_->identity();

    // partial overlap need to check both sides
    int mid = low + (high - low) / 2;
    T left = rangeSearch(low, mid, i, j, 2 * pos + 1);
    T right = rangeSearch(mid + 1 , high , i, j, 2 * pos + 2);

    return strategy_->merge(left, right);
}

template <typename T>
T segTree<T>::search(int i , int j)
{
    if( (i < 0 || i > numElem -1) 
        || (j < 0 || j > numElem -1) 
        || (i > j))
    {
        throw std::out_of_range("Index is out of bounds!");
    }

    return rangeSearch(0, numElem -1, i, j, 0);
}

template <typename T>
void segTree<T>::updateElem(int low, int high, int ind, const T &val, int pos)
{
    // This is total overlap
    if(low == high)
    {
        tree_[pos] = val;
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

    tree_[pos] = strategy_->merge(tree_[2 * pos + 1], tree_[2 * pos + 2]);
}

template <typename T>
void segTree<T>::update(int ind, const T &val)
{
    if( ind < 0 || ind > numElem - 1)
        throw std::out_of_range("Index is out of bounds!");

    updateElem(0, numElem -1, ind, val, 0);
}

/* Leaving below code as commented as its provide usage guidance. */
/*
class NumArray {

    private:
        unique_ptr<segTree<int>> tree;

    public:

        NumArray(vector<int>& nums) {
            tree = make_unique<segTree<int>>(nums, make_unique<sumSegTreeStrategy<int>>());
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
